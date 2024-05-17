//
//  NativeWorkletsModule.h
//  Pods
//
//  Created by Marc Rousavy on 15.05.24.
//

#pragma once

#if __has_include(<React-Codegen/RNWorkletsSpecJSI.h>)
// CocoaPods include (iOS)
#include <React-Codegen/RNWorkletsSpecJSI.h>
#elif __has_include(<RNWorkletsSpecJSI.h>)
// CMake include on Android
#include <RNWorkletsSpecJSI.h>
#else
#error Cannot find react-native-worklets-core spec! Try cleaning your cache and re-running CodeGen!
#endif

namespace facebook::react {

// The TurboModule itself
class NativeWorkletsModule : public NativeWorkletsCxxSpec<NativeWorkletsModule> {
public:
  NativeWorkletsModule(std::shared_ptr<CallInvoker> jsInvoker);
  ~NativeWorkletsModule();

  jsi::Object createWorkletsApi(jsi::Runtime& runtime);
  
private:
  std::shared_ptr<CallInvoker> _jsCallInvoker;
};

} // namespace facebook::react
