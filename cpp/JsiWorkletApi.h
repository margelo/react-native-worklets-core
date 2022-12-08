#pragma once

#include <jsi/jsi.h>

#include <vector>

#include "JsiHostObject.h"
#include "JsiSharedValue.h"
#include "JsiWorklet.h"
#include "JsiWorkletContext.h"
#include "JsiBaseDecorator.h"

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
    : _context(context) {
    // Add default decorators
    
  };

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

  JSI_HOST_FUNCTION(createContext) {
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
        runtime, createWorkletContext(nameStr));
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
  
  JSI_HOST_FUNCTION(isOnWorkletRuntime) {
    // Get the active context
    auto activeContext =
        count == 1 && arguments[0].isObject()
            ? arguments[0].asObject(runtime).getHostObject<JsiWorkletContext>(
                  runtime)
            : _context;

    if (activeContext == nullptr) {
      return _context->raiseError("createWorklet called with invalid context.");
    }
    
    return activeContext->isWorkletRuntime(runtime);
  }
  
  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorkletApi, createWorklet),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createSharedValue),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createContext),
                       JSI_EXPORT_FUNC(JsiWorkletApi, isOnWorkletRuntime))
  
  /**
   Creates a new worklet context
   @name Name of the context
   @returns A new worklet context that has been initialized and decorated.
   */
  std::shared_ptr<JsiWorkletContext> createWorkletContext(const std::string& name) {
    auto newContext =  std::make_shared<JsiWorkletContext>(name, _context);
    for (auto &decorator: _decorators) {
      newContext->decorate(decorator.get());
    }
    return newContext;
  }
  
  /**
   Adds a decorator that will be used to decorate all contexts.
   @decorator The decorator to add
   */
  void addDecorator(std::shared_ptr<JsiBaseDecorator> decorator) {
    _decorators.push_back(decorator);
    
    // Decorate default context
    _context->decorate(decorator.get());
  }

private:
  std::shared_ptr<JsiWorkletContext> _context;
  std::vector<std::shared_ptr<JsiBaseDecorator>> _decorators;
};
} // namespace RNWorklet
