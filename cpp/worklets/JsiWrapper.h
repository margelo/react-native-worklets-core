#pragma once

#include <map>
#include <mutex>
#include <vector>

#include <jsi/jsi.h>

#include <JsiArrayWrapper.h>

#define JSIFN(X, FN)                                                           \
  jsi::Function::createFromHostFunction(                                       \
      runtime, jsi::PropNameID::forUtf8(runtime, X), 0,                        \
      [=](jsi::Runtime & runtime, const jsi::Value &thisValue,                 \
          const jsi::Value *arguments, size_t count) -> jsi::Value FN)

namespace RNWorklet {
using namespace facebook;

enum JsiWrapperType {
  Undefined,
  Null,
  Bool,
  Number,
  String,
  Array,
  Object,
  HostObject,
  HostFunction,
  Worklet
};

using JsiFunctionResolver =
    std::function<jsi::HostFunctionType(const jsi::Value &)>;

class JsiWrapper : public jsi::HostObject {
public:
  /**
   * Constructs new wrapper with a value in the provided runtime
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value)
      : JsiWrapper(nullptr, nullptr, "") {
    setValueInternal(runtime, value);
  }

  /**
   * Constructs new wrapper with a value in the provided runtime
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @param resolver Worklet/function resolver
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value,
             std::shared_ptr<JsiFunctionResolver> resolver)
      : JsiWrapper(nullptr, resolver, "") {
    setValueInternal(runtime, value);
  }

  /**
   * Parent / child constructor
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @param parent Parent wrapper (for nested hierarcies)
   * @param nameHint name of element (in parent wrapper)
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent,
             const std::string &nameHint)
      : JsiWrapper(parent, nullptr, nameHint) {
    setValueInternal(runtime, value);
  }

  /**
   * Constructs new wrapper with a value in the provided runtime
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @param resolver Worklet/function resolver
   * @param nameHint name of element (in parent wrapper)
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value,
             std::shared_ptr<JsiFunctionResolver> resolver,
             const std::string &nameHint)
      : JsiWrapper(nullptr, resolver, nameHint) {
    setValueInternal(runtime, value);
  }

  /**
   * Parent / child constructor
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @param parent Parent wrapper (for nested hierarchies)
   * @param resolver Worklet/function resolver
   * @param nameHint name of element (in parent wrapper)
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent,
             std::shared_ptr<JsiFunctionResolver> resolver,
             const std::string &nameHint)
      : JsiWrapper(parent, resolver, nameHint) {
    setValueInternal(runtime, value);
  }

  /**
   * Destructor
   */
  ~JsiWrapper() {
    delete _readWriteMutex;
    _buffer = nullptr;
    _hostObject = nullptr;
    _hostFunction = nullptr;
    _functionResolver = nullptr;
    _parent = nullptr;
  }

  /**
   * Returns the value as a javascript value on the provided runtime
   * @param runtime Runtime
   * @param wrapper Wrapper to get value for
   * @return A new js value in the provided runtime with the wrapped value
   */
  static jsi::Value unwrap(jsi::Runtime &runtime,
                           std::shared_ptr<JsiWrapper> wrapper) {
    if (wrapper->getType() == JsiWrapperType::Array) {
      return jsi::Object::createFromHostObject(runtime,
                                               wrapper->getArrayObject());
    }
    if (wrapper->getType() == JsiWrapperType::Object) {
      return jsi::Object::createFromHostObject(runtime, wrapper);
    }
    return wrapper->getValue(runtime);
  }

  /**
   * Returns a wrapper for the value
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @param parent Parent wrapper (for nested hierarchies)
   * @param resolver Worklet/function resolver
   * @param nameHint name of element (in parent wrapper)
   * @return A new JsiWrapper
   */
  static std::shared_ptr<JsiWrapper>
  wrap(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent,
       std::shared_ptr<JsiFunctionResolver> resolver,
       const std::string &nameHint) {
    return std::make_shared<JsiWrapper>(runtime, value, parent, resolver,
                                        nameHint);
  }

  /**
   * Sets the value from a JS value
   * @param runtime runtime for the value
   * @param value Value to set
   */
  void setValue(jsi::Runtime &runtime, const jsi::Value &value);

  /**
   * Overridden jsi::HostObject set property method
   * @param rt Runtime
   * @param name Name of value to set
   * @param value Value to set
   */
  void set(jsi::Runtime &rt, const jsi::PropNameID &name,
           const jsi::Value &value) override {
    if (_type == JsiWrapperType::Object) {
      _properties.at(name.utf8(rt))->setValue(rt, value);
    }
  };

