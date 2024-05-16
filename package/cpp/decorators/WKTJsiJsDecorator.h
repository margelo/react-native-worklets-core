#pragma once

#include <memory>
#include <string>

#include "WKTJsiBaseDecorator.h"
#include "WKTJsiWrapper.h"
#include "WKTJsiWorkletContext.h"
#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiJsDecorator : public JsiBaseDecorator {
public:
  JsiJsDecorator(jsi::Runtime &runtime, const std::string &propertyName,
                 const jsi::Value &value) {
    // Capture the object on the current runtime and wrap it
    _objectWrapper = JsiWrapper::wrap(runtime, value.asObject(runtime));
    _propertyName = propertyName;
  }
  
  void decorateRuntime(jsi::Runtime &fromRuntime, std::weak_ptr<JsiWorkletContext> toContext) override {
    std::string propertyName = _propertyName;
    std::shared_ptr<JsiWrapper> objectWrapper = _objectWrapper;
    
    auto context = toContext.lock();
    if (context == nullptr) {
      throw std::runtime_error("Cannot decorate Runtime - target context is null!");
    }
    context->invokeOnWorkletThread([propertyName, objectWrapper](JsiWorkletContext*, jsi::Runtime& toRuntime) {
      // Inject the wrapped object into the target runtime's global
      toRuntime.global().setProperty(toRuntime, propertyName.c_str(),
                                     JsiWrapper::unwrap(toRuntime, objectWrapper));
    });
  }

private:
  std::shared_ptr<JsiWrapper> _objectWrapper;
  std::string _propertyName;
};
} // namespace RNWorklet
