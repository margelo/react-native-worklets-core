#pragma once

#include <jsi/jsi.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "WKTJsiHostObject.h"
#include "WKTJsiJsDecorator.h"
#include "WKTJsiPromiseWrapper.h"
#include "WKTJsiSharedValue.h"
#include "WKTJsiWorklet.h"
#include "WKTJsiWorkletContext.h"
#include "WKTJsiWrapper.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiWorkletApi : public JsiHostObject {
public:
  // Name of the Worklet API member (where to install on global)
  static const char *WorkletsApiName;

  /**
   * Installs the worklet API into the provided runtime
   */
  static void installApi(jsi::Runtime &runtime);

  /**
   Returns the worklet API
   */
  static std::shared_ptr<JsiWorkletApi> getInstance();

  /**
   Invalidate the api instance.
   */
  static void invalidateInstance();

  JSI_HOST_FUNCTION(createContext) {
    if (count == 0) {
      throw jsi::JSError(
          runtime, "createWorkletContext expects the context name parameter.");
    }

    if (!arguments[0].isString()) {
      throw jsi::JSError(runtime,
                         "createWorkletContext expects the context name "
                         "parameter as a string.");
    }

    auto nameStr = arguments[0].asString(runtime).utf8(runtime);
    return jsi::Object::createFromHostObject(runtime,
                                             createWorkletContext(nameStr));
  };

  JSI_HOST_FUNCTION(createSharedValue) {
    return jsi::Object::createFromHostObject(
        *JsiWorkletContext::getDefaultInstance()->getJsRuntime(),
        std::make_shared<JsiSharedValue>(arguments[0]));
  };

  JSI_HOST_FUNCTION(createRunOnJS) {
    if (count != 1) {
      throw jsi::JSError(runtime, "createRunOnJS expects one parameter.");
    }

    // Get the worklet function
    if (!arguments[0].isObject()) {
      throw jsi::JSError(
          runtime, "createRunOnJS expects a function as its first parameter.");
    }

    if (!arguments[0].asObject(runtime).isFunction(runtime)) {
      throw jsi::JSError(
          runtime, "createRunOnJS expects a function as its first parameter.");
    }

    auto caller =
        JsiWorkletContext::createCallInContext(runtime, arguments[0], nullptr);

    // Now let us create the caller function.
    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forAscii(runtime, "createRunOnJS"), 0,
        caller);
  }

  JSI_HOST_FUNCTION(runOnJS) {
    jsi::Value value = createRunOnJS(runtime, thisValue, arguments, count);
    jsi::Function func = value.asObject(runtime).asFunction(runtime);
    return func.call(runtime, nullptr, 0);
  }

  JSI_HOST_FUNCTION(createRunInJsFn) {
    // TODO: Remove these deprecated APIs after one or two versions.
    throw jsi::JSError(runtime,
                       "Worklets.createRunInJsFn(..) has been deprecated in "
                       "favor of Worklets.createRunOnJS(..) or "
                       "Worklets.runOnJS(..) - please migrate to the new API!");
  }

  JSI_HOST_FUNCTION(createRunInContextFn) {
    // TODO: Remove these deprecated APIs after one or two versions.
    throw jsi::JSError(runtime,
                       "Worklets.createRunInContextFn(context, ..) has been "
                       "deprecated in favor of context.createRunAsync(..) or "
                       "context.runAsync(..) - please migrate to the new API!");
  }

  JSI_HOST_FUNCTION(getCurrentThreadId) {
    static std::atomic<int> threadCounter = 0;
    static thread_local int thisThreadId = -1;
    if (thisThreadId == -1) {
      thisThreadId = threadCounter++;
    }
    return jsi::Value(thisThreadId);
  }

  JSI_HOST_FUNCTION(__jsi_is_array) {
    if (count == 0) {
      throw jsi::JSError(runtime, "__getTypeIsArray expects one parameter.");
    }

    if (arguments[0].isObject()) {
      auto obj = arguments[0].asObject(runtime);
      arguments[0].asObject(runtime).isArray(runtime);
      auto isArray = obj.isArray(runtime);
      return isArray;
    }

    return false;
  }

  JSI_HOST_FUNCTION(__jsi_is_object) {
    if (count == 0) {
      throw jsi::JSError(runtime, "__getTypeIsObject expects one parameter.");
    }

    if (arguments[0].isObject()) {
      return true;
    }

    return false;
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorkletApi, createSharedValue),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createContext),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createRunOnJS),
                       JSI_EXPORT_FUNC(JsiWorkletApi, runOnJS),
                       JSI_EXPORT_FUNC(JsiWorkletApi,
                                       createRunInContextFn), // <-- deprecated
                       JSI_EXPORT_FUNC(JsiWorkletApi,
                                       createRunInJsFn), // <-- deprecated
                       JSI_EXPORT_FUNC(JsiWorkletApi, getCurrentThreadId),
                       JSI_EXPORT_FUNC(JsiWorkletApi, __jsi_is_array),
                       JSI_EXPORT_FUNC(JsiWorkletApi, __jsi_is_object))

  JSI_PROPERTY_GET(defaultContext) {
    return jsi::Object::createFromHostObject(
        runtime, JsiWorkletContext::getDefaultInstanceAsShared());
  }

  JSI_PROPERTY_GET(currentContext) {
    auto current = JsiWorkletContext::getCurrent(runtime);
    if (!current)
      return jsi::Value::undefined();
    return jsi::Object::createFromHostObject(runtime,
                                             current->shared_from_this());
  }

  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(JsiWorkletApi,
                                                  defaultContext),
                              JSI_EXPORT_PROP_GET(JsiWorkletApi,
                                                  currentContext))

  /**
   Creates a new worklet context
   @name Name of the context
   @returns A new worklet context that has been initialized and decorated.
   */
  std::shared_ptr<JsiWorkletContext>
  createWorkletContext(const std::string &name);

private:
  // Instance/singletong
  static std::shared_ptr<JsiWorkletApi> instance;
};
} // namespace RNWorklet
