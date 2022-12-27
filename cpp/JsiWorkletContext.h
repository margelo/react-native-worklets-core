
#pragma once

#include "DispatchQueue.h"
#include "JsiBaseDecorator.h"
#include "JsiHostObject.h"

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiWorkletContext : public JsiHostObject {
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
   Static / singleton default context
   */
  static std::shared_ptr<JsiWorkletContext> getInstance() {
    if (instance == nullptr) {
      instance = std::make_shared<JsiWorkletContext>();
    }
    return instance;
  }

  /**
   Invalidates the instance
   */
  static void invalidateInstance() { instance = nullptr; }

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
  void invokeOnJsThread(std::function<void()> &&fp);

  /**
   Executes a function in the worklet thread
   */
  void invokeOnWorkletThread(std::function<void()> &&fp);

  /**
   Returns the list of decorators
   */
  static const std::vector<std::shared_ptr<JsiBaseDecorator>> &getDecorators() {
    return decorators;
  }

  /**
   Adds a global decorator. The decorator will be installed in the default
   context.
   */
  static void addDecorator(std::shared_ptr<JsiBaseDecorator> decorator);

private:
  /**
   Decorates the worklet runtime. The decorator is run in the worklet runtime
   and on the worklet thread, since it is not legal to access the worklet
   runtime from the javascript thread.
   */
  template <typename... Args> void decorate(Args &&...args);

  /**
   Decorates the worklet runtime. The decorator is run in the worklet runtime
   and on the worklet thread, since it is not legal to access the worklet
   runtime from the javascript thread.
   */
  void applyDecorators(
      const std::vector<std::shared_ptr<JsiBaseDecorator>> &decorators);

  jsi::Runtime *_jsRuntime;
  std::unique_ptr<jsi::Runtime> _workletRuntime;
  std::string _name;
  std::function<void(std::function<void()> &&)> _jsCallInvoker;
  std::function<void(std::function<void()> &&)> _workletCallInvoker;

  static std::vector<std::shared_ptr<JsiBaseDecorator>> decorators;
  static std::shared_ptr<JsiWorkletContext> instance;
};

} // namespace RNWorklet
