#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

static const char *WorkletObjectProxyName = "__createWorkletObjectProxy";

enum JsiWrapperType {
  Undefined,
  Null,
  Bool,
  Number,
  String,
  Array,
  Object,
  Promise,
  HostObject,
  HostFunction
};

class JsiWrapper {
public:
  /**
   * Constructor - called from static members
   * @param runtime Calling runtime
   * @param value Value to wrap
   * @param parent Optional parent wrapper
   * @paran type Type of wrapper
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent,
             JsiWrapperType type)
      : JsiWrapper(parent) {
    _type = type;
  }

  /**
   * Constructor - called from static members
   * @param runtime Calling runtime
   * @param value Value to wrap
   * @param parent Optional parent wrapper
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent)
      : JsiWrapper(parent) {}

  /**
   * Returns a wrapper for the a jsi value
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @return A new JsiWrapper
   */
  static std::shared_ptr<JsiWrapper> wrap(jsi::Runtime &runtime,
                                          const jsi::Value &value) {
    return JsiWrapper::wrap(runtime, value, nullptr);
  }

  /**
   * Returns the value as a javascript value on the provided runtime
   * @param runtime Runtime
   * @param wrapper Wrapper to get value for
   * @return A new js value in the provided runtime with the wrapped value
   */
  static jsi::Value unwrap(jsi::Runtime &runtime,
                          std::shared_ptr<JsiWrapper> wrapper) {
    // Defensive: if wrapper is null (e.g. bad/empty closure), just return
    // undefined instead of crashing on a null dereference.
    if (!wrapper) {
      return jsi::Value::undefined();
    }
    return wrapper->getValue(runtime);
  }

  /**
   Non-static variant of unwrapAsProxyOrValue
   @param runtime Runtime
   */
  jsi::Value unwrap(jsi::Runtime &runtime) { return this->getValue(runtime); }

  /**
   * Returns the value as a javascript proxy or value depending on need on the
   * provided runtime
   * @param runtime Runtime
   * @param wrapper Wrapper to get value for
   * @return A new js value in the provided runtime with the wrapped value
   */
  static jsi::Value unwrapAsProxyOrValue(jsi::Runtime &runtime,
                                        std::shared_ptr<JsiWrapper> wrapper) {
    if (!wrapper) {
      return jsi::Value::undefined();
    }
    return wrapper->getAsProxyOrValue(runtime);
  }

  /**
   Unwraps to a proxy if needed, to value if not.
   */
  jsi::Value unwrapAsProxyOrValue(jsi::Runtime &runtime) {
    return getAsProxyOrValue(runtime);
  }

  /**
   * Updates the value from a JS value
   * @param runtime runtime for the value
   * @param value Value to set
   */
  virtual void updateValue(jsi::Runtime &runtime, const jsi::Value &value);

  /**
   Returns true if the value provided can be contained in the wrapped instance.
   */
  virtual bool canUpdateValue(jsi::Runtime &runtime, const jsi::Value &value);

  /**
   * @return The type of wrapper
   */
  JsiWrapperType getType() { return _type; }

  /**
   * Returns the object as a string
   */
  virtual std::string toString(jsi::Runtime &runtime);

  /**
   * Add listener
   * @param listener callback to notify
   * @return id of the listener - used for removing the listener
   */
  size_t addListener(std::shared_ptr<std::function<void()>> listener) {
    auto id = _listenerId++;
    _listeners.emplace(id, listener);
    return id;
  }

  /**
   * Remove listener
   * @param listenerId id of listener to remove
   */
  void removeListener(size_t listenerId) { _listeners.erase(listenerId); }

protected:
  /**
   * Returns a wrapper for the value
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @param parent Parent wrapper (for nested hierarchies)
   * @return A new JsiWrapper
   */
  static std::shared_ptr<JsiWrapper>
  wrap(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent);

  /**
   * Call to notify parent that something has changed
   */
  void notify() {
    if (_parent != nullptr) {
      _parent->notify();
    }
    notifyListeners();
  }

  /**
   Returns self as a proxy object or a regular value, depending on wether the
   value needs to be a proxy.
   */
  virtual jsi::Value getAsProxyOrValue(jsi::Runtime &runtime) {
    return getValue(runtime);
  }

  /**
   * Update the type
   * @param type Type to set
   */
  void setType(JsiWrapperType type) { _type = type; }

  /**
   * @return The parent object
   */
  JsiWrapper *getParent() { return _parent; }

  /**
   Calls the Function and returns its value. This function will call the
   correct overload based on the this value
   */
  jsi::Value callFunction(jsi::Runtime &runtime, const jsi::Function &func,
                          const jsi::Value &thisValue,
                          const jsi::Value *arguments, size_t count);

protected:
  /**
   * Sets the value from a JS value
   * @param runtime runtime for the value
   * @param value Value to set
   */
  virtual void setValue(jsi::Runtime &runtime, const jsi::Value &value);

  /**
   * Returns the value as a javascript value on the provided runtime
   * @param runtime Runtime to set value in
   * @return A new js value in the provided runtime with the wrapped value
   */
  virtual jsi::Value getValue(jsi::Runtime &runtime);

  /**
   Creates a proxy for the host object so that we can make the runtime trust
   that this is a real JS object
   */
  jsi::Value getObjectAsProxy(jsi::Runtime &runtime,
                              std::shared_ptr<jsi::HostObject> hostObj) {
    auto createObjProxy =
        runtime.global().getProperty(runtime, WorkletObjectProxyName);
    if (createObjProxy.isUndefined()) {
      // Install worklet proxy helper into runtime
      static std::string code =
          "function (obj) {"
          "  return new Proxy(obj, {"
          "    getOwnPropertyDescriptor: function () {"
          "      return { configurable: true, enumerable: true, writable: true "
          "};"
          "    },"
          " set: function(target, prop, value) { return Reflect.set(target, "
          "prop, value); },"
          " get: function(target, prop) { return Reflect.get(target, prop); }"
          "  });"
          "}";

      auto codeBuffer =
          std::make_shared<const jsi::StringBuffer>("(" + code + "\n)");
      createObjProxy =
          runtime.evaluateJavaScript(codeBuffer, WorkletObjectProxyName);
      runtime.global().setProperty(runtime, WorkletObjectProxyName,
                                   createObjProxy);
    }

    auto createProxyFunc = createObjProxy.asObject(runtime).asFunction(runtime);
    auto retVal = createProxyFunc.call(
        runtime, jsi::Object::createFromHostObject(runtime, hostObj));

    return retVal;
  }

private:
  /**
   * Notify listeners that the value has changed
   */
  void notifyListeners() {
    for (auto listener : _listeners) {
      (*listener.second)();
    }
  }

  /**
   * Base Constructor
   * @param parent Parent wrapper
   */
  explicit JsiWrapper(JsiWrapper *parent) : _parent(parent) {
    _readWriteMutex = new std::mutex();
  }

  std::mutex *_readWriteMutex;
  JsiWrapper *_parent;

  JsiWrapperType _type;

  bool _boolValue;
  double _numberValue;
  std::string _stringValue;

  size_t _listenerId = 1000;
  std::map<size_t, std::shared_ptr<std::function<void()>>> _listeners;
};

} // namespace RNWorklet
