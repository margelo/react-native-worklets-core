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

namespace RNWorklet {

namespace jsi = facebook::jsi;

std::shared_ptr<JsiWorkletContext>
JsiWorkletApi::createWorkletContext(const std::string &name) {
  return std::make_shared<JsiWorkletContext>(name);
}

} // namespace RNWorklet
