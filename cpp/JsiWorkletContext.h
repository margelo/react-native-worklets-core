
#pragma once

#include "JsRuntimeFactory.h"
#include "JsiHostObject.h"

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
   * Ctor for the JsiWorkletContext
   * @param name Name of the context
   * @param jsRuntime Runtime for the main javascript runtime.
   * @param raiseError Callback for raising errors
   */
  JsiWorkletContext(const std::string& name,
                    jsi::Runtime *jsRuntime,
                    std::function<void(const std::exception &)> raiseError)
      : _name(name), _jsRuntime(jsRuntime), _raise(raiseError) {}
  
  /**
   * Ctor for the JsiWorkletContext
   * @param name Name of the context
   * @param parentContext parent worklet context
   */
  JsiWorkletContext(const std::string& name,
                    std::shared_ptr<JsiWorkletContext> parentContext)
      : _name(name), _jsRuntime(parentContext->getJsRuntime()), _raise(parentContext->_raise) {}

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
  const std::string& getName() { return _name; }

  /**
   Returns the worklet runtime. Lazy evaluated
   */
  jsi::Runtime &getWorkletRuntime() {
    if (_workletRuntime == nullptr) {
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
   * Returns true if the runtime provided is the worklet runtime
   * @param runtime Runtime to check
   * @return True if the runtime is the worklet runtime
   */
  bool isWorkletRuntime(jsi::Runtime &runtime) {
    auto isWorklet = runtime.global().getProperty(runtime, WorkletRuntimeFlag);
    return isWorklet.isBool() && isWorklet.getBool();
  }

private:
  jsi::Runtime *_jsRuntime;
  std::unique_ptr<jsi::Runtime> _workletRuntime;
  std::function<void(const std::exception &)> _raise;
  std::string _name;
};
} // namespace RNWorklet
