
#include "JsiWorkletContext.h"

#include "DispatchQueue.h"
#include "JsRuntimeFactory.h"
#include "JsiHostObject.h"

#include <exception>
#include <functional>
#include <memory>

#include <jsi/jsi.h>

namespace RNWorklet {

const char *WorkletRuntimeFlag = "__rn_worklets_runtime_flag";
std::vector<std::shared_ptr<JsiBaseDecorator>> JsiWorkletContext::_decorators;

namespace jsi = facebook::jsi;

JsiWorkletContext::JsiWorkletContext(const std::string &name) {
  // Create queue
  auto dispatchQueue =
      std::make_shared<DispatchQueue>(name + "_worklet_dispatch_queue", 1);

  // Initialize context
  initialize(name, JsiWorkletContext::getInstance()->_jsRuntime,
             JsiWorkletContext::getInstance()->_jsCallInvoker,
             [dispatchQueue](std::function<void()> &&f) {
               dispatchQueue->dispatch(std::move(f));
             });
};

JsiWorkletContext::JsiWorkletContext(
    const std::string &name,
    std::function<void(std::function<void()> &&)> workletCallInvoker) {
  // Initialize context
  initialize(name, JsiWorkletContext::getInstance()->_jsRuntime,
             JsiWorkletContext::getInstance()->_jsCallInvoker,
             workletCallInvoker);
};

void JsiWorkletContext::initialize(
    const std::string &name, jsi::Runtime *jsRuntime,
    std::function<void(std::function<void()> &&)> jsCallInvoker,
    std::function<void(std::function<void()> &&)> workletCallInvoker) {
  _name = name;
  _jsRuntime = jsRuntime;
  _jsCallInvoker = jsCallInvoker;
  _workletCallInvoker = workletCallInvoker;

  // Run decorators if we're not the singleton main context
  if (JsiWorkletContext::getInstance().get() != this) {
    applyDecorators(_decorators);
  }
}

void JsiWorkletContext::initialize(
    const std::string &name, jsi::Runtime *jsRuntime,
    std::function<void(std::function<void()> &&)> jsCallInvoker) {
  // Create queue
  auto dispatchQueue =
      std::make_shared<DispatchQueue>(name + "_worklet_dispatch_queue", 1);

  // Initialize invoker
  initialize(name, jsRuntime, jsCallInvoker,
             [dispatchQueue](std::function<void()> &&f) {
               dispatchQueue->dispatch(std::move(f));
             });
}

jsi::Runtime &JsiWorkletContext::getWorkletRuntime() {
  if (!_workletRuntime) {
    _workletRuntime = makeJSIRuntime();
    _workletRuntime->global().setProperty(*_workletRuntime, WorkletRuntimeFlag,
                                          true);
  }

  return *_workletRuntime;
}

void JsiWorkletContext::invokeOnJsThread(std::function<void()> &&fp) {
  if (_jsCallInvoker == nullptr) {
    throw std::runtime_error(
        "Expected Worklet context to have a JS call invoker.");
  }
  _jsCallInvoker(std::move(fp));
}

void JsiWorkletContext::invokeOnWorkletThread(std::function<void()> &&fp) {
  if (_workletCallInvoker == nullptr) {
    throw std::runtime_error(
        "Expected Worklet context to have a worklet call invoker.");
  }
  _workletCallInvoker(std::move(fp));
}

bool JsiWorkletContext::isWorkletRuntime(jsi::Runtime &runtime) {
  auto isWorkletRuntime =
      runtime.global().getProperty(runtime, WorkletRuntimeFlag);
  return isWorkletRuntime.isBool() && isWorkletRuntime.getBool();
}

void JsiWorkletContext::addDecorator(
    std::shared_ptr<JsiBaseDecorator> decorator) {
  _decorators.push_back(decorator);
  // decorate default context
  JsiWorkletContext::getInstance()->decorate(decorator);
}

template <typename... Args> void JsiWorkletContext::decorate(Args &&...args) {
  std::vector<std::shared_ptr<JsiBaseDecorator>> decorators = {args...};
  applyDecorators(decorators);
}

void JsiWorkletContext::applyDecorators(
    const std::vector<std::shared_ptr<JsiBaseDecorator>> &decorators) {
  std::mutex mu;
  std::condition_variable cond;
  bool isFinished = false;
  std::unique_lock<std::mutex> lock(mu);

  // Execute decoration in context' worklet thread/runtime
  _workletCallInvoker([&]() {
    std::lock_guard<std::mutex> lock(mu);
    for (size_t i = 0; i < decorators.size(); ++i) {
      decorators[i]->decorateRuntime(getWorkletRuntime());
    }
    isFinished = true;
    cond.notify_one();
  });

  // Wait untill the blocking code as finished
  cond.wait(lock, [&]() { return isFinished; });
}

} // namespace RNWorklet
