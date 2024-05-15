//
//  NativeWorkletsModule.cpp
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 15.05.24.
//

#include "NativeWorkletsModule.h"
#include "WKTJsiWorkletApi.h"

namespace facebook::react {

NativeWorkletsModule::NativeWorkletsModule(std::shared_ptr<CallInvoker> jsInvoker)
    : NativeWorkletsCxxSpec(jsInvoker), _jsCallInvoker(jsInvoker) { }

NativeWorkletsModule::~NativeWorkletsModule() {}

jsi::Object NativeWorkletsModule::createWorkletsApi(jsi::Runtime &runtime) {
  auto worklets = std::make_shared<RNWorklet::JsiWorkletApi>(_jsCallInvoker);
  return jsi::Object::createFromHostObject(runtime, worklets);
}

} // namespace facebook::react

