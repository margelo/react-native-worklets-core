#pragma once

#include <jsi/jsi.h>

#include <JsiDispatcher.h>
#include <JsiHostObject.h>
#include <JsiSharedValue.h>
#include <JsiWorkletContext.h>

namespace RNWorklet {

using namespace facebook;
using namespace RNJsi;

class JsiWorkletApi : public JsiHostObject {
public:
  /**
   * Create the worklet API
   * @param context Worklet context
   * make sure only the runOnJsThread method is available.
   */
  JsiWorkletApi(JsiWorkletContext *context);

  /**
   * Installs the worklet API into the provided runtime
   * @param context Worklet context to install API for
   */
  static void installApi(JsiWorkletContext *context) {
    auto workletApi = std::make_shared<JsiWorkletApi>(context);
    context->getJsRuntime()->global().setProperty(
        *context->getJsRuntime(), "Worklets",
        jsi::Object::createFromHostObject(*context->getJsRuntime(),
                                          std::move(workletApi)));
  }

private:
  JsiWorkletContext *getContext(const char *name);

  JsiWorkletContext *_context;
  std::map<std::string, std::shared_ptr<JsiWorkletContext>> _contexts;
};
} // namespace RNWorklet
