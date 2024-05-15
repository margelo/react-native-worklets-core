//
//  WorkletsOnLoad.mm
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 15.05.24.
//

#import "NativeWorkletsModule.h"
#import <Foundation/Foundation.h>
#import <ReactCommon/CxxTurboModuleUtils.h>

@interface WorkletsOnLoad : NSObject
@end

@implementation WorkletsOnLoad

+ (void)load {
  facebook::react::registerCxxModuleToGlobalModuleMap(
      std::string(facebook::react::NativeWorkletsModule::kModuleName),
      [&](std::shared_ptr<facebook::react::CallInvoker> jsInvoker) {
        return std::make_shared<facebook::react::NativeWorkletsModule>(jsInvoker);
      });
}

@end
