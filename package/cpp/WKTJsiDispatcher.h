#pragma once

#include <cxxabi.h>

#include <memory>
#include <vector>

#include <jsi/jsi.h>

#include "WKTJsiWrapper.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

class JsiDispatcher {
public:
  /**
   * Creates a function for calling a jsi::Function with args and correct this
   * value
   * @param runtime Runtime to run function in
   * @param thisValuePtr This object
   * @param functionPtr function to run
   * @param onSuccess Callback on success
   * @param onError Callback on error
   * @return A new dispatch function for running the provided function with
   * error handling
   */
  static std::function<void()> createDispatcher(
      jsi::Runtime &runtime, std::shared_ptr<JsiWrapper> thisValuePtr,
      const jsi::HostFunctionType &functionPtr,
      const std::function<void(std::shared_ptr<JsiWrapper>)> onSuccess,
      const std::function<void(const char *)> onError) {
    std::vector<std::shared_ptr<JsiWrapper>> emptyArgs;
    return createDispatcher(runtime, thisValuePtr, functionPtr, emptyArgs,
                            onSuccess, onError);
  }

  /**
   * Creates a function for calling a jsi::Function with args and correct this
   * value
   * @param runtime Runtime to run function in
   * @param thisValuePtr This object
   * @param functionPtr function to run
   * @param arguments Arguments for calling the function
   * @param onSuccess Callback on success
   * @param onError Callback on error
   * @return A new dispatch function for running the provided function with
   * error handling
   */
  static std::function<void()> createDispatcher(
      jsi::Runtime &runtime, std::shared_ptr<JsiWrapper> thisValuePtr,
      const jsi::HostFunctionType &functionPtr,
      const std::vector<std::shared_ptr<JsiWrapper>> &arguments,
      const std::function<void(std::shared_ptr<JsiWrapper>)> onSuccess,
      const std::function<void(const char *)> onError) {

    // Set up the callback to run on the correct runtime thread
    return [&runtime, arguments, thisValuePtr, functionPtr, onSuccess,
            onError]() {
      try {
        // Define return value
        jsi::Value retVal = jsi::Value::undefined();

        // Arguments
        jsi::Value *args = new jsi::Value[arguments.size()];
        for (int i = 0; i < arguments.size(); ++i) {
          args[i] = JsiWrapper::unwrap(runtime, arguments[i]);
        }

        // Call with this or not?
        if (thisValuePtr != nullptr &&
            thisValuePtr->getType() != JsiWrapperType::Null &&
            thisValuePtr->getType() != JsiWrapperType::Undefined) {
          retVal = functionPtr(
              runtime,
              JsiWrapper::unwrap(runtime, thisValuePtr).asObject(runtime),
              static_cast<const jsi::Value *>(args), arguments.size());
        } else {
          retVal = functionPtr(runtime, nullptr,
                               static_cast<const jsi::Value *>(args),
                               arguments.size());
        }

        delete[] args;

        // Wrap the result from calling the function
        if (onSuccess != nullptr) {
          onSuccess(JsiWrapper::wrap(runtime, retVal));
        }
      } catch (const jsi::JSError &err) {
        if (onError != nullptr)
          onError(err.getMessage().c_str());
      } catch (const std::exception &err) {
        if (onError != nullptr)
          onError(err.what());
      } catch (const std::runtime_error &err) {
        if (onError != nullptr)
          onError(err.what());
      } catch (...) {
        if (onError != nullptr)
          onError("An unknown error occurred when calling dispatch function.");
      }
    };
  }
};
} // namespace RNWorklet
