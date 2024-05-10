
#include "WKTJsiWorkletContext.h"
#include "WKTJsiWorkletApi.h"
#include "WKTRuntimeAwareCache.h"

#include "WKTArgumentsWrapper.h"
#include "WKTDispatchQueue.h"
#include "WKTJsRuntimeFactory.h"
#include "WKTJsiHostObject.h"
#include "WKTJsiPromiseWrapper.h"

#include "WKTJsiConsoleDecorator.h"
#include "WKTJsiJsDecorator.h"
#include "WKTJsiPerformanceDecorator.h"
#include "WKTJsiSetImmediateDecorator.h"

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <jsi/jsi.h>

#ifdef ANDROID
#include <fbjni/fbjni.h>
#endif

namespace RNWorklet {

const char *WorkletRuntimeFlag = "__rn_worklets_runtime_flag";
const char *GlobalPropertyName = "global";

std::shared_ptr<JsiWorkletContext> JsiWorkletContext::defaultInstance;
std::map<void *, JsiWorkletContext *> JsiWorkletContext::runtimeMappings;
size_t JsiWorkletContext::contextIdNumber = 1000;

namespace jsi = facebook::jsi;

JsiWorkletContext::JsiWorkletContext(const std::string &name) {
  // Initialize context
  initialize(name, JsiWorkletContext::getDefaultInstance()->_jsRuntime,
             JsiWorkletContext::getDefaultInstance()->_jsCallInvoker);
}

JsiWorkletContext::JsiWorkletContext(
    const std::string &name,
    std::function<void(std::function<void()> &&)> workletCallInvoker) {
  // Initialize context
  initialize(name, JsiWorkletContext::getDefaultInstance()->_jsRuntime,
             JsiWorkletContext::getDefaultInstance()->_jsCallInvoker,
             workletCallInvoker);
}

JsiWorkletContext::JsiWorkletContext(
    const std::string &name, jsi::Runtime *jsRuntime,
    std::function<void(std::function<void()> &&)> jsCallInvoker,
    std::function<void(std::function<void()> &&)> workletCallInvoker) {
  // Initialize context
  initialize(name, jsRuntime, jsCallInvoker, workletCallInvoker);
}

JsiWorkletContext::~JsiWorkletContext() {
  // Remove from thread contexts
  runtimeMappings.erase(&_workletRuntime);
}

void JsiWorkletContext::initialize(
    const std::string &name, jsi::Runtime *jsRuntime,
    std::function<void(std::function<void()> &&)> jsCallInvoker,
    std::function<void(std::function<void()> &&)> workletCallInvoker) {

  _name = name;
  _jsRuntime = jsRuntime;
  _jsCallInvoker = jsCallInvoker;
  _workletCallInvoker = workletCallInvoker;
  _contextId = ++contextIdNumber;

  _jsThreadId = std::this_thread::get_id();
  runtimeMappings.emplace(&getWorkletRuntime(), this);

  // Add default decorators
  addDecorator(std::make_shared<JsiSetImmediateDecorator>());
  addDecorator(std::make_shared<JsiPerformanceDecorator>());
  addDecorator(std::make_shared<JsiConsoleDecorator>());
}

void JsiWorkletContext::initialize(
    const std::string &name, jsi::Runtime *jsRuntime,
    std::function<void(std::function<void()> &&)> jsCallInvoker) {
  // Create queue
  auto dispatchQueue = std::make_shared<DispatchQueue>(
      name + "_worklet_dispatch_queue_" + std::to_string(_contextId));

  // Initialize invoker
  initialize(
      name, jsRuntime, jsCallInvoker,
      [dispatchQueue = std::move(dispatchQueue)](std::function<void()> &&f) {
        dispatchQueue->dispatch(std::move(f));
      });
}

jsi::Runtime &JsiWorkletContext::getWorkletRuntime() {
  if (!_workletRuntime) {
    // Lazy initialization of the worklet runtime
    _workletRuntime = makeJSIRuntime();
    _workletRuntime->global().setProperty(*_workletRuntime, WorkletRuntimeFlag,
                                          true);

    // Copy global which is expected to be found instead of the globalThis
    // object.
    _workletRuntime->global().setProperty(*_workletRuntime, GlobalPropertyName,
                                          _workletRuntime->global());

    // Install the WorkletAPI into the new runtime
    JsiWorkletApi::installApi(*_workletRuntime);
  }

  return *_workletRuntime;
}

void JsiWorkletContext::invokeOnJsThread(
    std::function<void(jsi::Runtime &runtime)> &&fp) {
  if (_jsCallInvoker == nullptr) {
    throw std::runtime_error(
        "Expected Worklet context to have a JS call invoker.");
  }
  _jsCallInvoker([fp = std::move(fp), weakSelf = weak_from_this()]() {
    auto self = weakSelf.lock();
    if (self) {
      assert(self->_jsThreadId == std::this_thread::get_id());
      fp(*self->getJsRuntime());
    }
  });
}

void JsiWorkletContext::invokeOnWorkletThread(
    std::function<void(JsiWorkletContext *context, jsi::Runtime &runtime)>
        &&fp) {
  if (_workletCallInvoker == nullptr) {
    throw std::runtime_error(
        "Expected Worklet context to have a worklet call invoker.");
  }
  _workletCallInvoker([fp = std::move(fp), weakSelf = weak_from_this()]() {
    auto self = weakSelf.lock();
    if (self) {
#ifdef ANDROID
      facebook::jni::ThreadScope::WithClassLoader([fp = std::move(fp), self]() {
        fp(self.get(), self->getWorkletRuntime());
      });
#else
      fp(self.get(), self->getWorkletRuntime());
#endif
    }
  });
}

void JsiWorkletContext::addDecorator(
    std::shared_ptr<JsiBaseDecorator> decorator) {
  std::mutex mu;
  std::condition_variable cond;
  bool isFinished = false;
  std::unique_lock<std::mutex> lock(mu);

  decorator->initialize(*getJsRuntime());

  // Execute decoration in context' worklet thread/runtime
  _workletCallInvoker([&]() {
    std::lock_guard<std::mutex> lock(mu);
    decorator->decorateRuntime(getWorkletRuntime());
    isFinished = true;
    cond.notify_one();
  });

  // Wait untill the blocking code as finished
  cond.wait(lock, [&]() { return isFinished; });
}

jsi::HostFunctionType
JsiWorkletContext::createCallInContext(jsi::Runtime &runtime,
                                       const jsi::Value &maybeFunc) {
  return createCallInContext(runtime, maybeFunc, this);
}

jsi::HostFunctionType
JsiWorkletContext::createCallInContext(jsi::Runtime &runtime,
                                       const jsi::Value &maybeFunc,
                                       JsiWorkletContext *ctx) {

  // Ensure that we are passing a function as the param.
  if (!maybeFunc.isObject() ||
      !maybeFunc.asObject(runtime).isFunction(runtime)) {
    throw jsi::JSError(
        runtime,
        "Parameter to callInContext is not a valid Javascript function.");
  }

  // Now we can save the function in a shared ptr to be able to keep it around
  auto func = std::make_shared<jsi::Function>(
      maybeFunc.asObject(runtime).asFunction(runtime));

  // Create a worklet of the function if the function is a worklet - it should
  // be allowed to create a caller function and call inside the same JS or ctx
  // without having to pass a worklet
  auto workletInvoker =
      JsiWorklet::isDecoratedAsWorklet(runtime, func)
          ? std::make_shared<WorkletInvoker>(runtime, maybeFunc)
          : nullptr;

  // Now return the caller function as a hostfunction type.
  return [workletInvoker, func,
          ctx](jsi::Runtime &runtime, const jsi::Value &thisValue,
               const jsi::Value *arguments, size_t count) -> jsi::Value {
    auto callingCtx = getCurrent(runtime);
    auto convention = getCallingConvention(callingCtx, ctx);

    // Start by wrapping the arguments
    ArgumentsWrapper argsWrapper(runtime, arguments, count);

    // Wrap the this value
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);

    // If we are calling directly from/to the JS context or within the same
    // context, we can just dispatch everything directly.
    if (convention == CallingConvention::JsToJs ||
        convention == CallingConvention::WithinCtx) {

      // Create promise
      auto promise = JsiPromiseWrapper::createPromiseWrapper(
          runtime,
          [ctx, convention, workletInvoker, callingCtx, thisWrapper,
           argsWrapper, func](jsi::Runtime &runtime,
                              std::shared_ptr<PromiseParameter> promise) {
            auto unwrappedThis = thisWrapper->unwrap(runtime);
            auto args = argsWrapper.getArguments(runtime);

            // We can resolve the result directly - we're in the same context.
            try {
              if (workletInvoker) {
                auto retVal = workletInvoker->call(
                    runtime, unwrappedThis, ArgumentsWrapper::toArgs(args),
                    argsWrapper.getCount());
                promise->resolve(runtime, retVal);
              } else {
                if (unwrappedThis.isObject()) {
                  auto retVal = func->callWithThis(
                      runtime, unwrappedThis.asObject(runtime),
                      ArgumentsWrapper::toArgs(args), argsWrapper.getCount());
                  promise->resolve(runtime, retVal);
                } else {
                  auto retVal =
                      func->call(runtime, ArgumentsWrapper::toArgs(args),
                                 argsWrapper.getCount());
                  promise->resolve(runtime, retVal);
                }
              }
            } catch (const jsi::JSError &err) {
              // TODO: Handle Stack!!
              promise->reject(runtime, jsi::String::createFromUtf8(
                                           runtime, err.getMessage()));
            } catch (const std::exception &err) {
              std::string a = typeid(err).name();
              std::string b = typeid(jsi::JSError).name();
              if (a == b) {
                const auto *jsError = static_cast<const jsi::JSError *>(&err);
                auto message = jsError->getMessage();
                // auto stack = jsError->getStack();
                // TODO: JsErrorWrapper reason(message, stack);
                promise->reject(runtime,
                                jsi::String::createFromUtf8(runtime, message));
              } else {
                // TODO: JsErrorWrapper reason("Unknown error in promise",
                // "[Uknown stack]");
                promise->reject(runtime,
                                jsi::String::createFromUtf8(
                                    runtime, "Unknown error in promise"));
              }
            } catch (...) {
              // TODO: JsErrorWrapper reason("Unknown error in promise",
              // "[Uknown stack]");
              promise->reject(runtime,
                              jsi::String::createFromUtf8(
                                  runtime, "Unknown error in promise"));
            }
          });

      return jsi::Object::createFromHostObject(runtime, promise);
    }

    // Now we are in a situation where we are calling cross context (js -> ctx,
    // ctx -> ctx, ctx -> js)

    // Ensure that the function is a worklet
    if (workletInvoker == nullptr && convention != CallingConvention::CtxToJs) {
      throw jsi::JSError(runtime, "In callInContext the function parameter "
                                  "is not a valid worklet and "
                                  "cannot be called between contexts or "
                                  "from/to JS from/to a context.");
    }

    auto callIntoCorrectContext =
        [&runtime, convention,
         ctx](std::function<void(jsi::Runtime & runtime)> &&func) {
          switch (convention) {
          case CallingConvention::JsToCtx:
          case CallingConvention::CtxToCtx:
            ctx->invokeOnWorkletThread(
                [func](JsiWorkletContext *, jsi::Runtime &rt) { func(rt); });
            break;
          case CallingConvention::CtxToJs:
            JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
                [func](jsi::Runtime &rt) { func(rt); });
            break;
          default:
            // Not used since the two last ones are only handling
            // inter context calling
            throw jsi::JSError(runtime,
                               "Should not be reached. Callback into context.");
          }
        };

