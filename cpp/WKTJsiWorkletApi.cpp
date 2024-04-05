#include "WKTJsiWorkletApi.h"

#include <jsi/jsi.h>

#include <memory>
#include <string>
#include <vector>

#include "WKTJsiBaseDecorator.h"
#include "WKTJsiHostObject.h"
#include "WKTJsiSharedValue.h"
#include "WKTJsiWorklet.h"
#include "WKTJsiWorkletContext.h"

#include "hybridObject/TestHybridObject.h"

namespace RNWorklet
{

  namespace jsi = facebook::jsi;

  const char *JsiWorkletApi::WorkletsApiName = "Worklets";
  std::shared_ptr<JsiWorkletApi> JsiWorkletApi::instance;

  /**
   * Installs the worklet API into the provided runtime
   */
  void JsiWorkletApi::installApi(jsi::Runtime &runtime)
  {
    auto context = JsiWorkletContext::getDefaultInstance();
    auto existingApi = (runtime.global().getProperty(runtime, WorkletsApiName));
    if (existingApi.isObject())
    {
      return;
    }

    runtime.global().setProperty(
        runtime, WorkletsApiName,
        jsi::Object::createFromHostObject(runtime, getInstance()));

    // Install test object
    std::shared_ptr<TestHybridObject> testObject = std::make_shared<TestHybridObject>();
    runtime.global().setProperty(
        runtime, "TestObject",
        jsi::Object::createFromHostObject(runtime, testObject));
  }

  std::shared_ptr<JsiWorkletContext>
  JsiWorkletApi::createWorkletContext(const std::string &name)
  {
    return std::make_shared<JsiWorkletContext>(name);
  }

  std::shared_ptr<JsiWorkletApi> JsiWorkletApi::getInstance()
  {
    if (instance == nullptr)
    {
      instance = std::make_shared<JsiWorkletApi>();
    }
    return instance;
  }

  /**
   Invalidate the api instance.
   */
  void JsiWorkletApi::invalidateInstance() { instance = nullptr; }

} // namespace RNWorklet
