#pragma once

#include <jsi/jsi.h>
#include <ReactCommon/TurboModuleUtils.h>

#include <JsiHostObject.h>
#include <JsiWorkletContext.h>
#include <JsiSharedValue.h>
#include <JsiDispatcher.h>
#include <JsiWorklet.h>

namespace RNWorklet {

    using namespace facebook;
    using namespace RNJsi;
    using namespace RNSkia;

    class JsiWorkletApi: public JsiHostObject {
    public:
        /**
         * Create the worklet API
         * @param context Worklet context
         * make sure only the runOnJsThread method is available.
         */
        JsiWorkletApi(JsiWorkletContext* context): _context(context) {
          
            installFunction("installWorklet", [this](jsi::Runtime & runtime,
                                                        const jsi::Value &thisValue,
                                                        const jsi::Value *arguments,
                                                        size_t count) -> jsi::Value {
                // Make sure this one is only called from the js runtime
                if(_context->isWorkletRuntime(runtime)) {
                    return _context->raiseError(
                        "installWorklet should only be called from the javascript runtime.");
                }

                // Make sure the function is a worklet
                if(!JsiWorklet::isWorklet(runtime, arguments[0])) {
                    return _context->raiseError("Expected a worklet, got a regular javascript function.");
                }
              
                // Let's see if we are launching this on the default context
                JsiWorkletContext* activeContext = getContext(count == 2 && arguments[1].isString() ?
                                                              arguments[1].asString(runtime).utf8(runtime).c_str() :
                                                              nullptr);
                
                // Install the worklet
                auto worklet = activeContext->getWorklet(arguments[0]);
              
                return jsi::Value::undefined();
            });
          
            installFunction("callbackToJavascript",
                [context](jsi::Runtime & runtime,
                const jsi::Value &thisValue,
                const jsi::Value *arguments,
                size_t count) -> jsi::Value {

                // Make sure this one is only called from the worklet runtime
                if(!context->isWorkletRuntime(runtime)) {
                  return context->raiseError("callbackToJavascript should only be called from the worklet runtime.");
                }

                // Make sure the function is a worklet
                if(!JsiWorklet::isWorklet(runtime, arguments[0])) {
                    return context->raiseError("Expected a worklet, got a regular javascript function");
                }

                // Get the js function represented by the worklet
                auto workletHash = JsiWorklet::getWorkletHash(runtime, arguments[0].asObject(runtime)
                        .asFunction(runtime));

                // get the original function
                auto originalFunction = context->getWorkletsCache().at(workletHash).original;

                // Now let's create a dispatcher function for calling with arguments.
                // This will also be the return value from this function so that we can
                // call using the following syntax in Javascript:
                //
                //   Worklets.runOnJsThread(somefunction)(args);
                //
                return jsi::Function::createFromHostFunction(
                        runtime,
                        jsi::PropNameID::forUtf8(runtime, "fn"),
                        0,
                        [context, originalFunction](jsi::Runtime & runtime,
                                                         const jsi::Value &thisValue,
                                                         const jsi::Value *arguments,
                                                         size_t count) -> jsi::Value  {

                    auto functionPtr = [originalFunction](jsi::Runtime& rt, const jsi::Value& thisVal, const jsi::Value* args, size_t count) -> jsi::Value {
                        if(thisVal.isObject()) {
                            return originalFunction->callWithThis(rt, thisVal.asObject(rt), args, count);
                        } else {
                            return originalFunction->call(rt, args, count);
                        }
                    };

                    // calling this
                    auto thisWrapper = std::make_shared<JsiWrapper>(runtime, thisValue);

                    // Copy arguments into wrappers
                    std::vector<std::shared_ptr<JsiWrapper>> argWrappers;
                    argWrappers.reserve(count);

                    for(size_t i=0; i<count; i++) {
                        argWrappers.push_back(std::make_shared<JsiWrapper>(runtime, arguments[i]));
                    }

                    // Create dispatcher
                    auto dispatcher = JsiDispatcher::createDispatcher(
                        *context->getJsRuntime(), thisWrapper, functionPtr, argWrappers,
                        nullptr, nullptr);

                    // Run on the Javascript thread
                    context->runOnJavascriptThread(dispatcher);

                    return jsi::Value::undefined();
                });
            });

            installFunction("runWorklet",
                [context, this](jsi::Runtime &runtime,
                const jsi::Value&,
                const jsi::Value *arguments,
                size_t count) -> jsi::Value {

                // Make sure this one is only called from the js runtime
                if(context->isWorkletRuntime(runtime)) {
                    return context->raiseError(
                        "runWorklet should only be called from the javascript runtime.");
                }

                // Make sure the function is a worklet
                if(!JsiWorklet::isWorklet(runtime, arguments[0])) {
                    return context->raiseError("Expected a worklet, got a regular javascript function.");
                }

                // Let's see if we are launching this on the default context
                JsiWorkletContext* activeContext = getContext(count == 2 && arguments[1].isString() ?
                                                              arguments[1].asString(runtime).utf8(runtime).c_str() :
                                                              nullptr);
                                
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
                        [activeContext, worklet](jsi::Runtime &runtime,
                           const jsi::Value &thisValue,
                           const jsi::Value *arguments,
                            size_t count) -> jsi::Value {

                    // Get the worklet runtime
                    auto workletRuntime = &activeContext->getWorkletRuntime();

                    // Wrap the calling this
                    auto thisWrapper = std::make_shared<JsiWrapper>(runtime, thisValue);

                    // Copy arguments into wrappers
                    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
                    argsWrapper.reserve(count);

                    for(size_t i=0; i<count; i++) {
                        argsWrapper.push_back(std::make_shared<JsiWrapper>(runtime, arguments[i]));
                    }

                    // Create promise
                    return react::createPromiseAsJSIValue(runtime,
                        [activeContext, thisWrapper, worklet, workletRuntime, argsWrapper](
                            jsi::Runtime &runtime,
                            std::shared_ptr<react::Promise> promise) {

                        // Create dispatcher
                        auto dispatcher = JsiDispatcher::createDispatcher(*workletRuntime,
                              thisWrapper, worklet, argsWrapper,
                              [promise, activeContext] (std::shared_ptr<JsiWrapper> wrapper) {
                                  // Resolve promise on the main js runtime / thread
                                  activeContext->runOnJavascriptThread([promise, wrapper, activeContext]() {
                                      promise->resolve(JsiWrapper::unwrap(*activeContext->getJsRuntime(), wrapper));
                                  });
                              },
                              [promise, activeContext](const char* err) {
                                  // Reject promise on the main js runtime / thread
                                  activeContext->runOnJavascriptThread([promise, err]() {
                                      promise->reject(err);
                                  });
                              });

                        // Run on the Worklet thread
                        activeContext->runOnWorkletThread(dispatcher);
                    });
                });
            });

            installFunction("createSharedValue", JSI_FUNC_SIGNATURE {

                if(context->isWorkletRuntime(runtime)) {
                    return context->raiseError(
                            "createSharedValue should only be called from the javascript runtime.");
                }

                return jsi::Object::createFromHostObject(
                        *context->getJsRuntime(),
                        std::make_shared<JsiSharedValue>(arguments[0], context));
            });
        }

    private:
      
      JsiWorkletContext* getContext(const char* name) {
          // Let's see if we are launching this on the default context
          std::string contextName = "default";
          if(name != nullptr) {
              contextName = name;
          }
        
          // We support multiple worklet contexts - so we'll find or create
          // the one requested, or use the default one if none is requested.
          // The default worklet context is also a separate context.
          if(_contexts.count(contextName) == 0) {
              // Create new context
              _contexts.emplace(contextName, _context);
          }
          return &_contexts.at(contextName);
      }
      
      JsiWorkletContext* _context;
      std::map<std::string, JsiWorkletContext> _contexts;
    };
}
