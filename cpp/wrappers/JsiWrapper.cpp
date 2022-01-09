#include "JsiWrapper.h"
#include <JsiArrayWrapper.h>
#include <JsiObjectWrapper.h>

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
  default:
    jsi::detail::throwJSError(runtime, "Value type not supported.");
    return jsi::Value::undefined();
  }
}

std::shared_ptr<JsiWrapper> JsiWrapper::wrap(jsi::Runtime &runtime,
                                             const jsi::Value &value,
                                             JsiWrapper *parent) {
  std::shared_ptr<JsiWrapper> retVal = nullptr;

  if (value.isUndefined() || value.isNull() || value.isBool() ||
      value.isNumber() || value.isString()) {
    retVal = std::make_shared<JsiWrapper>(runtime, value, parent);
  } else if (value.isObject()) {
    if (value.asObject(runtime).isArray(runtime)) {
      retVal = std::make_shared<JsiArrayWrapper>(runtime, value, parent);
    } else {
      retVal = std::make_shared<JsiObjectWrapper>(runtime, value, parent);
    }
  }

  if (retVal == nullptr) {
    jsi::detail::throwJSError(runtime, "Value type not supported.");
    return nullptr;
  }

  retVal->setValue(runtime, value);
  return retVal;
}

void JsiWrapper::setValue(jsi::Runtime &runtime, const jsi::Value &value) {
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
  } else {
    jsi::detail::throwJSError(runtime, "Value type not supported.");
  }
}

void JsiWrapper::updateValue(jsi::Runtime &runtime, const jsi::Value &value) {
  std::unique_lock<std::mutex> lock(*_readWriteMutex);
  setValue(runtime, value);
  // Notify changes
  notify();
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
  default:
    jsi::detail::throwJSError(runtime, "Value type not supported.");
    return "[Unknown]";
  }
}

jsi::Value JsiWrapper::callFunction(jsi::Runtime & runtime,
                                    const jsi::Function &func,
                                    const jsi::Value &thisValue,
                                    const jsi::Value *arguments, size_t count) {
  if (thisValue.isUndefined()) {
    return func.call(runtime, arguments, count);
  } else {
    return func.callWithThis(runtime, thisValue.asObject(runtime), arguments, count);
  }
}

} // namespace RNWorklet
