
#include "JsiWorkletContext.h"
#include "JsiWorkletApi.h"

#include "DispatchQueue.h"
#include "JsRuntimeFactory.h"
#include "JsiHostObject.h"
#include "JsiPromiseWrapper.h"

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <jsi/jsi.h>

namespace RNWorklet {

const char *WorkletRuntimeFlag = "__rn_worklets_runtime_flag";
const char *GlobalPropertyName = "global";

std::vector<std::shared_ptr<JsiBaseDecorator>> JsiWorkletContext::decorators;
std::shared_ptr<JsiWorkletContext> JsiWorkletContext::defaultInstance;
std::map<std::thread::id, JsiWorkletContext *>
    JsiWorkletContext::threadContexts;
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

JsiWorkletContext::~JsiWorkletContext() {
  // Remove from thread contexts
  threadContexts.erase(std::this_thread::get_id());
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

  // Initialize thread context - we save a pointer to this context
  // in the static threadContexts map so that we later can look
  // up the correct context from a given thread.
  std::mutex mu;
  std::condition_variable cond;
  bool isFinished = false;
  std::unique_lock<std::mutex> lock(mu);
  _workletCallInvoker([&isFinished, &cond, this]() {
    this->_threadId = std::this_thread::get_id();
    threadContexts.emplace(this->_threadId, this);
    isFinished = true;
    cond.notify_one();
  });
  // Wait untill the blocking code as finished
  cond.wait(lock, [&]() { return isFinished; });
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

    // Run decorators if we're not the singleton main context - no need to
    // do this in the worklet thread because we should already be in the
    // worklet thread now.
    if (JsiWorkletContext::getDefaultInstance().get() != this) {
      for (size_t i = 0; i < decorators.size(); ++i) {
        decorators[i]->decorateRuntime(getWorkletRuntime());
      }
    }
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
      assert(self->_threadId == std::this_thread::get_id());
      fp(self.get(), self->getWorkletRuntime());
    }
  });
}

void JsiWorkletContext::addDecorator(
    std::shared_ptr<JsiBaseDecorator> decorator) {
  decorators.push_back(decorator);
  // decorate default context
  JsiWorkletContext::getDefaultInstance()->decorate(decorator);
}

template <typename... Args> void JsiWorkletContext::decorate(Args &&...args) {
  std::vector<std::shared_ptr<JsiBaseDecorator>> decorators = {args...};
  applyDecorators(decorators);
}

