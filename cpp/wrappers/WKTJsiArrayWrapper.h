#pragma once

#include <jsi/jsi.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "WKTJsiHostObject.h"
#include "WKTJsiWrapper.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

const char *WorkletArrayProxyName = "__createWorkletArrayProxy";

class JsiWrapper;

class JsiArrayWrapper : public JsiHostObject,
                        public std::enable_shared_from_this<JsiArrayWrapper>,
                        public JsiWrapper {
public:
  /**
   * Constructs a new array wrapper
   * @param parent Parent wrapper object
   * @param useProxiesForUnwrapping use proxies when unwrapping
   */
  JsiArrayWrapper(JsiWrapper *parent, bool useProxiesForUnwrapping)
      : JsiWrapper(parent, useProxiesForUnwrapping, JsiWrapperType::Array) {}
                          
  /**
   Helper to evaluate any jsi::Value as a boolean
   */
  bool evaluateAsBoolean(jsi::Runtime& rt, const jsi::Value& value) {
    if (value.isUndefined() || value.isNull()) {
      return false;
    }
    
    if (value.isBool()) {
      return value.getBool();
    }
    
    if (value.isNumber()) {
      return value.asNumber() != 0;
    }
    
    // Passing objects or array into this function evals to true in JS
    return true;
  }

  JSI_HOST_FUNCTION(toStringImpl) {
    return jsi::String::createFromUtf8(runtime, toString(runtime));
  }

  JSI_PROPERTY_GET(length) {
    std::unique_lock lock(_readWriteMutex);
    return static_cast<double>(_array.size());
  }

  JSI_HOST_FUNCTION(iterator) {
    int index = 0;
    auto iterator = jsi::Object(runtime);
    auto next = [index,
                 this](jsi::Runtime &runtime, const jsi::Value &thisValue,
                       const jsi::Value *arguments, size_t count) mutable {
      std::unique_lock lock(_readWriteMutex);

      auto retVal = jsi::Object(runtime);
      if (index < _array.size()) {
        retVal.setProperty(runtime, "value", _array[index]->unwrap(runtime));
        retVal.setProperty(runtime, "done", false);
        index++;
      } else {
        retVal.setProperty(runtime, "done", true);
      }
      return retVal;
    };

    iterator.setProperty(
        runtime, "next",
        jsi::Function::createFromHostFunction(
            runtime, jsi::PropNameID::forUtf8(runtime, "next"), 0, next));

    return iterator;
  }

  JSI_HOST_FUNCTION(push) {
    std::unique_lock lock(_readWriteMutex);

    // Push all arguments to the array end
    for (size_t i = 0; i < count; i++) {
      _array.push_back(JsiWrapper::wrap(runtime, arguments[i], this,
                                        getUseProxiesForUnwrapping()));
    }
    notify();
    return static_cast<double>(_array.size());
  };

  JSI_HOST_FUNCTION(unshift) {
    std::unique_lock lock(_readWriteMutex);

    // Insert all arguments to the array beginning
    for (size_t i = 0; i < count; i++) {
      _array.insert(_array.begin(),
                    JsiWrapper::wrap(runtime, arguments[i], this,
                                     getUseProxiesForUnwrapping()));
    }
    notify();
    return static_cast<double>(_array.size());
  };

  JSI_HOST_FUNCTION(pop) {
    std::unique_lock lock(_readWriteMutex);

    // Pop last element from array
    if (_array.empty()) {
      return jsi::Value::undefined();
    }
    auto lastEl = _array.at(_array.size() - 1);
    _array.pop_back();
    notify();
    return lastEl->unwrap(runtime);
  };

  JSI_HOST_FUNCTION(shift) {
    std::unique_lock lock(_readWriteMutex);

    // Shift first element from array
    if (_array.empty()) {
      return jsi::Value::undefined();
    }
    auto firstEl = _array.at(0);
    _array.erase(_array.begin());
    notify();
    return firstEl->unwrap(runtime);
  };

  JSI_HOST_FUNCTION(forEach) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    
    std::vector<jsi::Value> args(3);
    args[2] = thisValue.asObject(runtime);
    
    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = _array.at(i)->unwrap(runtime);
      args[1] = jsi::Value(static_cast<double>(i));
      callFunction(runtime, callbackFn, thisValue, static_cast<const jsi::Value *>(args.data()), 3);
    }
    
    return jsi::Value::undefined();
  };

  JSI_HOST_FUNCTION(map) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    auto result = jsi::Array(runtime, _array.size());
    
    std::vector<jsi::Value> args(3);
    args[2] = thisValue.asObject(runtime);
    
    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = _array.at(i)->unwrap(runtime);
      args[1] = jsi::Value(static_cast<double>(i));
      auto retVal = callFunction(runtime, callbackFn, thisValue,
                                 static_cast<const jsi::Value *>(args.data()), 3);
      result.setValueAtIndex(runtime, i, retVal);
    }
    return result;
  };

  JSI_HOST_FUNCTION(filter) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    std::vector<std::shared_ptr<JsiWrapper>> result;
    
    std::vector<jsi::Value> args(3);
    args[2] = thisValue.asObject(runtime);

    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = _array.at(i)->unwrap(runtime);
      args[1] = jsi::Value(static_cast<double>(i));
      
      auto retVal = callFunction(runtime, callbackFn, thisValue,
                                 static_cast<const jsi::Value *>(args.data()), 3);

      if (evaluateAsBoolean(runtime, retVal)) {
        result.push_back(_array.at(i));
      }
    }
    auto returnValue = jsi::Array(runtime, result.size());
    for (size_t i = 0; i < result.size(); i++) {
      returnValue.setValueAtIndex(runtime, i, result.at(i)->unwrap(runtime));
    }
    return returnValue;
  };

  JSI_HOST_FUNCTION(find) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    
    std::vector<jsi::Value> args(3);
    args[2] = thisValue.asObject(runtime);
    
    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = _array.at(i)->unwrap(runtime);
      args[1] = jsi::Value(static_cast<double>(i));
      auto retVal = callFunction(runtime, callbackFn, thisValue,
                                 static_cast<const jsi::Value *>(args.data()), 3);

      if (evaluateAsBoolean(runtime, retVal)) {
        return _array.at(i)->unwrap(runtime);
      }
    }
    return jsi::Value::undefined();
  };

  JSI_HOST_FUNCTION(every) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    
    std::vector<jsi::Value> args(3);
    args[2] = thisValue.asObject(runtime);
    
    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = _array.at(i)->unwrap(runtime);
      args[1] = jsi::Value(static_cast<double>(i));
      
      auto retVal = callFunction(runtime, callbackFn, thisValue,
                                 static_cast<const jsi::Value *>(args.data()), 3);

      if (evaluateAsBoolean(runtime, retVal) == false) {
        return false;
      }
    }
    return true;
  };
                          
  JSI_HOST_FUNCTION(some) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    
    std::vector<jsi::Value> args(3);
    args[2] = thisValue.asObject(runtime);
    
    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = _array.at(i)->unwrap(runtime);
      args[1] = jsi::Value(static_cast<double>(i));
      
      auto retVal = callFunction(runtime, callbackFn, thisValue,
                                 static_cast<const jsi::Value *>(args.data()), 3);

      if (evaluateAsBoolean(runtime, retVal) == true) {
        return true;
      }
    }
    return false;
  };

  JSI_HOST_FUNCTION(findIndex) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    
    std::vector<jsi::Value> args(3);
    args[2] = thisValue.asObject(runtime);
    
    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = _array.at(i)->unwrap(runtime);
      args[1] = jsi::Value(static_cast<double>(i));
      
      auto retVal = callFunction(runtime, callbackFn, thisValue,
                                 static_cast<const jsi::Value *>(args.data()), 3);
      
      if (evaluateAsBoolean(runtime, retVal)) {
        return static_cast<double>(i);
      }
    }
    return -1;
  };

  JSI_HOST_FUNCTION(indexOf) {
    std::unique_lock lock(_readWriteMutex);

    auto wrappedArg = JsiWrapper::wrap(runtime, arguments[0], nullptr,
                                       getUseProxiesForUnwrapping());
    
    size_t fromIndex = 0;
    if (count == 2) {
      fromIndex = arguments[1].asNumber();
    }
    
    for (size_t i = fromIndex; i < _array.size(); i++) {
      // TODO: Add == operator to JsiWrapper
      if (wrappedArg->getType() == _array[i]->getType()) {
        if (wrappedArg->toString(runtime) == _array[i]->toString(runtime)) {
          return static_cast<double>(i);
        }
      }
    }
    return -1;
  };

  const std::vector<std::shared_ptr<JsiWrapper>>
  flat_internal(int depth, std::vector<std::shared_ptr<JsiWrapper>> &arr) {
    std::vector<std::shared_ptr<JsiWrapper>> result;
    for (auto it : arr) {
      if (it->getType() == JsiWrapperType::Array) {
        // Recursively call flat untill depth equals 0
        if (depth <= -1 || depth > 0) {
          auto childArray =
              (static_cast<JsiArrayWrapper *>(it.get()))->getArray();
          auto flattened = flat_internal(depth - 1, childArray);
          for (auto child : flattened) {
            result.push_back(child);
          }
        }
      } else {
        result.push_back(it);
      }
    }
    return result;
  }

  JSI_HOST_FUNCTION(flat) {
    std::unique_lock lock(_readWriteMutex);

    auto depth = count > 0 ? arguments[0].asNumber() : -1;
    std::vector<std::shared_ptr<JsiWrapper>> result =
        flat_internal(depth, _array);
    auto returnValue = jsi::Array(runtime, result.size());
    for (size_t i = 0; i < result.size(); i++) {
      returnValue.setValueAtIndex(runtime, i,
                                  JsiWrapper::unwrap(runtime, result.at(i)));
    }
    return returnValue;
  };

  JSI_HOST_FUNCTION(includes) {
    std::unique_lock lock(_readWriteMutex);

    auto wrappedArg = JsiWrapper::wrap(runtime, arguments[0], nullptr,
                                       getUseProxiesForUnwrapping());
    
    size_t fromIndex = 0;
    if (count == 2) {
      fromIndex = arguments[1].asNumber();
    }
    
    for (size_t i = fromIndex; i < _array.size(); i++) {
      // TODO: Add == operator to JsiWrapper!!!
      if (wrappedArg->getType() == _array[i]->getType()) {
        if (wrappedArg->toString(runtime) == _array[i]->toString(runtime)) {
          return true;
        }
      }
    }
    return false;
  };

  JSI_HOST_FUNCTION(concat) {
    std::unique_lock lock(_readWriteMutex);

    // Copy existing array
    std::vector<std::shared_ptr<JsiWrapper>> nextArray;
    nextArray.resize(_array.size());
    
    for (size_t i=0; i<_array.size(); i++) {
      nextArray[i] = _array[i];
    }
    
    // enumerate all the parameters and check for either value or sub array (only two levels)
    for (size_t i=0; i<count; i++) {
      if (arguments[0].isObject()) {
        auto obj = arguments[0].asObject(runtime);
        if (obj.isArray(runtime)) {
          // We have an array - loop through and append all the array elements
          auto arr = obj.asArray(runtime);
          for (size_t n = 0; n < arr.size(runtime); n++) {
            nextArray.push_back(JsiWrapper::wrap(runtime, arr.getValueAtIndex(runtime, n)));
          }
          // continue loop
          continue;
        }
      }
      
      // Not an array, let's add item itself
      nextArray.push_back(JsiWrapper::wrap(runtime, arguments[i]));
    }
    
    auto results = jsi::Array(runtime, static_cast<size_t>(nextArray.size()));

    for (size_t i = 0; i < nextArray.size(); i++) {
      results.setValueAtIndex(runtime, i,
                              JsiWrapper::unwrap(runtime, nextArray[i]));
    }

    return results;
  }

  JSI_HOST_FUNCTION(join) {
    std::unique_lock lock(_readWriteMutex);

    auto separator =
        count > 0 ? arguments[0].asString(runtime).utf8(runtime) : ",";
    auto result = std::string("");
    for (size_t i = 0; i < _array.size(); i++) {
      auto arg = _array.at(i)->unwrap(runtime);
      result += arg.toString(runtime).utf8(runtime);
      if (i < _array.size() - 1) {
        result += separator;
      }
    }
    return jsi::String::createFromUtf8(runtime, result);
  }

  JSI_HOST_FUNCTION(reduce) {
    std::unique_lock lock(_readWriteMutex);

    auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
    std::shared_ptr<JsiWrapper> acc =
        JsiWrapper::wrap(runtime, jsi::Value::undefined(), nullptr,
                         getUseProxiesForUnwrapping());
    if (count > 1) {
      acc = JsiWrapper::wrap(runtime, arguments[1], nullptr,
                             getUseProxiesForUnwrapping());
    }
    
    std::vector<jsi::Value> args(4);
    args[3] = thisValue.asObject(runtime);
    
    for (size_t i = 0; i < _array.size(); i++) {
      args[0] = acc->unwrap(runtime);
      args[1] = _array.at(i)->unwrap(runtime);
      args[2] = jsi::Value(static_cast<double>(i));
      acc = JsiWrapper::wrap(
          runtime,
          callFunction(runtime, callbackFn, thisValue,
                       static_cast<const jsi::Value *>(args.data()), 4),
          nullptr, getUseProxiesForUnwrapping());
    }
    return JsiWrapper::unwrap(runtime, acc);
  }

  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(JsiArrayWrapper, length))

  JSI_EXPORT_FUNCTIONS(
      JSI_EXPORT_FUNC(JsiArrayWrapper, push),
      JSI_EXPORT_FUNC(JsiArrayWrapper, pop),
      JSI_EXPORT_FUNC(JsiArrayWrapper, unshift),
      JSI_EXPORT_FUNC(JsiArrayWrapper, shift),
      JSI_EXPORT_FUNC(JsiArrayWrapper, forEach),
      JSI_EXPORT_FUNC(JsiArrayWrapper, map),
      JSI_EXPORT_FUNC(JsiArrayWrapper, filter),
      JSI_EXPORT_FUNC(JsiArrayWrapper, concat),
      JSI_EXPORT_FUNC(JsiArrayWrapper, find),
      JSI_EXPORT_FUNC(JsiArrayWrapper, every),
      JSI_EXPORT_FUNC(JsiArrayWrapper, some),
      JSI_EXPORT_FUNC(JsiArrayWrapper, findIndex),
      JSI_EXPORT_FUNC(JsiArrayWrapper, flat),
      JSI_EXPORT_FUNC(JsiArrayWrapper, includes),
      JSI_EXPORT_FUNC(JsiArrayWrapper, indexOf),
      JSI_EXPORT_FUNC(JsiArrayWrapper, join),
      JSI_EXPORT_FUNC(JsiArrayWrapper, reduce),
      JSI_EXPORT_FUNC_NAMED(JsiArrayWrapper, toStringImpl, toString),
      JSI_EXPORT_FUNC_NAMED(JsiArrayWrapper, toStringImpl, Symbol.toStringTag),

      JSI_EXPORT_FUNC_NAMED(JsiArrayWrapper, iterator, Symbol.iterator))

  /**
   * Overridden getValue method
   * @param runtime Calling runtime
   * @return jsi::Value representing this array
   */
  jsi::Value getValue(jsi::Runtime &runtime) override {
    if (getUseProxiesForUnwrapping()) {
      return getArrayProxy(runtime, shared_from_this());
    }

    // Copy array if we're not using proxies (shared values)
    auto result = jsi::Array(runtime, _array.size());
    for (size_t i = 0; i < _array.size(); i++) {
      result.setValueAtIndex(runtime, i, _array.at(i)->unwrap(runtime));
    }
    return result;
  }

  bool canUpdateValue(jsi::Runtime &runtime, const jsi::Value &value) override {
    return value.isObject() && value.asObject(runtime).isArray(runtime);
  }

  /**
   * Overridden setValue method
   * @param runtime Calling runtime
   * @param value Value to set
   */
  void setValue(jsi::Runtime &runtime, const jsi::Value &value) override {
    std::unique_lock lock(_readWriteMutex);

    assert(value.isObject());
    auto object = value.asObject(runtime);
    assert(object.isArray(runtime));
    auto array = object.asArray(runtime);

    size_t size = array.size(runtime);
    _array.resize(size);

    for (size_t i = 0; i < size; i++) {
      _array[i] = JsiWrapper::wrap(runtime, array.getValueAtIndex(runtime, i),
                                   this, getUseProxiesForUnwrapping());
    }
  }

  /**
   * Overridden jsi::HostObject set property method
   * @param runtime Runtime
   * @param name Name of value to set
   * @param value Value to set
   */
  void set(jsi::Runtime &runtime, const jsi::PropNameID &name,
           const jsi::Value &value) override {
    auto nameStr = name.utf8(runtime);
    if (!nameStr.empty() &&
        std::all_of(nameStr.begin(), nameStr.end(), ::isdigit)) {
      std::unique_lock lock(_readWriteMutex);

      // Return property by index
      auto index = std::stoi(nameStr.c_str());
      // Ensure we have the required length
      if (index >= _array.size()) {
        _array.resize(index + 1);
      }
      // Set value
      _array[index] = JsiWrapper::wrap(runtime, value, nullptr,
                                       getUseProxiesForUnwrapping());
      notify();
    } else {
      // This is an edge case where the array is used as a
      // hashtable to set a value outside the bounds of the
      // array (ie. outside out the range of items being pushed)
      throw jsi::JSError(runtime, "Array out of bounds");
    }
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
      std::unique_lock lock(_readWriteMutex);

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
    std::unique_lock lock(_readWriteMutex);

    std::string retVal = "";
    // Return array contents
    for (size_t i = 0; i < _array.size(); i++) {
      auto str = _array.at(i)->toString(runtime);
      retVal += (i > 0 ? "," : "") + str;
    }
    return "[" + retVal + "]";
  }

  std::vector<jsi::PropNameID>
  getPropertyNames(jsi::Runtime &runtime) override {
    std::unique_lock lock(_readWriteMutex);

    std::vector<jsi::PropNameID> propNames;
    propNames.reserve(_array.size());
    for (size_t i = 0; i < _array.size(); i++) {
      propNames.push_back(jsi::PropNameID::forUtf8(runtime, std::to_string(i)));
    }
    return propNames;
  }

  const std::vector<std::shared_ptr<JsiWrapper>> &getArray() { return _array; }

