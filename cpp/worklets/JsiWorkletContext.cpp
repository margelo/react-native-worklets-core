
#include <JsiDescribe.h>
#include <JsiSharedValue.h>
#include <JsiVisitor.h>
#include <JsiWorkletApi.h>
#include <JsiWorkletContext.h>

namespace RNWorklet {

JsiWorkletContext::JsiWorkletContext(
    jsi::Runtime *jsRuntime,
    std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
    std::shared_ptr<std::function<void(const std::exception &ex)>> errorHandler)
    : _jsRuntime(jsRuntime), _workletRuntime(makeJSIRuntime()),
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

jsi::HostFunctionType JsiWorkletContext::createWorklet(
    jsi::Runtime &runtime, const jsi::Value &context, const jsi::Value &value) {
  if (!value.isObject()) {
    raiseError("createWorklet expects a function parameter.");
    return nullptr;
  }

  if (!value.asObject(runtime).isFunction(runtime)) {
    raiseError("createWorklet expects a function parameter.");
    return nullptr;
  }

  auto function = value.asObject(runtime).asFunction(runtime);

  // Let us try to install the function in the worklet context
  auto code = function.getPropertyAsFunction(runtime, "toString")
                  .callWithThis(runtime, function, nullptr, 0)
                  .asString(runtime)
                  .utf8(runtime);

  auto worklet = std::make_shared<jsi::Function>(evalWorkletCode(code));

  // Let's wrap the closure
  auto closure = JsiWrapper::wrap(runtime, context);

  // TODO: Find all shared values in the worklet

  // Return a caller function wrapper for the worklet
  return [worklet, closure](jsi::Runtime &runtime, const jsi::Value &,
                            const jsi::Value *arguments,
                            size_t count) -> jsi::Value {
    // Add the closure as the first parameter
    size_t size = count + 1;
    jsi::Value *args = new jsi::Value[size];

    // Add context as first argument
    args[0] = JsiWrapper::unwrap(runtime, closure);

    // Shift array arguments one position to the right to make room for the
    // context as the first argument
    std::memmove(args + 1, arguments, sizeof(jsi::Value) * count);

    auto retVal =
        worklet->call(runtime, static_cast<const jsi::Value *>(args), size);

    delete[] args;

    return retVal;
  };
}

jsi::Function JsiWorkletContext::evalWorkletCode(const std::string &code) {

  jsi::Runtime *workletRuntime = &getWorkletRuntime();
  auto eval = workletRuntime->global().getPropertyAsFunction(
      getWorkletRuntime(), "eval");

  return eval.call(*workletRuntime, ("(" + code + ")").c_str())
      .asObject(*workletRuntime)
      .asFunction(*workletRuntime);
}

} // namespace RNWorklet
