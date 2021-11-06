#include <JsiWorkletApi.h>
#include <JsiDescribe.h>

namespace RNWorklet {
using namespace facebook;

JsiWorkletApi::JsiWorkletApi(JsiWorkletContext *context) : _context(context) {

  installFunction("createWorklet",
                  [this](jsi::Runtime &runtime, const jsi::Value &thisValue,
                         const jsi::Value *arguments, size_t count) -> jsi::Value {
    // Make sure this one is only called from the js runtime
    if (_context->isWorkletRuntime(runtime)) {
      return _context->raiseError("createWorklet should only be called "
                                  "from the javascript runtime.");
    }
    
    if(count == 0) {
      return _context->raiseError("createWorklet expects 1-2 parameters.");
    }
    
    if(!arguments[0].isObject()) {
      return _context->raiseError("1 parameter should be an object.");
    }
    
    if(!arguments[1].isObject()) {
      return _context->raiseError("2 parameter should be a function.");
    }
    
    if(!arguments[1].asObject(runtime).isFunction(runtime)) {
      return _context->raiseError("2 parameter should be a function.");
    }
    
    // Get the active context
    JsiWorkletContext *activeContext = getContext(
        count == 3 && arguments[2].isString()
            ? arguments[2].asString(runtime).utf8(runtime).c_str()
            : nullptr);
    
    // Install the worklet
    auto worklet = activeContext->createWorklet(runtime, arguments[0], arguments[1]);
    
    // Now we can create a caller function
    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forUtf8(runtime, "fn"), 0,
        [activeContext, worklet](jsi::Runtime &runtime, const jsi::Value &thisValue,
                                 const jsi::Value *arguments, size_t count) -> jsi::Value {
          
          // Get the worklet runtime
          auto workletRuntime = &activeContext->getWorkletRuntime();

          // Wrap the calling this as undefined - we don't care about it
          auto thisWrapper =
              std::make_shared<JsiWrapper>(runtime, jsi::Value::undefined());

          // Copy arguments into wrappers
          std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
          argsWrapper.reserve(count);

          for (size_t i = 0; i < count; i++) {
            argsWrapper.push_back(
                std::make_shared<JsiWrapper>(runtime, arguments[i]));
          }

          // Create promise
          return react::createPromiseAsJSIValue(
              runtime,
              [activeContext, thisWrapper, worklet, workletRuntime,
               argsWrapper](jsi::Runtime &runtime,
                            std::shared_ptr<react::Promise> promise) {
                // Create dispatcher
                auto dispatcher = JsiDispatcher::createDispatcher(
                    *workletRuntime, thisWrapper, worklet, argsWrapper,
                    [promise,
                     activeContext](std::shared_ptr<JsiWrapper> wrapper) {
                      // Resolve promise on the main js runtime / thread
                      activeContext->runOnJavascriptThread(
                          [promise, wrapper, activeContext]() {
                            promise->resolve(JsiWrapper::unwrap(
                                *activeContext->getJsRuntime(), wrapper));
                          });
                    },
                    [promise, activeContext](const char *err) {
                      // Reject promise on the main js runtime / thread
                      activeContext->runOnJavascriptThread(
                          [promise, err]() { promise->reject(err); });
                    });

                // Run on the Worklet thread
                activeContext->runOnWorkletThread(dispatcher);
              });
        });
  });
  
  installFunction(
      "installWorklet",
      [this](jsi::Runtime &runtime, const jsi::Value &thisValue,
             const jsi::Value *arguments, size_t count) -> jsi::Value {
        // Make sure this one is only called from the js runtime
        if (_context->isWorkletRuntime(runtime)) {
          return _context->raiseError("installWorklet should only be called "
                                      "from the javascript runtime.");
        }

        // Make sure the function is a worklet
        if (!JsiWorklet::isWorklet(runtime, arguments[0])) {
          return _context->raiseError(
              "Expected a worklet, got a regular javascript function.");
        }

        // Let's see if we are launching this on the default context
        JsiWorkletContext *activeContext = getContext(
            count == 2 && arguments[1].isString()
                ? arguments[1].asString(runtime).utf8(runtime).c_str()
                : nullptr);

        // Install the worklet
        auto worklet = activeContext->getWorklet(arguments[0]);

        return jsi::Value::undefined();
      });

