#pragma once

#include <jsi/jsi.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "JsiHostObject.h"
#include "JsiWrapper.h"
namespace RNWorklet {

static const char *PropNameWorkletHash = "__workletHash";
static const char *PropNameWorkletInitData = "__initData";
static const char *PropNameWorkletInitDataCode = "code";

static const char *PropNameWorkletInitDataLocation = "location";
static const char *PropNameWorkletInitDataSourceMap = "__sourceMap";

static const char *PropNameWorkletClosure = "_closure";
static const char *PropFunctionName = "name";

namespace jsi = facebook::jsi;

/**
 Class for wrapping jsi::JSError
 */
class JsErrorWrapper : public std::exception {
public:
  JsErrorWrapper(std::string message, std::string stack)
      : _message(std::move(message)), _stack(std::move(stack)) {}
  const std::string &getStack() const { return _stack; }
  const std::string &getMessage() const { return _message; }
  const char *what() const noexcept override { return _message.c_str(); }

private:
  std::string _message;
  std::string _stack;
};

/**
 Encapsulates a runnable function. A runnable function is a function
 that exists in both the main JS runtime and as an installed function
 in the worklet runtime. The worklet object exposes some methods and props
 for handling this. The worklet exists in a given worklet context.
 */
class JsiWorklet : public JsiHostObject,
                   public std::enable_shared_from_this<JsiWorklet> {
public:
  JsiWorklet(jsi::Runtime &runtime, const jsi::Value &arg) {
    createWorklet(runtime, arg);
  }
                     
  JsiWorklet(jsi::Runtime &runtime, std::shared_ptr<jsi::Function> func) {
   createWorklet(runtime, func);
  }

  JSI_HOST_FUNCTION(isWorklet) { return isWorklet(); }

  JSI_HOST_FUNCTION(getCode) {
    return jsi::String::createFromUtf8(runtime, _code);
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorklet, isWorklet),
                       JSI_EXPORT_FUNC(JsiWorklet, getCode))

  /**
   Returns true if the function is a worklet
   */
  bool isWorklet() { return _isWorklet; }

  /**
   Returns the name of the worklet function
   */
  const std::string getName(const std::string defaultName = "") {
    if (_name != "") {
      return _name;
    }
    return defaultName;
  }

  /**
   Returns the source location for the worklet
   */
  const std::string &getLocation() { return _location; }

  /**
  Returns true if the provided function is decorated with worklet info
  @runtime Runtime
  @value Function to check
  */
  static bool isDecoratedAsWorklet(jsi::Runtime &runtime,
                                   const jsi::Value &value) {
    if (!value.isObject()) {
      return false;
    }

    // get object
    auto obj = value.asObject(runtime);
    if (!obj.isFunction(runtime)) {
      return false;
    }

    return isDecoratedAsWorklet(runtime, std::make_shared<jsi::Function>(value.asObject(runtime).asFunction(runtime)));
  }
                     
  /**
   Returns true if the provided function is decorated with worklet info
   @runtime Runtime
   @value Function to check
   */
  static bool isDecoratedAsWorklet(jsi::Runtime &runtime,
                                   std::shared_ptr<jsi::Function> func) {
    auto hashProp = func->getProperty(runtime, PropNameWorkletHash);
    if (hashProp.isString()) {
      return true;
    }

    // Try to get the closure
    jsi::Value closure = func->getProperty(runtime, PropNameWorkletClosure);

    // Return if this is not a worklet
    if (closure.isUndefined() || closure.isNull()) {
      return false;
    }

    // Try to get the asString function
    jsi::Value initData = func->getProperty(runtime, PropNameWorkletInitData);
    if (initData.isUndefined() || initData.isNull() ||
        !initData.isObject()) {
      return false;
    }

    return true;
  }

  /**
   Creates a jsi::Function in the provided runtime for the worklet. This
   function can then be used to execute the worklet
   */
  std::shared_ptr<jsi::Function> getWorkletJsFunction(jsi::Runtime &runtime) {
    if (_workletJsFunction == nullptr) {
      auto evaluatedFunction =
          evaluteJavascriptInWorkletRuntime(runtime, _code);

      if (!evaluatedFunction.isObject()) {
        throw jsi::JSError(
            runtime, std::string("Could not create worklet from function. ") +
                         "Eval did not return an object:\n" + _code);
      }

      if (!evaluatedFunction.asObject(runtime).isFunction(runtime)) {
        throw jsi::JSError(
            runtime, std::string("Could not create worklet from function. ") +
                         "Eval did not return a function:\n" + _code);
      }

      _workletJsFunction = std::make_shared<jsi::Function>(
          evaluatedFunction.asObject(runtime).asFunction(runtime));
    }

    return _workletJsFunction;
  }

