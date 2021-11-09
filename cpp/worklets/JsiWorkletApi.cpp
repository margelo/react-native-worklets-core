#include <JsiDescribe.h>
#include <JsiWorklet.h>
#include <JsiWorkletApi.h>

namespace RNWorklet {
using namespace facebook;

JsiWorkletApi::JsiWorkletApi(JsiWorkletContext *context) : _context(context) {

  installFunction(
      "createWorklet",
      [this](jsi::Runtime &runtime, const jsi::Value &thisValue,
             const jsi::Value *arguments, size_t count) -> jsi::Value {
        // Make sure this one is only called from the js runtime
        if (_context->isWorkletRuntime(runtime)) {
          return _context->raiseError("createWorklet should only be called "
                                      "from the javascript runtime.");
        }

        if (count != 2) {
          return _context->raiseError("createWorklet expects 2 parameters: "
                                      "Worklet function and worklet closure.");
        }

        // Get the active context
        auto activeContext = getContext(
            count == 3 && arguments[2].isString()
                ? arguments[2].asString(runtime).utf8(runtime).c_str()
                : nullptr);

        // Install the worklet into the worklet runtime
        return jsi::Object::createFromHostObject(
            runtime, std::make_shared<JsiWorklet>(activeContext, runtime,
                                                  arguments[1], arguments[0]));

        /*       auto function =
        arguments[1].asObject(runtime).asFunction(runtime); auto worklet =
            activeContext->createWorklet(runtime, arguments[0], function);

        // Now we can create a caller function
        return jsi::Function::createFromHostFunction(
            runtime, jsi::PropNameID::forUtf8(runtime, "fn"), 0,
            [activeContext,
             worklet](jsi::Runtime &runtime, const jsi::Value &thisValue,
                      const jsi::Value *arguments, size_t count) -> jsi::Value {
              // Get the worklet runtime
              auto workletRuntime = &activeContext->getWorkletRuntime();

              // Wrap the calling this as undefined - we don't care about it
              auto thisWrapper =
                  JsiWrapper::wrap(runtime, jsi::Value::undefined());

              // Copy arguments into wrappers
              std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
              argsWrapper.reserve(count);

              for (size_t i = 0; i < count; i++) {
                argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
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
            });*/
      });

  installFunction(
      "createWorkletContext", JSI_FUNC_SIGNATURE {
        if (count == 0) {
          jsi::detail::throwJSError(
              runtime,
              "createWorkletContext expects the context name parameter.");
        }

        if (!arguments[0].isString()) {
          jsi::detail::throwJSError(
              runtime,
              "createWorkletContext expects the context name parameter.");
        }

        auto nameStr = arguments[0].asString(runtime).utf8(runtime);
        if (_contexts.count(nameStr) == 0) {
          _contexts.emplace(nameStr,
                            std::make_shared<JsiWorkletContext>(_context));
        } else {
          jsi::detail::throwJSError(runtime,
                                    "A context with this name already exists.");
        }
        return jsi::Value::undefined();
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

JsiWorkletContext *JsiWorkletApi::getContext(const char *name) {
  // Let's see if we are launching this on the default context
  if (name == nullptr) {
    return _context;
  }

  // Throw error if the context hasn't been created yet.
  if (_contexts.count(name) == 0) {
    _context->raiseError(
        std::runtime_error("A context with this name does not exist. Use the "
                           "createWorkletContext method to create it."));
    return nullptr;
  }
  return _contexts.at(name).get();
}

} // namespace RNWorklet
