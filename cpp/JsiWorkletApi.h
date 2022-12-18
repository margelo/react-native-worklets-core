#pragma once

#include <jsi/jsi.h>

#include "JsiHostObject.h"
#include "JsiJsDecorator.h"
#include "JsiPromise.h"
#include "JsiSharedValue.h"
#include "JsiWorklet.h"
#include "JsiWorkletContext.h"
#include "JsiWrapper.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiWorkletApi : public JsiHostObject {
public:
  // Name of the Worklet API member (where to install on global)
  static const char *WorkletsApiName;

  /**
   * Installs the worklet API into the provided runtime
   */
  static void installApi(jsi::Runtime &runtime);

  /**
   Returns the worklet API
   */
  static std::shared_ptr<JsiWorkletApi> getInstance();

  /**
   Invalidate the api instance.
   */
  static void invalidateInstance();

  JSI_HOST_FUNCTION(addDecorator) {
    if (JsiWorkletContext::getInstance()->isWorkletRuntime(runtime)) {
      throw jsi::JSError(runtime, "addDecorator should only be called "
                                  "from the javascript runtime.");
    }

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

  JSI_HOST_FUNCTION(createContext) {
    if (count == 0) {
      throw jsi::JSError(
          runtime, "createWorkletContext expects the context name parameter.");
    }

    if (!arguments[0].isString()) {
      throw jsi::JSError(runtime,
                         "createWorkletContext expects the context name "
                         "parameter as a string.");
    }

    auto nameStr = arguments[0].asString(runtime).utf8(runtime);
    return jsi::Object::createFromHostObject(runtime,
                                             createWorkletContext(nameStr));
  };

  JSI_HOST_FUNCTION(createSharedValue) {
    if (JsiWorkletContext::getInstance()->isWorkletRuntime(runtime)) {
      throw jsi::JSError(runtime, "createSharedValue should only be called "
                                  "from the javascript runtime.");
    }

    return jsi::Object::createFromHostObject(
        *JsiWorkletContext::getInstance()->getJsRuntime(),
        std::make_shared<JsiSharedValue>(arguments[0],
                                         JsiWorkletContext::getInstance()));
  };

  JSI_HOST_FUNCTION(createRunInJsFn) {
    if (count == 0) {
      throw jsi::JSError(runtime,
                         "createRunInJsFn expects at least one parameter.");
    }

    // Get the worklet function
    if (!arguments[0].isObject()) {
      throw jsi::JSError(
          runtime,
          "createRunInJsFn expects a function as its first parameter.");
    }

    if (!arguments[0].asObject(runtime).isFunction(runtime)) {
      throw jsi::JSError(
          runtime,
          "createRunInJsFn expects a function as its first parameter.");
    }

    // Now let's get the function
    auto func = std::make_shared<jsi::Function>(
        arguments[0].asObject(runtime).asFunction(runtime));

    // Get the active context
    auto activeContext =
        count == 2 && arguments[1].isObject()
            ? arguments[1].asObject(runtime).getHostObject<JsiWorkletContext>(
                  runtime)
            : JsiWorkletContext::getInstance();

    if (activeContext == nullptr) {
      throw jsi::JSError(runtime,
                         "createRunInJsFn called with invalid context.");
    }

    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forAscii(runtime, "runInJsFn"), 0,
        JSI_HOST_FUNCTION_LAMBDA {
          // Ensure that the function is called in the worklet context
          if (!activeContext->isWorkletRuntime(runtime)) {
            throw jsi::JSError(runtime, "createRunInJsFn must be called "
                                        "from a worklet runtime / thread.");
          }

          std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
          argsWrapper.reserve(count);

          for (size_t i = 0; i < count; i++) {
            argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
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

          // Now lets go back and into the safety of the good ol' javascript
          // thread.
          activeContext->invokeOnJsThread([&]() {
            std::lock_guard<std::mutex> lock(mu);

            jsi::Runtime &runtime = *activeContext->getJsRuntime();

            try {
              size_t size = argsWrapper.size();
              std::vector<jsi::Value> args(size);

              // Add the rest of the arguments
              for (size_t i = 0; i < size; i++) {
                args[i] = JsiWrapper::unwrap(runtime, argsWrapper.at(i));
              }

              auto result = func->call(
                  runtime, static_cast<const jsi::Value *>(args.data()), size);
              retVal = JsiWrapper::wrap(runtime, result);
            } catch (const jsi::JSError &err) {
              // When rethrowing we need to make sure that any jsi errors are
              // transferred to the correct runtime.
              try {
                throw JsErrorWrapper(err.getMessage(), err.getStack());
              } catch (...) {
                ex = std::current_exception();
              }
            } catch (const std::exception &err) {
              std::string a = typeid(err).name();
              std::string b = typeid(jsi::JSError).name();
              if (a == b) {
                const auto *jsError = static_cast<const jsi::JSError *>(&err);
                ex = std::make_exception_ptr(
                    JsErrorWrapper(jsError->getMessage(), jsError->getStack()));
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
        });
  }

  JSI_HOST_FUNCTION(createRunInContextFn) {
    // Make sure this one is only called from the js runtime
    if (JsiWorkletContext::getInstance()->isWorkletRuntime(runtime)) {
      throw jsi::JSError(runtime, "createRunInContextFn should only be called "
                                  "from the javascript runtime.");
    }

    if (count == 0) {
      throw jsi::JSError(
          runtime, "createRunInContextFn expects at least one parameter.");
    }

    // Get the worklet function
    if (!arguments[0].isObject()) {
      throw jsi::JSError(
          runtime,
          "createRunInContextFn expects a function as its first parameter.");
    }

    if (!JsiWorklet::isDecoratedAsWorklet(runtime, arguments[0])) {
      throw jsi::JSError(runtime, "createRunInContextFn expects a worklet "
                                  "decorated function as its first parameter.");
    }

    // Now let's get the worklet
    auto worklet = std::make_shared<JsiWorklet>(runtime, arguments[0]);

    // Get the active context
    auto activeContext =
        count == 2 && arguments[1].isObject()
            ? arguments[1].asObject(runtime).getHostObject<JsiWorkletContext>(
                  runtime)
            : JsiWorkletContext::getInstance();

    if (activeContext == nullptr) {
      throw jsi::JSError(runtime,
                         "createRunInContextFn called with invalid context.");
    }

    // Now let us create the caller function / return value. This is a function
    // that can be called FROM the JS thead / runtime and that will execute the
    // worklet on the context thread / runtime.
    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forAscii(runtime, "runInContextFn"), 0,
        JSI_HOST_FUNCTION_LAMBDA {
          // Ensure that the function is called in the worklet context
          if (&runtime != activeContext->getJsRuntime()) {
            throw jsi::JSError(runtime, "createRunInContextFn must be called "
                                        "from the JS runtime / thread.");
          }
          // Wrap the arguments
          std::vector<std::shared_ptr<JsiWrapper>> argsWrapper;
          argsWrapper.reserve(count);
          for (size_t i = 0; i < count; i++) {
            argsWrapper.push_back(JsiWrapper::wrap(runtime, arguments[i]));
          }

          // Create promise as return value when running on the worklet runtime
          return JsiPromise::createPromiseAsJSIValue(
              runtime,
              [this, argsWrapper, activeContext, worklet, count](
                  jsi::Runtime &runtime, std::shared_ptr<JsiPromise> promise) {
                // Run on the Worklet thread
                activeContext->invokeOnWorkletThread([=]() {
                  // Dispatch and resolve / reject promise
                  auto workletRuntime = &activeContext->getWorkletRuntime();

                  // Prepare result
                  try {
                    // Create arguments
                    size_t size = argsWrapper.size();
                    std::vector<jsi::Value> args(size);

                    // Add the rest of the arguments
                    for (size_t i = 0; i < size; i++) {
                      args[i] = JsiWrapper::unwrap(*workletRuntime,
                                                   argsWrapper.at(i));
                    }

                    auto retVal = worklet->call(
                        *workletRuntime, jsi::Value::undefined(),
                        static_cast<const jsi::Value *>(args.data()),
                        argsWrapper.size());

                    // Since we are returning this on another context, we need
                    // to wrap/unwrap the value
                    auto retValWrapper =
                        JsiWrapper::wrap(*workletRuntime, retVal);

                    // Resolve on main thread
                    activeContext->invokeOnJsThread([=]() {
                      promise->resolve(JsiWrapper::unwrap(
                          *activeContext->getJsRuntime(), retValWrapper));
                    });

                  } catch (...) {
                    auto err = std::current_exception();
                    activeContext->invokeOnJsThread([promise, err]() {
                      try {
                        std::rethrow_exception(err);
                      } catch (const std::exception &e) {
                        // A little trick to find out if the exception is a js
                        // error, comparing the typeid name and then doing a
                        // static_cast
                        std::string a = typeid(e).name();
                        std::string b = typeid(jsi::JSError).name();
                        if (a == b) {
                          const auto *jsError =
                              static_cast<const jsi::JSError *>(&e);
                          // Javascript error, pass message
                          promise->reject(jsError->getMessage(),
                                          jsError->getStack());
                        } else {
                          promise->reject(e.what(), "(unknown stack frame)");
                        }
                      }
                    });
                  }
                });
              });
        });
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorkletApi, createSharedValue),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createContext),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createRunInContextFn),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createRunInJsFn),
                       JSI_EXPORT_FUNC(JsiWorkletApi, addDecorator))

  /**
   Creates a new worklet context
   @name Name of the context
   @returns A new worklet context that has been initialized and decorated.
   */
  std::shared_ptr<JsiWorkletContext>
  createWorkletContext(const std::string &name);

  /**
   Adds a decorator that will be used to decorate all contexts.
   @decorator The decorator to add
   */
  void addDecorator(std::shared_ptr<JsiBaseDecorator> decorator);

  // Instance/singletong
  static std::shared_ptr<JsiWorkletApi> instance;
};
} // namespace RNWorklet
