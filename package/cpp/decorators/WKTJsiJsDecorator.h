#pragma once

#include <memory>
#include <string>

#include "WKTJsiBaseDecorator.h"
#include "WKTJsiWrapper.h"
#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiJsDecorator : public JsiBaseDecorator {
public:
  JsiJsDecorator(jsi::Runtime &runtime, const std::string &propertyName,
                 const jsi::Value &value) {
    _objectWrapper = JsiWrapper::wrap(runtime, value.asObject(runtime));
    _propertyName = propertyName;
  }

  void decorateRuntime(jsi::Runtime &runtime) override {
    runtime.global().setProperty(runtime, _propertyName.c_str(),
                                 JsiWrapper::unwrap(runtime, _objectWrapper));
  };

private:
  std::shared_ptr<JsiWrapper> _objectWrapper;
  std::string _propertyName;
};
} // namespace RNWorklet