void JsiWorkletContext::applyDecorators(
    const std::vector<std::shared_ptr<JsiBaseDecorator>> &decorators) {
  std::mutex mu;
  std::condition_variable cond;
  bool isFinished = false;
  std::unique_lock<std::mutex> lock(mu);

  // Execute decoration in context' worklet thread/runtime
  _workletCallInvoker([&]() {
    std::lock_guard<std::mutex> lock(mu);
    for (size_t i = 0; i < decorators.size(); ++i) {
      decorators[i]->decorateRuntime(getWorkletRuntime());
    }
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
  std::shared_ptr<JsiWorklet> worklet = JsiWorklet::isDecoratedAsWorklet(runtime, func)
                     ? std::make_shared<JsiWorklet>(runtime, func)
                     : nullptr;

  // Now return the caller function as a hostfunction type.
  return [worklet, func, 
          ctx](jsi::Runtime &runtime, const jsi::Value &thisValue,
               const jsi::Value *arguments, size_t count) -> jsi::Value {
    auto callingCtx = getCurrent();
    auto convention = getCallingConvention(callingCtx, ctx);

    // Start by wrapping the arguments
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper(count);
    for (size_t i = 0; i < count; i++) {
      argsWrapper[i] = JsiWrapper::wrap(runtime, arguments[i]);
    }

    // Wrap the this value
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);

    // If we are calling directly from/to the JS context or within the same
    // context, we can just dispatch everything directly.
    if (convention == CallingConvention::JsToJs ||
        convention == CallingConvention::WithinCtx) {

      // Create promise
      auto promise = std::make_shared<JsiPromiseWrapper>(
          runtime,
          [ctx, convention, worklet, callingCtx, thisWrapper, argsWrapper, func,
           count](
              jsi::Runtime &runtime,
              std::function<void(jsi::Runtime & runtime, const jsi::Value &val)>
                  resolve,
              std::function<void(jsi::Runtime & runtime,
                                 const jsi::Value &reason)>
                  reject) {
            // Convert arguments back to unwrapped in current context
            size_t size = argsWrapper.size();
            std::vector<jsi::Value> args(size);

            // Add the rest of the arguments
            for (size_t i = 0; i < size; i++) {
              args[i] = JsiWrapper::unwrap(runtime, argsWrapper.at(i));
            }

            auto unwrappedThis = thisWrapper->unwrap(runtime);

            // We can resolve the result directly - we're in the same context.
            try {
              if (worklet) {
                auto retVal = worklet->call(
                    runtime, unwrappedThis,
                    static_cast<const jsi::Value *>(args.data()), count);
                resolve(runtime, retVal);
              } else {
                if (unwrappedThis.isObject()) {
                  auto retVal = func->callWithThis(
                      runtime, unwrappedThis.asObject(runtime),
                      static_cast<const jsi::Value *>(args.data()), count);
                  resolve(runtime, retVal);
                } else {
                  auto retVal = func->call(
                      runtime, static_cast<const jsi::Value *>(args.data()),
                      count);
                  resolve(runtime, retVal);
                }
              }
            } catch (const jsi::JSError &err) {
              // TODO: Handle Stack!!
              reject(runtime,
                     jsi::String::createFromUtf8(runtime, err.getMessage()));
            } catch (const std::exception &err) {
              std::string a = typeid(err).name();
              std::string b = typeid(jsi::JSError).name();
              if (a == b) {
                const auto *jsError = static_cast<const jsi::JSError *>(&err);
                auto message = jsError->getMessage();
                // auto stack = jsError->getStack();
                // TODO: JsErrorWrapper reason(message, stack);
                reject(runtime, jsi::String::createFromUtf8(runtime, message));
              } else {
                // TODO: JsErrorWrapper reason("Unknown error in promise", "[Uknown stack]");
                reject(runtime, jsi::String::createFromUtf8(runtime, "Unknown error in promise"));
              }
            } catch (...) {
              // TODO: JsErrorWrapper reason("Unknown error in promise", "[Uknown stack]");
              reject(runtime, jsi::String::createFromUtf8(runtime, "Unknown error in promise"));
            }
          });

      return jsi::Object::createFromHostObject(runtime, promise);
    }

    // Now we are in a situation where we are calling cross context (js -> ctx,
    // ctx -> ctx, ctx -> js)

    // Ensure that the function is a worklet
    if (worklet == nullptr) {
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
            JsiWorkletContext::getCurrent()->invokeOnJsThread([func](jsi::Runtime &rt) { func(rt); });
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
        JsiWorkletContext::getDefaultInstance()->invokeOnJsThread([func](jsi::Runtime &rt) { func(rt); });
      } else {
        auto caller = callingCtx->getName();
        if (ctx) {
          auto callInto = ctx->getName();
          if (callInto == "") {}
        }
        callingCtx->invokeOnWorkletThread([func](JsiWorkletContext *, jsi::Runtime &rt) { func(rt); });
      }
      /*switch (convention) {
      case CallingConvention::JsToCtx:
        
      case CallingConvention::CtxToCtx:
        ctx->invokeOnJsThread([func](jsi::Runtime &rt) { func(rt); });
        break;
      case CallingConvention::CtxToJs:
        callingCtx->invokeOnWorkletThread(
            [func](JsiWorkletContext *, jsi::Runtime &rt) { func(rt); });
        break;
      default:
        // Not used since the two last ones are only handling
        // inter context calling
        throw jsi::JSError(runtime,
                           "Should not be reached. Callback from context.");
      }*/
    };

    // Let's create a promise that can initialize and resolve / reject in the
    // correct contexts
    auto promise = std::make_shared<JsiPromiseWrapper>(
        runtime,
        [ctx, worklet, convention, callingCtx, thisWrapper, argsWrapper,
         callIntoCorrectContext, callback, func](
            jsi::Runtime &runtime,
            std::function<void(jsi::Runtime & runtime, const jsi::Value &val)>
                resolve,
            std::function<void(jsi::Runtime & runtime,
                               const jsi::Value &reason)>
                reject) {
          // Create callback wrapper
          callIntoCorrectContext([callback, worklet, thisWrapper, argsWrapper,
                                  resolve, reject](jsi::Runtime &runtime) {
            try {

              size_t size = argsWrapper.size();
              std::vector<jsi::Value> args(size);

              // Add the rest of the arguments
              for (size_t i = 0; i < size; i++) {
                args[i] = JsiWrapper::unwrap(runtime, argsWrapper.at(i));
              }

              auto unwrappedThis = thisWrapper->unwrap(runtime);

              // Call the worklet
              auto results = worklet->call(
                  runtime, unwrappedThis,
                  static_cast<const jsi::Value *>(args.data()), size);

              auto retVal = JsiWrapper::wrap(runtime, results);

              // Callback with the results
              callback([retVal, resolve](jsi::Runtime &runtime) {
                resolve(runtime, retVal->unwrap(runtime));
              });
            } catch (const jsi::JSError &err) {
              auto message = err.getMessage();
              auto stack = err.getStack();
              // TODO: Stack
              callback([message, stack, reject](jsi::Runtime &runtime) {
                reject(runtime, jsi::String::createFromUtf8(runtime, message));
              });
            } catch (const std::exception &err) {
              std::string a = typeid(err).name();
              std::string b = typeid(jsi::JSError).name();
              if (a == b) {
                const auto *jsError = static_cast<const jsi::JSError *>(&err);
                auto message = jsError->getMessage();
                auto stack = jsError->getStack();
                // TODO: Stack
                callback([message, stack, reject](jsi::Runtime &runtime) {
                  reject(runtime,
                         jsi::String::createFromUtf8(runtime, message));
                });
              } else {
                // TODO: Stack
                callback([err, reject](jsi::Runtime &runtime) {
                  reject(runtime,
                         jsi::String::createFromUtf8(runtime, err.what()));
                });
              }
            } catch (...) {
              callback([reject](jsi::Runtime &runtime) {
                // TODO: Handle Stack!!
                reject(runtime, jsi::String::createFromUtf8(
                                    runtime, "Unknown error in promise."));
              });
            }
          });
        });

    return jsi::Object::createFromHostObject(runtime, promise);
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
