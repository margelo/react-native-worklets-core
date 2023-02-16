#pragma once

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiBaseDecorator {
public:
  /**
   Initializes the decorator on the JS thread with the JS runtime
   */
  virtual void initialize(jsi::Runtime &runtime) {}

  /**
   Called on the context's worklet thread, the runtime is the worklet runtime
   */
  virtual void decorateRuntime(jsi::Runtime &runtime) = 0;
  virtual ~JsiBaseDecorator() {}
};
} // namespace RNWorklet
