
#include <JsiDescribe.h>
#include <JsiSharedValue.h>
#include <JsiVisitor.h>
#include <JsiWorkletApi.h>
#include <JsiWorkletContext.h>

namespace RNWorklet {

JsiWorkletContext::JsiWorkletContext(
    const std::string& name,
    jsi::Runtime *jsRuntime,
    std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
    std::shared_ptr<std::function<void(const std::exception &ex)>> errorHandler)
    : _name(name), _jsRuntime(jsRuntime), _workletRuntime(makeJSIRuntime()),
      _jsCallInvoker(jsCallInvoker), _errorHandler(errorHandler) {

  // Create the dispatch queue
  _dispatchQueue = std::make_unique<JsiDispatchQueue>("worklet-queue", 1);

  // Decorate the runtime with the worklet flag so that checks using
  // this variable will work.
  _workletRuntime->global().setProperty(*_workletRuntime, WorkletRuntimeFlag,
                                        jsi::Value(true));
}

void JsiWorkletContext::runOnWorkletThread(std::function<void()> fp) {
  _dispatchQueue->dispatch(fp);
}

void JsiWorkletContext::runOnJavascriptThread(std::function<void()> fp) {
  _jsCallInvoker->invokeAsync(std::move(fp));
}

jsi::Value
JsiWorkletContext::evaluteJavascriptInWorkletRuntime(const std::string &code) {

  jsi::Runtime *workletRuntime = &getWorkletRuntime();
  auto eval = workletRuntime->global().getPropertyAsFunction(
      getWorkletRuntime(), "eval");

  return eval.call(*workletRuntime, ("(" + code + ")").c_str());
}

} // namespace RNWorklet
