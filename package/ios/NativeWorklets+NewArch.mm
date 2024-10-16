//
//  NativeWorklets+NewArch.mm
//  Pods
//
//  Created by Marc Rousavy on 16.10.24.
//

#import "NativeWorklets.h"

#ifdef RCT_NEW_ARCH_ENABLED

#import "InstallWorklets.h"

#import <ReactCommon/CallInvoker.h>
#import <ReactCommon/RCTTurboModuleWithJSIBindings.h>

using namespace facebook;
using namespace RNWorklet;

// Make NativeWorklets comply to RCTTurboModuleWithJSIBindings
@interface NativeWorklets () <RCTTurboModuleWithJSIBindings>
@end

/**
 * NativeWorklets implementation for the new architecture.
 * This uses `installJSIBindingsWithRuntime:` to install the `global.WorkletsProxy` into the JS Runtime.
 */
@implementation NativeWorklets {
  bool _didInstall;
  std::weak_ptr<react::CallInvoker> _callInvoker;
}

RCT_EXPORT_MODULE(Worklets)

- (void)installJSIBindingsWithRuntime:(facebook::jsi::Runtime &)runtime { 
  // 1. Get CallInvoker we cached statically
  auto callInvoker = _callInvoker.lock();
  if (callInvoker == nullptr) {
    throw std::runtime_error("Cannot install global.WorkletsProxy - CallInvoker was null!");
  }
  
  // 2. Install Worklets
  RNWorklet::install(runtime, callInvoker);
  _didInstall = true;
}

- (NSString * _Nullable)install { 
  if (_didInstall) {
    // installJSIBindingsWithRuntime ran successfully.
    return nil;
  } else {
    return @"installJSIBindingsWithRuntime: was not called - JSI Bindings could not be installed!";
  }
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:(const facebook::react::ObjCTurboModule::InitParams &)params {
  _callInvoker = params.jsInvoker;
  return std::make_shared<react::NativeWorkletsSpecJSI>(params);
}

@end

#endif
