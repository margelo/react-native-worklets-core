// ReactNativeWorklets.m
#import "ReactNativeWorklets.h"

#import <worklets/JsiWorkletApi.h>
#import <worklets/JsiWorkletContext.h>

#import <jsi/jsi.h>

#import <React/RCTBridge.h>
#import <React/RCTBridge+Private.h>
#import <ReactCommon/RCTTurboModule.h>
#import <React/RCTUtils.h>

@implementation ReactNativeWorklets {}

@synthesize bridge = _bridge;

RCT_EXPORT_MODULE()

+ (BOOL)requiresMainQueueSetup {
  return YES;
}

- (void)invalidate
{
  _bridge = nullptr;
}

- (void)setBridge:(RCTBridge *)bridge
{
  _bridge = bridge;
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)bridge;
    if (cxxBridge.runtime) {
      
      auto callInvoker = bridge.jsCallInvoker;
      jsi::Runtime* jsRuntime = (jsi::Runtime*)cxxBridge.runtime;
      
      // Create error handler
      auto errorHandler = std::make_shared<std::function<void(const std::exception &ex)>>([](const std::exception &err) {
        RCTFatal(RCTErrorWithMessage([NSString stringWithUTF8String:err.what()]));
      });
      
      // Create the worklet context
      auto workletContext = std::make_shared<RNWorklet::JsiWorkletContext>(jsRuntime, callInvoker, std::move(errorHandler));
      
      // Install the worklet API
      RNWorklet::JsiWorkletApi::installApi(workletContext);
    }
}

@end
