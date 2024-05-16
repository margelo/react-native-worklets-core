
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
#include "WKTJsiJsDecorator.h"
#include "WKTFunctionInvoker.h"

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

std::map<void *, JsiWorkletContext *> JsiWorkletContext::runtimeMappings;
size_t JsiWorkletContext::contextIdNumber = 1000;

namespace jsi = facebook::jsi;

JsiWorkletContext::JsiWorkletContext(
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
  addDecorator(std::make_shared<JsiConsoleDecorator>([jsRuntime, jsCallInvoker](std::function<void(jsi::Runtime&)> callback) {
    jsCallInvoker([jsRuntime, callback = std::move(callback)]() {
      callback(*jsRuntime);
    });
  }));
}

JsiWorkletContext::~JsiWorkletContext() {
  // Remove from thread contexts
  runtimeMappings.erase(&_workletRuntime);
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
      fp(self.get(), self->getWorkletRuntime());
    }
  });
}

void JsiWorkletContext::addDecorator(std::shared_ptr<JsiBaseDecorator> decorator) {
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

void JsiWorkletContext::addDecorator(jsi::Runtime &runtime, const std::string& propName, const jsi::Value& value) {
  // Create a JS based decorator
  addDecorator(std::make_shared<JsiJsDecorator>(runtime, propName, value));
}

std::string tryGetFunctionName(jsi::Runtime& runtime, const jsi::Value& maybeFunc) {
  try {
    jsi::Value name = maybeFunc.asObject(runtime).getProperty(runtime, "name");
    return name.asString(runtime).utf8(runtime);
  } catch (...) {
    return "anonymous";
  }
}


JSCallInvoker JsiWorkletContext::getCurrentRuntimeInvoker(jsi::Runtime& currentRuntime) {
  JsiWorkletContext* context = JsiWorkletContext::getCurrent(currentRuntime);
  if (context == nullptr) {
    // We are not on a Worklet runtime - let's use the main React JS CallInvoker.
    auto callInvoker = _jsCallInvoker;
    auto jsRuntime = _jsRuntime;
    return [callInvoker, jsRuntime](std::function<void(jsi::Runtime& targetRuntime)>&& callback) {
      callInvoker([jsRuntime, callback = std::move(callback)]() {
        callback(*jsRuntime);
      });
    };
  } else {
    // We are on a Worklet runtime, let's use the current context's worklet invoker!
    std::weak_ptr<JsiWorkletContext> weakContext = context->shared_from_this();
    std::string name = context->_name;
    return [weakContext, name](std::function<void(jsi::Runtime& targetRuntime)>&& callback) {
      std::shared_ptr<JsiWorkletContext> sharedContext = weakContext.lock();
      if (sharedContext == nullptr) {
        throw std::runtime_error("Cannot dispatch to Worklet Context \"" + name + "\" - it has already been destroyed!");
      }
      sharedContext->invokeOnWorkletThread([callback = std::move(callback)](JsiWorkletContext*, jsi::Runtime& targetRuntime) {
        callback(targetRuntime);
      });
    };
  }
}

jsi::HostFunctionType
JsiWorkletContext::createCallOnJS(jsi::Runtime &runtime, const jsi::Value &maybeFunc, JSCallInvoker&& jsCallInvoker) {
  // Create a FunctionInvoker that can invoke a Worklet or plain JS function.
  std::shared_ptr<FunctionInvoker> invoker = FunctionInvoker::createFunctionInvoker(runtime, maybeFunc);
  
  return [invoker, jsCallInvoker = std::move(jsCallInvoker)](jsi::Runtime &runtime,
                                                             const jsi::Value &thisValue,
                                                             const jsi::Value *arguments,
                                                             size_t count) -> jsi::Value {
    // Runs the given function on the default main JS runtime
    auto callOnTargetRuntime = [jsCallInvoker](std::function<void(jsi::Runtime& runtime)> callback) {
      jsCallInvoker(std::move(callback));
    };
    
    // Runs the given function on the Runtime that originally invoked this method call (to resolve/reject the Promise)
    JSCallInvoker callbackToOriginalRuntime;
    JsiWorkletContext* currentContext = getCurrent(runtime);
    if (currentContext != nullptr) {
      // This function is called from a Worklet context, so we wanna schedule back to that Worklet context to resolve the Promise.
      std::weak_ptr<JsiWorkletContext> weakContext = currentContext->shared_from_this();
      callbackToOriginalRuntime = [weakContext](std::function<void(jsi::Runtime& toRuntime)> callback) {
        auto context = weakContext.lock();
        if (context == nullptr) [[unlikely]] {
          throw std::runtime_error("Cannot call back to JS - the calling context has already been destroyed!");
        }
        context->invokeOnWorkletThread([callback = std::move(callback)](JsiWorkletContext*, jsi::Runtime& originalRuntime) {
          callback(originalRuntime);
        });
      };
    } else {
      // This functio nis called from the default JS Context, so we wanna just schedule back to JS to resolve the Promise.
      callbackToOriginalRuntime = jsCallInvoker;
    }
    
    // Create and run Promise.
    std::shared_ptr<JsiPromiseWrapper> promise = invoker->call(runtime, thisValue, arguments, count, std::move(callOnTargetRuntime), std::move(callbackToOriginalRuntime));
    return jsi::Object::createFromHostObject(runtime, promise);
  };
}

jsi::HostFunctionType JsiWorkletContext::createCallInContext(jsi::Runtime &runtime, const jsi::Value &maybeFunc) {
  // Create a FunctionInvoker that can invoke a Worklet or plain JS function.
  std::shared_ptr<FunctionInvoker> invoker = FunctionInvoker::createFunctionInvoker(runtime, maybeFunc);
  std::weak_ptr<JsiWorkletContext> weakSelf = shared_from_this();
  
  return [weakSelf, invoker](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments, size_t count) -> jsi::Value {
    // Runs the given function on the target Worklet Runtime (to run the actual worklet code)
    JSCallInvoker runOnTargetRuntime = [weakSelf](std::function<void(jsi::Runtime& toRuntime)> callback) {
      auto self = weakSelf.lock();
      if (self == nullptr) [[unlikely]] {
        throw std::runtime_error("Cannot call Worklet - the target context has already been destroyed!");
      }
      self->invokeOnWorkletThread([callback = std::move(callback)](JsiWorkletContext*, jsi::Runtime& toRuntime) {
        callback(toRuntime);
      });
    };
    
    // Runs the given function on the Runtime that originally invoked this method call (to resolve/reject the Promise)
    JSCallInvoker callbackToOriginalRuntime;
    JsiWorkletContext* currentContext = getCurrent(runtime);
    if (currentContext != nullptr) {
      // This function is called from a Worklet context, so we wanna schedule back to that Worklet context to resolve the Promise.
      std::weak_ptr<JsiWorkletContext> weakContext = currentContext->shared_from_this();
      callbackToOriginalRuntime = [weakContext](std::function<void(jsi::Runtime& toRuntime)> callback) {
        auto context = weakContext.lock();
        if (context == nullptr) [[unlikely]] {
          throw std::runtime_error("Cannot call back to JS - the calling context has already been destroyed!");
        }
        context->invokeOnWorkletThread([callback = std::move(callback)](JsiWorkletContext*, jsi::Runtime& originalRuntime) {
          callback(originalRuntime);
        });
      };
    } else {
      // This functio nis called from the default JS Context, so we wanna just schedule back to JS to resolve the Promise.
      callbackToOriginalRuntime = [weakSelf](std::function<void(jsi::Runtime& toRuntime)> callback) {
        auto self = weakSelf.lock();
        if (self == nullptr) [[unlikely]] {
          throw std::runtime_error("Cannot call back to JS - the target context has already been destroyed!");
        }
        self->invokeOnJsThread(std::move(callback));
      };
    }
    
    // Create and run Promise.
    std::shared_ptr<JsiPromiseWrapper> promise = invoker->call(runtime, thisValue, arguments, count, std::move(runOnTargetRuntime), std::move(callbackToOriginalRuntime));
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

    // We are on a worklet thread
    if (ctx == nullptr) {
      throw std::runtime_error("Failed to create Worklet invoker - this Runtime does not have a Worklet Context!");
    }
    ctx->invokeOnWorkletThread(
        [argsWrapper, rtPtr, func = std::move(func)](JsiWorkletContext *,
                                   jsi::Runtime &runtime) mutable {
          assert(&runtime == rtPtr && "Expected same runtime ptr!");
          auto args = argsWrapper.getArguments(runtime);
          func->call(runtime, ArgumentsWrapper::toArgs(args),
                     argsWrapper.getCount());
          func = nullptr;
        });

    return jsi::Value::undefined();
  };
}

} // namespace RNWorklet
