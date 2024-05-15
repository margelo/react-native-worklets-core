#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <hermes/hermes.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

static std::unique_ptr<jsi::Runtime> makeJSIRuntime() {
  return facebook::hermes::makeHermesRuntime();
}

} // namespace RNWorklet
