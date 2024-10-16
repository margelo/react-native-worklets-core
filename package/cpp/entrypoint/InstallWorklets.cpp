//
// Created by Hanno Gödecke on 16.10.24.
//

#include "InstallWorklets.h"

#include "WKTJsiWorkletApi.h"
#include "WKTJsiWorkletContext.h"

namespace RNWorklet {
    void install(jsi::Runtime &runtime, const std::shared_ptr<react::CallInvoker>& callInvoker) {
        // Create new instance of the Worklets API (JSI module)
        auto worklets = std::make_shared<RNWorklet::JsiWorkletApi>();
        // Initialize the shared default instance with a JS call invoker
        std::shared_ptr<RNWorklet::JsiWorkletContext> defaultInstance =
                RNWorklet::JsiWorkletContext::getDefaultInstanceAsShared();
        defaultInstance->initialize("default", &runtime,
                                    [callInvoker](std::function<void()> &&callback) {
                                        callInvoker->invokeAsync(std::move(callback));
                                    });
        // Install into JS global namespace
        jsi::Object proxy = jsi::Object::createFromHostObject(runtime, worklets);
        runtime.global().setProperty(runtime, "WorkletsProxy", proxy);
    }
} // RNWorklet
