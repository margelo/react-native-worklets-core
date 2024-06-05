//
//  NativeSampleModule.h
//  Pods
//
//  Created by Marc Rousavy on 15.05.24.
//

#pragma once

#if __has_include(<React-Codegen/RNWorkletsSpecJSI.h>)
// CocoaPods include (iOS)
#include <React-Codegen/RNSampleModuleJSI.h>
#elif __has_include(<RNWorkletsSpecJSI.h>)
// CMake include on Android
#include <RNSampleModuleJSI.h>
#else
#error \
    "Cannot find react-native-worklets-core spec! Try cleaning your cache and re-running CodeGen!"
#endif

#include <memory>
#include "Dispatcher.h"

namespace facebook::react
{

  using namespace margelo;

  // The TurboModule itself
  class NativeSampleModule : public NativeSampleModuleCxxSpec<NativeSampleModule>
  {
  public:
    explicit NativeSampleModule(std::shared_ptr<CallInvoker> jsInvoker);
    ~NativeSampleModule();

    jsi::Object createCustomWorkletContext(jsi::Runtime &rt);

  private:
    std::shared_ptr<CallInvoker> _jsCallInvoker;
    std::shared_ptr<Dispatcher> _customThreadDispatcher;
  };

} // namespace facebook::react
