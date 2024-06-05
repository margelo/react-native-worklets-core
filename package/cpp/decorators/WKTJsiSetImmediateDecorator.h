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

  void decorateRuntime(jsi::Runtime &runtime) override {
    auto setImmediate = jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forUtf8(runtime, PropNameSetImmediate), 2,
        JSI_HOST_FUNCTION_LAMBDA {
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
          if (context) {
            // Invoke function on context thread / runtime
            context->invokeOnWorkletThread(
                [dispatcher](JsiWorkletContext *context,
                             jsi::Runtime &runtime) {
                  printf("ctx %lu: setImmediate\n", context->getContextId());
                  dispatcher(runtime);
                });
          } else {
            // Invoke function in JS thread / runtime
            JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
                [dispatcher](jsi::Runtime &runtime) {
                  printf("ctx -1: setImmediate\n");
                  dispatcher(runtime);
                });
          }
          return 0;
        });
    runtime.global().setProperty(runtime, PropNameSetImmediate, setImmediate);
  };

private:
  std::shared_ptr<JsiWrapper> _objectWrapper;
  std::string _propertyName;
};
} // namespace RNWorklet