  /**
   * Overridden jsi::HostObject get property method. Returns functions from
   * the map of functions.
   * @param runtime Runtime
   * @param name Name of value to get
   * @return Value
   */
  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override {
    auto nameStr = name.utf8(runtime);
    if (nameStr == "toString") {
      return JSIFN("toString", {
        return jsi::String::createFromUtf8(runtime, toString(runtime));
      });
    }

    if (_type == JsiWrapperType::Object) {
      if (_properties.count(nameStr) != 0) {
        auto prop = _properties.at(nameStr);
        return JsiWrapper::unwrap(runtime, prop);
      }
    }
    return jsi::Value::undefined();
  }

  /**
   * Returns property names
   * @param rt Runtime
   * @return Property names vector
   */
  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override {
    std::vector<jsi::PropNameID> retVal;
    if (_type == JsiWrapperType::Object || _type == JsiWrapperType::Array) {
      for (auto it = _properties.begin(); it != _properties.end(); it++) {
        retVal.push_back(jsi::PropNameID::forUtf8(rt, it->first));
      }
    }
    return retVal;
  }

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

  /**
   * Notify listeners that the value has changed
   */
  void notifyListeners() {
    for (auto listener : _listeners) {
      (*listener.second)();
    }
  }

  /**
   * @return The type of wrapper
   */
  JsiWrapperType getType() { return _type; }

  /**
   * Returns the object as a string
   */
  std::string toString(jsi::Runtime &runtime);

protected:
  /**
   * Call to notify parent that something has changed
   */
  void notifyParent() {
    if (_parent != nullptr) {
      _parent->notifyParent();
    }
    notifyListeners();
  }

  /**
   * Returns the inner array object
   */
  std::shared_ptr<JsiArrayWrapper> getArrayObject() { return _arrayObject; }

private:
  /**
   * Base Constructor
   * @param parent Parent wrapper
   */
  JsiWrapper(JsiWrapper *parent,
             std::shared_ptr<JsiFunctionResolver> functionResolver,
             const std::string &nameHint)
      : _parent(parent), _functionResolver(functionResolver),
        _nameHint(nameHint) {
    _readWriteMutex = new std::mutex();
  }

  /**
   * Sets the value from a JS value
   * @param runtime runtime for the value
   * @param value Value to set
   */
  void setValueInternal(jsi::Runtime &runtime, const jsi::Value &value);

  /**
   * Returns the value as a javascript value on the provided runtime
   * @param runtime Runtime to set value in
   * @return A new js value in the provided runtime with the wrapped value
   */
  jsi::Value getValue(jsi::Runtime &runtime);

  void setArrayValue(jsi::Runtime &runtime, jsi::Object &obj);
  void setArrayBufferValue(jsi::Runtime &runtime, jsi::Object &obj);
  void setObjectValue(jsi::Runtime &runtime, jsi::Object &obj);
  void setHostObjectValue(jsi::Runtime &runtime, jsi::Object &obj);
  void setHostFunctionValue(jsi::Runtime &runtime, jsi::Object &obj);
  void setFunctionValue(jsi::Runtime &runtime, const jsi::Value &value);
  void setObjectValue(jsi::Runtime &runtime, const jsi::Value &value);

  jsi::Value getHostObjectValue(jsi::Runtime &runtime);
  jsi::Value getHostFunctionValue(jsi::Runtime &runtime);
  jsi::Value getWorkletValue(jsi::Runtime &runtime);

  std::mutex *_readWriteMutex;
  JsiWrapper *_parent;

  JsiWrapperType _type;

  bool _boolValue;
  double _numberValue;
  std::string _stringValue;
  std::map<std::string, std::shared_ptr<JsiWrapper>> _properties;
  std::shared_ptr<uint8_t *> _buffer;
  std::shared_ptr<jsi::HostObject> _hostObject;
  std::shared_ptr<JsiArrayWrapper> _arrayObject;
  std::shared_ptr<jsi::HostFunctionType> _hostFunction;
  double _workletHash;
  std::string _nameHint = "fn";
  std::shared_ptr<JsiFunctionResolver> _functionResolver;

  size_t _listenerId = 1000;
  std::map<size_t, std::shared_ptr<std::function<void()>>> _listeners;
};

} // namespace RNWorklet
