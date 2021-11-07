#include "JniReactNativeWorkletsModule.h"

#include <android/log.h>
#include <jni.h>
#include <string>
#include <utility>

using namespace facebook;

// JNI binding
void JniReactNativeWorkletsModule::registerNatives() {
    registerHybrid({ makeNativeMethod("initHybrid", JniReactNativeWorkletsModule::initHybrid) });
}

// JNI init
jni::local_ref<jni::HybridClass<JniReactNativeWorkletsModule>::jhybriddata> JniReactNativeWorkletsModule::initHybrid(
        jni::alias_ref<jhybridobject> jThis,
        jlong jsContext,
        JSCallInvokerHolder jsCallInvokerHolder) {

    // cast from JNI hybrid objects to C++ instances
    auto jsCallInvoker = jsCallInvokerHolder->cthis()->getCallInvoker();

    return makeCxxInstance(
            jThis,
            reinterpret_cast<jsi::Runtime *>(jsContext),
            jsCallInvoker);
}
