#pragma once

#include <jsi/jsi.h>

#include "JsiHostObject.h"
#include "JsiSharedValue.h"
#include "JsiWorklet.h"
#include "JsiWorkletContext.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiWorkletApi : public JsiHostObject {
public:
  /**
   * Create the worklet API
   * @param context Worklet context
   * make sure only the runOnJsThread method is available.
   */
  JsiWorkletApi(std::shared_ptr<JsiWorkletContext> context)
      : _context(context){};

  /**
   * Installs the worklet API into the provided runtime
   * @param context Worklet context to install API for
   */
  static void installApi(std::shared_ptr<JsiWorkletContext> context) {
    auto existingApi = (context->getJsRuntime()->global().getProperty(
        *context->getJsRuntime(), "Worklets"));
    if (existingApi.isObject()) {
      return;
    }

    auto workletApi = std::make_shared<JsiWorkletApi>(context);
    context->getJsRuntime()->global().setProperty(
        *context->getJsRuntime(), "Worklets",
        jsi::Object::createFromHostObject(*context->getJsRuntime(),
                                          std::move(workletApi)));
  }

  JSI_HOST_FUNCTION(createWorklet) {
    // Make sure this one is only called from the js runtime
    if (_context->isWorkletRuntime(runtime)) {
      return _context->raiseError("createWorklet should only be called "
                                  "from the javascript runtime.");
    }

    if (count > 2) {
      return _context->raiseError(
          "createWorklet expects a Javascript function as the first parameter, "
          "and an optional worklet context as the second parameter.");
    }

    // Get the active context
    auto activeContext =
        count == 2 && arguments[1].isObject()
            ? arguments[1].asObject(runtime).getHostObject<JsiWorkletContext>(
                  runtime)
            : _context;

    if (activeContext == nullptr) {
      return _context->raiseError("createWorklet called with invalid context.");
    }

    // Create the worklet host object and return to JS caller
    return jsi::Object::createFromHostObject(
        runtime, std::make_shared<JsiWorklet>(activeContext, arguments[0]));
  };

  JSI_HOST_FUNCTION(createWorkletContext) {
    if (count == 0) {
      _context->raiseError(
          "createWorkletContext expects the context name parameter.");
    }

    if (!arguments[0].isString()) {
      _context->raiseError("createWorkletContext expects the context name "
                           "parameter as a string.");
    }

    auto nameStr = arguments[0].asString(runtime).utf8(runtime);
    return jsi::Object::createFromHostObject(
        runtime, std::make_shared<JsiWorkletContext>(nameStr, _context));
  };

  JSI_HOST_FUNCTION(createSharedValue) {
    if (_context->isWorkletRuntime(runtime)) {
      return _context->raiseError("createSharedValue should only be called "
                                  "from the javascript runtime.");
    }

    return jsi::Object::createFromHostObject(
        *_context->getJsRuntime(),
        std::make_shared<JsiSharedValue>(arguments[0], _context));
  };

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorkletApi, createWorklet),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createSharedValue),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createWorkletContext))

private:
  std::shared_ptr<JsiWorkletContext> _context;
};
} // namespace RNWorklet
