#include "JniReactNativeWorkletsModule.h"

#include <android/log.h>
#include <jni.h>
#include <string>
#include <utility>

using namespace facebook;

// JNI binding
void JniReactNativeWorkletsModule::registerNatives() {
    registerHybrid({
        makeNativeMethod("initHybrid", JniReactNativeWorkletsModule::initHybrid),
        makeNativeMethod("installApi", JniReactNativeWorkletsModule::installApi)
    });
}

void JniReactNativeWorkletsModule::installApi() {
    // Create error handler
    auto errorHandler = std::make_shared<RNWorklet::JsiErrorHandler>([](const std::exception& err) {
        // Todo: implement and send from jni layer to java?
        throw err;
    });

    // Create/install worklet API
    _workletContext = std::make_shared<RNWorklet::JsiWorkletContext>("default", _jsRuntime,
        _jsCallInvoker, errorHandler);

    // Create / install the worklet API
    RNWorklet::JsiWorkletApi::installApi(_workletContext);
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
