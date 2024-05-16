#pragma once

#include <jsi/jsi.h>
#include <memory>

namespace RNWorklet {

class JsiWorkletContext;

namespace jsi = facebook::jsi;

class JsiBaseDecorator {
public:
  
  /**
   Initializes the Decorator with the given Runtime.
   For decorators that call back to the main JS runtime, this pulls values from the given fromRuntime.
   This needs to be called on the given source Runtime's Thread.
   */
  virtual void initialize(jsi::Runtime& fromRuntime) { }
  /**
   Decorates the given Runtime.
   This needs to be called on the given target Runtime's Thread.
   */
  virtual void decorateRuntime(jsi::Runtime &toRuntime) = 0;
  
  virtual ~JsiBaseDecorator() {}
};
} // namespace RNWorklet
