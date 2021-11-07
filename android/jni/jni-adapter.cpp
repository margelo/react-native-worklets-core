#include <jni.h>
#include <jsi/jsi.h>

#include <fbjni/fbjni.h>

#if __has_include(<ReactCommon/CallInvokerHolder.h>)
#include <ReactCommon/CallInvokerHolder.h>
#endif

#if __has_include(<ReactCommon/JSCallInvokerHolder.h>)
#include <ReactCommon/JSCallInvokerHolder.h>
#define CallInvokerHolder JSCallInvokerHolder
#endif

#include <JsiWorkletApi.h>

using namespace facebook;

extern "C" JNIEXPORT void JNICALL
Java_com_rnworklets_ReactNativeWorkletsModule_initialize(JNIEnv *env,
                                     jclass clazz,
                                     jlong jsiPtr,
                                     jobject callInvokerHolderImpl)
{
    /*auto runtime = reinterpret_cast<jsi::Runtime *>(jsiPtr);
    auto callInvokerHolder = std::shared_ptr<facebook::react::CallInvokerHolder>(
            reinterpret_cast<facebook::react::CallInvokerHolder*>(callInvokerHolderImpl));

    auto errorHandler = std::make_shared<RNWorklet::JsiErrorHandler>(
            [](const std::exception &err) {

    });

    auto workletContext = std::make_shared<RNWorklet::JsiWorkletContext>(
            runtime, callInvokerHolder->getJSCallInvoker(), errorHandler);

    auto workletApi = std::make_shared<RNWorklet::JsiWorkletApi>(std::move(workletContext));

    runtime->global().setProperty(*runtime,
                                  "Worklets",
                                  jsi::Object::createFromHostObject(
                                          *runtime,
                                          std::move(workletApi)));*/
}

extern "C" JNIEXPORT void JNICALL
Java_com_rnworklets_reactnativesequel_ReactNativeWorkletsModule_destruct(JNIEnv *env, jclass clazz)
{

}