  /**
   Calls the
   */
  jsi::Value call(jsi::Runtime &runtime, const jsi::Value &thisValue,
                  const jsi::Value *arguments, size_t count) {

    // Unwrap closure
    auto unwrappedClosure = JsiWrapper::unwrap(runtime, _closureWrapper);

    // Get the worklet function - the js function wrapping a call to
    // the js code.
    auto workletFunction = getWorkletJsFunction(runtime);

    // Prepare return value
    jsi::Value retVal;
    
    // Resolve this Value
    std::unique_ptr<jsi::Object> resolvedThisValue;
    if (!thisValue.isObject()) {
      resolvedThisValue = std::make_unique<jsi::Object>(runtime);
    } else {
      resolvedThisValue = std::make_unique<jsi::Object>(thisValue.asObject(runtime));
    }
    
    resolvedThisValue->setProperty(runtime, PropNameWorkletClosure, unwrappedClosure);

    // Call the unwrapped function
    retVal = workletFunction->callWithThis(
        runtime, *resolvedThisValue, arguments, count);
  
    return retVal;
  }

private:
  /**
   Installs the worklet function into the worklet runtime
  */
  void createWorklet(jsi::Runtime &runtime, const jsi::Value &arg) {
    // Make sure arg is a function
    if (!arg.isObject() || !arg.asObject(runtime).isFunction(runtime)) {
      throw jsi::JSError(runtime,
                         "Worklets must be initialized from a valid function.");
    }
    
    createWorklet(runtime, std::make_shared<jsi::Function>(arg.asObject(runtime).asFunction(runtime)));
  }
                     
  /**
   Installs the worklet function into the worklet runtime
   */
  void createWorklet(jsi::Runtime &runtime, std::shared_ptr<jsi::Function> func) {

    // This is a worklet
    _isWorklet = false;

    // Save pointer to function
    _jsFunction = func;

    // Try to get the closure
    jsi::Value closure = _jsFunction->getProperty(runtime, PropNameWorkletClosure);

    // Return if this is not a worklet
    if (closure.isUndefined() || closure.isNull()) {
      return;
    }

    // Try to get the asString function
    jsi::Value initDataProp =
        _jsFunction->getProperty(runtime, PropNameWorkletInitData);
    if (initDataProp.isUndefined() || initDataProp.isNull() ||
        !initDataProp.isObject()) {
      return;
    }

    // Get location
    jsi::Value locationProp =
        initDataProp.asObject(runtime).getProperty(runtime, PropNameWorkletInitDataLocation);
    
    if (locationProp.isUndefined() || locationProp.isNull() ||
        !locationProp.isString()) {
      return;
    }

    // Set location
    _location = locationProp.asString(runtime).utf8(runtime);

    // This is a worklet
    _isWorklet = true;

    // Create closure wrapper so it will be accessible across runtimes
    _closureWrapper = JsiWrapper::wrap(runtime, closure);

    // Let us try to install the function in the worklet context
    _code = initDataProp.asObject(runtime).getProperty(runtime, PropNameWorkletInitDataCode).asString(runtime).utf8(runtime);

    // Try get the name of the function
    auto nameProp = _jsFunction->getProperty(runtime, PropFunctionName);
    if (nameProp.isString()) {
      _name = nameProp.asString(runtime).utf8(runtime);
    }
  }

  jsi::Value evaluteJavascriptInWorkletRuntime(jsi::Runtime &runtime,
                                               const std::string &code) {
    auto codeBuffer =
        std::make_shared<const jsi::StringBuffer>("(" + code + "\n)");
    return runtime.evaluateJavaScript(codeBuffer, _location);
  }

  bool _isWorklet = false;
  std::shared_ptr<jsi::Function> _jsFunction;
  std::shared_ptr<jsi::Function> _workletJsFunction;
  std::shared_ptr<JsiWrapper> _closureWrapper;
  std::string _location = "";
  std::string _code = "";
  std::string _name = "fn";
  std::string _hash;
  double _workletHash = 0;
};
} // namespace RNWorklet
