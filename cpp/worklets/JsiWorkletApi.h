#pragma once

#include <jsi/jsi.h>

#include <JsiHostObject.h>
#include <JsiSharedValue.h>
#include <JsiWorklet.h>
#include <JsiWorkletContext.h>

namespace RNWorklet {

using namespace facebook;
using namespace RNJsi;

class JsiWorkletApi : public JsiHostObject {
public:
  /**
   * Create the worklet API
   * @param context Worklet context
   * make sure only the runOnJsThread method is available.
   */
  JsiWorkletApi(JsiWorkletContext *context) : _context(context) {};

  /**
   * Installs the worklet API into the provided runtime
   * @param context Worklet context to install API for
   */
  static void installApi(JsiWorkletContext *context) {
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

    if (count > 3) {
      return _context->raiseError("createWorklet expects 2 parameters: "
                                  "Worklet function and worklet closure.");
    }

    // Get the active context
    auto activeContext = getContext(
        count == 3 && arguments[2].isString()
            ? arguments[2].asString(runtime).utf8(runtime).c_str()
            : nullptr);

    // Install the worklet into the worklet runtime
    return jsi::Object::createFromHostObject(
        runtime, std::make_shared<JsiWorklet>(activeContext, runtime,
                                              arguments[1], arguments[0]));
  };

  JSI_HOST_FUNCTION(createWorkletContext) {
    if (count == 0) {
      jsi::detail::throwJSError(
          runtime,
          "createWorkletContext expects the context name parameter.");
    }

    if (!arguments[0].isString()) {
      jsi::detail::throwJSError(
          runtime,
          "createWorkletContext expects the context name parameter.");
    }

    auto nameStr = arguments[0].asString(runtime).utf8(runtime);
    if (_contexts.count(nameStr) == 0) {
      _contexts.emplace(nameStr,
                        std::make_shared<JsiWorkletContext>(_context));
    } else {
      jsi::detail::throwJSError(runtime,
                                "A context with this name already exists.");
    }
    return jsi::Value::undefined();
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
  JsiWorkletContext *getContext(const char *name);

  JsiWorkletContext *_context;
  std::map<std::string, std::shared_ptr<JsiWorkletContext>> _contexts;
};
} // namespace RNWorklet
