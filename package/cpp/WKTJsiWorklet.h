#pragma once

#include <jsi/jsi.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "WKTJsiHostObject.h"
#include "WKTJsiWorkletContext.h"
#include "WKTJsiWrapper.h"
#include "WKTRuntimeAwareCache.h"

namespace RNWorklet {

static const char *PropNameWorkletHash = "__workletHash";
static const char *PropNameWorkletInitData = "__initData";
static const char *PropNameWorkletInitDataCode = "code";

static const char *PropNameJsThis = "jsThis";

static const char *PropNameWorkletInitDataLocation = "location";
static const char *PropNameWorkletInitDataSourceMap = "sourceMap";

static const char *PropNameWorkletLocation = "__location";
static const char *PropNameWorkletAsString = "asString";

static const char *PropNameWorkletClosure = "__closure";
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
  Class for wrapping jsThis when executing worklets
  */
class JsThisWrapper {
public:
  JsThisWrapper(jsi::Runtime &runtime, const jsi::Object &thisValue) {
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

    return isDecoratedAsWorklet(
        runtime, std::make_shared<jsi::Function>(
                     value.asObject(runtime).asFunction(runtime)));
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

    // Try to get the code
    auto initData = func->getProperty(runtime, PropNameWorkletInitData);
    if (!initData.isObject()) {
      // Try old way of getting code
      auto asString = func->getProperty(runtime, PropNameWorkletAsString);
      if (!asString.isString()) {
        return false;
      }
    }

    return true;
  }

  /**
   Creates a jsi::Function in the provided runtime for the worklet. This
   function can then be used to execute the worklet
   */
  std::shared_ptr<jsi::Function>
  createWorkletJsFunction(jsi::Runtime &runtime) {
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

    return std::make_shared<jsi::Function>(
        evaluatedFunction.asObject(runtime).asFunction(runtime));
  }

  /**
   Calls the Worklet function with the given arguments.
   */
  jsi::Value call(std::shared_ptr<jsi::Function> workletFunction,
                  jsi::Runtime &runtime, const jsi::Value &thisValue,
                  const jsi::Value *arguments, size_t count) {

    // Unwrap closure
    auto unwrappedClosure = JsiWrapper::unwrap(runtime, _closureWrapper);

    if (_isRea30Compat) {

      // Resolve this Value
      std::unique_ptr<jsi::Object> resolvedThisValue;
      if (!thisValue.isObject()) {
        resolvedThisValue = std::make_unique<jsi::Object>(runtime);
      } else {
        resolvedThisValue =
            std::make_unique<jsi::Object>(thisValue.asObject(runtime));
      }

      resolvedThisValue->setProperty(runtime, PropNameWorkletClosure,
                                     unwrappedClosure);

      // Call the unwrapped function
      return workletFunction->callWithThis(runtime, *resolvedThisValue,
                                           arguments, count);
    } else {
      // Prepare jsThis
      jsi::Object jsThis(runtime);
      jsThis.setProperty(runtime, PropNameWorkletClosure, unwrappedClosure);
      JsThisWrapper thisWrapper(runtime, jsThis);

      // Call the unwrapped function
      if (thisValue.isObject()) {
        return workletFunction->callWithThis(
            runtime, thisValue.asObject(runtime), arguments, count);
      } else {
        return workletFunction->call(runtime, arguments, count);
      }
    }
  }

  /**
   Returns true if the character is a whitespace character
   */
  static bool isWhitespace(unsigned char c) { return std::isspace(c); }

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

    createWorklet(runtime, std::make_shared<jsi::Function>(
                               arg.asObject(runtime).asFunction(runtime)));
  }

  /**
   Installs the worklet function into the worklet runtime
   */
  void createWorklet(jsi::Runtime &runtime,
                     std::shared_ptr<jsi::Function> func) {

    // This is a worklet
    _isWorklet = false;

    // Try to get the closure
    jsi::Value closure = func->getProperty(runtime, PropNameWorkletClosure);

    // Return if this is not a worklet
    if (closure.isUndefined() || closure.isNull()) {
      return;
    }

    // Try to get the asString function
    jsi::Value initDataProp =
        func->getProperty(runtime, PropNameWorkletInitData);

    if (initDataProp.isObject()) {
      // Get location
      jsi::Value locationProp = initDataProp.asObject(runtime).getProperty(
          runtime, PropNameWorkletInitDataLocation);

      // Check location must be a string if it is defined
      if (!locationProp.isUndefined() && !locationProp.isNull() &&
          locationProp.isString()) {
        // Set location
        _location = locationProp.asString(runtime).utf8(runtime);
      } else {
        _location = "(unknown)";
      }

      // Let us try to install the function in the worklet context
      _code = initDataProp.asObject(runtime)
                  .getProperty(runtime, PropNameWorkletInitDataCode)
                  .asString(runtime)
                  .utf8(runtime);

      // Set compat level
      _isRea30Compat = true;

    } else {
      // try old way
      _code = func->getProperty(runtime, PropNameWorkletAsString)
                  .asString(runtime)
                  .utf8(runtime);
      _location = func->getProperty(runtime, PropNameWorkletLocation)
                      .asString(runtime)
                      .utf8(runtime);
    }

    // Double-check if the code property is valid.
    bool isCodeEmpty = std::all_of(_code.begin(), _code.end(), isWhitespace);
    if (isCodeEmpty) {
      std::string error =
          "Failed to create Worklet, the provided code is empty. Tips:\n"
          "* Is the babel plugin correctly installed?\n"
          "* If you are using react-native-reanimated, make sure the "
          "react-native-reanimated plugin does not override the "
          "react-native-worklets-core/plugin.\n"
          "* Make sure the JS Worklet contains a \"" +
          std::string(PropNameWorkletInitDataCode) +
          "\" property with the function's code.";
      throw jsi::JSError(runtime, error);
    }

    // This is a worklet
    _isWorklet = true;

    // Create closure wrapper so it will be accessible across runtimes
    _closureWrapper = JsiWrapper::wrap(runtime, closure);

    // Try get the name of the function
    auto nameProp = func->getProperty(runtime, PropFunctionName);
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
  std::shared_ptr<JsiWrapper> _closureWrapper;
  std::string _location = "";
  std::string _code = "";
  std::string _name = "fn";
  std::string _hash;
  bool _isRea30Compat = false;
  double _workletHash = 0;
};

class WorkletInvoker {
public:
  explicit WorkletInvoker(std::shared_ptr<JsiWorklet> worklet)
      : _worklet(worklet) {}
  WorkletInvoker(jsi::Runtime &runtime, const jsi::Value &value)
      : WorkletInvoker(std::make_shared<JsiWorklet>(runtime, value)) {}

  jsi::Value call(jsi::Runtime &runtime, const jsi::Value &thisValue,
                  const jsi::Value *arguments, size_t count) {
    if (_workletFunction.get(runtime) == nullptr) {
      _workletFunction.get(runtime) =
          _worklet->createWorkletJsFunction(runtime);
    }
    return _worklet->call(_workletFunction.get(runtime), runtime, thisValue,
                          arguments, count);
  }

private:
  RuntimeAwareCache<std::shared_ptr<jsi::Function>> _workletFunction;
  std::shared_ptr<JsiWorklet> _worklet;
};
} // namespace RNWorklet