  installFunction(
      "callbackToJavascript",
      [context](jsi::Runtime &runtime, const jsi::Value &thisValue,
                const jsi::Value *arguments, size_t count) -> jsi::Value {
        // Make sure this one is only called from the worklet runtime
        if (!context->isWorkletRuntime(runtime)) {
          return context->raiseError("callbackToJavascript should only be "
                                     "called from the worklet runtime.");
        }

        // Make sure the function is a worklet
        if (!JsiWorklet::isWorklet(runtime, arguments[0])) {
          return context->raiseError(
              "Expected a worklet, got a regular javascript function");
        }

        // Get the js function represented by the worklet
        auto workletHash = JsiWorklet::getWorkletHash(
            runtime, arguments[0].asObject(runtime).asFunction(runtime));

        // get the original function
        auto originalFunction =
            context->getWorkletsCache().at(workletHash).original;

        // Now let's create a dispatcher function for calling with arguments.
        // This will also be the return value from this function so that we can
        // call using the following syntax in Javascript:
        //
        //   Worklets.runOnJsThread(somefunction)(args);
        //
        return jsi::Function::createFromHostFunction(
            runtime, jsi::PropNameID::forUtf8(runtime, "fn"), 0,
            [context, originalFunction](
                jsi::Runtime &runtime, const jsi::Value &thisValue,
                const jsi::Value *arguments, size_t count) -> jsi::Value {
              auto functionPtr =
                  [originalFunction](
                      jsi::Runtime &rt, const jsi::Value &thisVal,
                      const jsi::Value *args, size_t count) -> jsi::Value {
                if (thisVal.isObject()) {
                  return originalFunction->callWithThis(
                      rt, thisVal.asObject(rt), args, count);
                } else {
                  return originalFunction->call(rt, args, count);
                }
              };

              // calling this
              auto thisWrapper =
                  std::make_shared<JsiWrapper>(runtime, thisValue);

              // Copy arguments into wrappers
              std::vector<std::shared_ptr<JsiWrapper>> argWrappers;
              argWrappers.reserve(count);

              for (size_t i = 0; i < count; i++) {
                argWrappers.push_back(
                    std::make_shared<JsiWrapper>(runtime, arguments[i]));
              }

              // Create dispatcher
              auto dispatcher = JsiDispatcher::createDispatcher(
                  *context->getJsRuntime(), thisWrapper, functionPtr,
                  argWrappers, nullptr, nullptr);

              // Run on the Javascript thread
              context->runOnJavascriptThread(dispatcher);

              return jsi::Value::undefined();
            });
      });

  installFunction(
      "runWorklet",
      [context, this](jsi::Runtime &runtime, const jsi::Value &,
                      const jsi::Value *arguments, size_t count) -> jsi::Value {
        // Make sure this one is only called from the js runtime
        if (context->isWorkletRuntime(runtime)) {
          return context->raiseError(
              "runWorklet should only be called from the javascript runtime.");
        }

        // Make sure the function is a worklet
        if (!JsiWorklet::isWorklet(runtime, arguments[0])) {
          return context->raiseError(
              "Expected a worklet, got a regular javascript function.");
        }

        // Let's see if we are launching this on the default context
        JsiWorkletContext *activeContext = getContext(
            count == 2 && arguments[1].isString()
                ? arguments[1].asString(runtime).utf8(runtime).c_str()
                : nullptr);

        // Get the worklet.
        auto worklet = activeContext->getWorklet(arguments[0]);

        // Now let's create a dispatcher function for calling with arguments.
        // This will also be the return value from this function so that we can
        // call using the following syntax in Javascript:
        //
        //   Worklets.runOnWorkletThread(somefunction)(args);
        //
        return jsi::Function::createFromHostFunction(
            runtime, jsi::PropNameID::forUtf8(runtime, "fn"), 0,
            [activeContext,
             worklet](jsi::Runtime &runtime, const jsi::Value &thisValue,
                      const jsi::Value *arguments, size_t count) -> jsi::Value {
              // Get the worklet runtime
              auto workletRuntime = &activeContext->getWorkletRuntime();

              auto str = RNJsi::JsiDescribe::describe(runtime, thisValue);
              // Wrap the calling this
              auto thisWrapper =
                  std::make_shared<JsiWrapper>(runtime, jsi::Value::undefined());

              // Copy arguments into wrappers
              std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
              argsWrapper.reserve(count);

              for (size_t i = 0; i < count; i++) {
                argsWrapper.push_back(
                    std::make_shared<JsiWrapper>(runtime, arguments[i]));
              }

              // Create promise
              return react::createPromiseAsJSIValue(
                  runtime,
                  [activeContext, thisWrapper, worklet, workletRuntime,
                   argsWrapper](jsi::Runtime &runtime,
                                std::shared_ptr<react::Promise> promise) {
                    // Create dispatcher
                    auto dispatcher = JsiDispatcher::createDispatcher(
                        *workletRuntime, thisWrapper, worklet, argsWrapper,
                        [promise,
                         activeContext](std::shared_ptr<JsiWrapper> wrapper) {
                          // Resolve promise on the main js runtime / thread
                          activeContext->runOnJavascriptThread(
                              [promise, wrapper, activeContext]() {
                                promise->resolve(JsiWrapper::unwrap(
                                    *activeContext->getJsRuntime(), wrapper));
                              });
                        },
                        [promise, activeContext](const char *err) {
                          // Reject promise on the main js runtime / thread
                          activeContext->runOnJavascriptThread(
                              [promise, err]() { promise->reject(err); });
                        });

                    // Run on the Worklet thread
                    activeContext->runOnWorkletThread(dispatcher);
                  });
            });
      });

  installFunction(
      "createSharedValue", JSI_FUNC_SIGNATURE {
        if (context->isWorkletRuntime(runtime)) {
          return context->raiseError("createSharedValue should only be called "
                                     "from the javascript runtime.");
        }

        return jsi::Object::createFromHostObject(
            *context->getJsRuntime(),
            std::make_shared<JsiSharedValue>(arguments[0], context));
      });
}
} // namespace RNWorklet
