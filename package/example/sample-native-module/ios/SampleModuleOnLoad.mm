//
//  WorkletsOnLoad.mm
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 15.05.24.
//

#import "NativeSampleModule.h"
#import "NSThreadDispatcher.h"
#import <Foundation/Foundation.h>
#import <ReactCommon/CxxTurboModuleUtils.h>

@interface SampleModuleOnLoad : NSObject
@end

@implementation SampleModuleOnLoad

+ (void)load {
  facebook::react::registerCxxModuleToGlobalModuleMap(
      std::string(facebook::react::NativeSampleModule::kModuleName),
      [&](std::shared_ptr<facebook::react::CallInvoker> jsInvoker) {
 
//        std::shared_ptr<margelo::NSThreadDispatcher> customThreadDispatcher = std::make_shared<margelo::NSThreadDispatcher>("extra.thread");
        
        return std::make_shared<facebook::react::NativeSampleModule>(jsInvoker);
      });
}

@end