private:
  /**
   Creates a proxy for the host object so that we can make the runtime trust
   that this is a real JS array
   */
  jsi::Value getArrayProxy(jsi::Runtime &runtime,
                           std::shared_ptr<jsi::HostObject> hostObj) {
    auto createArrayProxy =
        runtime.global().getProperty(runtime, WorkletArrayProxyName);

    // Install factory for creating an array proxy
    if (createArrayProxy.isUndefined()) {
      // Install worklet proxy helper into runtime
      static std::string code =
          "function (target) {"
          "        const dummy = [];"
          "        return new Proxy(dummy, {"
          "          ownKeys: function (_target) {"
          "            return Reflect.ownKeys(target).concat(['length']);"
          "          },"
          "          getOwnPropertyDescriptor: function (_target, prop) {"
          "            return {"
          "              ...Reflect.getOwnPropertyDescriptor(target, prop),"
          "              configurable: prop !== 'length',"
          "              writable: true,"
          "              enumerable: prop !== 'length',"
          "            };"
          "          },"
          "          set: function (_target, prop, value, _receiver) {"
          "            return Reflect.set(target, prop, value, target);"
          "          },"
          "          get: function (_target, prop, receiver) {"
          "            if (prop === 'length') return "
          "Object.keys(target).length;"
          "            if (prop === 'keys') return Object.keys(target);"
          "            if (prop === 'values') return Object.values(target);"
          "            return Reflect.get(target, prop, receiver);"
          "          },"
          "        });"
          "      }";

      // Format code as an installable function
      auto codeBuffer =
          std::make_shared<const jsi::StringBuffer>("(" + code + "\n)");

      // Create function
      createArrayProxy =
          runtime.evaluateJavaScript(codeBuffer, WorkletArrayProxyName);

      // Set in runtime
      runtime.global().setProperty(runtime, WorkletArrayProxyName,
                                   createArrayProxy);
    }

    // Get the create proxy factory function
    auto createProxyFunc =
        createArrayProxy.asObject(runtime).asFunction(runtime);

    // Create the proxy that converts the HostObject to an Array
    return createProxyFunc.call(
        runtime, jsi::Object::createFromHostObject(runtime, hostObj));
  }

  std::vector<std::shared_ptr<JsiWrapper>> _array;
};
} // namespace RNWorklet
