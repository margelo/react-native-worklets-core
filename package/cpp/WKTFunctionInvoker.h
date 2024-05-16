//
//  WKTFunctionInvoker.h
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 16.05.24.
//

#pragma once

#include <jsi/jsi.h>
#include <memory>
#include "WKTJsiWorklet.h"
#include "WKTJsiPromiseWrapper.h"
#include <memory>
#include "JSCallInvoker.h"

namespace RNWorklet {

class JsiWorkletContext;

typedef enum {
  // Main React JS -> Main React JS, no Thread hop.
  JsToJs = 0,
  // Context A -> Main React JS, requires Thread hop.
  CtxToJs = 1,
  // Context A -> Context A, no Thread hop.
  WithinCtx = 2,
  // Context A -> Context B, requires Thread hop.
  CtxToCtx = 3,
  // Main React JS -> Context A, requires Thread hop.
  JsToCtx = 4
} CallingConvention;


/**
 A class that wraps either a plain JS function, or a Worklet function.
 */
class FunctionInvoker: public std::enable_shared_from_this<FunctionInvoker> {
private:
  FunctionInvoker(jsi::Runtime& runtime, std::shared_ptr<jsi::Function> function): _originalRuntime(&runtime), _plainFunctionOrNull(function) { }
  FunctionInvoker(jsi::Runtime& runtime, std::shared_ptr<WorkletInvoker> worklet): _originalRuntime(&runtime), _workletFunctionOrNull(worklet) { }
  
public:
  /**
   Create a new instance of a Function Invoker using the given JSI Function as it's implementation.
   - If the given JSI Function is a Worklet, it will use the worklet calling convention to perform a thread hop & function call.
   - If the given JSI Function is a plain function, it will try to call that function using a plain function call.
   - If the given JSI Value is not a Function, it will throw an error.
   */
  static std::shared_ptr<FunctionInvoker> createFunctionInvoker(jsi::Runtime& runtime, const jsi::Value& maybeFunc);
  
  /**
   Calls the underlying function or worklet on the given worklet dispatcher.
   */
  std::shared_ptr<JsiPromiseWrapper> call(jsi::Runtime& fromRuntime,
                                          const jsi::Value& thisValue,
                                          const jsi::Value* arguments,
                                          size_t count,
                                          JSCallInvoker&& runOnTargetRuntime,
                                          JSCallInvoker&& callbackToOriginalRuntime);
  
protected:
  bool isWorkletFunction() { return _workletFunctionOrNull != nullptr; }
  bool isPlainJSFunction() { return _plainFunctionOrNull != nullptr; }

private:
  jsi::Runtime* _originalRuntime;
  std::shared_ptr<jsi::Function> _plainFunctionOrNull;
  std::shared_ptr<WorkletInvoker> _workletFunctionOrNull;
};

}