    auto callback = [&runtime, convention, callingCtx,
                     ctx](std::function<void(jsi::Runtime & rt)> &&func) {
      // Always resolve in the calling context or the JS context if null
      if (callingCtx == nullptr) {
        JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
            [func](jsi::Runtime &rt) { func(rt); });
      } else {
        callingCtx->invokeOnWorkletThread(
            [func](JsiWorkletContext *, jsi::Runtime &rt) { func(rt); });
      }
    };

    // Let's create a promise that can initialize and resolve / reject in the
    // correct contexts
    auto promise = JsiPromiseWrapper::createPromiseWrapper(
        runtime, [ctx, workletInvoker, convention, callingCtx, thisWrapper,
                  argsWrapper, callIntoCorrectContext, callback,
                  func](jsi::Runtime &runtime,
                        std::shared_ptr<PromiseParameter> promise) {
          // Create callback wrapper
          callIntoCorrectContext([callback, workletInvoker, thisWrapper,
                                  argsWrapper, promise,
                                  func](jsi::Runtime &runtime) mutable {
            try {

              auto args = argsWrapper.getArguments(runtime);

              jsi::Value result;
              if (workletInvoker != nullptr) {
                result = workletInvoker->call(
                    runtime, thisWrapper->unwrap(runtime),
                    ArgumentsWrapper::toArgs(args), argsWrapper.getCount());
              } else {
                result = func->call(runtime, ArgumentsWrapper::toArgs(args),
                                    argsWrapper.getCount());
              }

              auto retVal = JsiWrapper::wrap(runtime, result);

              // Callback with the results
              callback([retVal, promise](jsi::Runtime &runtime) {
                promise->resolve(runtime, retVal->unwrap(runtime));
              });
            } catch (const jsi::JSError &err) {
              auto message = err.getMessage();
              auto stack = err.getStack();
              // TODO: Stack
              callback([message, stack, promise](jsi::Runtime &runtime) {
                promise->reject(runtime,
                                jsi::String::createFromUtf8(runtime, message));
              });
            } catch (const std::exception &err) {
              std::string a = typeid(err).name();
              std::string b = typeid(jsi::JSError).name();
              if (a == b) {
                const auto *jsError = static_cast<const jsi::JSError *>(&err);
                auto message = jsError->getMessage();
                auto stack = jsError->getStack();
                // TODO: Stack
                callback([message, stack, promise](jsi::Runtime &runtime) {
                  promise->reject(
                      runtime, jsi::String::createFromUtf8(runtime, message));
                });
              } else {
                // TODO: Stack
                callback([err, promise](jsi::Runtime &runtime) {
                  promise->reject(runtime, jsi::String::createFromUtf8(
                                               runtime, err.what()));
                });
              }
            } catch (...) {
              callback([promise](jsi::Runtime &runtime) {
                // TODO: Handle Stack!!
                promise->reject(runtime,
                                jsi::String::createFromUtf8(
                                    runtime, "Unknown error in promise."));
              });
            }
            
            // We need to explicitly clear the func shared pointer here to avoid it being
            // deleted on another thread
            promise = nullptr;
          });
        });

    return jsi::Object::createFromHostObject(runtime, promise);
  };
}

