#pragma once

#include <memory>
#include <vector>

#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

class ArgumentsWrapper {
public:
  ArgumentsWrapper(jsi::Runtime &runtime, const jsi::Value *arguments,
                   size_t count)
      : _count(count) {
    _arguments.resize(count);
    for (size_t i = 0; i < count; ++i) {
      _arguments[i] = JsiWrapper::wrap(runtime, arguments[i]);
    }
  }

  size_t getCount() const { return _count; }

  std::vector<jsi::Value> getArguments(jsi::Runtime &runtime) const {
    std::vector<jsi::Value> args(_count);
    for (size_t i = 0; i < _count; ++i) {
      args[i] = JsiWrapper::unwrap(runtime, _arguments.at(i));
    }
    return args;
  }

  static const jsi::Value *toArgs(const std::vector<jsi::Value> &args) {
    return static_cast<const jsi::Value *>(args.data());
  }

private:
  size_t _count;
  std::vector<std::shared_ptr<JsiWrapper>> _arguments;
};

} // namespace RNWorklet
