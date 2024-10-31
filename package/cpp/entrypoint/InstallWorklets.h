//
// Created by Hanno Gödecke on 16.10.24.
//

#pragma once

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>

namespace RNWorklet {
using namespace facebook;
void install(jsi::Runtime& runtime, const std::shared_ptr<react::CallInvoker>& callInvoker);
} // namespace RNWorklet
