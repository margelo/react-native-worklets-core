//
//  WKTJsiWorklet.cpp
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 16.05.24.
//

#include "WKTJsiWorklet.h"

namespace RNWorklet {


std::string tryGetFunctionName(jsi::Runtime& runtime, const jsi::Value& maybeFunc) {
  try {
    jsi::Value name = maybeFunc.asObject(runtime).getProperty(runtime, "name");
    return name.asString(runtime).utf8(runtime);
  } catch (...) {
    return "anonymous";
  }
}

FunctionOrWorklet WorkletInvoker::getFunctionInvoker(jsi::Runtime& runtime, const jsi::Value& maybeFunc) {
 // Ensure that we are passing a function as the param.
 if (!maybeFunc.isObject() ||
     !maybeFunc.asObject(runtime).isFunction(runtime)) [[unlikely]] {
   std::string name = tryGetFunctionName(runtime, maybeFunc);
   throw std::runtime_error("Failed to get Function " + name + " - it's not a valid jsi::Function!");
 }

 // Now we can save the function in a shared ptr to be able to keep it around
 auto func = std::make_shared<jsi::Function>(
     maybeFunc.asObject(runtime).asFunction(runtime));
 
 if (JsiWorklet::isDecoratedAsWorklet(runtime, func)) {
   // It's a Worklet - return Worklet:
   auto workletInvoker = std::make_shared<WorkletInvoker>(runtime, func);
   return FunctionOrWorklet(workletInvoker);
 } else {
   // It's a plain JSI Function, return that:
   return FunctionOrWorklet(func);
 }
}


} // namespace RNWorklet
