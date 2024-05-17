#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "WKTJsiBaseDecorator.h"
#include "WKTJsiHostObject.h"
#include "WKTJsiWrapper.h"
#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

static const char *PropNamePerformance = "performance";

class JsiPerformanceImpl : public JsiHostObject {
public:
  JSI_HOST_FUNCTION(now) {
    auto time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        time.time_since_epoch())
                        .count();

    constexpr double NANOSECONDS_IN_MILLISECOND = 1000000.0;
    return jsi::Value(duration / NANOSECONDS_IN_MILLISECOND);
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiPerformanceImpl, now))
};

/**
 Decorator for performance.now
 */
class JsiPerformanceDecorator : public JsiBaseDecorator {
public:
  void decorateRuntime(jsi::Runtime &runtime) override {
    auto performanceObj = std::make_shared<JsiPerformanceImpl>();
    runtime.global().setProperty(
        runtime, PropNamePerformance,
        jsi::Object::createFromHostObject(runtime, performanceObj));
  };
};
} // namespace RNWorklet
