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
  
  void decorateRuntime(jsi::Runtime &toRuntime) override {
    // Inject the wrapped object into the target runtime's global
    toRuntime.global().setProperty(toRuntime, _propertyName.c_str(),
                                   JsiWrapper::unwrap(toRuntime, _objectWrapper));
  }

private:
  std::shared_ptr<JsiWrapper> _objectWrapper;
  std::string _propertyName;
};
} // namespace RNWorklet
