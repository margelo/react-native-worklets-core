/// Entry point for JNI.

#include "JWorklets.h"
#include <fbjni/fbjni.h>
#include <jni.h>

using namespace RNWorklet;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    return facebook::jni::initialize(vm, [] {
        JWorklets::registerNatives();
    });
}
