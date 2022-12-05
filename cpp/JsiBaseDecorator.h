#pragma once

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiBaseDecorator {
public:
  virtual void decorateRuntime(jsi::Runtime& runtime) = 0;
};
} // namespace RNWorklet
