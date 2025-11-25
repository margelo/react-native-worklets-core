project("rnworklets")
cmake_minimum_required(VERSION 3.8)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  string(APPEND CMAKE_CXX_FLAGS " -DDEBUG")
endif()

set(PACKAGE_NAME "rnworklets")


# Pre-set Folly flags from React Native core
include("${REACT_NATIVE_DIR}/ReactAndroid/cmake-utils/folly-flags.cmake")
add_compile_options(${folly_FLAGS})

# Consume shared libraries and headers from prefabs
find_package(fbjni REQUIRED CONFIG)
find_package(ReactAndroid REQUIRED CONFIG)

file(GLOB_RECURSE SOURCES_COMMON CONFIGURE_DEPENDS "../cpp/**.cpp")

add_library(
  ${PACKAGE_NAME}
  SHARED
  ${SOURCES_COMMON}
  cpp-adapter.cpp
)

# includes
target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../cpp
  ../cpp/base
  ../cpp/decorators
  ../cpp/dispatch
  ../cpp/sharedvalues
  ../cpp/wrappers
  "${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/turbomodule"
  "${REACT_NATIVE_DIR}/ReactCommon"
  "${REACT_NATIVE_DIR}/ReactCommon/callinvoker"
  "${REACT_NATIVE_DIR}/ReactCommon/runtimeexecutor"
)

# build shared lib
set_target_properties(${PACKAGE_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(
  ${PACKAGE_NAME}
  log
  android
)

target_link_libraries(
  ${PACKAGE_NAME}
  ReactAndroid::jsi
  fbjni::fbjni
)

if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 76)
  target_link_libraries(
    ${PACKAGE_NAME}
    ReactAndroid::reactnative             # <-- RN: Native Modules umbrella prefab
  )
else()
  target_link_libraries(
    ${PACKAGE_NAME}
    ReactAndroid::reactnativejni           # <-- RN: React Native JNI bindings
    ReactAndroid::folly_runtime
    ReactAndroid::glog
  )
endif()

# Add the appropriate JS runtime
if(${JS_RUNTIME} STREQUAL "hermes")
  find_package(hermes-engine REQUIRED CONFIG)

  string(APPEND CMAKE_CXX_FLAGS " -DJS_RUNTIME_HERMES=1")

  if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 82)
        target_link_libraries(
          ${PACKAGE_NAME}
          hermes-engine::hermesvm
        )
  else()
        target_link_libraries(
        ${PACKAGE_NAME}
        hermes-engine::libhermes
  )
  endif()
  

  if(${HERMES_ENABLE_DEBUGGER})
    if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 76)
      target_link_libraries(
        ${PACKAGE_NAME}
        ReactAndroid::hermestooling
      )
    else()
      target_link_libraries(
        ${PACKAGE_NAME}
        ReactAndroid::hermes_executor
      )
    endif()
  endif()
elseif(${JS_RUNTIME} STREQUAL "jsc")
  string(APPEND CMAKE_CXX_FLAGS " -DJS_RUNTIME_JSC=1")

  if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 76)
    target_link_libraries(
      ${PACKAGE_NAME}
      ReactAndroid::jsctooling
    )
  else()
    target_link_libraries(
      ${PACKAGE_NAME}
      ReactAndroid::jscexecutor
    )
  endif()
else()
  message(FATAL_ERROR "Unknown JS runtime ${JS_RUNTIME}.")
endif()
