#pragma once

#include <ReactCommon/TurboModuleUtils.h>
#include <jsi/jsi.h>

#include <JsiDispatcher.h>
#include <JsiHostObject.h>
#include <JsiSharedValue.h>
#include <JsiWorklet.h>
#include <JsiWorkletContext.h>

namespace RNWorklet {

using namespace facebook;
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
    return &_contexts.at(contextName);
  }

  JsiWorkletContext *_context;
  std::map<std::string, JsiWorkletContext> _contexts;
};
} // namespace RNWorklet
