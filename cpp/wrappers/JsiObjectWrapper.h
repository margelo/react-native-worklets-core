#pragma once

#include <JsiWrapper.h>
#include <jsi/jsi.h>

namespace RNWorklet {
using namespace facebook;

class JsiObjectWrapper : public jsi::HostObject,
                         public std::enable_shared_from_this<JsiObjectWrapper>,
                         public JsiWrapper {
public:
  /**
   * Constructor
   * @param runtime Calling runtie
   * @param value value to wrap
   * @param parent optional parent wrapper
   */
  JsiObjectWrapper(jsi::Runtime &runtime, const jsi::Value &value,
                   JsiWrapper *parent)
      : JsiWrapper(runtime, value, parent) {}

  /**
   * Overridden setValue
   * @param runtime Value's runtime
   * @param value Value to set
   */
  void setValue(jsi::Runtime &runtime, const jsi::Value &value) override {
    assert(value.isObject());
    auto object = value.asObject(runtime);
    assert(!object.isArray(runtime));

    if (object.isHostObject(runtime)) {
      setHostObjectValue(runtime, object);
    } else if (object.isFunction(runtime)) {
      auto func = object.asFunction(runtime);
      if (func.isHostFunction(runtime)) {
        setHostFunctionValue(runtime, object);
      } else {
        setFunctionValue(runtime, value);
      }
    } else if (object.isArrayBuffer(runtime)) {
      setArrayBufferValue(runtime, object);
    } else {
      setObjectValue(runtime, object);
    }
  }

  /**
   * Overridden get value where we convert from the internal representation to
   * a jsi value
   * @param runtime Runtime to convert value into
   * @return Value converted to a jsi::Value
   */
  jsi::Value getValue(jsi::Runtime &runtime) override {
    switch (getType()) {
    case JsiWrapperType::HostObject:
      return jsi::Object::createFromHostObject(runtime, _hostObject);
    case JsiWrapperType::HostFunction:
      return jsi::Function::createFromHostFunction(
          runtime, jsi::PropNameID::forUtf8(runtime, "fn"), 0,
          *_hostFunction.get());
    case JsiWrapperType::Object:
      return jsi::Object::createFromHostObject(runtime, shared_from_this());
    default:
      jsi::detail::throwJSError(runtime, "Value type not supported.");
      return jsi::Value::undefined();
    }
  }

  /**
   * jsi::HostObject's overridden set function
   * @param runtime Runtime to set value from
   * @param name Name of property to set
   * @param value Value to set
   */
  void set(jsi::Runtime &runtime, const jsi::PropNameID &name,
           const jsi::Value &value) override {
    auto nameStr = name.utf8(runtime);
    if (_properties.count(nameStr) == 0) {
      _properties.emplace(nameStr, JsiWrapper::wrap(runtime, value, this));
    } else {
      _properties.at(nameStr)->updateValue(runtime, value);
    }
  }

  /**
   * jsi::HostObject's overridden get function
   * @param runtime Runtime to return value to
   * @param name Name of property to return
   * @return Property value or undefined.
   */
  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override {
    auto nameStr = name.utf8(runtime);
    if (nameStr == "toString") {
      return jsi::Function::createFromHostFunction(
          runtime, jsi::PropNameID::forUtf8(runtime, "toString"), 0,
          [this](jsi::Runtime &runtime, const jsi::Value &thisValue,
                 const jsi::Value *arguments, size_t count) -> jsi::Value {
            return jsi::String::createFromUtf8(runtime, toString(runtime));
          });
    }

    if (_properties.count(nameStr) != 0) {
      auto prop = _properties.at(nameStr);
      return JsiWrapper::unwrap(runtime, prop);
    }

    return jsi::Value::undefined();
  }

  /**
   * jsi::HostObject's overridden getPropertyNames
   * @param runtime Calling runtime
   * @return List of property names
   */
  std::vector<jsi::PropNameID>
  getPropertyNames(jsi::Runtime &runtime) override {
    std::vector<jsi::PropNameID> retVal;
    for (auto it = _properties.begin(); it != _properties.end(); it++) {
      retVal.push_back(jsi::PropNameID::forUtf8(runtime, it->first));
    }
    return retVal;
  }

  /**
   * Overridden toString method
   * @param runtime Runtime to return to
   * @return String description of object
   */
  std::string toString(jsi::Runtime &runtime) override {
    switch (getType()) {
    case JsiWrapperType::HostObject:
      return "[Object hostObject]";
    case JsiWrapperType::HostFunction:
      return "[Function hostFunction]";
    case JsiWrapperType::Object:
      return "[Object object]";
    default:
      jsi::detail::throwJSError(runtime, "Value type not supported.");
      return "[unsupported]";
    }
  }

private:
  void setArrayBufferValue(jsi::Runtime &runtime, jsi::Object &obj) {
    jsi::detail::throwJSError(
        runtime, "Array buffers are not supported as shared values.");
  }

  void setObjectValue(jsi::Runtime &runtime, jsi::Object &obj) {
    setType(JsiWrapperType::Object);
    _properties.clear();
    auto propNames = obj.getPropertyNames(runtime);
    for (size_t i = 0; i < propNames.size(runtime); i++) {
      auto nameString =
          propNames.getValueAtIndex(runtime, i).asString(runtime).utf8(runtime);

      auto value = obj.getProperty(runtime, nameString.c_str());
      _properties.emplace(nameString, JsiWrapper::wrap(runtime, value, this));
    }
  }

  void setHostObjectValue(jsi::Runtime &runtime, jsi::Object &obj) {
    setType(JsiWrapperType::HostObject);
    _hostObject = obj.asHostObject(runtime);
  }

  void setHostFunctionValue(jsi::Runtime &runtime, jsi::Object &obj) {
    setType(JsiWrapperType::HostFunction);
    _hostFunction = std::make_shared<jsi::HostFunctionType>(
        obj.asFunction(runtime).getHostFunction(runtime));
  }

  void setFunctionValue(jsi::Runtime &runtime, const jsi::Value &value) {
    jsi::detail::throwJSError(
        runtime, "Regular javascript functions cannot be shared.");
  }

  std::map<std::string, std::shared_ptr<JsiWrapper>> _properties;
  std::shared_ptr<jsi::HostFunctionType> _hostFunction;
  std::shared_ptr<jsi::HostObject> _hostObject;
};
} // namespace RNWorklet
