#include "JsiWrapper.h"
#include <JsiDescribe.h>
#include <JsiWorklet.h>

namespace RNWorklet {
using namespace facebook;

jsi::Value JsiWrapper::getValue(jsi::Runtime &runtime) {
  std::unique_lock<std::mutex> lock(*_readWriteMutex);
  switch (_type) {
  case JsiWrapperType::Undefined:
    return jsi::Value::undefined();
  case JsiWrapperType::Null:
    return jsi::Value::null();
  case JsiWrapperType::Bool:
    return jsi::Value((bool)_boolValue);
  case JsiWrapperType::Number:
    return jsi::Value((double)_numberValue);
  case JsiWrapperType::String:
    return jsi::String::createFromUtf8(runtime, _stringValue);
  case JsiWrapperType::HostObject:
    return getHostObjectValue(runtime);
  case JsiWrapperType::HostFunction:
    return getHostFunctionValue(runtime);
  case JsiWrapperType::Worklet:
    return getWorkletValue(runtime);
  default:
    jsi::detail::throwJSError(runtime, "Value type not supported.");
    return jsi::Value::undefined();
  }
}

void JsiWrapper::setValueInternal(jsi::Runtime &runtime,
                                  const jsi::Value &value) {
  if (value.isUndefined()) {
    _type = JsiWrapperType::Undefined;
  } else if (value.isNull()) {
    _type = JsiWrapperType::Null;
  } else if (value.isBool()) {
    _type = JsiWrapperType::Bool;
    _boolValue = value.getBool();
  } else if (value.isNumber()) {
    _type = JsiWrapperType::Number;
    _numberValue = value.getNumber();
  } else if (value.isString()) {
    _type = JsiWrapperType::String;
    _stringValue = value.asString(runtime).utf8(runtime);
  } else if (value.isObject()) {
    setObjectValue(runtime, value);
  } else {
    jsi::detail::throwJSError(runtime, "Value type not supported.");
  }
}

void JsiWrapper::setValue(jsi::Runtime &runtime, const jsi::Value &value) {
  std::unique_lock<std::mutex> lock(*_readWriteMutex);
  setValueInternal(runtime, value);
  // Notify changes
  notifyParent();
}

void JsiWrapper::setObjectValue(jsi::Runtime &runtime,
                                const jsi::Value &value) {
  auto object = value.asObject(runtime);
  if (object.isArray(runtime)) {
    setArrayValue(runtime, object);
  } else if (object.isHostObject(runtime)) {
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

void JsiWrapper::setArrayValue(jsi::Runtime &runtime, jsi::Object &obj) {
  _type = JsiWrapperType::Array;
  auto array = obj.asArray(runtime);
  _arrayObject = std::make_shared<JsiArrayWrapper>(runtime, array, this,
                                                   _functionResolver);
}

void JsiWrapper::setArrayBufferValue(jsi::Runtime &runtime, jsi::Object &obj) {
  jsi::detail::throwJSError(
      runtime, "Array buffers are not supported as shared values.");
}

void JsiWrapper::setObjectValue(jsi::Runtime &runtime, jsi::Object &obj) {
  _type = JsiWrapperType::Object;
  _properties.clear();
  auto propNames = obj.getPropertyNames(runtime);
  for (size_t i = 0; i < propNames.size(runtime); i++) {
    auto nameString =
        propNames.getValueAtIndex(runtime, i).asString(runtime).utf8(runtime);
    auto value = obj.getProperty(runtime, nameString.c_str());

    _properties.emplace(
        nameString, std::make_shared<JsiWrapper>(
                        runtime, value, this, _functionResolver, nameString));
  }
}

void JsiWrapper::setHostObjectValue(jsi::Runtime &runtime, jsi::Object &obj) {
  _type = JsiWrapperType::HostObject;
  _hostObject = obj.asHostObject(runtime);
}

void JsiWrapper::setHostFunctionValue(jsi::Runtime &runtime, jsi::Object &obj) {
  _type = JsiWrapperType::HostFunction;
  _hostFunction = std::make_shared<jsi::HostFunctionType>(
      obj.asFunction(runtime).getHostFunction(runtime));
}

void JsiWrapper::setFunctionValue(jsi::Runtime &runtime,
                                  const jsi::Value &value) {
  // Try to resolve as worklet
  if (_functionResolver != nullptr) {
    // The resolver will try to resolve the function as a worklet. If it is not
    // a worklet it will return a function that throws a warning if we try to
    // use it on another runtime
    _hostFunction =
        std::make_shared<jsi::HostFunctionType>((*_functionResolver)(value));
    _type = JsiWrapperType::Worklet;
    // We accept regular functions as well - since they might be copied but
    // never used. So let us check if this is a worklet before we try to get the
    // hash
    if (JsiWorklet::isWorklet(runtime, value)) {
      _workletHash = JsiWorklet::getWorkletHash(runtime, value);
    } else {
      _workletHash = -1;
    }
    return;
  }
  jsi::detail::throwJSError(runtime,
                            "Regular javascript functions not supported.");
}

jsi::Value JsiWrapper::getHostObjectValue(jsi::Runtime &runtime) {
  return jsi::Object::createFromHostObject(runtime, _hostObject);
}

jsi::Value JsiWrapper::getHostFunctionValue(jsi::Runtime &runtime) {
  return jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, _nameHint), 0,
      *_hostFunction.get());
}

jsi::Value JsiWrapper::getWorkletValue(jsi::Runtime &runtime) {
  auto retVal = jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, _nameHint), 0,
      *_hostFunction.get());

  // We might have a regular function here, let us test for the workletHash
  // value
  if (_workletHash > 1.0) {
    JsiWorklet::setWorkletHash(runtime, retVal, _workletHash);
    JsiWorklet::markAsWorklet(runtime, retVal);
  }
  return retVal;
}

std::string JsiWrapper::toString(jsi::Runtime &runtime) {
  switch (_type) {
  case JsiWrapperType::Undefined:
    return "undefined";
  case JsiWrapperType::Null:
    return "NULL";
  case JsiWrapperType::Bool:
    return std::to_string(_boolValue);
  case JsiWrapperType::Number:
    return std::to_string((double)_numberValue);
  case JsiWrapperType::String:
    return _stringValue;
  case JsiWrapperType::HostObject:
    return "[hostobject]";
  case JsiWrapperType::HostFunction:
    return "[hostfunction]";
  case JsiWrapperType::Worklet:
    return "[worklet]";
  case JsiWrapperType::Array:
    return _arrayObject->toString(runtime);
  case JsiWrapperType::Object:
    return "[Object object]";
  default:
    jsi::detail::throwJSError(runtime, "Value type not supported.");
    return "[unsupported]";
  }
}
} // namespace RNWorklet
