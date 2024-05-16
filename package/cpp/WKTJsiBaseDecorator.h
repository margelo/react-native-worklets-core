#pragma once

#include <jsi/jsi.h>
#include <memory>

namespace RNWorklet {

class JsiWorkletContext;

namespace jsi = facebook::jsi;

class JsiBaseDecorator {
public:
  /**
   Decorates the given Worklet Context, using the given source runtime to copy the result from.
   */
  virtual void decorateRuntime(jsi::Runtime &fromRuntime, std::weak_ptr<JsiWorkletContext> toContext) = 0;
  virtual ~JsiBaseDecorator() {}
};
} // namespace RNWorklet
