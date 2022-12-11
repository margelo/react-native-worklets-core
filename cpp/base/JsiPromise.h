#pragma once

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiPromise;

using PromiseSetupFunctionType =
    std::function<void(jsi::Runtime &rt, std::shared_ptr<JsiPromise>)>;

class JsiPromise {
public:
  explicit JsiPromise(jsi::Runtime &rt, jsi::Function resolve,
                      jsi::Function reject)
      : runtime_(rt), resolve_(std::move(resolve)), reject_(std::move(reject)) {
  }

  static jsi::Value
  createPromiseAsJSIValue(jsi::Runtime &rt,
                          const PromiseSetupFunctionType func) {
    jsi::Function JSPromise = rt.global().getPropertyAsFunction(rt, "Promise");
    jsi::Function fn = jsi::Function::createFromHostFunction(
        rt, jsi::PropNameID::forAscii(rt, "fn"), 2,
        [func](jsi::Runtime &rt2, const jsi::Value &thisVal,
               const jsi::Value *args, size_t count) {
          jsi::Function resolve = args[0].getObject(rt2).getFunction(rt2);
          jsi::Function reject = args[1].getObject(rt2).getFunction(rt2);
          auto wrapper = std::make_shared<JsiPromise>(rt2, std::move(resolve),
                                                      std::move(reject));
          func(rt2, wrapper);
          return jsi::Value::undefined();
        });

    return JSPromise.callAsConstructor(rt, fn);
  }

  void resolve(const jsi::Value &result) { resolve_.call(runtime_, result); }

  void reject(const std::string &message, const std::string &stack) {
    jsi::Object error(runtime_);
    error.setProperty(runtime_, "message",
                      jsi::String::createFromUtf8(runtime_, message));
    error.setProperty(runtime_, "stack",
                      jsi::String::createFromUtf8(runtime_, stack));
    reject_.call(runtime_, error);
  }

private:
  jsi::Runtime &runtime_;
  jsi::Function resolve_;
  jsi::Function reject_;
};

} // namespace RNWorklet
