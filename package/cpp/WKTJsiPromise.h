//
//  WKTJsiPromise.h
//  react-native-worklets-core
//
//  Created by Marc Rousavy on 16.05.24.
//

#pragma once

#include <jsi/jsi.h>
#include "WKTJsiWrapper.h"
#include "JSCallInvoker.h"

namespace RNWorklet {

using namespace facebook;

class JsiPromise {
public:
  using Resolver = std::function<void(std::shared_ptr<JsiWrapper>)>;
  using Rejecter = std::function<void(std::exception)>;
  using RunPromiseCallback = std::function<void(jsi::Runtime& runtime, Resolver resolve, Rejecter reject)>;
  
  static jsi::Value createPromise(jsi::Runtime& runtime, JSCallInvoker runOnMainJSRuntime, RunPromiseCallback&& run);
};

} // namespace RNWorklets
