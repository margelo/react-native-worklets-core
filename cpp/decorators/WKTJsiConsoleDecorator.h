#pragma once

#include <memory>
#include <string>

#include "WKTArgumentsWrapper.h"
#include "WKTJsiBaseDecorator.h"
#include "WKTJsiHostObject.h"
#include "WKTJsiWrapper.h"
#include <jsi/jsi.h>

namespace RNWorklet {

namespace jsi = facebook::jsi;

static const char *PropNameConsole = "console";

class JsiConsoleImpl : public JsiHostObject,
                       public std::enable_shared_from_this<JsiConsoleImpl> {
public:
  JsiConsoleImpl(jsi::Runtime &runtime, const jsi::Value &consoleObj)
      : _consoleObj(consoleObj.asObject(runtime)),
        _logFn(_consoleObj.getPropertyAsFunction(runtime, "log")),
        _infoFn(_consoleObj.getPropertyAsFunction(runtime, "info")),
        _warnFn(_consoleObj.getPropertyAsFunction(runtime, "warn")),
        _errorFn(_consoleObj.getPropertyAsFunction(runtime, "error")) {}

  JSI_HOST_FUNCTION(log) {
    ArgumentsWrapper argsWrapper(runtime, arguments, count);
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
    JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
        [weakSelf = weak_from_this(), argsWrapper, thisWrapper,
         count](jsi::Runtime &runtime) {
          auto args = argsWrapper.getArguments(runtime);
          auto self = weakSelf.lock();
          if (self) {
            if (thisWrapper->getType() == JsiWrapperType::Object) {
              auto thisObject = thisWrapper->unwrap(runtime);
              self->_logFn.callWithThis(runtime, thisObject.asObject(runtime),
                                        ArgumentsWrapper::toArgs(args), count);
            } else {
              self->_logFn.call(runtime, ArgumentsWrapper::toArgs(args), count);
            }
          }
        });
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(warn) {
    ArgumentsWrapper argsWrapper(runtime, arguments, count);
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
    JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
        [weakSelf = weak_from_this(), argsWrapper, thisWrapper,
         count](jsi::Runtime &runtime) {
          auto args = argsWrapper.getArguments(runtime);
          auto self = weakSelf.lock();
          if (self) {
            if (thisWrapper->getType() == JsiWrapperType::Object) {
              auto thisObject = thisWrapper->unwrap(runtime);
              self->_warnFn.callWithThis(runtime, thisObject.asObject(runtime),
                                         ArgumentsWrapper::toArgs(args), count);
            } else {
              self->_warnFn.call(runtime, ArgumentsWrapper::toArgs(args),
                                 count);
            }
          }
        });
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(error) {
    ArgumentsWrapper argsWrapper(runtime, arguments, count);
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
    JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
        [weakSelf = weak_from_this(), argsWrapper, thisWrapper,
         count](jsi::Runtime &runtime) {
          auto args = argsWrapper.getArguments(runtime);
          auto self = weakSelf.lock();
          if (self) {
            if (thisWrapper->getType() == JsiWrapperType::Object) {
              auto thisObject = thisWrapper->unwrap(runtime);
              self->_errorFn.callWithThis(runtime, thisObject.asObject(runtime),
                                          ArgumentsWrapper::toArgs(args),
                                          count);
            } else {
              self->_errorFn.call(runtime, ArgumentsWrapper::toArgs(args),
                                  count);
            }
          }
        });
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(info) {
    ArgumentsWrapper argsWrapper(runtime, arguments, count);
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
    JsiWorkletContext::getDefaultInstance()->invokeOnJsThread(
        [weakSelf = weak_from_this(), argsWrapper, thisWrapper,
         count](jsi::Runtime &runtime) {
          auto args = argsWrapper.getArguments(runtime);
          auto self = weakSelf.lock();
          if (self) {
            if (thisWrapper->getType() == JsiWrapperType::Object) {
              auto thisObject = thisWrapper->unwrap(runtime);
              self->_infoFn.callWithThis(runtime, thisObject.asObject(runtime),
                                         ArgumentsWrapper::toArgs(args), count);
            } else {
              self->_infoFn.call(runtime, ArgumentsWrapper::toArgs(args),
                                 count);
            }
          }
        });
    return jsi::Value::undefined();
  }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiConsoleImpl, log),
                       JSI_EXPORT_FUNC(JsiConsoleImpl, warn),
                       JSI_EXPORT_FUNC(JsiConsoleImpl, error),
                       JSI_EXPORT_FUNC(JsiConsoleImpl, info))

private:
  jsi::Object _consoleObj;
  jsi::Function _logFn;
  jsi::Function _warnFn;
  jsi::Function _infoFn;
  jsi::Function _errorFn;
};

/**
 Decorator for console
 */
class JsiConsoleDecorator : public JsiBaseDecorator {
public:
  void initialize(jsi::Runtime &runtime) override {
    auto consoleObj = runtime.global().getProperty(runtime, PropNameConsole);
    _jsiConsoleImpl = std::make_shared<JsiConsoleImpl>(runtime, consoleObj);
  }

  void decorateRuntime(jsi::Runtime &runtime) override {
    runtime.global().setProperty(
        runtime, PropNameConsole,
        jsi::Object::createFromHostObject(runtime, _jsiConsoleImpl));
  };

private:
  std::shared_ptr<JsiConsoleImpl> _jsiConsoleImpl;
};
} // namespace RNWorklet
