// ReactNativeWorklets.m
#import "ReactNativeWorklets.h"

#import <worklets/JsiWorkletApi.h>
#import <worklets/JsiWorkletContext.h>

#import <jsi/jsi.h>

#import <React/RCTBridge.h>
#import <React/RCTBridge+Private.h>
#import <ReactCommon/RCTTurboModule.h>

@implementation ReactNativeWorklets {
  std::unique_ptr<RNWorklet::JsiWorkletContext> _workletContext;
}

@synthesize bridge = _bridge;

RCT_EXPORT_MODULE()

+ (BOOL)requiresMainQueueSetup {
  return YES;
}

- (void)invalidate
{
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
      });
      
      // Create the worklet context
      _workletContext = std::make_unique<RNWorklet::JsiWorkletContext>(jsRuntime, callInvoker, errorHandler);
      
      // Create the Worklet API
      auto workletApi = std::make_shared<RNWorklet::JsiWorkletApi>(_workletContext.get());
      jsRuntime->global().setProperty(*jsRuntime, "Worklets",
            jsi::Object::createFromHostObject(*jsRuntime, std::move(workletApi)));
    }
}

@end
