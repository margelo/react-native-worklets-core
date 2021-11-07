#include <JsiDescribe.h>
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

        if (count == 0) {
          return _context->raiseError("createWorklet expects 1-2 parameters.");
        }

        if (!arguments[0].isObject()) {
          return _context->raiseError("1 parameter should be an object.");
        }

        if (!arguments[1].isObject()) {
          return _context->raiseError("2 parameter should be a function.");
        }

        if (!arguments[1].asObject(runtime).isFunction(runtime)) {
          return _context->raiseError("2 parameter should be a function.");
        }

        // Get the active context
        auto activeContext = getContext(
            count == 3 && arguments[2].isString()
                ? arguments[2].asString(runtime).utf8(runtime).c_str()
                : nullptr);

        // Install the worklet
        auto worklet =
            activeContext->createWorklet(runtime, arguments[0], arguments[1]);

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
