#pragma once

#include <jsi/jsi.h>

#include <vector>

#include "JsiBaseDecorator.h"
#include "JsiHostObject.h"
#include "JsiSharedValue.h"
#include "JsiWorklet.h"
#include "JsiWorkletContext.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

static const char *WorkletsApiName = "Worklets";

class JsiWorkletApi : public JsiHostObject {
public:
  /**
   * Create the worklet API
   * @param context Worklet context
   * make sure only the runOnJsThread method is available.
   */
  JsiWorkletApi(std::shared_ptr<JsiWorkletContext> context)
      : _context(context){
            // TODO:Add default decorators
            // Here we want some console objects at least
        };

  /**
   * Installs the worklet API into the provided runtime
   * @param context Worklet context to install API for
   */
  static void installApi(std::shared_ptr<JsiWorkletContext> context) {
    auto existingApi = (context->getJsRuntime()->global().getProperty(
        *context->getJsRuntime(), WorkletsApiName));
    if (existingApi.isObject()) {
      return;
    }

    auto workletApi = std::make_shared<JsiWorkletApi>(context);
    context->getJsRuntime()->global().setProperty(
        *context->getJsRuntime(), WorkletsApiName,
        jsi::Object::createFromHostObject(*context->getJsRuntime(),
                                          std::move(workletApi)));
  }

  JSI_HOST_FUNCTION(createWorklet) {
    // Make sure this one is only called from the js runtime
    if (_context->isWorkletRuntime(runtime)) {
      throw jsi::JSError(runtime, "createWorklet should only be called "
                                  "from the javascript runtime.");
    }

    if (count > 2) {
      throw jsi::JSError(
          runtime,
          "createWorklet expects a Javascript function as the first parameter, "
          "and an optional worklet context as the second parameter.");
    }

    // Get the active context
    auto activeContext =
        count == 2 && arguments[1].isObject()
            ? arguments[1].asObject(runtime).getHostObject<JsiWorkletContext>(
                  runtime)
            : _context;

    if (activeContext == nullptr) {
      throw jsi::JSError(runtime, "createWorklet called with invalid context.");
    }

    // Create the worklet host object and return to JS caller
    return jsi::Object::createFromHostObject(
        runtime, std::make_shared<JsiWorklet>(runtime, arguments[0]));
  };

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
    if (_context->isWorkletRuntime(runtime)) {
      throw jsi::JSError(runtime, "createSharedValue should only be called "
                                  "from the javascript runtime.");
    }

    return jsi::Object::createFromHostObject(
        *_context->getJsRuntime(),
        std::make_shared<JsiSharedValue>(arguments[0], _context));
  };

  JSI_HOST_FUNCTION(createRunInJsFn) {
    // Make sure this one is only called from the js runtime
    if (_context->isWorkletRuntime(runtime)) {
      throw jsi::JSError(runtime, "createRunInJsFn should only be called "
                                  "from the javascript runtime.");
    }

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

    if (!JsiWorklet::isDecoratedAsWorklet(runtime, arguments[0])) {
      throw jsi::JSError(runtime, "createRunInJsFn expects a worklet "
                                  "decorated function as its first parameter.");
    }

    // Now let's get the worklet
    auto worklet = std::make_shared<JsiWorklet>(runtime, arguments[0]);

    // Get the active context
    auto activeContext =
        count == 2 && arguments[1].isObject()
            ? arguments[1].asObject(runtime).getHostObject<JsiWorkletContext>(
                  runtime)
            : _context;

    if (activeContext == nullptr) {
      throw jsi::JSError(runtime,
                         "createRunInJsFn called with invalid context.");
    }

    return jsi::Function::createFromHostFunction(
        runtime, jsi::PropNameID::forAscii(runtime, "runInContextFn"), 0,
        JSI_HOST_FUNCTION_LAMBDA {
          // Ensure that the function is called in the worklet context
          if (&runtime == activeContext->getJsRuntime()) {
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

          activeContext->invokeOnJsThread([&]() {
            std::lock_guard<std::mutex> lock(mu);

            try {
              auto result = worklet->call(runtime, argsWrapper);
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
    if (_context->isWorkletRuntime(runtime)) {
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
            : _context;

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
          return react::createPromiseAsJSIValue(
              runtime, [this, argsWrapper, activeContext, worklet,
                        count](jsi::Runtime &runtime,
                               std::shared_ptr<react::Promise> promise) {
                // Run on the Worklet thread
                activeContext->invokeOnWorkletThread([=]() {
                  // Dispatch and resolve / reject promise
                  auto workletRuntime = &activeContext->getWorkletRuntime();

                  // Prepare result
                  try {
                    auto retVal = worklet->call(*workletRuntime, argsWrapper);

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
                          promise->reject(jsError->getMessage());
                        } else {
                          promise->reject(e.what());
                        }
                      }
                    });
                  }
                });
              });
        });
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiWorkletApi, createWorklet),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createSharedValue),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createContext),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createRunInContextFn),
                       JSI_EXPORT_FUNC(JsiWorkletApi, createRunInJsFn))

  /**
   Creates a new worklet context
   @name Name of the context
   @returns A new worklet context that has been initialized and decorated.
   */
  std::shared_ptr<JsiWorkletContext>
  createWorkletContext(const std::string &name) {
    auto newContext = std::make_shared<JsiWorkletContext>(name, _context);
    for (auto &decorator : _decorators) {
      newContext->decorate(decorator.get());
    }
    return newContext;
  }

  /**
   Adds a decorator that will be used to decorate all contexts.
   @decorator The decorator to add
   */
  void addDecorator(std::shared_ptr<JsiBaseDecorator> decorator) {
    _decorators.push_back(decorator);

    // Decorate default context
    _context->decorate(decorator.get());
  }

private:
  std::shared_ptr<JsiWorkletContext> _context;
  std::vector<std::shared_ptr<JsiBaseDecorator>> _decorators;
};
} // namespace RNWorklet
