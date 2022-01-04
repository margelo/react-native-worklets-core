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

/**
 Encapsulates a runnable function. A runnable function is a function
 that exists in both the main JS runtime and as an installed function
 in the worklet runtime. The worklet object exposes some methods and props
 for handling this. The worklet exists in a given worklet context.
 */
class JsiWorklet : public JsiHostObject {
public:
  JsiWorklet(std::shared_ptr<JsiWorkletContext> context, jsi::Runtime &runtime,
             const jsi::Value &function, const jsi::Value &closure)
      : _context(context) {

    // Make sure we call this from the main runtime
    if (context->isWorkletRuntime(runtime)) {
      jsi::detail::throwJSError(
          runtime, "A worklet must be created from the main runtime.");
      return;
    }

    // Install function in worklet runtime
    installInWorkletRuntime(context, runtime, function, closure);
  }
  
  ~JsiWorklet() {
    _jsFunction.reset();
    _workletFunction.reset();
  }

  // Returns true for worklets
  JSI_PROPERTY_GET(isWorklet) { return true; };

  // Returns the context of the worklet
  JSI_PROPERTY_GET(context) {
    return jsi::Object::createFromHostObject(runtime, _context);
  };

  JSI_HOST_FUNCTION(callInJSContext) {
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
    argsWrapper.reserve(count);

    for (size_t i = 0; i < count; i++) {
      argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
    }

    // Is this called on the javascript thread?
    if (!_context->isWorkletRuntime(runtime)) {
      // The we can just call it directly
      try {
        return callInJsRuntime(argsWrapper);
      } catch (const jsi::JSError &err) {
        throw err;
      } catch (const std::exception &err) {
        jsi::detail::throwJSError(runtime, err.what());
      } catch (const std::runtime_error &err) {
        jsi::detail::throwJSError(runtime, err.what());
      } catch (...) {
        jsi::detail::throwJSError(
            runtime, "An unknown error occurred when calling the worklet.");
      }
      return jsi::Value::undefined();
    }

    // Create simple dispatcher without promise since we don't support
    // promises on Android in the Javascript runtime
    _context->runOnJavascriptThread([=]() {
      // Call in main runtime
      auto runtime = _context->getJsRuntime();
      try {
        callInJsRuntime(argsWrapper);
      } catch (const jsi::JSError &err) {
        throw err;
      } catch (const std::exception &err) {
        jsi::detail::throwJSError(*runtime, err.what());
      } catch (const std::runtime_error &err) {
        jsi::detail::throwJSError(*runtime, err.what());
      } catch (...) {
        jsi::detail::throwJSError(
            *runtime, "An unknown error occurred when calling the worklet.");
      }
    });

    return jsi::Value::undefined();
  };

  JSI_HOST_FUNCTION(callInWorkletContext) {
    // Wrap the arguments
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
    argsWrapper.reserve(count);
    for (size_t i = 0; i < count; i++) {
      argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
    }

    // Create promise as return value when running on the worklet runtime
    return react::createPromiseAsJSIValue(
        runtime,
        [this, argsWrapper, count](jsi::Runtime &runtime,
                                   std::shared_ptr<react::Promise> promise) {
          // Is this called from the worklet thread?
          if (_context->isWorkletRuntime(runtime)) {
            // The we can just call it directly
            try {
              promise->resolve(callInWorkletRuntime(argsWrapper));

            } catch (const jsi::JSError &err) {
              promise->reject(err.getMessage().c_str());
            } catch (const std::exception &err) {
              promise->reject(err.what());
            } catch (const std::runtime_error &err) {
              promise->reject(err.what());
            } catch (...) {
              promise->reject(
                  "An unknown error occurred when calling the worklet.");
            }

          } else {
            // Run on the Worklet thread
            _context->runOnWorkletThread([=]() {
              // Dispatch and resolve / reject promise
              auto workletRuntime = &_context->getWorkletRuntime();

              // Prepare result
              try {
                auto retValWrapper = JsiWrapper::wrap(
                    *workletRuntime, callInWorkletRuntime(argsWrapper));

                _context->runOnJavascriptThread([=]() {
                  promise->resolve(JsiWrapper::unwrap(*_context->getJsRuntime(),
                                                      retValWrapper));
                });

              } catch (const jsi::JSError &err) {
                auto errorMessage = err.getMessage().c_str();
                _context->runOnJavascriptThread(
                    [=]() { promise->reject(errorMessage); });
              } catch (const std::exception &err) {
                auto errorMessage = err.what();
                _context->runOnJavascriptThread(
                    [=]() { promise->reject(errorMessage); });
              } catch (const std::runtime_error &err) {
                auto errorMessage = err.what();
                _context->runOnJavascriptThread(
                    [=]() { promise->reject(errorMessage); });
              } catch (...) {
                _context->runOnJavascriptThread([=]() {
                  promise->reject(
                      "An unknown error occurred when calling the worklet.");
                });
              }
            });
          }
        });
  };

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorklet, callInJSContext),
                       JSI_EXPORT_FUNC(JsiWorklet, callInWorkletContext))

  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(JsiWorklet, context),
                              JSI_EXPORT_PROP_GET(JsiWorklet, isWorklet))

