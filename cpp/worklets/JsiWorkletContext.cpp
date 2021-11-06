
#include <JsiSharedValue.h>
#include <JsiVisitor.h>
#include <JsiWorklet.h>
#include <JsiWorkletApi.h>
#include <JsiWorkletContext.h>
#include <JsiDescribe.h>

namespace RNWorklet {

static const char *WorkletAsStringPropertyName = "asString";
static const char *WorkletClosurePropertyName = "_closure";
static const char *WorkletLocationPropertyName = "__location";
static const char *WorkletJsThisName = "jsThis";

JsiWorkletContext::JsiWorkletContext(
    jsi::Runtime *jsRuntime,
    std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
    std::shared_ptr<std::function<void(const std::exception &ex)>> errorHandler)
    : _jsRuntime(jsRuntime), _workletRuntime(makeJSIRuntime()),
      _jsCallInvoker(jsCallInvoker), _errorHandler(errorHandler) {

  // Create the dispatch queue
  _dispatchQueue = std::make_unique<JsiDispatchQueue>("skia-render-queue", 1);

  // Decorate the runtime with the worklet flag so that checks using
  // this variable will work.
  _workletRuntime->global().setProperty(*_workletRuntime, "_WORKLET",
                                        jsi::Value(true));
        
  // Install worklet API in the worklet runtime
  auto workletApi = std::make_shared<JsiWorkletApi>(this);
  _workletRuntime->global().setProperty(
      *_workletRuntime, "WorkletApi",
      jsi::Object::createFromHostObject(*_workletRuntime,
                                        std::move(workletApi)));
}

void JsiWorkletContext::runOnWorkletThread(std::function<void()> fp) {
  _dispatchQueue->dispatch(fp);
}

void JsiWorkletContext::runOnJavascriptThread(std::function<void()> fp) {
  _jsCallInvoker->invokeAsync(std::move(fp));
}

std::shared_ptr<JsiWrapper>
JsiWorkletContext::copyToWorkletRuntime(const jsi::Value &value) {
  // Get runtimes
  jsi::Runtime *workletRuntime = &getWorkletRuntime();
  jsi::Runtime *jsRuntime = getJsRuntime();

  JsiFunctionResolver resolveFunction =
      [this, jsRuntime,
       workletRuntime](const jsi::Value &candidate) -> jsi::HostFunctionType {
    if (JsiWorklet::isWorklet(*jsRuntime, candidate)) {
      // We'll try to install this function as a worklet
      return getWorklet(candidate);
    } else {
      // We can return a function, but it can never be called - so we wrap it
      // in a host object function throwing an error on call
      auto name =
          candidate.asObject(*jsRuntime).getProperty(*jsRuntime, "name");
      std::string nameStr = "[unknown]";
      if (!name.isUndefined() && name.isString()) {
        nameStr = name.asString(*jsRuntime).utf8(*jsRuntime);
      }

      return [nameStr, jsRuntime](jsi::Runtime &rt, const jsi::Value &thisObj,
                                  const jsi::Value *arguments,
                                  size_t count) -> jsi::Value {
        jsi::detail::throwJSError(
            *jsRuntime,
            std::string(
                "Could not call function " + nameStr +
                " on a worklet runtime because the function is not a worklet")
                .c_str());

        return jsi::Value::undefined();
      };
    }
  };

  // Create wrapper
  auto retVal = std::make_shared<JsiWrapper>(
      *jsRuntime, value,
      std::make_shared<JsiFunctionResolver>(resolveFunction));

  return retVal;
}

jsi::Function JsiWorkletContext::getWorkletOrThrow(const jsi::Value &value) {
  jsi::Runtime *jsRuntime = getJsRuntime();
  // Check that we have a worklet
  if (!JsiWorklet::isWorklet(*jsRuntime, value)) {
    auto name = value.asObject(*jsRuntime).getProperty(*jsRuntime, "name");
    auto asString = value.toString(*jsRuntime).utf8(*jsRuntime);

    std::string nameStr = "[unknown]";
    if (name.isString()) {
      nameStr = name.asString(*jsRuntime).utf8(*jsRuntime);
    }

    jsi::detail::throwJSError(
        *jsRuntime, std::string("Installing worklet failed.\nFunction " +
                                nameStr + " is not a worklet:\n\n" + asString)
                        .c_str());
  }

  // Get the function / worklet
  return value.asObject(*jsRuntime).asFunction(*jsRuntime);
}

jsi::Value JsiWorkletContext::getWorkletClosure(const jsi::Function &worklet) {
  jsi::Runtime *jsRuntime = getJsRuntime();
  return worklet.getProperty(*jsRuntime, WorkletClosurePropertyName)
      .asObject(*jsRuntime);
}

jsi::HostFunctionType JsiWorkletContext::createWorklet(jsi::Runtime& runtime,
                                      const jsi::Value& context,
                                      const jsi::Value& value) {
  if(!value.isObject()) {
    raiseError("createWorklet expects a function parameter.");
    return nullptr;
  }
  
  if(!value.asObject(runtime).isFunction(runtime)) {
    raiseError("createWorklet expects a function parameter.");
    return nullptr;
  }
  
  auto function = value.asObject(runtime).asFunction(runtime);
  
  // Let us try to install the function in the worklet context
  auto code = function.getPropertyAsFunction(runtime, "toString")
    .callWithThis(runtime, function, nullptr, 0)
    .asString(runtime)
    .utf8(runtime);
  
  auto worklet = evalWorkletCode(code);
  
  // Let's wrap the closure
  auto closure = JsiWrapper::wrap(runtime, context, nullptr, nullptr, "");
  
  // Return a caller function wrapper for the worklet
  return [worklet, closure](jsi::Runtime& runtime, const jsi::Value&,
                   const jsi::Value *arguments, size_t count) -> jsi::Value {
  
    // Add the closure as the first parameter
    size_t size = count + 1;
    jsi::Value* args = new jsi::Value[size];
    args[0] = JsiWrapper::unwrap(runtime, closure);
    std::memmove(args + 1, arguments, sizeof(args) + sizeof(jsi::Value*));
    
    auto retVal = worklet->call(runtime, static_cast<const jsi::Value*>(args), size);
    delete [] args;
    return retVal;
  };
}

std::shared_ptr<jsi::Function> JsiWorkletContext::evalWorkletCode(const std::string& code) {
  
  jsi::Runtime *workletRuntime = &getWorkletRuntime();
  auto eval = workletRuntime->global().getPropertyAsFunction(
      getWorkletRuntime(), "eval");
  
  return std::make_shared<jsi::Function>(
      eval.call(*workletRuntime, ("(" + code + ")").c_str())
          .asObject(*workletRuntime)
          .asFunction(*workletRuntime));
}

jsi::HostFunctionType
JsiWorkletContext::getWorklet(const jsi::Function &worklet) {
  jsi::Runtime *jsRuntime = getJsRuntime();
  jsi::Runtime *workletRuntime = &getWorkletRuntime();

  if (!JsiWorklet::isWorklet(*jsRuntime, worklet)) {
    auto name = worklet.getProperty(*jsRuntime, "name");

    std::string nameStr = "[unknown]";
    if (name.isString()) {
      nameStr = name.asString(*jsRuntime).utf8(*jsRuntime);
    }

    jsi::detail::throwJSError(
        *jsRuntime, std::string("Installing worklet failed.\nFunction " +
                                nameStr + " is not a worklet.")
                        .c_str());
  }

  // Get the closure for the worklet
  auto closure = getWorkletClosure(worklet);

  // Get the worklet hash for identification
  auto workletHash = JsiWorklet::getWorkletHash(*jsRuntime, worklet);

  // Check if we have the worklet installed in the worklet runtime
  // If not we'll evaluate the worklet code and add it to our cache
  if (getWorkletsCache().count(workletHash) == 0) {
    // Get the code for the worklet
    auto code = worklet.getProperty(*jsRuntime, WorkletAsStringPropertyName)
                    .asString(*jsRuntime)
                    .utf8(*jsRuntime);

    // Evaluate and store function in cache
    auto eval = getWorkletRuntime().global().getPropertyAsFunction(
        getWorkletRuntime(), "eval");
    _workletsCache.emplace(
        workletHash,
        JsiWorkletInfo {
            std::make_shared<jsi::Function>(
                eval.call(*workletRuntime, ("(" + code + ")").c_str())
                    .asObject(*workletRuntime)
                    .asFunction(*workletRuntime)),
            std::make_shared<jsi::Function>(worklet.asFunction(*jsRuntime))});
  }

  auto installedWorklet = _workletsCache.at(workletHash);

  // Deep copy values from the source runtime to the worklet runtime
  auto closureWrapper = copyToWorkletRuntime(closure);

  // Create workletDispatcher for calling the worklet
  return [workletRuntime, installedWorklet, closureWrapper,
          this](jsi::Runtime &rt, const jsi::Value &thisObj,
                const jsi::Value *arguments, size_t count) -> jsi::Value {
    if (&rt != workletRuntime) {
      jsi::detail::throwJSError(*workletRuntime,
                                "Worklet called on another runtime than the "
                                "one it was installed into.");
    }

    // Now we are ready and set up to run this one. Start by storing the
    // previous jsThis object
    jsi::Value oldJSThis = rt.global().getProperty(rt, WorkletJsThisName);

    // Set up new jsThis
    auto jsThis = jsi::Object(rt);

    // Copy closure into the jsThis object
    jsThis.setProperty(rt, WorkletClosurePropertyName,
                       JsiWrapper::unwrap(rt, closureWrapper).asObject(rt));

    // Install this
    rt.global().setProperty(rt, WorkletJsThisName, jsThis);

    // Set up return value
    jsi::Value retVal = jsi::Value::undefined();

    try {
      if (thisObj.isUndefined() || thisObj.isNull()) {
        // Call the worklet on the worklet runtime
        retVal = installedWorklet.worklet->call(rt, arguments, count);
      } else {
        // Call the worklet on the worklet runtime with this
        retVal = installedWorklet.worklet->callWithThis(
            rt, thisObj.asObject(rt), arguments, count);
      }

    } catch (const jsi::JSError &e) {
      // Reset this
      rt.global().setProperty(rt, WorkletJsThisName, oldJSThis);

      // Call error handler
      return raiseError(e);

    } catch (const jsi::JSIException &e) {
      // Reset this
      rt.global().setProperty(rt, WorkletJsThisName, oldJSThis);

      // Call error handler
      return raiseError(e);

    } catch (const std::exception &e) {
      // Reset this
      rt.global().setProperty(rt, WorkletJsThisName, oldJSThis);

      // Call error handler
      return raiseError(e);

    } catch (const std::runtime_error &e) {
      // Reset this
      rt.global().setProperty(rt, WorkletJsThisName, oldJSThis);

      // Call error handler
      return raiseError(e);
    } catch (...) {
      // Reset this
      rt.global().setProperty(rt, WorkletJsThisName, oldJSThis);

      // Rethrow
      jsi::Value location = jsThis.getProperty(rt, WorkletLocationPropertyName);

      std::string str = "Javascript Worklet Error";

      if (location.isString()) {
        str += "\nIn file: " + location.asString(rt).utf8(rt);
      }

      // Call error handler
      return raiseError(jsi::JSError(rt, str));
    }

    // Reset this
    rt.global().setProperty(rt, WorkletJsThisName, oldJSThis);

    return retVal;
  };
}

jsi::HostFunctionType
JsiWorkletContext::getWorklet(const jsi::Value &function) {
  // Get the function / worklet
  auto worklet = getWorkletOrThrow(function);
  return getWorklet(worklet);
}

std::vector<JsiSharedValue *>
JsiWorkletContext::getSharedValues(const jsi::Function &worklet) {
  jsi::Runtime *jsRuntime = getJsRuntime();

  if (!JsiWorklet::isWorklet(*jsRuntime, worklet)) {
    auto name = worklet.getProperty(*jsRuntime, "name");

    std::string nameStr = "[unknown]";
    if (name.isString()) {
      nameStr = name.asString(*jsRuntime).utf8(*jsRuntime);
    }

    jsi::detail::throwJSError(
        *jsRuntime, std::string("Installing worklet failed.\nFunction " +
                                nameStr + " is not a worklet.")
                        .c_str());
  }

  // Get closure
  auto closure = getWorkletClosure(worklet);

  // Find any dependant shared values in the closure
  std::vector<JsiSharedValue *> sharedValues;
  JsiVisitor::visit(
      *getJsRuntime(), closure,
      [&sharedValues](jsi::Runtime &runtime, const jsi::Value &value) {
        if (value.isObject() && value.asObject(runtime).isHostObject(runtime)) {
          // Check if this is a shared value object
          auto hostObject = value.asObject(runtime).asHostObject(runtime);
          JsiSharedValue *wrapper =
              dynamic_cast<JsiSharedValue *>(hostObject.get());
          if (wrapper != nullptr) {
            sharedValues.push_back(wrapper);
          }
        }
      });
  return sharedValues;
}

} // namespace RNWorklet
