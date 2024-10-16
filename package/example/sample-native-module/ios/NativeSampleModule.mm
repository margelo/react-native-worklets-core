//
//  NativeSampleModule.cpp
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 15.05.24.
//

#include "NSThreadDispatcher.h"
#include "NativeSampleModule.h"
#include "WKTJsiWorkletApi.h"
#include "WKTJsiWorkletContext.h"
#include "CallInvokerDispatcher.h"
#include <memory>
#include <utility>

namespace facebook::react
{

using namespace margelo;

  NativeSampleModule::NativeSampleModule(std::shared_ptr<CallInvoker> jsInvoker) : NativeSampleModuleCxxSpec(jsInvoker), _jsCallInvoker(jsInvoker) {
    _customThreadDispatcher = std::make_shared<NSThreadDispatcher>("extra.thread");
  }

  NativeSampleModule::~NativeSampleModule() {}

  jsi::Object NativeSampleModule::createCustomWorkletContext(jsi::Runtime &rt)
  {
    auto runOnJS = [=](std::function<void()>&& function) { _jsCallInvoker->invokeAsync(std::move(function)); };
    auto runOnWorklet = [=](std::function<void()>&& function) { _customThreadDispatcher->runAsync(std::move(function)); };
    
    // Make sure the worklets API is initialized - Note: we do this on the JS side for now as its a limited use-case
//    std::shared_ptr<RNWorklet::JsiWorkletContext> defaultInstance = RNWorklet::JsiWorkletContext::getDefaultInstanceAsShared();
//    defaultInstance->initialize("default", &rt, runOnJS);
    
    auto workletContext = std::make_shared<RNWorklet::JsiWorkletContext>("CustomNativeWorkletContext", &rt, runOnJS, runOnWorklet);
    
    return jsi::Object::createFromHostObject(rt, workletContext);
  }

} // namespace facebook::react
