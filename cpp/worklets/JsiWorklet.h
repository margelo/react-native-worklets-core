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
 */
class JsiWorklet : public JsiHostObject {
public:
  JsiWorklet(std::shared_ptr<JsiWorkletContext> context, jsi::Runtime &runtime,
             const jsi::Value &function, const jsi::Value &closure): _context(context) {
    // Make sure we call this from the main runtime
    if (context->isWorkletRuntime(runtime)) {
      jsi::detail::throwJSError(
          runtime, "A worklet must be created from the main runtime.");
      return;
    }

    // Create the worklet call function
    _dispatchers =
        createWorkletFunction(context, runtime, function, closure);
  }
  
  ~JsiWorklet() {
    delete _dispatchers;
  }

  // Returns true for worklets
  JSI_PROPERTY_GET(isWorklet) { return true; };
  
  JSI_PROPERTY_GET(context) { return jsi::Object::createFromHostObject(runtime, _context); };

  JSI_HOST_FUNCTION(call) {
    // Otherwise we need to call the worklet edition of the function from
    // within the worklet thread
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
    argsWrapper.reserve(count);

    for (size_t i = 0; i < count; i++) {
      argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
    }

    // Is this called on the main thread?
    if (!_context->isWorkletRuntime(runtime)) {
      // The we can just call it directly
      return _dispatchers->callInMainRuntime(runtime, thisWrapper, argsWrapper);
    }

    // Create simple dispatcher without promise since we don't support
    // promises on Android / Worklet runtime
    _context->runOnJavascriptThread([=]() {
      // Call in main runtime
      _dispatchers->callInMainRuntime(*_context->getJsRuntime(), thisWrapper, argsWrapper);
    });

    return jsi::Value::undefined();
  };

  JSI_HOST_FUNCTION(callAsync) {
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
        runtime, [this, thisWrapper, argsWrapper, count](jsi::Runtime &runtime,
                         std::shared_ptr<react::Promise> promise) {
                           
        // Is this called from the worklet thread?
        if (_context->isWorkletRuntime(runtime)) {
          // The we can just call it directly
          jsi::Value *args = new jsi::Value[argsWrapper.size()];
          for (int i = 0; i < argsWrapper.size(); ++i) {
            args[i] = JsiWrapper::unwrap(runtime, argsWrapper[i]);
          }

          try {
            auto retVal = _dispatchers->callInWorkletRuntime(
                runtime, thisWrapper, argsWrapper);
            
            promise->resolve(retVal);

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

          delete[] args;

        } else {
          // Run on the Worklet thread
          _context->runOnWorkletThread([=]() {
            // Dispatch and resolve / reject promise
            auto workletRuntime = &_context->getWorkletRuntime();
            
            // Prepare result
            try {
              
              auto retVal = _dispatchers->callInWorkletRuntime(
                  *workletRuntime, thisWrapper, argsWrapper);

              auto retValWrapper =
                  JsiWrapper::wrap(*workletRuntime, retVal);
              
              _context->runOnJavascriptThread([=]() {
                promise->resolve(JsiWrapper::unwrap(
                    *_context->getJsRuntime(), retValWrapper));
              });

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

          });
        }
      });
  };
  
  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorklet, call),
                       JSI_EXPORT_FUNC(JsiWorklet, callAsync))
  
  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(JsiWorklet, context),
                              JSI_EXPORT_PROP_GET(JsiWorklet, isWorklet))

private:
  /**
   Creates the worklet call dispatchers that can be called on a separapte
   runtime
   @param context Context to create dispatchers in
   @param runtime Calling runtime
   @param function Function to create dispatchers for
   @param closure Function's closure
   */
  JsiWorkletDispatchers* createWorkletFunction(std::shared_ptr<JsiWorkletContext> context,
                                               jsi::Runtime &runtime,
                                               const jsi::Value &function,
                                               const jsi::Value &closure) {

    if (!closure.isUndefined() && !closure.isObject()) {
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

    return new JsiWorkletDispatchers{callInWorkletRuntime, callInMainRuntime};
  }

  /**
   Creates dispatchers for the worklet function
   @param context Context to create dispatchers in
   @param runtime Calling runtime
   @param functionPtr Function to create dispatchers for
   @param closure Function's closure
   */
  JsiWorkletDispatcher
  createCallerWithClosure(std::shared_ptr<JsiWorkletContext> context,
                          jsi::Runtime &runtime,
                          std::shared_ptr<jsi::Function> functionPtr,
                          const jsi::Value &closure) {
    // Wrap the closure
    auto closureWrapper = JsiWrapper::wrap(runtime, closure);

    // Return a caller function wrapper for the worklet
    return [context, functionPtr, closureWrapper](
               jsi::Runtime &runtime, std::shared_ptr<JsiWrapper> thisWrapper,
               const std::vector<std::shared_ptr<JsiWrapper>> &arguments)
               -> jsi::Value {
      
      // Create arguments
      size_t size = arguments.size();
      jsi::Value *args = new jsi::Value[size];

      // Add the rest of the arguments
      for (size_t i = 0; i < size; i++) {
        args[i] = JsiWrapper::unwrap(runtime, arguments.at(i));
      }

      auto unwrappedClosure = JsiWrapper::unwrap(runtime, closureWrapper);
              
      jsi::Value retVal;
      
      if(!unwrappedClosure.isObject()) {
        retVal = functionPtr->call(runtime,
                                   static_cast<const jsi::Value *>(args),
                                   size);
      } else {
        retVal = functionPtr->callWithThis(runtime,
                                           unwrappedClosure.asObject(runtime),
                                           static_cast<const jsi::Value *>(args),
                                           size);
      }
      
      
      delete[] args;

      return retVal;
    };
  }
  
  std::shared_ptr<JsiWorkletContext> _context;
  JsiWorkletDispatchers *_dispatchers;
};
} // namespace RNWorklet
