//
// Created by Christian Falch on 01/11/2021.
//
#pragma once

#include <JsiHostObject.h>
#include <JsiWorkletContext.h>

#include <ReactCommon/TurboModuleUtils.h>
#include <jsi/jsi.h>

namespace RNWorklet {

using namespace facebook;
using namespace RNJsi;

using JsiWorkletDispatcher =
    std::function<jsi::Value(jsi::Runtime &, std::shared_ptr<JsiWrapper>,
                             const std::vector<std::shared_ptr<JsiWrapper>> &)>;

using JsiWorkletDispatchers = struct {
  JsiWorkletDispatcher callInWorkletRuntime;
  JsiWorkletDispatcher callInMainRuntime;
};

/**
 Encapsulates a runnable function. A runnable function is a function
 that exists in both the main JS runtime and as an installed function
 in the worklet runtime. The worklet object exposes some methods and props
 for handling this. The worklet exists in a given worklet context.

 The signature for a worklet is as follows:

 worklet: {
  // Returns true for worklets
  isWorklet: boolean;
  // Returns true if we are currently on the main thread
  isMainThread: boolean;
  // Runs the worklet on the main thread and in the main js runtime
  runOnMainThread: (...args) => any;
  // Runs the worklet on the worklet thread and on the worklet runtime
  runOnWorkletThread: (...args) => Promise<any>;
 }
 */
class JsiWorklet : public JsiHostObject {
public:
  JsiWorklet(JsiWorkletContext *context, jsi::Runtime &runtime,
             const jsi::Value &function, const jsi::Value &closure) {
    // Make sure we call this from the main runtime
    if (context->isWorkletRuntime(runtime)) {
      jsi::detail::throwJSError(
          runtime, "A worklet must be created from the main runtime.");
      return;
    }

    // Create the worklet call function
    auto workletDispatchers =
        createWorkletFunction(context, runtime, function, closure);

    // isMainThread
    // Returns true if called from the main thread, false if called from the
    // context's worklet thread.
    installReadonlyProperty("isMainThread",
                            [context](jsi::Runtime &runtime) -> jsi::Value {
                              return (bool)!context->isWorkletRuntime(runtime);
                            });

    // Returns true for worklets
    installReadonlyProperty(
        "isWorklet",
        [context](jsi::Runtime &runtime) -> jsi::Value { return true; });

    installFunction(
        "runOnMainThread", JSI_FUNC_SIGNATURE {
          // Otherwise we need to call the worklet edition of the function from
          // within the worklet thread
          auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
          std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
          argsWrapper.reserve(count);

          for (size_t i = 0; i < count; i++) {
            argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
          }

          // Is this called on the main thread?
          if (!context->isWorkletRuntime(runtime)) {
            // The we can just call it directly
            return workletDispatchers.callInMainRuntime(runtime, thisWrapper,
                                                        argsWrapper);
          }

          // Create simple dispatcher without promise since we don't support
          // promises on Android / Worklet runtime
          context->runOnJavascriptThread([=]() {
            // Call in main runtime
            workletDispatchers.callInMainRuntime(*context->getJsRuntime(),
                                                 thisWrapper, argsWrapper);
          });

          return jsi::Value::undefined();
        });

    installFunction(
        "runOnWorkletThread", JSI_FUNC_SIGNATURE {
          // Wrap the this object
          auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);

          // Wrap the arguments
          std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
          argsWrapper.reserve(count);
          for (size_t i = 0; i < count; i++) {
            argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
          }

          // Create promise as return value when running on the worklet runtime
          return react::createPromiseAsJSIValue(
              runtime, [context, thisWrapper, argsWrapper, workletDispatchers,
                        count](jsi::Runtime &runtime,
                               std::shared_ptr<react::Promise> promise) {
                // Is this called on the main thread?
                if (context->isWorkletRuntime(runtime)) {
                  // The we can just call it directly
                  jsi::Value *args = new jsi::Value[argsWrapper.size()];
                  for (int i = 0; i < argsWrapper.size(); ++i) {
                    args[i] = JsiWrapper::unwrap(runtime, argsWrapper[i]);
                  }

                  try {
                    auto retVal = workletDispatchers.callInWorkletRuntime(
                        runtime, thisWrapper, argsWrapper);
                    promise->resolve(retVal);

                  } catch (...) {
                    promise->reject("Error");
                  }
                  delete[] args;

                } else {
                  // Run on the Worklet thread
                  context->runOnWorkletThread([=]() {
                    // Dispatch and resolve / reject promise
                    auto workletRuntime = &context->getWorkletRuntime();
                    // Prepare result
                    try {
                      auto retVal = workletDispatchers.callInWorkletRuntime(
                          *workletRuntime, thisWrapper, argsWrapper);

                      auto retValWrapper =
                          JsiWrapper::wrap(*workletRuntime, retVal);
                      context->runOnJavascriptThread([=]() {
                        promise->resolve(JsiWrapper::unwrap(
                            *context->getJsRuntime(), retValWrapper));
                      });

                    } catch (...) {
                      promise->reject("error");
                    }
                  });
                }
              });
        });
  }

private:
  /**
   Creates the worklet call dispatchers that can be called on a separapte
   runtime
   @param context Context to create dispatchers in
   @param runtime Calling runtime
   @param function Function to create dispatchers for
   @param closure Function's closure
   */
  JsiWorkletDispatchers createWorkletFunction(JsiWorkletContext *context,
                                              jsi::Runtime &runtime,
                                              const jsi::Value &function,
                                              const jsi::Value &closure) {

    if (!closure.isObject()) {
      context->raiseError("Expected an object for the closure parameter.");
      return {};
    }

    if (!function.isObject() ||
        !function.asObject(runtime).isFunction(runtime)) {
      context->raiseError(
          "Expected a function for the worklet function parameter.");
      return {};
    }

    auto funcPtr = std::make_shared<jsi::Function>(
        function.asObject(runtime).asFunction(runtime));

    // Let us try to install the function in the worklet context
    auto code =
        funcPtr->getPropertyAsFunction(runtime, "toString")
            .callWithThis(runtime, (jsi::Function &)*funcPtr, nullptr, 0)
            .asString(runtime)
            .utf8(runtime);

    auto workletPtr = std::make_shared<jsi::Function>(
        context->evaluteJavascriptInWorkletRuntime(code)
            .asObject(runtime)
            .asFunction(runtime));

    auto callInWorkletRuntime =
        createCallerWithClosure(context, runtime, workletPtr, closure);
    auto callInMainRuntime =
        createCallerWithClosure(context, runtime, funcPtr, closure);

    return JsiWorkletDispatchers{callInWorkletRuntime, callInMainRuntime};
  }

