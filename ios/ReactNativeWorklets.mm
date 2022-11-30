// ReactNativeWorklets.m
#import "ReactNativeWorklets.h"

#import <JsiWorkletApi.h>
#import <JsiWorkletContext.h>

#import <jsi/jsi.h>

#import <React/RCTBridge+Private.h>
#import <React/RCTBridge.h>
#import <React/RCTUtils.h>
#import <ReactCommon/RCTTurboModule.h>

@implementation ReactNativeWorklets {
}

@synthesize bridge = _bridge;

RCT_EXPORT_MODULE()

+ (BOOL)requiresMainQueueSetup {
  return YES;
}

- (void)invalidate {
  _bridge = nullptr;
}

- (void)setBridge:(RCTBridge *)bridge {
  _bridge = bridge;
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)bridge;
  if (cxxBridge.runtime) {

    auto callInvoker = bridge.jsCallInvoker;
    facebook::jsi::Runtime *jsRuntime =
        (facebook::jsi::Runtime *)cxxBridge.runtime;

    // Create error handler
    auto errorHandler = [](const std::exception &err) {
      RCTFatal(RCTErrorWithMessage([NSString stringWithUTF8String:err.what()]));
    };

    // Create the worklet context
    auto workletContext = std::make_shared<RNWorklet::JsiWorkletContext>(
        "default", jsRuntime,
        [=](std::function<void()> &&f) {
          callInvoker->invokeAsync(std::move(f));
        },
        std::move(errorHandler));

    // Install the worklet API
    RNWorklet::JsiWorkletApi::installApi(workletContext);
  }
}

@end
