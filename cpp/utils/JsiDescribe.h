
#pragma once

#include <jsi/jsi.h>
#include <map>
#include <memory>
#include <string>

template <typename... Args>
std::string string_format(const std::string &format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
               1; // Extra space for '\0'
  if (size_s <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  auto size = static_cast<size_t>(size_s);
  auto buf = std::make_unique<char[]>(size);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1); // We don't want the '\0' inside
}

namespace RNWorklet {

using namespace facebook;

class JsiDescribe {
public:
  static std::string describe(jsi::Runtime &runtime, const jsi::Value &value) {
    std::map<size_t, std::string> cache;
    return describe(runtime, value, "", 0, cache);
  }

  static std::string describe(jsi::Runtime &runtime, jsi::Object &object) {
    std::map<size_t, std::string> cache;
    return describeObject(runtime, object, "", 0, cache);
  }

private:
  static std::string fromLevel(int level) {
    return std::string(level * 2, ' ');
  }

  static std::string describe(jsi::Runtime &source, const jsi::Value &value,
                              const std::string &name, int level,
                              std::map<size_t, std::string> &cache) {
    // for simple types we don't care about the cache
    if (value.isBool()) {
      return string_format("%s%s: %i (bool)\n", fromLevel(level).c_str(),
                           name.c_str(), value.getBool());
    }

    if (value.isNull()) {
      return string_format("%s%s: null\n", fromLevel(level).c_str(),
                           name.c_str());
    }

    if (value.isNumber()) {
      return string_format("%s%s: %.f (number)\n", fromLevel(level).c_str(),
                           name.c_str(), value.asNumber());
    }

    if (value.isString()) {
      auto string = value.asString(source).utf8(source);
      return string_format("%s%s: %s (string)\n", fromLevel(level).c_str(),
                           name.c_str(), string.c_str());
    }

    if (value.isSymbol()) {
      return string_format("%s%s: [symbol]\n", fromLevel(level).c_str(),
                           name.c_str());
    }

    if (value.isUndefined()) {
      return string_format("%s%s: undefined\n", fromLevel(level).c_str(),
                           name.c_str());
    }

    if (value.isObject()) {
      auto object = value.asObject(source);
      return describeObject(source, object, name, level, cache);
    }

    // We shouldn't get here..
    return "Unknown type for value";
  }

  static std::string describeObject(jsi::Runtime &source, jsi::Object &object,
                                    const std::string &name, int level,
                                    std::map<size_t, std::string> &cache) {
    // Array
    if (object.isArray(source)) {
      auto arr = object.asArray(source);
      size_t size = arr.size(source);

      std::string retVal =
          string_format("%s%s: %i (Array size) = [\n", fromLevel(level).c_str(),
                        name.c_str(), size);

      for (size_t i = 0; i < size; i++) {
        auto child = arr.getValueAtIndex(source, i);
        if (child.isObject() && child.asObject(source).isFunction(source)) {
          jsi::detail::throwJSError(source,
                                    "Functions in arrays not supported");
        }

        if (child.isObject()) {
          if (cache.count((size_t)&object) == 0) {
            // We don't have the value in the cache
            cache.emplace((size_t)&object,
                          describe(source, child,
                                   name + "[" + std::to_string(i) + "]",
                                   level + 1, cache));
          }
          retVal += cache.at((size_t)&object);
        }
      }

      retVal +=
          string_format("%s] // %s\n", fromLevel(level).c_str(), name.c_str());

      return retVal;
    }

    // Host object
    if (object.isHostObject(source)) {
      return string_format("%s%s: [hostobject]\n", fromLevel(level).c_str(),
                           name.c_str());
    }

    if (object.isArrayBuffer(source)) {
      return string_format("%s%s: [ArrayBuffer]\n", fromLevel(level).c_str(),
                           name.c_str());
    }

    if (object.isFunction(source)) {
      auto func = object.asFunction(source);
      if (func.isHostFunction(source)) {
        return string_format("%s%s: [hostfunction]\n", fromLevel(level).c_str(),
                             name.c_str());
      } else {
        // Regular javascript function
        auto nameProp = object.getProperty(source, "name");
        std::string nameStr = "";
        if (nameProp.isString()) {
          nameStr = nameProp.asString(source).utf8(source);
        }
        auto toString = object.getPropertyAsFunction(source, "toString");
        auto code = toString.callWithThis(source, object);
        auto prop = object.getProperty(source, "__worklet");
        auto isWorklet = prop.isBool() ? prop.getBool() : false;
        return string_format("%s%s: %s = %s [%s]\n", fromLevel(level).c_str(),
                             name.c_str(), nameStr.c_str(),
                             code.asString(source).utf8(source).c_str(),
                             isWorklet ? "worklet" : "regular");
      }
    }

    // Create new object
    auto retVal =
        string_format("%s%sObject {\n", fromLevel(level).c_str(), name.c_str());

    // Enum properties by name
    auto names = getPropertyNames(source, object);

    for (size_t i = 0; i < names.length(source); i++) {
      auto propName =
          names.getValueAtIndex(source, i).asString(source).utf8(source);

      auto child = object.getProperty(source, propName.c_str());
      if (cache.count((size_t)&object) == 0) {
        std::string descr = describe(source, child, propName, level + 1, cache);
        cache.emplace((size_t)&object, descr);
      }
      retVal += cache.at((size_t)&object);
    }

    retVal += string_format("%s}\n", fromLevel(level).c_str(), name.c_str());

    return retVal;
  }

  static std::unique_ptr<jsi::Function>
  getFunctionOn(jsi::Runtime &runtime, jsi::Object &object, const char *name) {
    auto getOwnPropertySymbols = object.getProperty(runtime, name);
    if (getOwnPropertySymbols.isObject()) {
      if (getOwnPropertySymbols.asObject(runtime).isFunction(runtime)) {
        return std::make_unique<jsi::Function>(
            getOwnPropertySymbols.asObject(runtime).asFunction(runtime));
      }
    }
    return nullptr;
  }

  static jsi::Array getPropertyNames(jsi::Runtime &runtime, jsi::Object &obj) {
    std::unique_ptr<jsi::Function> getOwnPropertyNames;
    auto objectClass = runtime.global().getProperty(runtime, "Object");
    if (objectClass.isObject()) {
      auto obj = objectClass.asObject(runtime);
      getOwnPropertyNames = getFunctionOn(runtime, obj, "getOwnPropertyNames");
    }

    std::vector<std::string> names;

    // get property names
    auto propNames = getOwnPropertyNames->call(runtime, obj);

    if (propNames.isObject()) {
      if (propNames.asObject(runtime).isArray(runtime)) {
        auto arr = propNames.asObject(runtime).asArray(runtime);
        for (size_t i = 0; i < arr.size(runtime); i++) {
          auto name =
              arr.getValueAtIndex(runtime, i).asString(runtime).utf8(runtime);
          names.push_back(name);
        }
      }

      jsi::Array retVal(runtime, names.size());
      for (size_t i = 0; i < names.size(); i++) {
        retVal.setValueAtIndex(
            runtime, i, jsi::String::createFromUtf8(runtime, names.at(i)));
      }
      return retVal;
    }

    return obj.getPropertyNames(runtime);
  }
};
} // namespace RNWorklet
