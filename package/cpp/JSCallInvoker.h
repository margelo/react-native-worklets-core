//
//  JSCallInvoker.h
//  Pods
//
//  Created by Marc Rousavy on 16.05.24.
//

#include <jsi/jsi.h>
#include <functional>

namespace RNWorklet {

using namespace facebook;

using JSCallInvoker = std::function<void(std::function<void(jsi::Runtime& targetRuntime)>&&)>;

} // namespace RNWorklet
