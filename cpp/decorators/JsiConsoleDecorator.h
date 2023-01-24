#pragma once

#include <memory>
#include <string>

#include "JsiBaseDecorator.h"
#include "JsiHostObject.h"
#include "JsiWrapper.h"
#include <jsi/jsi.h>

#ifdef ANDROID
#include <android/log.h>
#else
#include <syslog.h>
#endif

namespace RNWorklet {

namespace jsi = facebook::jsi;

static const char *PropNameConsole = "console";

class JsiConsoleImpl : public JsiHostObject {
public:
  JSI_HOST_FUNCTION(log) {
    logToConsole(getArgsAsString(runtime, arguments, count));
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(warn) {
    logToConsole("Warning: " + getArgsAsString(runtime, arguments, count));
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(error) {
    logToConsole("Error: " + getArgsAsString(runtime, arguments, count));
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(info) { return log(runtime, thisValue, arguments, count); }

  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(JsiConsoleImpl, log),
                       JSI_EXPORT_FUNC(JsiConsoleImpl, warn),
                       JSI_EXPORT_FUNC(JsiConsoleImpl, error),
                       JSI_EXPORT_FUNC(JsiConsoleImpl, info))

private:
  const std::string getArgsAsString(jsi::Runtime &runtime,
                                    const jsi::Value *arguments, size_t count) {
    std::string retVal = "";
    for (size_t i = 0; i < count; i++) {
      auto wrapper = JsiWrapper::wrap(runtime, arguments[i]);
      retVal += (retVal != "" ? " " : "") + wrapper->toString(runtime);
    }
    return retVal;
  }
  /**
   * Logs message to console
   * @param message Message to be written out
   */
  static void logToConsole(std::string message) {
#ifdef ANDROID
    __android_log_write(ANDROID_LOG_INFO, "RNWorklets", message.c_str());
#else

    syslog(LOG_ERR, "%s\n", message.c_str());
#endif
  }
};

/**
 Decorator for console
 */
class JsiConsoleDecorator : public JsiBaseDecorator {
public:
  void decorateRuntime(jsi::Runtime &runtime) override {
    auto consoleObj = std::make_shared<JsiConsoleImpl>();
    runtime.global().setProperty(
        runtime, PropNameConsole,
        jsi::Object::createFromHostObject(runtime, consoleObj));
  };
};
} // namespace RNWorklet
