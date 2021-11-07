#pragma once

#include <jsi/jsi.h>

#include <JsiDispatcher.h>
#include <JsiHostObject.h>
#include <JsiSharedValue.h>
#include <JsiWorkletContext.h>
#include <ReactCommon/TurboModuleUtils.h>

using namespace facebook;

namespace RNWorklet {

using namespace RNJsi;
using namespace RNSkia;

class JsiWorkletApi : public JsiHostObject {
public:
  /**
   * Create the worklet API
   * @param context Worklet context
   * make sure only the runOnJsThread method is available.
   */
  JsiWorkletApi(JsiWorkletContext *context);

  /**
   Destructor
   */
  ~JsiWorkletApi() {
    _context = nullptr;
    _contexts.clear();
  }

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
  JsiWorkletContext *getContext(const char *name) {
    // Let's see if we are launching this on the default context
    std::string contextName = "default";
    if (name != nullptr) {
      contextName = name;
    }

    // We support multiple worklet contexts - so we'll find or create
    // the one requested, or use the default one if none is requested.
    // The default worklet context is also a separate context.
    if (_contexts.count(contextName) == 0) {
      // Create new context
      _contexts.emplace(contextName, _context);
    }
    return _contexts.at(contextName);
  }

  JsiWorkletContext *_context;
  std::map<std::string, JsiWorkletContext *> _contexts;
};
} // namespace RNWorklet
