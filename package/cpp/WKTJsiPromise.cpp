//
//  WKTJsiPromise.cpp
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 16.05.24.
//

#include "WKTJsiPromise.h"
#include "WKTJsiWorkletContext.h"

namespace RNWorklet {

jsi::Value JsiPromise::createPromise(jsi::Runtime& runtime, JSCallInvoker runOnMainJSRuntime, RunPromiseCallback&& run) {
  // C++ equivalent of the following JS Code:
  //   return new Promise((resolve, reject) => {
  //     run(resolve, reject)
  //   })
  
  jsi::Function promiseCtor = runtime.global().getPropertyAsFunction(runtime, "Promise");
  jsi::HostFunctionType runCtor = [run = std::move(run), runOnMainJSRuntime](jsi::Runtime& runtime,
                                                                             const jsi::Value& thisValue,
                                                                             const jsi::Value* arguments,
                                                                             size_t count) -> jsi::Value {
    std::shared_ptr<jsi::Function> resolve = std::make_shared<jsi::Function>(arguments[0].asObject(runtime).asFunction(runtime));
    std::shared_ptr<jsi::Function> reject = std::make_shared<jsi::Function>(arguments[1].asObject(runtime).asFunction(runtime));
    
    Resolver resolveWrapper;
    Rejecter rejectWrapper;
    
    JsiWorkletContext* callingContext = JsiWorkletContext::getCurrent(runtime);
    if (callingContext != nullptr) {
      // This is being called from a Worklet Context.
      // We need to resolve/reject on this context.
      std::weak_ptr<JsiWorkletContext> weakContext = callingContext->shared_from_this();
      resolveWrapper = [weakContext, resolve](std::shared_ptr<JsiWrapper> wrappedResult) {
        auto context = weakContext.lock();
        if (context == nullptr) {
          throw std::runtime_error("Cannot resolve Promise - calling Worklet Context has already been destroyed!");
        }
        context->invokeOnWorkletThread([wrappedResult, resolve](JsiWorkletContext*, jsi::Runtime& runtime) {
          jsi::Value result = wrappedResult->unwrap(runtime);
          resolve->call(runtime, result);
        });
      };
      rejectWrapper = [weakContext, reject](std::exception error) {
        auto context = weakContext.lock();
        if (context == nullptr) {
          throw std::runtime_error("Cannot resolve Promise - calling Worklet Context has already been destroyed!");
        }
        context->invokeOnWorkletThread([error](JsiWorkletContext*, jsi::Runtime& runtime) {
          // TODO: Stack!!
          jsi::JSError(runtime, error.what());
        });
      };
    } else {
      // This is being called from the main JS Context.
      // We need to resolve/reject on this context.
      resolveWrapper = [resolve, runOnMainJSRuntime](std::shared_ptr<JsiWrapper> wrappedResult) {
        runOnMainJSRuntime([wrappedResult, resolve](jsi::Runtime& runtime) {
          jsi::Value result = wrappedResult->unwrap(runtime);
          resolve->call(runtime, result);
        });
      };
      rejectWrapper = [reject, runOnMainJSRuntime](std::exception error) {
        runOnMainJSRuntime([error](jsi::Runtime& runtime) {
          // TODO: Stack!!
          jsi::JSError(runtime, error.what());
        });
      };
    }
    
    run(runtime, resolveWrapper, rejectWrapper);
    return jsi::Value::undefined();
  };
  
  jsi::Function argument = jsi::Function::createFromHostFunction(runtime, jsi::PropNameID::forUtf8(runtime, "run"), 2, runCtor);
  return promiseCtor.callAsConstructor(runtime, argument);
}

} // namespace RNWorklets