jsi::HostFunctionType
JsiWorkletContext::createInvoker(jsi::Runtime &runtime,
                                 const jsi::Value *maybeFunc) {
  auto rtPtr = &runtime;
  auto ctx = JsiWorkletContext::getCurrent(runtime);

  // Create host function
  return [rtPtr, ctx,
          func = std::make_shared<jsi::Function>(
              maybeFunc->asObject(runtime).asFunction(runtime))](
             jsi::Runtime &runtime, const jsi::Value &thisValue,
             const jsi::Value *arguments, size_t count) {
    // If we are in the same context let's just call the function directly
    if (&runtime == rtPtr) {
      return func->call(runtime, arguments, count);
    }

    // We're about to cross contexts and will need to wrap args
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
    ArgumentsWrapper argsWrapper(runtime, arguments, count);

    if (ctx != nullptr) {
      // We are on a worklet thread
      ctx->invokeOnWorkletThread(
          [argsWrapper, rtPtr, func = std::move(func)](JsiWorkletContext *,
                                     jsi::Runtime &runtime) mutable {
            assert(&runtime == rtPtr && "Expected same runtime ptr!");
            auto args = argsWrapper.getArguments(runtime);
            func->call(runtime, ArgumentsWrapper::toArgs(args),
                       argsWrapper.getCount());
            func = nullptr;
          });
    } else {
      JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
          [argsWrapper, rtPtr, func = std::move(func)](jsi::Runtime &runtime) mutable {
            assert(&runtime == rtPtr && "Expected same runtime ptr!");
            auto args = argsWrapper.getArguments(runtime);
            func->call(runtime, ArgumentsWrapper::toArgs(args),
                       argsWrapper.getCount());
            func = nullptr;
          });
    }

    return jsi::Value::undefined();
  };
}

JsiWorkletContext::CallingConvention
JsiWorkletContext::getCallingConvention(JsiWorkletContext *fromContext,
                                        JsiWorkletContext *toContext) {

  if (toContext == nullptr) {
    // Calling into JS
    if (fromContext == nullptr) {
      // Calling from JS
      return CallingConvention::JsToJs;
    } else {
      // Calling from a context
      return CallingConvention::CtxToJs;
    }
  } else {
    // Calling into another ctx
    if (fromContext == nullptr) {
      // Calling from Js
      return CallingConvention::JsToCtx;
    } else {
      // Calling from Ctx
      if (fromContext == toContext) {
        return CallingConvention::WithinCtx;
      } else {
        return CallingConvention::CtxToCtx;
      }
    }
  }
}

} // namespace RNWorklet
