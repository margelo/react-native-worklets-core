#pragma once

#include <jsi/jsi.h>

#include <JsiDispatcher.h>
#include <JsiHostObject.h>
#include <JsiSharedValue.h>
#include <JsiWorklet.h>
#include <JsiWorkletContext.h>

using namespace facebook;

namespace facebook {
namespace react {

struct Promise {
  Promise(jsi::Runtime &rt, jsi::Function resolve, jsi::Function reject);

  void resolve(const jsi::Value &result);

  void reject(const std::string &error);

  jsi::Runtime &runtime_;
  jsi::Function resolve_;
  jsi::Function reject_;
};

using PromiseSetupFunctionType =
    std::function<void(jsi::Runtime &rt, std::shared_ptr<Promise>)>;

jsi::Value createPromiseAsJSIValue(jsi::Runtime &rt,
                                   const PromiseSetupFunctionType func);
} // namespace react
} // namespace facebook

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
  JsiWorkletApi(std::shared_ptr<JsiWorkletContext> context);
  JsiWorkletApi(JsiWorkletContext *context) : JsiWorkletApi(std::make_shared<JsiWorkletContext>(context)) {}

private:
    std::shared_ptr<JsiWorkletContext> getContext(const char *name) {
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

  std::shared_ptr<JsiWorkletContext> _context;
  std::map<std::string, std::shared_ptr<JsiWorkletContext>> _contexts;
};
} // namespace RNWorklet
