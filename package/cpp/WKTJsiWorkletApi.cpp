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
JsiWorkletApi::createWorkletContext(jsi::Runtime& runtime, const std::string &name) {
    // Create an arbitrary background dispatch-queue
    auto dispatchQueue = std::make_shared<DispatchQueue>("worklets_queue_" + name);
    // Capture JS callinvoker
    auto callInvoker = _jsCallInvoker;
    // Initialize default context
    return std::make_shared<JsiWorkletContext>(name, &runtime, [callInvoker](std::function<void()>&& func) {
      callInvoker->invokeAsync(std::move(func));
    }, [dispatchQueue](std::function<void()>&& func) {
      dispatchQueue->dispatch(std::move(func));
    });
}

} // namespace RNWorklet
