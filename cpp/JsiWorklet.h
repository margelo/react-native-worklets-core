#pragma once

#include <ReactCommon/TurboModuleUtils.h>
#include <jsi/jsi.h>

#include "JsiHostObject.h"
#include "JsiWorkletContext.h"
#include "JsiWrapper.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;
namespace react = facebook::react;

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
  JsiWorklet(std::shared_ptr<JsiWorkletContext> context, const jsi::Value &arg)
      : _context(context) {
    createWorklet(arg);
  }

  JSI_HOST_FUNCTION(isWorklet) { return isWorklet(); }

  JSI_HOST_FUNCTION(callInJSContext) {
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
    argsWrapper.reserve(count);

    for (size_t i = 0; i < count; i++) {
      argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
    }

    // Is this called on the javascript thread?
    if (!_context->isWorkletRuntime(runtime)) {
      // The we can just call it directly - we can also leave out any
      // error handling since this will be handled correctly.
      return callInJsRuntime(argsWrapper);
    }

    // Create simple dispatcher without promise since we don't support
    // promises on Android in the Javascript runtime. This will then run
    // blocking
    std::mutex mu;
    std::condition_variable cond;
    std::exception_ptr ex;
    std::shared_ptr<JsiWrapper> retVal;
    bool isFinished = false;
    std::unique_lock<std::mutex> lock(mu);

    _context->invokeOnJsThread([&]() {
      std::lock_guard<std::mutex> lock(mu);

      try {
        auto result = callInJsRuntime(argsWrapper);
        retVal = JsiWrapper::wrap(runtime, result);
      } catch (const jsi::JSError &err) {
        // When rethrowing we need to make sure that any jsi errors are
        // transferred to the correct runtime.
        try {
          throw JsErrorWrapper(err.getMessage(), err.getStack());
        } catch (...) {
          ex = std::current_exception();
        }
      } catch (const std::exception& err) {
        std::string a = typeid(err).name();
        std::string b = typeid(jsi::JSError).name();
        if (a == b) {
          const auto *jsError = static_cast<const jsi::JSError *>(&err);
          ex = std::make_exception_ptr(JsErrorWrapper(jsError->getMessage(), jsError->getStack()));
        } else {
          ex = std::make_exception_ptr(err);
        }
      }

      isFinished = true;
      cond.notify_one();
    });

    // Wait untill the blocking code as finished
    cond.wait(lock, [&]() { return isFinished; });

    // Check for errors
    if (ex != nullptr) {
      std::rethrow_exception(ex);
    } else {
      return JsiWrapper::unwrap(runtime, retVal);
    }
  }

  JSI_HOST_FUNCTION(callInWorkletContext) {
    // Wrap the arguments
    std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
    argsWrapper.reserve(count);
    for (size_t i = 0; i < count; i++) {
      argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
    }

    // Create promise as return value when running on the worklet runtime
    return react::createPromiseAsJSIValue(
        runtime,
        [this, argsWrapper, count](jsi::Runtime &runtime,
                                   std::shared_ptr<react::Promise> promise) {
          // Is this called from the worklet thread?
          if (_context->isWorkletRuntime(runtime)) {
            // The we can just call it directly
            try {
              auto result = callInWorkletRuntime(argsWrapper);
              promise->resolve(result);
            } catch (const jsi::JSError &err) {
              promise->reject(err.getMessage());
            } catch (const std::exception &err) {
              promise->reject(err.what());
            } catch (const std::runtime_error &err) {
              promise->reject(err.what());
            } catch (...) {
              promise->reject(
                  "An unknown error occurred when calling the worklet.");
            }

          } else {
            // Run on the Worklet thread
            _context->invokeOnWorkletThread([=]() {
              // Dispatch and resolve / reject promise
              auto workletRuntime = &_context->getWorkletRuntime();

              // Prepare result
              try {
                auto retVal = callInWorkletRuntime(argsWrapper);

                // Since we are returning this on another context, we need
                // to wrap/unwrap the value
                auto retValWrapper = JsiWrapper::wrap(*workletRuntime, retVal);

                _context->invokeOnJsThread([=]() {
                  // Return on Javascript thread/context
                  promise->resolve(JsiWrapper::unwrap(*_context->getJsRuntime(),
                                                      retValWrapper));
                });

              } catch (...) {
                auto err = std::current_exception();
                _context->invokeOnJsThread([promise, err]() {
                  try {
                    std::rethrow_exception(err);
                  } catch (const std::exception& e) {
                    // A little trick to find out if the exception is a js error,
                    // comparing the typeid name and then doing a static_cast
                    std::string a = typeid(e).name();
                    std::string b = typeid(jsi::JSError).name();
                    if (a == b) {
                      const auto *jsError = static_cast<const jsi::JSError *>(&e);
                      // Javascript error, pass message
                      promise->reject(jsError->getMessage());
                    } else {
                      promise->reject(e.what());
                    }
                  }
                });
              }
            });
          }
        });
  }
  
  JSI_HOST_FUNCTION(getCode) {
    return jsi::String::createFromUtf8(runtime, _code);
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorklet, isWorklet),
                       JSI_EXPORT_FUNC(JsiWorklet, callInJSContext),
                       JSI_EXPORT_FUNC(JsiWorklet, callInWorkletContext),
                       JSI_EXPORT_FUNC(JsiWorklet, getCode))

  JSI_PROPERTY_GET(context) {
    return jsi::Object::createFromHostObject(runtime, _context);
  }

  JSI_EXPORT_PROPERTY_GETTERS(JSI_EXPORT_PROP_GET(JsiWorklet, context))

  /**
   Returns true if the function is a worklet
   */
  bool isWorklet() { return _isWorklet; }

  /**
   Returns the source location for the worklet
   */
  const std::string &getLocation() { return _location; }

  /**
   Returns the current runtime depending on wether we are running as a worklet
   or not.
   */
  jsi::Runtime &getRuntime() {
    if (isWorklet()) {
      return _context->getWorkletRuntime();
    } else {
      return *_context->getJsRuntime();
    }
  }

