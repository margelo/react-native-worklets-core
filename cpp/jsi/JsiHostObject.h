#pragma once

#include <map>
#include <jsi/jsi.h>

#define JSI_FUNC_SIGNATURE           \
    [=](jsi::Runtime & runtime,      \
        const jsi::Value &thisValue, \
        const jsi::Value *arguments, \
        size_t count) -> jsi::Value

namespace RNJsi {

    using namespace facebook;

    using JsPropertyType = struct {
        std::function<jsi::Value(jsi::Runtime &)> get;
        std::function<void(jsi::Runtime &, const jsi::Value &)> set;
    };

    /**
     * Base class for jsi host objects
     */
    class JsiHostObject : public jsi::HostObject {

    protected:
        /**
         * Overridden jsi::HostObject set property method
         * @param rt Runtime
         * @param name Name of value to set
         * @param value Value to set
         */
        void set(
                jsi::Runtime &rt,
                const jsi::PropNameID &name,
                const jsi::Value &value) override {
            auto nameVal = name.utf8(rt);
            auto nameStr = nameVal.c_str();
            if (_propMap.count(nameStr) > 0) {
                auto prop = _propMap.at(nameStr);
                (prop.set)(rt, value);
            }
        };

        /**
         * Overridden jsi::HostObject get property method. Returns functions from
         * the map of functions.
         * @param runtime Runtime
         * @param name Name of value to get
         * @return Value
         */
        jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name)
        override {
            auto nameVal = name.utf8(runtime);
            auto nameStr = nameVal.c_str();

            if (_funcMap.count(nameStr) > 0) {
                return jsi::Function::createFromHostFunction(
                        runtime, name, 0, _funcMap.at(nameStr));
            }

            if (_propMap.count(nameStr) > 0) {
                auto prop = _propMap.at(nameStr);
                return (prop.get)(runtime);
            }

            return jsi::Value::undefined();
        }

        /**
         * Overridden getPropertyNames from jsi::HostObject. Returns all keys in the
         * function and property maps
         * @param runtime Runtime
         * @return List of property names
         */
        std::vector<jsi::PropNameID> getPropertyNames(
                jsi::Runtime &runtime) override {
            std::vector<jsi::PropNameID> retVal;
            // functions
            for (auto it = _funcMap.begin(); it != _funcMap.end(); ++it) {
                retVal.push_back(jsi::PropNameID::forAscii(runtime, it->first));
            }
            // props
            for (auto it = _propMap.begin(); it != _propMap.end(); ++it) {
                retVal.push_back(jsi::PropNameID::forAscii(runtime, it->first));
            }
            return retVal;
        }

        /**
         * Installs a function into the function map
         */
        void installFunction(
                const std::string &name,
                const jsi::HostFunctionType &function) {
            _funcMap.emplace(name, function);
        }

        /**
         * Installs a property with get/set
         * @param name Name of property to install
         * @param get Getter function
         * @param set Setter function
         */
        void installProperty(
                const std::string &name,
                const std::function<jsi::Value(jsi::Runtime &)> &get,
                const std::function<void(jsi::Runtime &, const jsi::Value &)> &set) {
            _propMap.emplace(name, JsPropertyType{.get = get, .set = set});
        }

        /**
         * Installs a property with only gettter
         * @param name Name of property to install
         * @param get Getter function
         */
        void installReadonlyProperty(
                const std::string &name,
                const std::function<jsi::Value(jsi::Runtime &)> &get) {
            _propMap.emplace(
                    name,
                    JsPropertyType{
                            .get = get,
                            .set = [](jsi::Runtime &, const jsi::Value &) {},
                    });
        }

        /**
         * Installs a property wich points to a given host object
         * @param name Name of property to install
         * @param hostObject Object to return
         */
        void installReadonlyProperty(
                const std::string &name,
                std::shared_ptr<jsi::HostObject> hostObject) {
            _propMap.emplace(
                    name,
                    JsPropertyType{
                            .get =
                            [hostObject](jsi::Runtime &runtime) {
                                return jsi::Object::createFromHostObject(
                                        runtime, hostObject);
                            },
                            .set = [](jsi::Runtime &, const jsi::Value &) {},
                    });
        }

    private:
        std::map<std::string, jsi::HostFunctionType> _funcMap;
        std::map<std::string, JsPropertyType> _propMap;
    };
}
