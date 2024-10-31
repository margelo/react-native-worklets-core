#pragma once

#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>
#include <jsi/jsi.h>

namespace RNWorklet {

using namespace facebook;

class JWorklets final : public jni::HybridClass<JWorklets> {
public:
  static auto constexpr kJavaDescriptor = "Lcom/margelo/worklets/Worklets;";

private:
  explicit JWorklets();

private:
  // JNI Methods
  static jni::local_ref<JWorklets::jhybriddata>
  initHybrid(jni::alias_ref<jhybridobject> javaThis);
  void install(
      jlong runtimePointer,
      jni::alias_ref<react::CallInvokerHolder::javaobject> callInvokerHolder);

private:
  static auto constexpr TAG = "Worklets";
  using HybridBase::HybridBase;
  friend HybridBase;

public:
  static void registerNatives();
};

} // namespace RNWorklet
