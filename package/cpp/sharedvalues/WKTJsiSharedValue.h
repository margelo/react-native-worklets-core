#pragma once

#include <jsi/jsi.h>
#include <map>

#include <memory>

#include "WKTJsiDispatcher.h"
#include "WKTJsiHostObject.h"
#include "WKTJsiWorkletContext.h"
#include "WKTJsiWrapper.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiSharedValue : public JsiHostObject {
public:
  /**
   Constructs a shared value - which is a wrapped value that can be accessed in
   a thread safe across two javascript runtimes.
   */
  explicit JsiSharedValue(const jsi::Value &value)
      : _valueWrapper(JsiWrapper::wrap(
            *JsiWorkletContext::getDefaultInstance()->getJsRuntime(), value,
            nullptr, true)) {}

  /**
    Destructor
   */
  ~JsiSharedValue() { _valueWrapper = nullptr; }

  JSI_HOST_FUNCTION(toString) {
    return jsi::String::createFromUtf8(runtime,
                                       _valueWrapper->toString(runtime));
  }

  JSI_PROPERTY_GET(value) { return _valueWrapper->unwrap(runtime); }

  JSI_PROPERTY_SET(value) {
    if (_valueWrapper->canUpdateValue(runtime, value)) {
      _valueWrapper->updateValue(runtime, value);
    } else {
      _valueWrapper = JsiWrapper::wrap(runtime, value, nullptr, true);
    }
  }

  JSI_HOST_FUNCTION(addListener) {
    // Verify arguments
    if (arguments[0].isUndefined() || arguments[0].isNull() ||
        arguments[0].isObject() == false ||
        arguments[0].asObject(runtime).isFunction(runtime) == false) {
      throw jsi::JSError(runtime,
                         "addListener expects a function as its parameter.");
    }

    // Wrap the callback into a dispatcher with error handling. This
    // callback will always be called on the main js thread/runtime so we
    // can just use values directly.
    auto functionToCall = std::make_shared<jsi::Function>(
        arguments[0].asObject(runtime).asFunction(runtime));

    auto functionPtr =
        [functionToCall](jsi::Runtime &rt, const jsi::Value &thisVal,
                         const jsi::Value *args, size_t count) -> jsi::Value {
      if (thisVal.isObject()) {
        return functionToCall->callWithThis(rt, thisVal.asObject(rt), args,
                                            count);
      } else {
        return functionToCall->call(rt, args, count);
      }
    };

    // Do not Wrap this Value - replace with undefined
    auto thisValuePtr =
        JsiWrapper::wrap(runtime, jsi::Value::undefined(), nullptr, true);

    auto dispatcher = JsiDispatcher::createDispatcher(
        runtime, thisValuePtr, functionPtr, nullptr,
        [&runtime, this](const char *err) {
          JsiWorkletContext::getCurrent(runtime)->invokeOnJsThread(
              [err](jsi::Runtime &runtime) {
                throw jsi::JSError(runtime, err);
              });
        });

    // Set up the callback to run on the correct runtime thread.
    auto callback = std::make_shared<std::function<void()>>(dispatcher);

    auto listenerId = _valueWrapper->addListener(callback);

    // Return functionPtr for removing the observer
    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forUtf8(runtime, "unsubscribe"), 0,
        [=](jsi::Runtime &runtime, const jsi::Value &thisValue,
            const jsi::Value *arguments, size_t count) -> jsi::Value {
          _valueWrapper->removeListener(listenerId);
          return jsi::Value::undefined();
        });
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiSharedValue, toString),
                       JSI_EXPORT_FUNC(JsiSharedValue, addListener))

  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(JsiSharedValue, value))
  JSI_EXPORT_PROPERTY_SETTERS(JSI_EXPORT_PROP_SET(JsiSharedValue, value))

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
