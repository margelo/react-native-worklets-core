#include "JsiWorkletApi.h"

#include <jsi/jsi.h>

#include <memory>
#include <string>
#include <vector>

#include "JsiBaseDecorator.h"
#include "JsiHostObject.h"
#include "JsiSharedValue.h"
#include "JsiWorklet.h"
#include "JsiWorkletContext.h"

#include "JsiJsDecorator.h"
#include "JsiPerformanceDecorator.h"
#include "JsiSetImmediateDecorator.h"
#include "JsiConsoleDecorator.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

const char *JsiWorkletApi::WorkletsApiName = "Worklets";
std::shared_ptr<JsiWorkletApi> JsiWorkletApi::instance;

/**
 * Installs the worklet API into the provided runtime
 */
void JsiWorkletApi::installApi(jsi::Runtime &runtime) {
  auto context = JsiWorkletContext::getDefaultInstance();
  auto existingApi = (runtime.global().getProperty(runtime, WorkletsApiName));
  if (existingApi.isObject()) {
    return;
  }

  runtime.global().setProperty(
      runtime, WorkletsApiName,
      jsi::Object::createFromHostObject(runtime, getInstance()));
}

std::shared_ptr<JsiWorkletContext>
JsiWorkletApi::createWorkletContext(const std::string &name) {
  return std::make_shared<JsiWorkletContext>(name);
}

void JsiWorkletApi::addDecorator(std::shared_ptr<JsiBaseDecorator> decorator) {
  // Decorate default context
  JsiWorkletContext::getDefaultInstance()->addDecorator(decorator);
}

std::shared_ptr<JsiWorkletApi> JsiWorkletApi::getInstance() {
  if (instance == nullptr) {
    instance = std::make_shared<JsiWorkletApi>();
    instance->addDecorator(std::make_shared<JsiSetImmediateDecorator>());
    instance->addDecorator(std::make_shared<JsiPerformanceDecorator>());
    // in JS for now:
    // instance->addDecorator(std::make_shared<JsiConsoleDecorator>());
  }
  return instance;
}

/**
 Invalidate the api instance.
 */
void JsiWorkletApi::invalidateInstance() { instance = nullptr; }

} // namespace RNWorklet