  /**
   Creates dispatchers for the worklet function
   @param context Context to create dispatchers in
   @param runtime Calling runtime
   @param functionPtr Function to create dispatchers for
   @param closure Function's closure
   */
  JsiWorkletDispatcher
  createCallerWithClosure(JsiWorkletContext *context, jsi::Runtime &runtime,
                          std::shared_ptr<jsi::Function> functionPtr,
                          const jsi::Value &closure) {
    // Wrap the closure
    auto closureWrapper = JsiWrapper::wrap(runtime, closure);

    // Return a caller function wrapper for the worklet
    return [context, functionPtr, closureWrapper](
               jsi::Runtime &runtime, std::shared_ptr<JsiWrapper> thisWrapper,
               const std::vector<std::shared_ptr<JsiWrapper>> &arguments)
               -> jsi::Value {
      // Add the closure as the first parameter to the calling argument
      size_t size = arguments.size() + 1;
      jsi::Value *args = new jsi::Value[size];

      // Add context as first argument
      args[0] = JsiWrapper::unwrap(runtime, closureWrapper);

      // Add the rest of the arguments
      for (size_t i = 1; i < size; i++) {
        args[i] = JsiWrapper::unwrap(runtime, arguments.at(i - 1));
      }

      jsi::Value retVal;
      try {
        retVal = functionPtr->call(runtime,
                                   static_cast<const jsi::Value *>(args), size);
      } catch (const jsi::JSError &err) {
        context->raiseError(err.getMessage().c_str());
      } catch (const std::exception &err) {
        context->raiseError(err.what());
      } catch (const std::runtime_error &err) {
        context->raiseError(err.what());
      } catch (...) {
        context->raiseError(
            "An unknown error occurred when calling dispatch function.");
      }

      delete[] args;

      return retVal;
    };
  }
};
} // namespace RNWorklet
