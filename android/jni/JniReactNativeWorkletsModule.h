
#pragma once

#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <JsiWorkletContext.h>
#include <JsiWorkletApi.h>

using namespace facebook;

using JSCallInvokerHolder =
jni::alias_ref<facebook::react::CallInvokerHolder::javaobject>;

class JniReactNativeWorkletsModule : public jni::HybridClass<JniReactNativeWorkletsModule> {
public:
    static auto constexpr kJavaDescriptor = "Lcom/rnworklets/ReactNativeWorkletsModule;";

    static jni::local_ref<jni::HybridClass<JniReactNativeWorkletsModule>::jhybriddata> initHybrid(
            jni::alias_ref<jhybridobject> jThis,
            jlong jsContext,
            JSCallInvokerHolder jsCallInvokerHolder);

    static void registerNatives();

    JniReactNativeWorkletsModule() {
        _workletContext = nullptr;
    }

    explicit JniReactNativeWorkletsModule(
            jni::alias_ref<JniReactNativeWorkletsModule::jhybridobject> jThis,
            jsi::Runtime *runtime,
            std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker)
            : _javaPart(jni::make_global(jThis)),
              _jsRuntime(runtime),
              _jsCallInvoker(jsCallInvoker) {
    }

    void installApi();

private:
    friend HybridBase;

    jni::global_ref<JniReactNativeWorkletsModule::javaobject> _javaPart;
    jsi::Runtime *_jsRuntime;
    std::shared_ptr<facebook::react::CallInvoker> _jsCallInvoker;

    std::shared_ptr<RNWorklet::JsiWorkletContext> _workletContext;

};
