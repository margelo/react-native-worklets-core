#pragma once

#include <memory>
#include <string>

#include "WKTArgumentsWrapper.h"
#include "WKTJsiBaseDecorator.h"
#include "WKTJsiHostObject.h"
#include "WKTJsiWrapper.h"
#include <jsi/jsi.h>

namespace RNWorklet {

using JSRuntimeInvoker = std::function<void(std::function<void(jsi::Runtime &runtime)>)>;

namespace jsi = facebook::jsi;

static const char *PropNameConsole = "console";

class JsiConsoleImpl : public JsiHostObject,
                       public std::enable_shared_from_this<JsiConsoleImpl> {
public:
  JsiConsoleImpl(jsi::Runtime &runtime, const jsi::Value &consoleObj, const JSRuntimeInvoker& jsInvoker)
      : _jsInvoker(jsInvoker), _consoleObj(consoleObj.asObject(runtime)),
        _logFn(_consoleObj.getPropertyAsFunction(runtime, "log")),
        _infoFn(_consoleObj.getPropertyAsFunction(runtime, "info")),
        _warnFn(_consoleObj.getPropertyAsFunction(runtime, "warn")),
        _errorFn(_consoleObj.getPropertyAsFunction(runtime, "error")) {}
 
public:

  JSI_HOST_FUNCTION(log) {
    ArgumentsWrapper argsWrapper(runtime, arguments, count);
    auto thisWrapper = JsiWrapper::wrap(runtime, thisValue);
    _jsInvoker([weakSelf = weak_from_this(), argsWrapper, thisWrapper,
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
    _jsInvoker([weakSelf = weak_from_this(), argsWrapper, thisWrapper,
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
    _jsInvoker([weakSelf = weak_from_this(), argsWrapper, thisWrapper,
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
    _jsInvoker([weakSelf = weak_from_this(), argsWrapper, thisWrapper,
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
  JSRuntimeInvoker _jsInvoker;
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
  JsiConsoleDecorator(JSRuntimeInvoker&& jsInvoker): _jsInvoker(jsInvoker) { }
  
  void initialize(jsi::Runtime& fromRuntime) override {
    auto consoleObj = fromRuntime.global().getProperty(fromRuntime, PropNameConsole);
    _jsConsoleImpl = std::make_shared<JsiConsoleImpl>(fromRuntime, consoleObj, _jsInvoker);
  }
  
  void decorateRuntime(jsi::Runtime &toRuntime) override {
    toRuntime.global().setProperty(toRuntime, PropNameConsole,
                                   jsi::Object::createFromHostObject(toRuntime, _jsConsoleImpl));
  };
  
private:
  JSRuntimeInvoker _jsInvoker;
  std::shared_ptr<JsiConsoleImpl> _jsConsoleImpl;
};
} // namespace RNWorklet
