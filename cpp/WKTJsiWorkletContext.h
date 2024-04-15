
#pragma once

#include "WKTDispatchQueue.h"
#include "WKTJsiBaseDecorator.h"
#include "WKTJsiHostObject.h"
#include "WKTJsiJsDecorator.h"

#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiWorkletContext
    : public JsiHostObject,
      public std::enable_shared_from_this<JsiWorkletContext> {
public:
  /**
   * Creates a new worklet context that can be used to run javascript functions
   * as worklets in a separate threads / js runtimes. After construction the
   * context needs to be initialized.
   */
  JsiWorkletContext() {}

  /**
   Constructs a new worklet context using the same values and configuration as
   the default context. No need to run initialize on the runtime. The only thing
   not copied from the default context is the worklet invoker which will be a
   default queue.
   @param name Name of the context
   */
  explicit JsiWorkletContext(const std::string &name);

  /**
   Constructs a new worklet context using the same values and configuration as
   the default context. No need to run initialize on the runtime.
   @param name Name of the context
   @param workletCallInvoker Callback for running a function on the worklet
   thread
   */
  JsiWorkletContext(
      const std::string &name,
      std::function<void(std::function<void()> &&)> workletCallInvoker);

  /**
   Constructs a new worklet context using the given runtime and call invokers.
   @param name Name of the context
   @param jsRuntime The main JS Runtime
   @param jsCallInvoker Callback for running a function on the main React JS
   Thread
   @param workletCallInvoker Callback for running a function on the worklet
   thread
   */
  JsiWorkletContext(
      const std::string &name, jsi::Runtime *jsRuntime,
      std::function<void(std::function<void()> &&)> jsCallInvoker,
      std::function<void(std::function<void()> &&)> workletCallInvoker);

  /**
   Destructor
   */
  ~JsiWorkletContext();

  /**
   * Initialializes the worklet context
   * @param name Name of the context
   * @param jsRuntime Main javascript runtime.
   * @param jsCallInvoker Callback for running a function on the JS thread.
   * @param workletCallInvoker Callback for running a function on the worklet
   * thread
   */
  void
  initialize(const std::string &name, jsi::Runtime *jsRuntime,
             std::function<void(std::function<void()> &&)> jsCallInvoker,
             std::function<void(std::function<void()> &&)> workletCallInvoker);

  /**
   * Initialializes the worklet context
   * @param name Name of the context
   * @param jsRuntime Runtime for the main javascript runtime.
   * @param jsCallInvoker Callback for running a function on the JS thread.
   */
  void initialize(const std::string &name, jsi::Runtime *jsRuntime,
                  std::function<void(std::function<void()> &&)> jsCallInvoker);

  /**
   Get the default context
   */
  static std::shared_ptr<JsiWorkletContext> getDefaultInstanceAsShared() {
    if (defaultInstance == nullptr) {
      defaultInstance = std::make_shared<JsiWorkletContext>();
    }
    return defaultInstance;
  }

  /**
   * @brief Get the Default Instance object
   *
   * @return JsiWorkletContext*
   */
  static JsiWorkletContext *getDefaultInstance() {
    return JsiWorkletContext::getDefaultInstanceAsShared().get();
  }

  /**
   Returns the worklet context for the current thread. If called from the
   JS thread (or any other invalid context thread) nullptr is returned.
   */
  static JsiWorkletContext *getCurrent(jsi::Runtime &runtime) {
    auto rtPtr = static_cast<void *>(&runtime);
    if (runtimeMappings.count(rtPtr) != 0) {
      return runtimeMappings.at(rtPtr);
    }
    return nullptr;
  }

  size_t getContextId() { return _contextId; }

  /**
   Adds a global decorator. The decorator will be installed in the default
   context.
   */
  void addDecorator(std::shared_ptr<JsiBaseDecorator> decorator);

  /**
   Invalidates the instance
   */
  static void invalidateDefaultInstance() { defaultInstance = nullptr; }

  JSI_HOST_FUNCTION(addDecorator) {
    if (count != 2) {
      throw jsi::JSError(runtime, "addDecorator expects a property name and a "
                                  "Javascript object as its arguments.");
    }
    if (!arguments[0].isString()) {
      throw jsi::JSError(runtime, "addDecorator expects a property name and a "
                                  "Javascript object as its arguments.");
    }

    if (!arguments[1].isObject()) {
      throw jsi::JSError(runtime, "addDecorator expects a property name and a "
                                  "Javascript object as its arguments.");
    }

    // Create / add the decorator
    addDecorator(std::make_shared<JsiJsDecorator>(
        runtime, arguments[0].asString(runtime).utf8(runtime), arguments[1]));

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(createRunAsync) {
    if (count != 1) {
      throw jsi::JSError(runtime, "createRunAsync expects one parameter.");
    }

    auto caller =
        JsiWorkletContext::createCallInContext(runtime, arguments[0], this);

    // Now let us create the caller function.
    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forAscii(runtime, "createRunAsync"), 0,
        caller);
  }

  JSI_HOST_FUNCTION(runAsync) {
    jsi::Value value = createRunAsync(runtime, thisValue, arguments, count);
    jsi::Function func = value.asObject(runtime).asFunction(runtime);
    return func.call(runtime, nullptr, 0);
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorkletContext, addDecorator),
                       JSI_EXPORT_FUNC(JsiWorkletContext, createRunAsync),
                       JSI_EXPORT_FUNC(JsiWorkletContext, runAsync))

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
  jsi::Runtime &getWorkletRuntime();

  /**
   Executes a function in the JS thread
   */
  void invokeOnJsThread(std::function<void(jsi::Runtime &runtime)> &&fp);

  /**
   Executes a function in the worklet thread
   */
  void invokeOnWorkletThread(std::function<void(JsiWorkletContext *context,
                                                jsi::Runtime &runtime)> &&fp);

  static jsi::HostFunctionType createInvoker(jsi::Runtime &runtime,
                                             const jsi::Value *maybeFunc);

  /**
   Calls a worklet function in a given context (or in the JS context if the ctx
   parameter is null.
   @param runtime Runtime for the calling context
   @param maybeFunc Function to call - might be a worklet or might not - depends
   on wether we call cross context or not.
   @param ctx Context to call the function in
   @returns A host function type that will return a promise calling the
   maybeFunc.
   */
  static jsi::HostFunctionType createCallInContext(jsi::Runtime &runtime,
                                                   const jsi::Value &maybeFunc,
                                                   JsiWorkletContext *ctx);

  /**
   Calls a worklet function in a given context (or in the JS context if the ctx
   parameter is null.
   @param runtime Runtime for the calling context
   @param maybeFunc Function to call - might be a worklet or might not - depends
   on wether we call cross context or not.
   @returns A host function type that will return a promise calling the
   maybeFunc.
   */
  jsi::HostFunctionType createCallInContext(jsi::Runtime &runtime,
                                            const jsi::Value &maybeFunc);

  // Resolve type of call we're about to do
  typedef enum {
    JsToJs = 0,
    CtxToJs = 1,
    WithinCtx = 2,
    CtxToCtx = 3,
    JsToCtx = 4
  } CallingConvention;

  /**
   Returns the calling convention for a given from/to context. This method is
   used to find out if we are calling a worklet from or to the JS context or
   from to a Worklet context or a combination of these.
   */
  static CallingConvention getCallingConvention(JsiWorkletContext *fromContext,
                                                JsiWorkletContext *toContext);

  /**
   Verifies that the runtime is the correct runtime for the current context
   (worklet context or js context). NOTE: Only verifies in debug mode
   */
  static void verifyRuntime(jsi::Runtime &runtime) {
#if DEBUG
    auto ctx = JsiWorkletContext::getCurrent(runtime);
    if (ctx) {
      assert(&ctx->getWorkletRuntime() == &runtime &&
             "Worklet runtime is not the same as the provided runtime");
    } else {
      assert(JsiWorkletContext::getDefaultInstance()->getJsRuntime() ==
                 &runtime &&
             "Expected JS runtime, got other runtime");
    }
#endif
  }

private:
  jsi::Runtime *_jsRuntime;
  std::unique_ptr<jsi::Runtime> _workletRuntime;
  std::string _name;
  std::function<void(std::function<void()> &&)> _jsCallInvoker;
  std::function<void(std::function<void()> &&)> _workletCallInvoker;
  size_t _contextId;
  std::thread::id _jsThreadId;

  static std::shared_ptr<JsiWorkletContext> defaultInstance;
  static std::map<void *, JsiWorkletContext *> runtimeMappings;
  static size_t contextIdNumber;
};

} // namespace RNWorklet