private:
  /**
   Installs the worklet function into the worklet runtime
   */
  void installInWorkletRuntime(std::shared_ptr<JsiWorkletContext> context,
                               jsi::Runtime &runtime,
                               const jsi::Value &function,
                               const jsi::Value &closure) {
    if (!closure.isUndefined() && !closure.isObject()) {
      context->raiseError("Expected an object for the closure parameter.");
      return;
    }

    if (!function.isObject() ||
        !function.asObject(runtime).isFunction(runtime)) {
      context->raiseError(
          "Expected a function for the worklet function parameter.");
      return;
    }

    // Save pointer to function
    _jsFunction = std::make_shared<jsi::Function>(
        function.asObject(runtime).asFunction(runtime));

    // Create closure wrapper so it will be accessible across runtimes
    _closureWrapper = JsiWrapper::wrap(runtime, closure);

    // Install function
    auto func = function.asObject(runtime).asFunction(runtime);

    // Let us try to install the function in the worklet context
    auto code = func.getPropertyAsFunction(runtime, "toString")
                    .callWithThis(runtime, func, nullptr, 0)
                    .asString(runtime)
                    .utf8(runtime);

    _workletFunction = std::make_shared<jsi::Function>(
        context->evaluteJavascriptInWorkletRuntime(code)
            .asObject(runtime)
            .asFunction(runtime));
  }

  /**
   Calls the worklet in the worklet runtime
   */
  jsi::Value callInWorkletRuntime(
      const std::vector<std::shared_ptr<JsiWrapper>> &argsWrapper) {
    auto runtime = &_context->getWorkletRuntime();

    // Unwrap closure
    auto unwrappedClosure = JsiWrapper::unwrap(*runtime, _closureWrapper);

    // Create arguments
    size_t size = argsWrapper.size();
    std::vector<jsi::Value> args(size);

    // Add the rest of the arguments
    for (size_t i = 0; i < size; i++) {
      args[i] = JsiWrapper::unwrap(*runtime, argsWrapper.at(i));
    }

    // Prepare return value
    jsi::Value retVal;

    if (!unwrappedClosure.isObject()) {
      retVal = _workletFunction->call(
          *runtime, static_cast<const jsi::Value *>(args.data()),
          argsWrapper.size());
    } else {
      retVal = _workletFunction->callWithThis(
          *runtime, unwrappedClosure.asObject(*runtime),
          static_cast<const jsi::Value *>(args.data()), argsWrapper.size());
    }

    return retVal;
  }

  /**
   Calls the worklet in the Javascript runtime
   */
  jsi::Value
  callInJsRuntime(const std::vector<std::shared_ptr<JsiWrapper>> &argsWrapper) {
    auto runtime = _context->getJsRuntime();

    // Unwrap closure
    auto unwrappedClosure = JsiWrapper::unwrap(*runtime, _closureWrapper);

    // Create arguments
    size_t size = argsWrapper.size();
    std::vector<jsi::Value> args(size);

    // Add the rest of the arguments
    for (size_t i = 0; i < size; i++) {
      args[i] = JsiWrapper::unwrap(*runtime, argsWrapper.at(i));
    }

    // Prepare return value
    jsi::Value retVal;

    if (!unwrappedClosure.isObject()) {
      // Call without this object
      retVal = _jsFunction->call(*runtime,
                                 static_cast<const jsi::Value *>(args.data()),
                                 argsWrapper.size());
    } else {
      // Call with closure as this object
      retVal = _jsFunction->callWithThis(
          *runtime, unwrappedClosure.asObject(*runtime),
          static_cast<const jsi::Value *>(args.data()), argsWrapper.size());
    }

    return retVal;
  }

  std::shared_ptr<jsi::Function> _jsFunction;
  std::shared_ptr<jsi::Function> _workletFunction;
  std::shared_ptr<JsiWorkletContext> _context;
  std::shared_ptr<JsiWrapper> _closureWrapper;
};
} // namespace RNWorklet
