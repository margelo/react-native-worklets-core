
#pragma once

#include <memory>

namespace RNJsi {

    using namespace facebook;

    class JsiVisitor {
    public:
        /**
         * Visitor for a jsi::Value
         * @param runtime Runtime to visit in
         * @param value Value to visit
         */
        static void visit(jsi::Runtime &runtime,
                          jsi::Value &value,
                          std::function<void(jsi::Runtime&, const jsi::Value&)> visitor) {
          std::map<size_t, bool> cache;
          visit(runtime, value, visitor, cache);
        }
      
    private:
      static void visit(jsi::Runtime &runtime,
                        jsi::Value &value,
                        std::function<void(jsi::Runtime&, const jsi::Value&)> visitor,
                        std::map<size_t, bool>& cache) {
      
            visitor(runtime, value);

            if (value.isObject()) {
                // Check for object types
                auto object = value.asObject(runtime);

                // Array
                if (object.isArray(runtime)) {
                    auto arr = object.asArray(runtime);
                    size_t size = arr.size(runtime);
                    for (size_t i = 0; i < size; i++) {
                        auto child = arr.getValueAtIndex(runtime, i);
                        visit(runtime, child, visitor, cache);
                    }
                    return;
                }

                if(!object.isFunction(runtime)) {
                    // Enum Object properties by name
                    auto names = getPropertyNames(runtime, object);
                    for (size_t i = 0; i < names.length(runtime); i++) {
                        auto propName =
                                names.getValueAtIndex(runtime, i).asString(runtime).utf8(runtime);
                        auto child = object.getProperty(runtime, propName.c_str());
                        visit(runtime, child, visitor, cache);                        
                    }
                }
            }
        }

        static std::unique_ptr<jsi::Function> getFunctionOn(
                jsi::Runtime &runtime,
                jsi::Object &object,
                const char *name) {
            auto getOwnPropertySymbols = object.getProperty(runtime, name);
            if (getOwnPropertySymbols.isObject()) {
                if (getOwnPropertySymbols.asObject(runtime).isFunction(runtime)) {
                    return std::make_unique<jsi::Function>(
                            getOwnPropertySymbols.asObject(runtime).asFunction(
                                    runtime));
                }
            }
            return nullptr;
        }

        static jsi::Array getPropertyNames(
                jsi::Runtime &runtime,
                jsi::Object &obj) {
            std::unique_ptr<jsi::Function> getOwnPropertyNames;
            auto objectClass = runtime.global().getProperty(runtime, "Object");
            if (objectClass.isObject()) {
                auto obj = objectClass.asObject(runtime);
                getOwnPropertyNames =
                        getFunctionOn(runtime, obj, "getOwnPropertyNames");
            }

            std::vector<std::string> names;

            // get property names
            auto propNames = getOwnPropertyNames->call(runtime, obj);

            if (propNames.isObject()) {
                if (propNames.asObject(runtime).isArray(runtime)) {
                    auto arr = propNames.asObject(runtime).asArray(runtime);
                    for (size_t i = 0; i < arr.size(runtime); i++) {
                        auto name = arr.getValueAtIndex(runtime, i)
                                .asString(runtime)
                                .utf8(runtime);
                        names.push_back(name);
                    }
                }

                jsi::Array retVal(runtime, names.size());
                for (size_t i = 0; i < names.size(); i++) {
                    retVal.setValueAtIndex(
                            runtime,
                            i,
                            jsi::String::createFromUtf8(runtime, names.at(i)));
                }
                return retVal;
            }

            return obj.getPropertyNames(runtime);
        }
    };
}
