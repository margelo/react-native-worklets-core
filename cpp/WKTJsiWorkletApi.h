#pragma once

#include <jsi/jsi.h>

#include <memory>
#include <string>
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
        std::make_shared<JsiSharedValue>(
            arguments[0], JsiWorkletContext::getDefaultInstance()));
  };

  JSI_HOST_FUNCTION(createRunInJsFn) {
    if (count == 0) {
      throw jsi::JSError(runtime, "createRunInJsFn expects one parameter.");
    }

    // Get the worklet function
    if (!arguments[0].isObject()) {
      throw jsi::JSError(
          runtime,
          "createRunInJsFn expects a function as its first parameter.");
    }

    if (!arguments[0].asObject(runtime).isFunction(runtime)) {
      throw jsi::JSError(
          runtime,
          "createRunInJsFn expects a function as its first parameter.");
    }

    auto caller =
        JsiWorkletContext::createCallInContext(runtime, arguments[0], nullptr);

    // Now let us create the caller function.
    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forAscii(runtime, "runInJsFn"), 0,
        JSI_HOST_FUNCTION_LAMBDA {
          return caller(runtime, thisValue, arguments, count);
        });
  }

  JSI_HOST_FUNCTION(createRunInContextFn) {
    if (count == 0) {
      throw jsi::JSError(
          runtime, "createRunInContextFn expects at least one parameter.");
    }

    // Get the active context
    auto activeContext =
        count == 2 && arguments[1].isObject()
            ? arguments[1].asObject(runtime).getHostObject<JsiWorkletContext>(
                  runtime)
            : JsiWorkletContext::getDefaultInstance();

    if (activeContext == nullptr) {
      throw jsi::JSError(runtime,
                         "createRunInContextFn called with invalid context.");
    }

    auto caller = JsiWorkletContext::createCallInContext(runtime, arguments[0],
                                                         activeContext.get());

    // Now let us create the caller function.
    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forAscii(runtime, "runInContextFn"), 0,
        JSI_HOST_FUNCTION_LAMBDA {
          return caller(runtime, thisValue, arguments, count);
        });
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorkletApi, createSharedValue),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createContext),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createRunInContextFn),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createRunInJsFn))

  JSI_PROPERTY_GET(defaultContext) {
    return jsi::Object::createFromHostObject(
        runtime, JsiWorkletContext::getDefaultInstance());
  }

  JSI_PROPERTY_GET(currentContext) {
    auto current = JsiWorkletContext::getCurrent(runtime);
    if (!current) return jsi::Value::undefined();
    return jsi::Object::createFromHostObject(
        runtime, current->shared_from_this());
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
