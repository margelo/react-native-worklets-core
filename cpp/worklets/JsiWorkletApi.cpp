#include <JsiDescribe.h>
#include <JsiWorkletApi.h>

namespace RNWorklet {
using namespace facebook;

std::shared_ptr<JsiWorkletContext> JsiWorkletApi::getContext(const char *name) {
  // Let's see if we are launching this on the default context
  if (name == nullptr) {
    return _context;
  }

  // Throw error if the context hasn't been created yet.
  if (_contexts.count(name) == 0) {
    _context->raiseError(
        std::runtime_error("A context with the name '" + std::string(name) + "' does not exist. Use the "
                           "createWorkletContext method to create it."));
    return nullptr;
  }
  return _contexts.at(name);
}

} // namespace RNWorklet
