#pragma once

#include <jsi/jsi.h>

namespace RNWorklet {

class JsiWorkletContext;

namespace jsi = facebook::jsi;

class JsiBaseDecorator {
public:
  /**
   Decorates the given Worklet Context, using the given source runtime to copy the result from.
   */
  virtual void decorateRuntime(jsi::Runtime &fromRuntime, JsiWorkletContext& toContext) = 0;
  virtual ~JsiBaseDecorator() {}
};
} // namespace RNWorklet
