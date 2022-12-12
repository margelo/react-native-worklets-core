#ifdef __cplusplus
#import "JsiWorkletApi.h"
#endif

#ifdef RCT_NEW_ARCH_ENABLED
#import "RNWorkletsSpec.h"

@interface Worklets : NSObject <NativeWorkletsSpec>
#else
#import <React/RCTBridgeModule.h>

@interface Worklets : NSObject <RCTBridgeModule>
#endif

@end
