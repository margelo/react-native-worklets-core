
#pragma once

#include "DispatchQueue.h"
#include "JsRuntimeFactory.h"
#include "JsiHostObject.h"
#include "JsiBaseDecorator.h"

#include <exception>
#include <functional>
#include <memory>

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiWorkletContext : public JsiHostObject {
public:
  const char *WorkletRuntimeFlag = "__rn_worklets_runtime_flag";

  /**
   * Creates a new worklet context that can be used to run jsi::Functions as
   * worklets in a separate thread / js runtime.
   * @param name Name of the context
   * @param jsRuntime Main javascript runtime.
   * @param jsCallInvoker Callback for running a function on the JS thread.
   * @param workletCallInvoker Callback for running a function on the worklet
   * thread
   * @param raiseError Callback for raising errors
   */
  JsiWorkletContext(
      const std::string &name, jsi::Runtime *jsRuntime,
      std::function<void(std::function<void()> &&)> jsCallInvoker,
      std::function<void(std::function<void()> &&)> workletCallInvoker,
      std::function<void(const std::exception &)> raiseError)
      : _name(name), _jsRuntime(jsRuntime), _raise(raiseError),
        _jsCallInvoker(jsCallInvoker), _workletCallInvoker(workletCallInvoker) {
  }

  /**
   * Ctor for the JsiWorkletContext
   * @param name Name of the context
   * @param jsRuntime Runtime for the main javascript runtime.
   * @param jsCallInvoker Callback for running a function on the JS thread.
   * @param raiseError Callback for raising errors
   */
  JsiWorkletContext(const std::string &name, jsi::Runtime *jsRuntime,
                    std::function<void(std::function<void()> &&)> jsCallInvoker,
                    std::function<void(const std::exception &)> raiseError)
      : _name(name), _jsRuntime(jsRuntime), _raise(raiseError),
        _jsCallInvoker(jsCallInvoker) {
    auto dispatchQueue =
        std::make_shared<DispatchQueue>(name + "_worklet_dispatch_queue", 1);
    _workletCallInvoker = [dispatchQueue](std::function<void()> &&f) {
      dispatchQueue->dispatch(std::move(f));
    };
  }

  /**
   * Creates a new worklet context from a parent context. The new worklet
   * context will inherit from the parent context, but will create its own
   * worklet runtime and worklet executor thread.
   * @param name Name of the context
   * @param parentContext parent worklet context
   */
  JsiWorkletContext(const std::string &name,
                    std::shared_ptr<JsiWorkletContext> parentContext)
      : JsiWorkletContext(name, parentContext->_jsRuntime,
                          parentContext->_jsCallInvoker,
                          parentContext->_raise) {}

  /**
   * Destructor
   */
  virtual ~JsiWorkletContext() {}

  JSI_PROPERTY_GET(name) {
    return jsi::String::createFromUtf8(runtime, getName());
  }

  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(JsiWorkletContext, name))

  /**
   Returns the main javascript runtime
   */
  jsi::Runtime *getJsRuntime() { return _jsRuntime; }

  /**
   Returns the name of the context
   */
  const std::string &getName() { return _name; }

  /**
   Returns the worklet runtime. Lazy evaluated
   */
  jsi::Runtime &getWorkletRuntime() {
    if (!_workletRuntime) {
      _workletRuntime = makeJSIRuntime();
      _workletRuntime->global().setProperty(*_workletRuntime,
                                            WorkletRuntimeFlag, true);
    }
    return *_workletRuntime;
  }

  /**
   * Raises an exception on the platform. This function does not necessarily
   * throw an exception and stop execution, so it is important to stop execution
   * by returning after calling the function
   * @param err Error to raise
   */
  jsi::Value raiseError(const std::exception &err) {
    _raise(err);
    return jsi::Value::undefined();
  };

  /**
   * Raises an exception on the platform. This function does not necessarily
   * throw an exception and stop execution, so it is important to stop execution
   * by returning after calling the function
   * @param message Message to show
   */
  jsi::Value raiseError(const std::string &message) {
    raiseError(std::runtime_error(message));
    return jsi::Value::undefined();
  }

  /**
   Executes a function in the JS thread
   */
  void invokeOnJsThread(std::function<void()> &&fp) {
    if (_jsCallInvoker == nullptr) {
      throw std::runtime_error(
          "Expected Worklet context to have a JS call invoker.");
    }
    _jsCallInvoker(std::move(fp));
  }

  /**
   Executes a function in the worklet thread
   */
  void invokeOnWorkletThread(std::function<void()> &&fp) {
    if (_workletCallInvoker == nullptr) {
      throw std::runtime_error(
          "Expected Worklet context to have a worklet call invoker.");
    }
    _workletCallInvoker(std::move(fp));
  }

  /**
   * Returns true if the runtime provided is the worklet runtime
   * @param runtime Runtime to check
   * @return True if the runtime is the worklet runtime
   */
  bool isWorkletRuntime(jsi::Runtime &runtime) {
    auto isWorklet = runtime.global().getProperty(runtime, WorkletRuntimeFlag);
    return isWorklet.isBool() && isWorklet.getBool();
  }

  /**
   Decorates the worklet runtime.
   */
  void decorate(JsiBaseDecorator* decorator) {
    decorator->decorateRuntime(*_workletRuntime);
  }

private:
  jsi::Runtime *_jsRuntime;
  std::unique_ptr<jsi::Runtime> _workletRuntime;
  std::function<void(const std::exception &)> _raise;
  std::string _name;
  std::function<void(std::function<void()> &&)> _jsCallInvoker;
  std::function<void(std::function<void()> &&)> _workletCallInvoker;
};
} // namespace RNWorklet
