//
//  NativeWorklets+OldArch.m
//  react-native-worklets-core
//
//  Created by Hanno Gödecke on 16.10.24.
//

#import "NativeWorklets.h"

#ifndef RCT_NEW_ARCH_ENABLED

#import "InstallWorklets.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTBridge.h>

using namespace facebook;
using namespace RNWorklet;

// forward-declaration (private API)
@interface RCTBridge (JSIRuntime)
- (void *)runtime;
- (std::shared_ptr<react::CallInvoker>)jsCallInvoker;
@end

/**
 * NativeWorklets implementation for the old architecture.
 * This uses `RCTBridge` to grab the `jsi::Runtime` and `react::CallInvoker`.
 */
@implementation NativeWorklets
RCT_EXPORT_MODULE(Worklets)

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install) {
  try {
    // 1. Cast RCTBridge to a RCTCxxBridge (ObjC)
    RCTCxxBridge *cxxBridge = (RCTCxxBridge *)RCTBridge.currentBridge;
    if (!cxxBridge) {
      return @"RCTBridge is not a RCTCxxBridge!";
    }

    // 2. Access jsi::Runtime and cast from void*
    jsi::Runtime *runtime = reinterpret_cast<jsi::Runtime *>(cxxBridge.runtime);
    if (!runtime) {
      return @"jsi::Runtime on RCTCxxBridge was null!";
    }

    // 3. Access react::CallInvoker
    std::shared_ptr<react::CallInvoker> callInvoker = cxxBridge.jsCallInvoker;
    if (!callInvoker) {
      return @"react::CallInvoker on RCTCxxBridge was null!";
    }

    // 4. Install workelts
    RNWorklet::install(*runtime, callInvoker);
    return nil;
  } catch (std::exception &error) {
    // ?. Any C++ error occurred (probably in nitro::install()?)
    return [NSString stringWithCString:error.what()
                              encoding:kCFStringEncodingUTF8];
  } catch (...) {
    // ?. Any non-std error occured (probably in ObjC code)
    return @"Unknown non-std error occurred while installing "
           @"react-native-worklets-core!";
  }
}

@end

#endif
