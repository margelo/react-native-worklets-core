#include "JWorklets.h"
#include <android/log.h>
// #include "InstallNitro.hpp"

#include <exception>

namespace RNWorklet {

JWorklets::JWorklets() = default;

jni::local_ref<JWorklets::jhybriddata> JWorklets::initHybrid(jni::alias_ref<JWorklets::jhybridobject>) {
  return makeCxxInstance();
}

void JWorklets::install(jlong runtimePointer, jni::alias_ref<react::CallInvokerHolder::javaobject> callInvokerHolder) {
  auto runtime = reinterpret_cast<jsi::Runtime*>(runtimePointer);
  if (runtime == nullptr) {
    throw std::invalid_argument("jsi::Runtime was null!");
  }

  if (callInvokerHolder == nullptr) {
    throw std::invalid_argument("CallInvokerHolder was null!");
  }
  auto callInvoker = callInvokerHolder->cthis()->getCallInvoker();
  if (callInvoker == nullptr) {
    throw std::invalid_argument("CallInvoker was null!");
  }

//  auto dispatcher = std::make_shared<CallInvokerDispatcher>(callInvoker);
//   margelo::nitro::install(*runtime, dispatcher);
}

void JWorklets::registerNatives() {
  registerHybrid({makeNativeMethod("initHybrid", JWorklets::initHybrid), makeNativeMethod("install", JWorklets::install)});
}

} // namespace margelo::nitro
