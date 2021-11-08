#pragma once

#include <JsiHostObject.h>
#include <JsiWrapper.h>
#include <jsi/jsi.h>

namespace RNWorklet {
using namespace facebook;
class JsiWrapper;

class JsiArrayWrapper : public RNJsi::JsiHostObject,
                        public std::enable_shared_from_this<JsiArrayWrapper>,
                        public JsiWrapper {
public:
  /**
   * Constructs a new array wrapper
   * @param runtime In runtime
   * @param value Value to wrap
   * @param parent Parent wrapper object
   */
  JsiArrayWrapper(jsi::Runtime &runtime, const jsi::Value &value,
                  JsiWrapper *parent)
      : JsiWrapper(runtime, value, parent) {
    // Install some functions and properties for arrays
    installReadonlyProperty("length", [this](jsi::Runtime &rt) -> jsi::Value {
      return (double)_array.size();
    });

    installFunction(
        "push", JSI_FUNC_SIGNATURE {
          // Push all arguments to the array
          auto lastIndex = _array.size();
          for (size_t i = 0; i < count; i++) {
            std::string indexString = std::to_string(lastIndex++);
            _array.push_back(JsiWrapper::wrap(runtime, arguments[i], this));
          }
          notifyListeners();
          return (double)_array.size();
        });

    installFunction(
        "pop", JSI_FUNC_SIGNATURE {
          // Pop last element from array
          if (_array.empty()) {
            return jsi::Value::undefined();
          }
          auto lastEl = _array.at(_array.size() - 1);
          _array.pop_back();
          notifyListeners();
          return JsiWrapper::unwrap(runtime, lastEl);
        });

    installFunction(
        "forEach", JSI_FUNC_SIGNATURE {
          // Enumerate and call back
          auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
          jsi::Value thisArg = count > 1 ? arguments[1].asObject(runtime)
                                         : jsi::Value::undefined();
          for (size_t i = 0; i < _array.size(); i++) {
            if (thisArg.isUndefined()) {
              callbackFn.call(runtime,
                              JsiWrapper::unwrap(runtime, _array.at(i)));
            } else {
              callbackFn.callWithThis(runtime, thisValue.asObject(runtime),
                                      JsiWrapper::unwrap(runtime, _array.at(i)),
                                      thisArg);
            }
          }
          return jsi::Value::undefined();
        });

    installFunction(
        "map", JSI_FUNC_SIGNATURE {
          // Enumerate and return
          auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
          jsi::Value thisArg = count > 1 ? arguments[1].asObject(runtime)
                                         : jsi::Value::undefined();

          auto result = jsi::Array(runtime, _array.size());
          for (size_t i = 0; i < _array.size(); i++) {
            jsi::Value retVal;
            if (thisArg.isUndefined()) {
              retVal = callbackFn.call(
                  runtime, JsiWrapper::unwrap(runtime, _array.at(i)));
            } else {
              retVal = callbackFn.callWithThis(
                  runtime, thisValue.asObject(runtime),
                  JsiWrapper::unwrap(runtime, _array.at(i)), thisArg);
            }
            result.setValueAtIndex(runtime, i, retVal);
          }
          return result;
        });

    installFunction(
        "filter", JSI_FUNC_SIGNATURE {
          // Filter array
          auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
          jsi::Value thisArg = count > 1 ? arguments[1].asObject(runtime)
                                         : jsi::Value::undefined();

          std::vector<std::shared_ptr<JsiWrapper>> result;

          for (size_t i = 0; i < _array.size(); i++) {
            jsi::Value retVal;
            if (thisArg.isUndefined()) {
              retVal = callbackFn.call(
                  runtime, JsiWrapper::unwrap(runtime, _array.at(i)));
            } else {
              retVal = callbackFn.callWithThis(
                  runtime, thisValue.asObject(runtime),
                  JsiWrapper::unwrap(runtime, _array.at(i)), thisArg);
            }
            if (retVal.getBool() == true) {
              result.push_back(_array.at(i));
            }
          }
          auto returnValue = jsi::Array(runtime, result.size());
          for (size_t i = 0; i < result.size(); i++) {
            returnValue.setValueAtIndex(
                runtime, i, JsiWrapper::unwrap(runtime, result.at(i)));
          }
          return returnValue;
        });

    installFunction(
        "toString", JSI_FUNC_SIGNATURE {
          return jsi::String::createFromUtf8(runtime, toString(runtime));
        });
  }

  /**
   * Overridden getValue method
   * @param runtime Calling runtime
   * @return jsi::Value representing this array
   */
  jsi::Value getValue(jsi::Runtime &runtime) override {
    return jsi::Object::createFromHostObject(runtime, shared_from_this());
  }

  /**
   * Overridden setValue method
   * @param runtime Calling runtime
   * @param value Value to set
   */
  void setValue(jsi::Runtime &runtime, const jsi::Value &value) override {
    assert(value.isObject());
    auto object = value.asObject(runtime);
    assert(object.isArray(runtime));
    auto array = object.asArray(runtime);

    size_t size = array.size(runtime);
    _array.resize(size);

    for (size_t i = 0; i < size; i++) {
      _array[i] =
          JsiWrapper::wrap(runtime, array.getValueAtIndex(runtime, i), this);
    }
  }

  /**
   * Overridden jsi::HostObject set property method
   * @param rt Runtime
   * @param name Name of value to set
   * @param value Value to set
   */
  void set(jsi::Runtime &rt, const jsi::PropNameID &name,
           const jsi::Value &value) override {
    // Do nada - an array set method is never called
  }

  /**
   * Overridden jsi::HostObject get property method. Returns functions from
   * the map of functions.
   * @param runtime Runtime
   * @param name Name of value to get
   * @return Value
   */
  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override {
    auto nameStr = name.utf8(runtime);
    if (!nameStr.empty() &&
        std::all_of(nameStr.begin(), nameStr.end(), ::isdigit)) {
      // Return property by index
      auto index = std::stoi(nameStr.c_str());
      auto prop = _array[index];
      return JsiWrapper::unwrap(runtime, prop);
    }
    // Return super JsiHostObject's get
    return JsiHostObject::get(runtime, name);
  }

  /**
   * Returns the array as a string
   * @param runtime Runtime to return in
   * @return Array as string
   */
  std::string toString(jsi::Runtime &runtime) override {
    std::string retVal = "";
    // Return array contents
    for (size_t i = 0; i < _array.size(); i++) {
      auto str = _array.at(i)->toString(runtime);
      retVal += (i > 0 ? ", " : "") + str;
    }
    return retVal;
  }

private:
  std::vector<std::shared_ptr<JsiWrapper>> _array;
};
} // namespace RNWorklet
