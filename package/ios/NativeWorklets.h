//
//  NativeWorklets.h
//  Pods
//
//  Created by Marc Rousavy on 16.10.24.
//

#ifdef RCT_NEW_ARCH_ENABLED

// New Architecture uses the Codegen'd Spec (TurboModule)
#import "RNWorkletsSpec/RNWorkletsSpec.h"
@interface NativeWorklets : NSObject <NativeWorkletsSpec>
@end

#else

// Old Architecture is an untyped RCTBridgeModule
#import <React/RCTBridgeModule.h>
@interface NativeWorklets : NSObject <RCTBridgeModule>
@end

#endif
