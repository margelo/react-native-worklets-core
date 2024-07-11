#pragma once

#include <jsi/jsi.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "WKTJsiHostObject.h"
#include "WKTJsiWorklet.h"
#include "WKTJsiWrapper.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiWrapper;

static const char *ThenPropName = "then";
static const char *CatchPropName = "catch";

class JsiPromiseWrapper;

struct PromiseQueueItem {
  std::shared_ptr<JsiPromiseWrapper> controlledPromise;
  jsi::HostFunctionType fulfilledFn;
  jsi::HostFunctionType catchFn;
};

struct FinallyQueueItem {
  std::shared_ptr<JsiPromiseWrapper> controlledPromise;
  jsi::HostFunctionType sideEffectFn;
};

class PromiseParameter {
public:
  virtual void resolve(jsi::Runtime &runtime, const jsi::Value &val) = 0;
  virtual void reject(jsi::Runtime &runtime, const jsi::Value &reason) = 0;
};

using PromiseResolveFunction =
    std::function<void(jsi::Runtime &runtime, const jsi::Value &val)>;
using PromiseRejectFunction =
    std::function<void(jsi::Runtime &runtime, const jsi::Value &reason)>;

using PromiseComputationFunction = std::function<void(
    jsi::Runtime &runtime, std::shared_ptr<PromiseParameter> promise)>;

/**
 Wraps a Promise so that it can be shared between multiple runtimes as arguments
 or return values.
 */
class JsiPromiseWrapper
    : public JsiHostObject,
      public JsiWrapper,
      public PromiseParameter,
      public std::enable_shared_from_this<JsiPromiseWrapper> {
public:
  static std::shared_ptr<JsiPromiseWrapper>
  createPromiseWrapper(jsi::Runtime &runtime,
                       PromiseComputationFunction computation);

  JsiPromiseWrapper(JsiWrapper *parent, bool useProxiesForUnwrapping)
      : JsiWrapper(parent, useProxiesForUnwrapping, JsiWrapperType::Promise) {}

  ~JsiPromiseWrapper() {}
  /**
   Returns true if the object is a thenable object - ie. an object with a then
   function. Which is basically what a promise is.
   */
  static bool isThenable(jsi::Runtime &runtime, jsi::Object &obj);

  /**
   Returns true if the object is a thenable object - ie. an object with a then
   function. Which is basically what a promise is.
   */
  static bool isThenable(jsi::Runtime &runtime, jsi::Value &value);

  JSI_HOST_FUNCTION(then) {
    auto thenFn = count > 0 ? &arguments[0] : nullptr;
    auto catchFn = count > 1 ? &arguments[1] : nullptr;
    return then(runtime, thisValue, thenFn, catchFn);
  }

  JSI_HOST_FUNCTION(_catch) {
    auto catchFn = count > 0 ? &arguments[0] : nullptr;
    return then(runtime, thisValue, nullptr, catchFn);
  }

  JSI_HOST_FUNCTION(finally) {
    auto sideEffectsFn = count > 0 ? &arguments[0] : nullptr;
    return finally(runtime, thisValue, sideEffectsFn);
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC_NAMED(JsiPromiseWrapper, _catch,
                                             "catch"),
                       JSI_EXPORT_FUNC(JsiPromiseWrapper, then),
                       JSI_EXPORT_FUNC(JsiPromiseWrapper, finally))

  /**
   Creates a wrapped promise that is resolved with the given value
   */
  static std::shared_ptr<JsiPromiseWrapper>
  resolve(jsi::Runtime &runtime, std::shared_ptr<JsiWrapper> value);

  /**
   Creates a wrapped promise that is rejected with the given reason
   */
  static std::shared_ptr<JsiPromiseWrapper>
  reject(jsi::Runtime &runtime, std::shared_ptr<JsiWrapper> reason);
  void onFulfilled(jsi::Runtime &runtime, const jsi::Value &val);
  void onRejected(jsi::Runtime &runtime, const jsi::Value &reason);

  void resolve(jsi::Runtime &runtime, const jsi::Value &value) override {
    onFulfilled(runtime, value);
  }

  void reject(jsi::Runtime &runtime, const jsi::Value &reason) override {
    onRejected(runtime, reason);
  }

protected:
  jsi::Value then(jsi::Runtime &runtime, const jsi::Value &thisValue,
                  const jsi::Value *thenFn, const jsi::Value *catchFn);

  std::shared_ptr<JsiPromiseWrapper> then(jsi::Runtime &runtime,
                                          std::shared_ptr<JsiWrapper> thisValue,
                                          const jsi::HostFunctionType &thenFn,
                                          const jsi::HostFunctionType &catchFn);

  // TODO: Same pattern as for then!
  jsi::Value finally(jsi::Runtime &runtime, const jsi::Value &thisValue,
                     const jsi::Value *sideEffectFn);

  /**
   Basically makes the promise wrapper readonly (which it should be)
   */
  bool canUpdateValue(jsi::Runtime &runtime, const jsi::Value &value) override {
    return false;
  }

  /*
   Converts to a promise
   */
  jsi::Value getValue(jsi::Runtime &runtime) override;

  /**
   Converts from a promise
   */
  void setValue(jsi::Runtime &runtime, const jsi::Value &value) override;

private:
  void runComputation(jsi::Runtime &runtime,
                      PromiseComputationFunction computation);

  typedef enum { Pending = 0, Fulfilled = 1, Rejected = 2 } PromiseState;

  void propagateFulfilled(jsi::Runtime &runtime);
  void propagateRejected(jsi::Runtime &runtime);

  PromiseState _state = PromiseState::Pending;
  std::shared_ptr<JsiWrapper> _value;
  std::shared_ptr<JsiWrapper> _reason;
  std::vector<PromiseQueueItem> _thenQueue;
  std::vector<FinallyQueueItem> _finallyQueue;
};
} // namespace RNWorklet
