#include <jni.h>

#include <jsi/jsi.h>
#include <ReactCommon/CallInvokerHolder.h>

#include "WKTJsiWorkletApi.h"

using namespace facebook; // NOLINT

extern "C" JNIEXPORT jboolean JNICALL
Java_com_worklets_WorkletsModule_nativeInstall(JNIEnv* env, jclass obj, jlong jsiRuntimeRef, jobject jsCallInvokerHolder)
{
    auto jsiRuntime{ reinterpret_cast<jsi::Runtime*>(jsiRuntimeRef) };
    auto jsCallInvoker{ jni::alias_ref<react::CallInvokerHolder::javaobject>{ reinterpret_cast<react::CallInvokerHolder::javaobject>(jsCallInvokerHolder) }->cthis()->getCallInvoker() };

    RNWorklet::JsiWorkletContext::getDefaultInstance()->initialize(
    "default", jsiRuntime, [=](std::function<void()> &&f) {
        jsCallInvoker->invokeAsync(std::move(f));
    });

    // Install the worklet API
    RNWorklet::JsiWorkletApi::installApi(*jsiRuntime);

    return true;
}
