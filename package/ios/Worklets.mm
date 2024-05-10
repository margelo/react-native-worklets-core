#import "Worklets.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTBridge.h>
#import <React/RCTUtils.h>
#import <ReactCommon/RCTTurboModule.h>

@implementation Worklets

@synthesize bridge = _bridge;

RCT_EXPORT_MODULE()

- (void)invalidate {
  RNWorklet::JsiWorkletContext::invalidateDefaultInstance();
  RNWorklet::JsiWorkletApi::invalidateInstance();
  _bridge = nil;
}

- (void)setBridge:(RCTBridge *)bridge {
  _bridge = bridge;
}

+ (BOOL)requiresMainQueueSetup {
  return YES;
}

void installApi(std::shared_ptr<facebook::react::CallInvoker> callInvoker,
                facebook::jsi::Runtime *runtime) {
  // Initialize the default worklet context
  RNWorklet::JsiWorkletContext::getDefaultInstance()->initialize(
      "default", runtime, [=](std::function<void()> &&f) {
        callInvoker->invokeAsync(std::move(f));
      });

  // Install the worklet API
  RNWorklet::JsiWorkletApi::installApi(*runtime);
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install) {
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)_bridge;
  if (cxxBridge.runtime != nullptr) {
    auto callInvoker = cxxBridge.jsCallInvoker;
    facebook::jsi::Runtime *jsRuntime =
        (facebook::jsi::Runtime *)cxxBridge.runtime;

    installApi(callInvoker, jsRuntime);
    return @true;
  }
  return @false;
}

// Don't compile this code when we build for the old architecture.
#ifdef RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
  // Call installApi with c++ brigde or something similar.
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)_bridge;
  auto callInvoker = cxxBridge.jsCallInvoker;
  facebook::jsi::Runtime *jsRuntime =
      (facebook::jsi::Runtime *)cxxBridge.runtime;

  installApi(callInvoker, jsRuntime);

  return std::make_shared<facebook::react::NativeWorkletsSpecJSI>(params);
}
#endif

@end
