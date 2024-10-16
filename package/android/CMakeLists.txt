project("rnworklets")
cmake_minimum_required(VERSION 3.8)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(PACKAGE_NAME "rnworklets")

# Compile sources
add_library(
        ${PACKAGE_NAME}
        SHARED
        ../cpp/base/WKTJsiHostObject.cpp
        ../cpp/base/WKTRuntimeLifecycleMonitor.cpp
        ../cpp/dispatch/WKTDispatchQueue.cpp
        ../cpp/wrappers/WKTJsiPromiseWrapper.cpp
        ../cpp/wrappers/WKTJsiWrapper.cpp
        ../cpp/WKTJsiWorkletApi.cpp
        ../cpp/WKTJsiWorkletContext.cpp
        ../cpp/entrypoint/InstallWorklets.cpp

set(PACKAGE_NAME "rnworklets")

# Pre-set Folly flags from React Native core
include("${REACT_NATIVE_DIR}/ReactAndroid/cmake-utils/folly-flags.cmake")
add_compile_options(${folly_FLAGS})

# Consume shared libraries and headers from prefabs
find_package(fbjni REQUIRED CONFIG)
find_package(ReactAndroid REQUIRED CONFIG)

if(${JS_RUNTIME} STREQUAL "hermes")
  find_package(hermes-engine REQUIRED CONFIG)
endif()

file(GLOB_RECURSE SOURCES_COMMON CONFIGURE_DEPENDS "../cpp/**.cpp")

add_library(
  ${PACKAGE_NAME}
  SHARED
  ../cpp/base/WKTJsiHostObject.cpp
  ../cpp/base/WKTRuntimeLifecycleMonitor.cpp
  ../cpp/dispatch/WKTDispatchQueue.cpp
  ../cpp/wrappers/WKTJsiPromiseWrapper.cpp
  ../cpp/wrappers/WKTJsiWrapper.cpp
  ../cpp/WKTJsiWorkletApi.cpp
  ../cpp/WKTJsiWorkletContext.cpp

  # Android-specific C++ sources
  src/main/cpp/JNIOnLoad.cpp
  src/main/cpp/JWorklets.cpp
)

# includes
target_include_directories(
        ${PACKAGE_NAME}
        PUBLIC
        ../cpp
        ../cpp/base
        ../cpp/decorators
        ../cpp/dispatch
        ../cpp/sharedvalues
        ../cpp/wrappers
        ../cpp/entrypoint/

  # Android-specific C++ includes
  src/main/cpp
)

# Add Hermes (from com.facebook.react:hermes-android)
find_package(hermes-engine REQUIRED CONFIG)

# Add React core libraries (JSI, ...)
find_package(ReactAndroid REQUIRED CONFIG)

# Add fbjni (from React core)
find_package(fbjni REQUIRED CONFIG)

find_library(LOG_LIB log)

# Link everything together
target_link_libraries(
  ${PACKAGE_NAME}
  ${LOG_LIB} # <-- Logcat logger
  hermes-engine::libhermes # <-- Hermes Prefab
  fbjni::fbjni # <-- fbjni
  ReactAndroid::jsi # <-- RN: JSI
  ReactAndroid::react_nativemodule_core # <-- RN: TurboModules Core
  ReactAndroid::turbomodulejsijni # <-- RN: TurboModules utils (e.g. CallInvokerHolder)
)

target_link_libraries(
  ${PACKAGE_NAME}
  ReactAndroid::folly_runtime
  ReactAndroid::glog
  ReactAndroid::jsi
  ReactAndroid::reactnativejni
  fbjni::fbjni
)

if(${JS_RUNTIME} STREQUAL "hermes")
  string(APPEND CMAKE_CXX_FLAGS " -DJS_RUNTIME_HERMES=1")

  # From prefab from module `com.facebook.react:hermes-android`
  set(HERMES_LIB hermes-engine::libhermes)

  target_link_libraries(
    ${PACKAGE_NAME}
    ${HERMES_LIB}
  )

  if(${HERMES_ENABLE_DEBUGGER})
    set(HERMES_EXECUTOR_LIB ReactAndroid::hermes_executor)

    target_link_libraries(
      ${PACKAGE_NAME}
      ${HERMES_EXECUTOR_LIB}
    )
  endif()
elseif(${JS_RUNTIME} STREQUAL "jsc")
  string(APPEND CMAKE_CXX_FLAGS " -DJS_RUNTIME_JSC=1")

  set(JSEXECUTOR_LIB ReactAndroid::jscexecutor)

  target_link_libraries(${PACKAGE_NAME} ${JSEXECUTOR_LIB})
else()
  message(FATAL_ERROR "Unknown JS runtime ${JS_RUNTIME}.")
endif()
