#pragma once

#include <jsi/jsi.h>

namespace RNWorklet {
using namespace facebook;
class JsiWrapper;

using JsiFunctionResolver =
    std::function<jsi::HostFunctionType(const jsi::Value &)>;

class JsiArrayWrapper : public jsi::HostObject {
public:
  /**
   * Constructs a new array wrapper
   * @param runtime In runtime
   * @param array Array to wrap
   * @param parent Parent wrapper object
   * @param functionResolver resolver for jsi::Functions
   */
  JsiArrayWrapper(jsi::Runtime &runtime, jsi::Array &array, JsiWrapper *parent,
                  std::shared_ptr<JsiFunctionResolver> functionResolver);

  /**
   * Overridden jsi::HostObject set property method
   * @param rt Runtime
   * @param name Name of value to set
   * @param value Value to set
   */
  void set(jsi::Runtime &rt, const jsi::PropNameID &name,
           const jsi::Value &value) override;

  /**
   * Overridden jsi::HostObject get property method. Returns functions from
   * the map of functions.
   * @param runtime Runtime
   * @param name Name of value to get
   * @return Value
   */
  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  /**
   * Returns the array as a string
   * @param runtime Runtime to return in
   * @return Array as string
   */
  std::string toString(jsi::Runtime &runtime);

private:
  std::vector<std::shared_ptr<JsiWrapper>> _array;
  std::shared_ptr<JsiFunctionResolver> _functionResolver;
  JsiWrapper *_parent;
};
} // namespace RNWorklet
