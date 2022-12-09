#pragma once

#include <ReactCommon/TurboModuleUtils.h>
#include <jsi/jsi.h>

#include "JsiHostObject.h"
#include "JsiWorkletContext.h"
#include "JsiWrapper.h"

namespace RNWorklet {

static const char *PropHiddenWorkletName = "__worklet";
static const char *PropClosureName = "_closure";
static const char *PropAsStringName = "asString";
static const char *PropLocationName = "__location";
static const char *PropNameJsThis = "jsThis";

namespace jsi = facebook::jsi;
namespace react = facebook::react;

class JsThisWrapper {
public:
  JsThisWrapper(jsi::Runtime &runtime, const jsi::Value &thisValue) {
    _oldThis = runtime.global().getProperty(runtime, PropNameJsThis);
    runtime.global().setProperty(runtime, PropNameJsThis, thisValue);
    _runtime = &runtime;
  }

  ~JsThisWrapper() {
    _runtime->global().setProperty(*_runtime, PropNameJsThis, _oldThis);
  }

private:
  jsi::Value _oldThis;
  jsi::Runtime *_runtime;
};

/// Class for wrapping jsi::JSError
class JsErrorWrapper : public std::exception {
public:
  JsErrorWrapper(std::string message, std::string stack)
      : _message(std::move(message)), _stack(std::move(stack)){};
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
class JsiWorklet : public JsiHostObject {
public:
  JsiWorklet(jsi::Runtime &runtime, const jsi::Value &arg) {
    createWorklet(runtime, arg);
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
   Returns the source location for the worklet
   */
  const std::string &getLocation() { return _location; }

  /**
   Returns the JsiWorklet object from the provided function. The function MUST
   be a javascript decorated worklet function
   */
  static std::shared_ptr<JsiWorklet> getWorklet(jsi::Runtime &runtime,
                                                const jsi::Value &value) {
    if (!value.isObject()) {
      throw std::runtime_error("Expected an object / function when calling the "
                               "getWorklet function.");
    }

    auto obj = value.asObject(runtime);
    if (!obj.isFunction(runtime)) {
      throw std::runtime_error(
          "Expected a function when calling the getWorklet function.");
    }
    auto workletProp = obj.getProperty(runtime, PropHiddenWorkletName);
    if (!workletProp.isObject()) {
      // Create and associate worklet
      /*auto context = JsiWorkletContext::getContextInRuntime(runtime);
      auto worklet =
          std::make_shared<JsiWorklet>(context->shared_from_this(), value);
      obj.setProperty(runtime, PropHiddenWorkletName,
                      jsi::Object::createFromHostObject(runtime, worklet));
      return worklet;*/
    }
    // return the worklet
    return workletProp.asObject(runtime).asHostObject<JsiWorklet>(runtime);
  }

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

    // We have a quick path here - we add the worklet as a hidden property on
    // the function object!
    auto workletProp = obj.getProperty(runtime, PropHiddenWorkletName);
    if (workletProp.isObject()) {
      return true;
    }

    // get function
    jsi::Function jsFunction = obj.asFunction(runtime);

    // Try to get the closure
    jsi::Value closure = jsFunction.getProperty(runtime, PropClosureName);

    // Return if this is not a worklet
    if (closure.isUndefined() || closure.isNull()) {
      return false;
    }

    // Try to get the asString function
    jsi::Value asStringProp = jsFunction.getProperty(runtime, PropAsStringName);
    if (asStringProp.isUndefined() || asStringProp.isNull() ||
        !asStringProp.isString()) {
      return false;
    }

    // Get location
    jsi::Value locationProp = jsFunction.getProperty(runtime, PropLocationName);
    if (locationProp.isUndefined() || locationProp.isNull() ||
        !locationProp.isString()) {
      return false;
    }

    return true;
  }

  /**
   Creates a jsi::Function in the provided runtime
   */
  jsi::Function createJsFunctionInRuntime(jsi::Runtime &runtime) {
    auto evaluatedFunction = evaluteJavascriptInWorkletRuntime(runtime, _code);
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

    return evaluatedFunction.asObject(runtime).asFunction(runtime);
  }

  /**
   Calls the
   */
  jsi::Value call(jsi::Runtime &runtime, const jsi::Value *arguments,
                  size_t count) {
    // Wrap the arguments
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
    argsWrapper.reserve(count);
    for (size_t i = 0; i < count; i++) {
      argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
    }

    return call(runtime, argsWrapper);
  }

  /**
     Calls the worklet in the provided runtime
     */
  jsi::Value call(jsi::Runtime &runtime,
                  const std::vector<std::shared_ptr<JsiWrapper>> &argsWrapper) {
    // Unwrap closure
    auto unwrappedClosure = JsiWrapper::unwrap(runtime, _closureWrapper);

    // Create arguments
    size_t size = argsWrapper.size();
    std::vector<jsi::Value> args(size);

    // Add the rest of the arguments
    for (size_t i = 0; i < size; i++) {
      args[i] = JsiWrapper::unwrap(runtime, argsWrapper.at(i));
    }

    // Create the worklet function
    auto workletFunction = createJsFunctionInRuntime(runtime);

    // Prepare return value
    jsi::Value retVal;

    // Prepare jsThis - we don't use it in our plugin, but REA uses it
    // when decorating worklet functions. TODO: We need to solve this,
    // should we do as REA or just use this like we do?
    auto thisWrapper =
        std::make_unique<JsThisWrapper>(runtime, unwrappedClosure);

    // Call the unwrapped function
    if (!unwrappedClosure.isObject()) {
      retVal = workletFunction.call(
          runtime, static_cast<const jsi::Value *>(args.data()),
          argsWrapper.size());
    } else {
      retVal = workletFunction.callWithThis(
          runtime, unwrappedClosure.asObject(runtime),
          static_cast<const jsi::Value *>(args.data()), argsWrapper.size());
    }

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

    // This is a worklet
    _isWorklet = false;

    // Save pointer to function
    _jsFunction = std::make_shared<jsi::Function>(
        arg.asObject(runtime).asFunction(runtime));

    // Try to get the closure
    jsi::Value closure = _jsFunction->getProperty(runtime, PropClosureName);

    // Return if this is not a worklet
    if (closure.isUndefined() || closure.isNull()) {
      return;
    }

    // Try to get the asString function
    jsi::Value asStringProp =
        _jsFunction->getProperty(runtime, PropAsStringName);
    if (asStringProp.isUndefined() || asStringProp.isNull() ||
        !asStringProp.isString()) {
      return;
    }

    // Get location
    jsi::Value locationProp =
        _jsFunction->getProperty(runtime, PropLocationName);
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
    _code = asStringProp.asString(runtime).utf8(runtime);
  }

  jsi::Value evaluteJavascriptInWorkletRuntime(jsi::Runtime &runtime,
                                               const std::string &code) {
    auto codeBuffer =
        std::make_shared<const jsi::StringBuffer>("(" + code + "\n)");
    return runtime.evaluateJavaScript(codeBuffer, _location);
  }

  bool _isWorklet = false;
  std::shared_ptr<jsi::Function> _jsFunction;
  std::shared_ptr<JsiWrapper> _closureWrapper;
  std::string _location = "";
  std::string _code = "";
  double _workletHash = 0;
};
} // namespace RNWorklet
