#pragma once

#include <jsi/jsi.h>

#include <map>
#include <vector>

#if __has_include(<ReactCommon/CallInvoker.h>)
#include <ReactCommon/CallInvoker.h>
#endif

#if __has_include(<ReactCommon/JSCallInvoker.h>)
#include <ReactCommon/JSCallInvoker.h>
#define CallInvoker JSCallInvoker
#endif

#include <DispatchQueue.h>
#include <JsRuntimeFactory.h>
#include <JsiWrapper.h>

namespace RNWorklet {

using namespace facebook;
using namespace RNSkia;

class JsiSharedValue;
class JsiWrapper;

using JsiWorkletInfo = struct {
  std::shared_ptr<jsi::Function> worklet;
  std::shared_ptr<jsi::Function> original;
};

using JsiWorkletCache = std::map<long, JsiWorkletInfo>;
using JsiErrorHandler = std::function<void(const std::exception &ex)>;

/**
 * A worklet context holds the worklet runtime and the cache of installed
 * worklets for that runtime. It also contains methods for running the worklet
 * in the worklet thread.
 */
class JsiWorkletContext {
public:
  /**
   * Constructs a new worklet context.
   * @param jsRuntime Javascript Runtime
   * @param jsCallInvoker Call invoker for js runtime
   * @param errorHandler Callback for handling errors
   */
  JsiWorkletContext(jsi::Runtime *jsRuntime,
                    std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
                    std::shared_ptr<JsiErrorHandler> errorHandler);

  /**
    Destructor
  */
  ~JsiWorkletContext() {
    _workletsCache.clear();
    _jsRuntime = nullptr;
    _workletRuntime = nullptr;
    _dispatchQueue = nullptr;
    _jsCallInvoker = nullptr;
    _errorHandler = nullptr;
  }

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
    auto isWorklet = runtime.global().getProperty(runtime, "_WORKLET");
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
   * Returns the worklet hash for the given worklet
   * @param function Worklet to get hash for
   * @param runtime Runtime to get hash from
   * @return Worklet hash
   */
  double getWorkletHash(jsi::Runtime &runtime, const jsi::Function &function);

  /**
   * Returns the cache of worklets in this context
   * @return Cache of worklets. Key is the worklet's hash and the destination
   * runtime as a pointer
   */
  JsiWorkletCache &getWorkletsCache() { return _workletsCache; }

  /**
   * Install the worklet in the worklet runtime and returns a wrapper function
   * for calling the worklet.
   * @param function Worklet to install
   * @return A jsi::Function that can be called
   */
  jsi::HostFunctionType getWorklet(const jsi::Value &function);

  /**
   * Install the worklet in the worklet runtime and returns a wrapper function
   * for calling the worklet.
   * @param worklet Worklet to install
   * @return A jsi::Function that can be called
   */
  jsi::HostFunctionType getWorklet(const jsi::Function &worklet);

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
   * Returns a list of shared values that are dependencies in the worklet
   * closure
   * @param function Function to get dependant values from
   * @return List of shared values
   */
  std::vector<JsiSharedValue *> getSharedValues(const jsi::Function &function);

  std::shared_ptr<jsi::Function> evalWorkletCode(const std::string &code);

  jsi::HostFunctionType createWorklet(jsi::Runtime &runtime,
                                      const jsi::Value &context,
                                      const jsi::Value &value);

private:
  /**
   * Copies from the main js context to the worklet context
   * @param value Value to copy
   * @return Returns a copy of the value on the worklet runtime
   */
  std::shared_ptr<JsiWrapper> copyToWorkletRuntime(const jsi::Value &value);

  /**
   * Returns a worklet if the provided value is a worklet.
   * @param value Function to get worklet from
   * @return Returns a worklet function if the provided value is a worklet,
   * otherwise the function will throw a Js error.
   */
  jsi::Function getWorkletOrThrow(const jsi::Value &value);

  /**
   * Returns the worklet's closure for the worklet.
   * @param worklet Worklet to get closure for
   * @return Closure or undefined
   */
  jsi::Value getWorkletClosure(const jsi::Function &worklet);

  // The main JS JSI Runtime
  jsi::Runtime *_jsRuntime;

  // Cached worklet wrappers
  JsiWorkletCache _workletsCache;

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
