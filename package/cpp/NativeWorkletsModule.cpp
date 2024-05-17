//
//  NativeWorkletsModule.cpp
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 15.05.24.
//

#include "NativeWorkletsModule.h"
#include "WKTJsiWorkletApi.h"
#include "WKTJsiWorkletContext.h"

namespace facebook::react {

NativeWorkletsModule::NativeWorkletsModule(std::shared_ptr<CallInvoker> jsInvoker)
    : NativeWorkletsCxxSpec(jsInvoker), _jsCallInvoker(jsInvoker) {}

NativeWorkletsModule::~NativeWorkletsModule() {}

jsi::Object NativeWorkletsModule::createWorkletsApi(jsi::Runtime &runtime) {
  // Create new instance of the Worklets API (JSI module)
  auto worklets = std::make_shared<RNWorklet::JsiWorkletApi>();
  // Initialize the shared default instance with a JS call invoker
  std::shared_ptr<RNWorklet::JsiWorkletContext> defaultInstance = RNWorklet::JsiWorkletContext::getDefaultInstanceAsShared();
  auto callInvoker = _jsCallInvoker;
  defaultInstance->initialize("default", &runtime, [callInvoker](std::function<void()>&& callback) {
    callInvoker->invokeAsync(std::move(callback));
  });
  // Return it to JS.
  return jsi::Object::createFromHostObject(runtime, worklets);
}

} // namespace facebook::react

