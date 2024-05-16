#pragma once

#include <memory>
#include <string>
#include <vector>

#include "WKTJsiBaseDecorator.h"
#include "WKTJsiWrapper.h"
#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

static const char *PropNameSetImmediate = "setImmediate";

/**
 Decorator for setImmediate
 setImmediate is implemented simply by pushing the function and executing it
 through the worklet invoker.
 */
class JsiSetImmediateDecorator : public JsiBaseDecorator {
public:
  JsiSetImmediateDecorator() {}

  static jsi::Value setImmediate(jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) {
    if (count == 0) {
      throw jsi::JSError(
          runtime,
          "setImmediate expects a function as its first parameter");
    }
    if (!arguments[0].isObject() ||
        !arguments[0].asObject(runtime).isFunction(runtime)) {
      throw jsi::JSError(
          runtime,
          "setImmediate expects a function as its first parameter");
    }

    // Get func
    auto func = std::make_shared<jsi::Function>(
        arguments[0].asObject(runtime).asFunction(runtime));

    // Save this
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);

    // Save args
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper(count - 1);
    for (size_t i = 1; i < count; i++) {
      argsWrapper[i - 1] = JsiWrapper::wrap(runtime, arguments[i]);
    }

    // Create dispatcher
    auto dispatcher = [thisWrapper, argsWrapper,
                       func](jsi::Runtime &runtime) {
      size_t size = argsWrapper.size();
      std::vector<jsi::Value> args(size);

      // Add the rest of the arguments
      for (size_t i = 0; i < size; i++) {
        args[i] = JsiWrapper::unwrap(runtime, argsWrapper.at(i));
      }

      if (thisWrapper->getType() == JsiWrapperType::Null ||
          thisWrapper->getType() == JsiWrapperType::Undefined) {
        func->call(runtime, static_cast<const jsi::Value *>(args.data()),
                   argsWrapper.size());
      } else {
        func->callWithThis(
            runtime,
            JsiWrapper::unwrap(runtime, thisWrapper).asObject(runtime),
            static_cast<const jsi::Value *>(args.data()),
            argsWrapper.size());
      }
    };

    auto context = JsiWorkletContext::getCurrent(runtime);
    if (context == nullptr) {
      throw std::runtime_error("setImmediate failed: This Runtime does not have a Worklet Context!");
    }
    // Invoke function on context thread / runtime
    context->invokeOnWorkletThread(
        [dispatcher](JsiWorkletContext *context,
                     jsi::Runtime &runtime) {
          printf("ctx %lu: setImmediate\n", context->getContextId());
          dispatcher(runtime);
        });
    return 0;
  }

  void decorateRuntime(jsi::Runtime &fromRuntime, std::weak_ptr<JsiWorkletContext> toContext) override {
    auto context = toContext.lock();
    if (context == nullptr) {
      throw std::runtime_error("Cannot decorate Runtime - target context is null!");
    }
    context->invokeOnWorkletThread([](JsiWorkletContext *, jsi::Runtime &toRuntime) {
      // Inject global.setImmediate
      jsi::HostFunctionType hostFunction = JsiSetImmediateDecorator::setImmediate;
      auto setImmediate = jsi::Function::createFromHostFunction(toRuntime, jsi::PropNameID::forUtf8(toRuntime, PropNameSetImmediate), 2, hostFunction);
      toRuntime.global().setProperty(toRuntime, PropNameSetImmediate, setImmediate);
    });
  }
};
} // namespace RNWorklet
