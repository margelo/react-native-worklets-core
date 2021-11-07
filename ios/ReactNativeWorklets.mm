// ReactNativeWorklets.m
#import "ReactNativeWorklets.h"

#import <worklets/JsiWorkletApi.h>
#import <worklets/JsiWorkletContext.h>

#import <jsi/jsi.h>

#import <React/RCTBridge.h>
#import <React/RCTBridge+Private.h>
#import <ReactCommon/RCTTurboModule.h>
#import <React/RCTUtils.h>

@implementation ReactNativeWorklets {
  RNWorklet::JsiWorkletContext* _workletContext;
}

@synthesize bridge = _bridge;

RCT_EXPORT_MODULE()

+ (BOOL)requiresMainQueueSetup {
  return YES;
}

- (void)invalidate
{
  delete _workletContext;
  _workletContext = nullptr;
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
      _workletContext = new RNWorklet::JsiWorkletContext(jsRuntime, callInvoker, std::move(errorHandler));
      
      // Install the worklet API
      RNWorklet::JsiWorkletApi::installApi(_workletContext);
    }
}

@end
