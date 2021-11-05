//
// Created by Christian Falch on 01/11/2021.
//
#pragma once

#include <jsi/jsi.h>

namespace RNWorklet {

    static const char *WorkletHashPropertyName = "__workletHash";
    static const char *WorkletPropertyName = "__worklet";

    using namespace facebook;

    class JsiWorklet {
    public:
        /**
         * Returns the worklet hash for the given worklet
         * @param runtime Worklet runtime
         * @param worklet Worklet to get hash for
         * @return Worklet hash
         */
        static double getWorkletHash(jsi::Runtime& runtime, const jsi::Object& worklet) {
            auto prop = worklet.getProperty(runtime, WorkletHashPropertyName);
            if (prop.isUndefined()) {
                jsi::detail::throwJSError(
                        runtime, "Could not read the hash from the worklet.");
            }
            return prop.asNumber();
        }

        /**
         * Returns the worklet hash for the given value
         * @param runtime Worklet runtime
         * @param value Worklet to get hash for - needs to be a function
         * @return Worklet hash
         */
        static double getWorkletHash(jsi::Runtime& runtime, const jsi::Value &value) {
            return getWorkletHash(runtime, value.asObject(runtime).asFunction(runtime));
        }

        /**
         * Sets the worklet hash on a worklet object
         * @param runtime Worklet runtime
         * @param worklet worklet to add hash to
         * @param hash hash to set
         */
        static void setWorkletHash(jsi::Runtime& runtime, jsi::Object& worklet, double hash) {
            worklet.setProperty(runtime, WorkletHashPropertyName, hash);
        }

        /**
         * Marks the worklet as a worklet
         * @param runtime Worklet runtime
         * @param worklet Worklet to mark
         */
        static void markAsWorklet(jsi::Runtime& runtime, jsi::Object& worklet) {
            worklet.setProperty(runtime, WorkletPropertyName, true);
        }

        /**
         * Returns true if the provided function is a worklet
         * @param runtime Owner runtime
         * @param value Value to check
         * @return true if this is a worklet
         */
        static bool isWorklet(jsi::Runtime &runtime, const jsi::Function &value) {
            auto prop = value.getProperty(runtime, WorkletPropertyName);
            return prop.isBool() ? prop.getBool() : false;
        }

        /**
         * Returns true if the provided value is a function and is a worklet
         * @param runtime Owner runtime
         * @param value Value to check
         * @return true if this is a worklet
         */
        static bool isWorklet(jsi::Runtime &runtime, const jsi::Value &value) {
            if (!value.isObject()) {
                return false;
            }

            auto obj = value.asObject(runtime);
            if (!obj.isFunction(runtime)) {
                return false;
            }

            return isWorklet(runtime, obj.asFunction(runtime));
        }

        /**
         * Returns the name of the given function
         * @param function Function to get name for
         * @param runtime Runtie to get function in
         * @param nameHint Hint if name is not set
         * @return Name of function
         */
        static std::string getFunctionName(jsi::Runtime &runtime,
                                           const jsi::Function &function,
                                           const std::string& nameHint) {
            auto prop = function.getProperty(runtime, "name");
            if(prop.isString()) {
                return prop.asString(runtime).utf8(runtime);
            }
            return nameHint;
        }
    };
}
