#include "JWorklets.h"
#include <android/log.h>
#include "InstallWorklets.h"

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

  RNWorklet::install(*runtime, callInvoker);
}

void JWorklets::registerNatives() {
  registerHybrid({makeNativeMethod("initHybrid", JWorklets::initHybrid), makeNativeMethod("install", JWorklets::install)});
}

} // namespace margelo::nitro
