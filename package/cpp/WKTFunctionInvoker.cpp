//
//  WKTFunctionInvoker.cpp
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 16.05.24.
//

#include "WKTFunctionInvoker.h"
#include "WKTJsiWorkletContext.h"
#include "WKTJsiWrapper.h"
#include "WKTArgumentsWrapper.h"

namespace RNWorklet {

inline std::string tryGetFunctionName(jsi::Runtime& runtime, const jsi::Value& maybeFunc) {
  try {
    jsi::Value name = maybeFunc.asObject(runtime).getProperty(runtime, "name");
    return name.asString(runtime).utf8(runtime);
  } catch (...) {
    return "anonymous";
  }
}

std::shared_ptr<FunctionInvoker> FunctionInvoker::createFunctionInvoker(jsi::Runtime &runtime, const jsi::Value &maybeFunc) {
  
  // Ensure that we are passing a function as the param.
  if (!maybeFunc.isObject() ||
      !maybeFunc.asObject(runtime).isFunction(runtime)) [[unlikely]] {
    std::string name = tryGetFunctionName(runtime, maybeFunc);
    throw std::runtime_error("Failed to get Function " + name + " - it's not a valid jsi::Function!");
  }

  // Now we can save the function in a shared ptr to be able to keep it around
  auto function = std::make_shared<jsi::Function>(maybeFunc.asObject(runtime).asFunction(runtime));
  
  // Create the FunctionInvoker
  if (JsiWorklet::isDecoratedAsWorklet(runtime, function)) {
    // It's a Worklet - return Worklet:
    auto workletInvoker = std::make_shared<WorkletInvoker>(runtime, function);
    return std::shared_ptr<FunctionInvoker>(new FunctionInvoker(runtime, workletInvoker));
  } else {
    // It's a plain JSI Function, return that:
    return std::shared_ptr<FunctionInvoker>(new FunctionInvoker(runtime, function));
  }
}


/**
 Returns the calling convention for a given from/to context. This method is
 used to find out if we are calling a worklet from or to the JS context or
 from to a Worklet context or a combination of these.
 */
CallingConvention getCallingConvention(JsiWorkletContext *fromContextOrNull,
                                       JsiWorkletContext *toContextOrNull) {
  if (toContextOrNull == nullptr) {
    // Calling into JS
    if (fromContextOrNull == nullptr) {
      // Calling from JS
      return CallingConvention::JsToJs;
    } else {
      // Calling from a context
      return CallingConvention::CtxToJs;
    }
  } else {
    // Calling into another ctx
    if (fromContextOrNull == nullptr) {
      // Calling from Js
      return CallingConvention::JsToCtx;
    } else {
      // Calling from Ctx
      if (fromContextOrNull == toContextOrNull) {
        return CallingConvention::WithinCtx;
      } else {
        return CallingConvention::CtxToCtx;
      }
    }
  }
}

std::shared_ptr<JsiPromiseWrapper> FunctionInvoker::call(jsi::Runtime& fromRuntime,
                                                         const jsi::Value& thisValue,
                                                         const jsi::Value* arguments,
                                                         size_t count,
                                                         JSCallInvoker&& runOnTargetRuntime,
                                                         JSCallInvoker&& callbackToOriginalRuntime) {
  // Start by wrapping the arguments
  ArgumentsWrapper argsWrapper(fromRuntime, arguments, count);

  // Wrap the `this` value (or undefined)
  std::shared_ptr<JsiWrapper> thisWrapper = JsiWrapper::wrap(fromRuntime, thisValue);
  
  // Create Promise
  std::shared_ptr<FunctionInvoker> self = shared_from_this();
  auto promise = JsiPromiseWrapper::createPromiseWrapper(fromRuntime, [self,
                                                                       thisWrapper,
                                                                       argsWrapper = std::move(argsWrapper),
                                                                       runOnTargetRuntime = std::move(runOnTargetRuntime),
                                                                       callbackToOriginalRuntime = std::move(callbackToOriginalRuntime)
                                                                      ](jsi::Runtime&,
                                                                        std::shared_ptr<PromiseParameter> promise) {
    // Here we are still on the caller Runtime. We need to dispatch this call to the given target runtime now..
    runOnTargetRuntime([self,
                        promise,
                        thisWrapper,
                        argsWrapper = std::move(argsWrapper),
                        callbackToOriginalRuntime = std::move(callbackToOriginalRuntime)
                       ](jsi::Runtime& targetRuntime) {
      // Now we are on the target Runtime, let's unwrap all arguments and extract them into this target Runtime.
      auto unwrappedThis = thisWrapper->unwrap(targetRuntime);
      auto args = argsWrapper.getArguments(targetRuntime);
      
      try {
        // Call the actual function or worklet
        jsi::Value result = jsi::Value::undefined();
        if (self->isWorkletFunction()) {
          // It's a Worklet, so we need to inject the captured values and call it as a Worklet
          auto workletInvoker = self->_workletFunctionOrNull;
          result = workletInvoker->call(targetRuntime, unwrappedThis, ArgumentsWrapper::toArgs(args), argsWrapper.getCount());
        } else {
          // It's a normal JS func, so we just call it.
          auto plainFunction = self->_plainFunctionOrNull;
          if (unwrappedThis.isObject()) {
            // ...with `this`
            result = plainFunction->callWithThis(targetRuntime, unwrappedThis.asObject(targetRuntime), ArgumentsWrapper::toArgs(args), argsWrapper.getCount());
          } else {
            // ...without `this`
            result = plainFunction->call(targetRuntime, ArgumentsWrapper::toArgs(args), argsWrapper.getCount());
          }
        }
        
        // Wrap returned value so we can send it back to the original calling Runtime
        auto wrappedResult = JsiWrapper::wrap(targetRuntime, result);
        callbackToOriginalRuntime([wrappedResult, promise](jsi::Runtime& originalRuntime) {
          // Now back on the original calling runtime, unwrap the result and resolve the Promise.
          jsi::Value unwrappedResult = wrappedResult->unwrap(originalRuntime);
          promise->resolve(originalRuntime, unwrappedResult);
        });
      } catch (std::exception& exception) {
        // TODO: What do we do here now?
        std::string message = exception.what();
        callbackToOriginalRuntime([message, promise](jsi::Runtime& originalRuntime) {
          jsi::JSError error(originalRuntime, message);
          promise->reject(originalRuntime, error.value());
        });
      }
    });
  });
  return promise;
}


} // namespace RNWorklet
