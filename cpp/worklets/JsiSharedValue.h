#pragma once

#include <map>

#include <JsiDispatcher.h>
#include <JsiHostObject.h>
#include <JsiWorkletContext.h>
#include <JsiWrapper.h>
#include <jsi/jsi.h>

namespace RNWorklet {

using namespace facebook;
using namespace RNJsi;

class JsiSharedValue : public JsiHostObject {
public:
  /**
   Constructs a shared value - which is a wrapped value that can be accessed in
   a thread safe across two javascript runtimes.
   */
  JsiSharedValue(const jsi::Value &value, JsiWorkletContext *context)
      : _valueWrapper(
            std::make_shared<JsiWrapper>(*context->getJsRuntime(), value)) {

    installProperty(
        "value",
        [this](jsi::Runtime &rt) -> jsi::Value {
          // Arrays and objects are handled specifically to
          // support settings values and getting notifications in
          // child values
          return JsiWrapper::unwrap(rt, _valueWrapper);
        },
        [this](jsi::Runtime &rt, const jsi::Value &value) {
          // Update the internal value
          _valueWrapper->setValue(rt, value);
          return jsi::Value::undefined();
        });

    installFunction(
        "addListener", JSI_FUNC_SIGNATURE {
          // This functionPtr should only be callable from the js runtime
          if (context->isWorkletRuntime(runtime)) {
            jsi::detail::throwJSError(runtime,
                                      "addListener can only be called from the "
                                      "main Javascript code.");
          }

          // Verify arguments
          if (arguments[0].isUndefined() || arguments[0].isNull() ||
              arguments[0].isObject() == false ||
              arguments[0].asObject(runtime).isFunction(runtime) == false) {
            jsi::detail::throwJSError(
                runtime, "addListener expects a function as its parameter.");
          }

          // Wrap the callback into a dispatcher with error handling. This
          // callback will always be called on the main js thread/runtime so we
          // can just use values directly.
          auto functionToCall = std::make_shared<jsi::Function>(
              arguments[0].asObject(runtime).asFunction(runtime));
          auto functionPtr = [functionToCall](jsi::Runtime &rt,
                                              const jsi::Value &thisVal,
                                              const jsi::Value *args,
                                              size_t count) -> jsi::Value {
            if (thisVal.isObject()) {
              return functionToCall->callWithThis(rt, thisVal.asObject(rt),
                                                  args, count);
            } else {
              return functionToCall->call(rt, args, count);
            }
          };

          // Do not Wrap this Value
          auto thisValuePtr =
              std::make_shared<JsiWrapper>(runtime, jsi::Value::undefined());

          auto dispatcher = JsiDispatcher::createDispatcher(
              runtime, thisValuePtr, functionPtr, nullptr,
              [&runtime, context](const char *err) {
                context->runOnJavascriptThread([err, &runtime]() {
                  jsi::detail::throwJSError(runtime, err);
                });
              });

          // Set up the callback to run on the correct runtime thread.
          auto callback =
              std::make_shared<std::function<void()>>([context, dispatcher]() {
                context->runOnJavascriptThread(dispatcher);
              });

          auto listenerId = _valueWrapper->addListener(callback);

          // Return functionPtr for removing the observer
          return jsi::Function::createFromHostFunction(
              runtime, jsi::PropNameID::forUtf8(runtime, "unsubscribe"), 0,
              [=](jsi::Runtime &runtime, const jsi::Value &thisValue,
                  const jsi::Value *arguments, size_t count) -> jsi::Value {
                _valueWrapper->removeListener(listenerId);
                return jsi::Value::undefined();
              });
        });
  }

  /**
    Destructor
   */
  ~JsiSharedValue() { _valueWrapper = nullptr; }

  /**
   * Add listener
   * @param listener callback to notify
   * @return id of the listener - used for removing the listener
   */
  size_t addListener(std::shared_ptr<std::function<void()>> listener) {
    return _valueWrapper->addListener(listener);
  }

  /**
   * Remove listener
   * @param listenerId id of listener to remove
   */
  void removeListener(size_t listenerId) {
    _valueWrapper->removeListener(listenerId);
  }

private:
  std::shared_ptr<JsiWrapper> _valueWrapper;
};
} // namespace RNWorklet
