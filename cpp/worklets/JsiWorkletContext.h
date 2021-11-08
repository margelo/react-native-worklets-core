#pragma once

#include <jsi/jsi.h>

#include <map>
#include <vector>

#include <ReactCommon/CallInvoker.h>

#include <DispatchQueue.h>
#include <JsRuntimeFactory.h>
#include <JsiWrapper.h>

namespace RNWorklet {

using namespace facebook;
using namespace RNSkia;

class JsiSharedValue;
class JsiWrapper;

using JsiErrorHandler = std::function<void(const std::exception &ex)>;

/**
 * A worklet context holds the worklet runtime and the cache of installed
 * worklets for that runtime. It also contains methods for running the worklet
 * in the worklet thread.
 */
class JsiWorkletContext {
public:
  const char *WorkletRuntimeFlag = "__WORKLET_RUNTIME";

  /**
   * Constructs a new worklet context.
   * @param jsRuntime Javascript Runtime
   * @param jsCallInvoker Call invoker for js runtime
   * @param errorHandler Callback for handling errors
   */
  JsiWorkletContext(jsi::Runtime *jsRuntime,
                    std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
                    std::shared_ptr<JsiErrorHandler> errorHandler);


  /*
     Constructor that creates a new worklet context with separate
     runtime and thread from an existing context
     @param context Context to create a new worklet context from
  */
  JsiWorkletContext(JsiWorkletContext *context)
      : JsiWorkletContext(context->_jsRuntime, context->_jsCallInvoker,
                          context->_errorHandler) {}

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
   * Calls the active error handler with the error. Will not throw an
   * exception, so you need to call return after calling this method or
   * return the results of this function which is an undefined jsi value.
   * @param err Error to raise
   * @returns An undefined jsi value
   */
  jsi::Value raiseError(const std::exception &err) {
    (*_errorHandler.get())(err);
    return jsi::Value::undefined();
  }

  /**
   * Calls the active error handler with the error. Will not throw an
   * exception, so you need to call return after calling this method or
   * return the results of this function which is an undefined jsi value.
   * @param message Error message
   * @returns An undefined jsi value
   */
  jsi::Value raiseError(const std::string &message) {
    return raiseError(std::runtime_error(message));
  }

  /**
   * Returns the runtime for this worklet context
   * @return Runtime
   */
  jsi::Runtime &getWorkletRuntime() { return *_workletRuntime; }

  /**
   * Returns the source runtime - this should be the main JS runtime where the
   * worklet was initially defined.
   * @return Runtime
   */
  jsi::Runtime *getJsRuntime() { return _jsRuntime; }

  /**
   * Runs the function on the worklet thread. The function should only
   * contain code accessing the worklet runtime
   * @param fp Function to run
   */
  void runOnWorkletThread(std::function<void()> fp);

  /**
   * Runs the function on the javascript thread
   * @param fp Function to run
   */
  void runOnJavascriptThread(std::function<void()> fp);

  /**
   * Creates a worklet - a function - that will run on the worklet
   * runtime/thread
   * @param runtime Calling runtime
   * @param context Calling context
   * @param value Function value
   * @return A function that can be run on the worklet thread/runtime
   */
  jsi::HostFunctionType createWorklet(jsi::Runtime &runtime,
                                      const jsi::Value &context,
                                      const jsi::Value &value);

private:
  /**
   * Evaluates the javascript code and returns a function
   * @param code Code representing a function
   * @return Jsi Function
   */
  jsi::Function evalWorkletCode(const std::string &code);

  // The main JS JSI Runtime
  jsi::Runtime *_jsRuntime;

  // This context's worklet JSI Runtime
  std::unique_ptr<jsi::Runtime> _workletRuntime;

  // Dispatch Queue
  std::unique_ptr<JsiDispatchQueue> _dispatchQueue;

  // Call invoker
  std::shared_ptr<facebook::react::CallInvoker> _jsCallInvoker;

  // Error handler
  std::shared_ptr<JsiErrorHandler> _errorHandler;
};
} // namespace RNWorklet
