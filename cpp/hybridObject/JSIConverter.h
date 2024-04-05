//
// Created by Marc Rousavy on 21.02.24.
//

#pragma once

#include "HybridObject.h"
#include <ReactCommon/CallInvoker.h>
#include <array>
#include <future>
#include <jsi/jsi.h>
#include <memory>
#include <type_traits>
#include <unordered_map>

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#endif

namespace RNWorklet
{

  using namespace facebook;

  // Unknown type (error)
  template <typename ArgType, typename Enable = void>
  struct JSIConverter
  {
    static ArgType fromJSI(jsi::Runtime &, const jsi::Value &)
    {
      static_assert(always_false<ArgType>::value, "This type is not supported by the JSIConverter!");
      return ArgType();
    }
    static jsi::Value toJSI(jsi::Runtime &, ArgType)
    {
      static_assert(always_false<ArgType>::value, "This type is not supported by the JSIConverter!");
      return jsi::Value::undefined();
    }

  private:
    template <typename>
    struct always_false : std::false_type
    {
    };
  };

  // [](Args...) -> T {} <> (Args...) => T
  template <typename ReturnType, typename... Args>
  struct JSIConverter<std::function<ReturnType(Args...)>>
  {
    static std::function<ReturnType(Args...)> fromJSI(jsi::Runtime &runtime, const jsi::Value &arg)
    {
      jsi::Function function = arg.asObject(runtime).asFunction(runtime);
      std::shared_ptr<jsi::Function> sharedFunction = std::make_shared<jsi::Function>(std::move(function));
      return [&runtime, sharedFunction](Args... args) -> ReturnType
      {
        jsi::Value result = sharedFunction->call(runtime, JSIConverter<std::decay_t<Args>>::toJSI(runtime, args)...);
        if constexpr (std::is_same_v<ReturnType, void>)
        {
          // it is a void function (returns undefined)
          return;
        }
        else
        {
          // it returns a custom type, parse it from the JSI value.
          return JSIConverter<ReturnType>::fromJSI(runtime, std::move(result));
        }
      };
    }

    template <size_t... Is>
    static jsi::Value callHybridFunction(const std::function<ReturnType(Args...)> &function, jsi::Runtime &runtime, const jsi::Value *args,
                                         std::index_sequence<Is...>)
    {
      if constexpr (std::is_same_v<ReturnType, void>)
      {
        // it is a void function (will return undefined in JS)
        function(JSIConverter<std::decay_t<Args>>::fromJSI(runtime, args[Is])...);
        return jsi::Value::undefined();
      }
      else
      {
        // it is a custom type, parse it to a JS value
        ReturnType result = function(JSIConverter<std::decay_t<Args>>::fromJSI(runtime, args[Is])...);
        return JSIConverter<ReturnType>::toJSI(runtime, result);
      }
    }
    static jsi::Value toJSI(jsi::Runtime &runtime, const std::function<ReturnType(Args...)> &function)
    {
      jsi::HostFunctionType jsFunction = [function = std::move(function)](jsi::Runtime &runtime, const jsi::Value &thisValue,
                                                                          const jsi::Value *args, size_t count) -> jsi::Value
      {
        if (count != sizeof...(Args))
        {
          [[unlikely]];
          throw jsi::JSError(runtime, "Function expected " + std::to_string(sizeof...(Args)) + " arguments, but received " +
                                          std::to_string(count) + "!");
        }
        return callHybridFunction(function, runtime, args, std::index_sequence_for<Args...>{});
      };
      return jsi::Function::createFromHostFunction(runtime, jsi::PropNameID::forUtf8(runtime, "hostFunction"), sizeof...(Args), jsFunction);
    }
  };

  // HybridObject <> {}
  template <typename T>
  struct is_shared_ptr_to_host_object : std::false_type
  {
  };

  template <typename T>
  struct is_shared_ptr_to_host_object<std::shared_ptr<T>> : std::is_base_of<jsi::HostObject, T>
  {
  };

  template <typename T>
  struct JSIConverter<T, std::enable_if_t<is_shared_ptr_to_host_object<T>::value>>
  {
    using TPointee = typename T::element_type;

#if DEBUG
    inline static std::string getFriendlyTypename()
    {
      std::string name = std::string(typeid(TPointee).name());
#if __has_include(<cxxabi.h>)
      int status = 0;
      char *demangled_name = abi::__cxa_demangle(name.c_str(), NULL, NULL, &status);
      if (status == 0)
      {
        name = demangled_name;
        std::free(demangled_name);
      }
#endif
      return name;
    }

    inline static std::string invalidTypeErrorMessage(const std::string &typeDescription, const std::string &reason)
    {
      return "Cannot convert \"" + typeDescription + "\" to HostObject<" + getFriendlyTypename() + ">! " + reason;
    }
#endif

    static T fromJSI(jsi::Runtime &runtime, const jsi::Value &arg)
    {
#if DEBUG
      if (arg.isUndefined())
      {
        [[unlikely]];
        throw jsi::JSError(runtime, invalidTypeErrorMessage("undefined", "It is undefined!"));
      }
      if (!arg.isObject())
      {
        [[unlikely]];
        std::string stringRepresentation = arg.toString(runtime).utf8(runtime);
        throw jsi::JSError(runtime, invalidTypeErrorMessage(stringRepresentation, "It is not an object!"));
      }
#endif
      jsi::Object object = arg.getObject(runtime);
#if DEBUG
      if (!object.isHostObject<TPointee>(runtime))
      {
        [[unlikely]];
        std::string stringRepresentation = arg.toString(runtime).utf8(runtime);
        throw jsi::JSError(runtime, invalidTypeErrorMessage(stringRepresentation, "It is a different HostObject<T>!"));
      }
#endif
      return object.getHostObject<TPointee>(runtime);
    }
    static jsi::Value toJSI(jsi::Runtime &runtime, const T &arg)
    {
#if DEBUG
      if (arg == nullptr)
      {
        [[unlikely]];
        throw jsi::JSError(runtime, "Cannot convert nullptr to HostObject<" + getFriendlyTypename() + ">!");
      }
#endif
      return jsi::Object::createFromHostObject(runtime, arg);
    }
  };

} // namespace margelo