private:
  /**
   Calls the worklet in the worklet runtime
   */
  jsi::Value callInWorkletRuntime(
      const std::vector<std::shared_ptr<JsiWrapper>> &argsWrapper) {
    auto runtime = &_context->getWorkletRuntime();

    // Unwrap closure
    auto unwrappedClosure = JsiWrapper::unwrap(*runtime, _closureWrapper);

    // Create arguments
    size_t size = argsWrapper.size();
    std::vector<jsi::Value> args(size);

    // Add the rest of the arguments
    for (size_t i = 0; i < size; i++) {
      args[i] = JsiWrapper::unwrap(*runtime, argsWrapper.at(i));
    }

    // Prepare return value
    jsi::Value retVal;

    // Prepare this
    auto oldThis = runtime->global().getProperty(*runtime, "jsThis");
    auto newThis = jsi::Object(*runtime);
    newThis.setProperty(*runtime, "_closure", unwrappedClosure);
    runtime->global().setProperty(*runtime, "jsThis", newThis);

    // TODO: Wrap the THIS set/reset in a auto release object!!

    if (!unwrappedClosure.isObject()) {
      retVal = _workletFunction->call(
          *runtime, static_cast<const jsi::Value *>(args.data()),
          argsWrapper.size());
    } else {
      retVal = _workletFunction->callWithThis(
          *runtime, unwrappedClosure.asObject(*runtime),
          static_cast<const jsi::Value *>(args.data()), argsWrapper.size());
    }

    runtime->global().setProperty(*runtime, "jsThis", oldThis);

    return retVal;
  }

  /**
   Calls the worklet in the Javascript runtime
   */
  jsi::Value
  callInJsRuntime(const std::vector<std::shared_ptr<JsiWrapper>> &argsWrapper) {
    auto runtime = _context->getJsRuntime();

    // Unwrap closure
    auto unwrappedClosure = JsiWrapper::unwrap(*runtime, _closureWrapper);

    // Create arguments
    size_t size = argsWrapper.size();
    std::vector<jsi::Value> args(size);

    // Add the rest of the arguments
    for (size_t i = 0; i < size; i++) {
      args[i] = JsiWrapper::unwrap(*runtime, argsWrapper.at(i));
    }

    // Prepare return value
    jsi::Value retVal;

    if (!unwrappedClosure.isObject()) {
      // Call without this object
      retVal = _jsFunction->call(*runtime,
                                 static_cast<const jsi::Value *>(args.data()),
                                 argsWrapper.size());
    } else {
      // Call with closure as this object
      retVal = _jsFunction->callWithThis(
          *runtime, unwrappedClosure.asObject(*runtime),
          static_cast<const jsi::Value *>(args.data()), argsWrapper.size());
    }

    return retVal;
  }

  /**
   Installs the worklet function into the worklet runtime
   */
  void createWorklet(const jsi::Value &arg) {

    jsi::Runtime &runtime = *_context->getJsRuntime();

    // Make sure arg is a function
    if (!arg.isObject() || !arg.asObject(runtime).isFunction(runtime)) {
      _context->raiseError(
          "Worklets must be initialized from a valid function.");
      return;
    }

    // This is a worklet
    _isWorklet = false;

    // Save pointer to function
    _jsFunction = std::make_shared<jsi::Function>(
        arg.asObject(runtime).asFunction(runtime));

    // Try to get the closure
    jsi::Value closure = _jsFunction->getProperty(runtime, "_closure");

    // Return if this is not a worklet
    if (closure.isUndefined() || closure.isNull()) {
      return;
    }

    // Try to get the asString function
    jsi::Value asStringProp = _jsFunction->getProperty(runtime, "asString");
    if (asStringProp.isUndefined() || asStringProp.isNull() ||
        !asStringProp.isString()) {
      return;
    }
    
    // Get location
    jsi::Value locationProp = _jsFunction->getProperty(runtime, "__location");
    if (locationProp.isUndefined() ||
        locationProp.isNull() ||
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

    jsi::Runtime *workletRuntime = &_context->getWorkletRuntime();

    auto evaluatedFunction = evaluteJavascriptInWorkletRuntime(_code);
    if (!evaluatedFunction.isObject()) {
      _context->raiseError(
          std::string("Could not create worklet from function. ") +
          "Eval did not return an object:\n" + _code);
      return;
    }
    if (!evaluatedFunction.asObject(*workletRuntime)
             .isFunction(*workletRuntime)) {
      _context->raiseError(
          std::string("Could not create worklet from function. ") +
          "Eval did not return a function:\n" + _code);
      return;
    }
    // TODO: This object fails when doing hot reloading, since
    // it tries to destroy the worklet object after the worklet
    // runtime has been removed.
    _workletFunction = std::make_shared<jsi::Function>(
        evaluatedFunction.asObject(*workletRuntime)
            .asFunction(*workletRuntime));
  }

  jsi::Value evaluteJavascriptInWorkletRuntime(const std::string &code) {
    auto workletRuntime = &_context->getWorkletRuntime();
    auto codeBuffer = std::make_shared<const jsi::StringBuffer>("(" + code + "\n)");
    return workletRuntime->evaluateJavaScript(codeBuffer, _location);    
  }

  bool _isWorklet = false;
  std::shared_ptr<jsi::Function> _jsFunction;
  std::shared_ptr<jsi::Function> _workletFunction;
  std::shared_ptr<JsiWorkletContext> _context;
  std::shared_ptr<JsiWrapper> _closureWrapper;
  std::string _location = "";
#if DEBUG
  std::string _code = "";
#endif
  double _workletHash = 0;
};
} // namespace RNWorklet
