#include <JsiArrayWrapper.h>
#include <ostream>
#include <JsiWrapper.h>

namespace RNWorklet {

    using namespace facebook;

    JsiArrayWrapper::JsiArrayWrapper(jsi::Runtime &runtime, jsi::Array& array,
                                     JsiWrapper* parent,
                                     std::shared_ptr<JsiFunctionResolver> functionResolver)
            : _functionResolver(functionResolver), _parent(parent) {
        // Copy data
        size_t size = array.size(runtime);
        _array.resize(size);

        for (size_t i = 0; i < size; i++) {
            _array[i] = JsiWrapper::wrap(
                  runtime, array.getValueAtIndex(runtime, i), parent, functionResolver, "");
        }
    }

    void JsiArrayWrapper::set(jsi::Runtime &rt,
                              const jsi::PropNameID &name,
                              const jsi::Value &value) {
        // Do nada - an array set method is never called
    }

    jsi::Value JsiArrayWrapper::get(jsi::Runtime &runtime, const jsi::PropNameID &name) {
        auto nameStr = name.utf8(runtime);

        // Start by checking if this is a call to get an index:
        if(!nameStr.empty() && std::all_of(nameStr.begin(), nameStr.end(), ::isdigit)) {
            // Return property by index
            auto index = std::stoi(nameStr.c_str());
            auto prop = _array[index];
            return JsiWrapper::unwrap(runtime, prop);
        }

        // Now let's check for functions
        if(nameStr == "length") {
            return (double)_array.size();
        } else if(nameStr == "push") {
            return JSIFN("push", {
                // Push all arguments to the array
                auto lastIndex = _array.size();
                for(size_t i=0; i<count; i++) {
                    std::string indexString = std::to_string(lastIndex++);
                    _array.push_back(JsiWrapper::wrap(
                            runtime, arguments[i], _parent, _functionResolver, indexString));
                }
                if(_parent != nullptr) {
                  _parent->notifyListeners();
                }
                return (double)_array.size();
            });
        } else if(nameStr == "pop") {
            return JSIFN("pop", {
                // Pop last element from array
                if (_array.empty()) {
                    return jsi::Value::undefined();
                }
                auto lastEl = _array.at(_array.size() - 1);
                _array.pop_back();
                if(_parent != nullptr) {
                  _parent->notifyListeners();
                }
                return JsiWrapper::unwrap(runtime, lastEl);
            });
        } else if(nameStr == "forEach") {
            return JSIFN("forEach", {
                // Enumerate and call back
                auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
                jsi::Value thisArg = count > 1 ? arguments[1].asObject(runtime) : jsi::Value::undefined();
                for(size_t i=0; i<_array.size(); i++) {
                    if(thisArg.isUndefined()) {
                        callbackFn.call(runtime, JsiWrapper::unwrap(runtime, _array.at(i)));
                    } else {
                      callbackFn.callWithThis(runtime, thisValue.asObject(runtime),
                                              JsiWrapper::unwrap(runtime, _array.at(i)), thisArg);
                    }
                }
                return jsi::Value::undefined();
            });
        } else if(nameStr == "map") {
          return JSIFN("map", {
              // Enumerate and return
              auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
              jsi::Value thisArg = count > 1 ? arguments[1].asObject(runtime) : jsi::Value::undefined();
              
              auto result = jsi::Array(runtime, _array.size());
              for(size_t i=0; i<_array.size(); i++) {
                  jsi::Value retVal;
                  if(thisArg.isUndefined()) {
                      retVal = callbackFn.call(runtime, JsiWrapper::unwrap(runtime, _array.at(i)));
                  } else {
                      retVal = callbackFn.callWithThis(runtime, thisValue.asObject(runtime),
                                            JsiWrapper::unwrap(runtime, _array.at(i)), thisArg);
                  }
                  result.setValueAtIndex(runtime, i, retVal);
              }
              return result;
          });
      } else if(nameStr == "filter") {
        return JSIFN("filter", {
            // Filter array
            auto callbackFn = arguments[0].asObject(runtime).asFunction(runtime);
            jsi::Value thisArg = count > 1 ? arguments[1].asObject(runtime) : jsi::Value::undefined();
            
            std::vector<std::shared_ptr<JsiWrapper>> result;
          
            for(size_t i=0; i<_array.size(); i++) {
                jsi::Value retVal;
                if(thisArg.isUndefined()) {
                    retVal = callbackFn.call(runtime, JsiWrapper::unwrap(runtime, _array.at(i)));
                } else {
                    retVal = callbackFn.callWithThis(runtime, thisValue.asObject(runtime),
                                          JsiWrapper::unwrap(runtime, _array.at(i)), thisArg);
                }
                if(retVal.getBool() == true) {
                    result.push_back(_array.at(i));
                }
            }
            auto returnValue = jsi::Array(runtime, result.size());
            for(size_t i = 0; i < result.size(); i++) {
                returnValue.setValueAtIndex(runtime, i, JsiWrapper::unwrap(runtime, result.at(i)));
            }
            return returnValue;
        });
      } else if(nameStr == "toString") {
          return JSIFN("toString", {
              return jsi::String::createFromUtf8(runtime, toString(runtime));
          });
        }
        return jsi::Value::undefined();
    }

    std::string JsiArrayWrapper::toString(jsi::Runtime &runtime) {
        std::string retVal = "";
        // Return array contents
        for(size_t i=0; i<_array.size(); i++) {
            auto str = _array.at(i)->toString(runtime);
            retVal += (i > 0 ? ", " : "") + str;
        }
        return retVal;
    }
}
