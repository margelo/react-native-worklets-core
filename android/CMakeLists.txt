cmake_minimum_required(VERSION 3.4.1)

project("rnworklets")

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 17)

set(PACKAGE_NAME, "RNWorklets")
set(BUILD_DIR ${CMAKE_SOURCE_DIR}/build)

if(${IS_NEW_ARCHITECTURE_ENABLED})
  set (ALL_FLAGS "${DEFAULT_FLAGS} -DRCT_NEW_ARCH_ENABLED")
else()
  set (ALL_FLAGS ${DEFAULT_FLAGS})
endif()
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set (ALL_FLAGS "${ALL_FLAGS} -DDEBUG")
endif()
set (CMAKE_CXX_FLAGS ${ALL_FLAGS})

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

message("-- ABI      : " ${ANDROID_ABI})
message("-- BUILD    : " ${BUILD_DIR})
message("-- PREBUILD : " ${PREBUILT_DIR})
message("-- HERMES   : " ${FOR_HERMES})

link_directories(../libs/android/${ANDROID_ABI}/)

file(GLOB LIBRN_DIR "${PREBUILT_DIR}/${ANDROID_ABI}")
file(GLOB libfbjni_link_DIRS "${BUILD_DIR}/fbjni*.aar/jni/${ANDROID_ABI}")
file(GLOB libfbjni_include_DIRS "${BUILD_DIR}/fbjni-*-headers.jar/")

if(${FOR_HERMES})
  string(APPEND CMAKE_CXX_FLAGS " -DFOR_HERMES=1")
  if(${REACT_NATIVE_VERSION} LESS 69)
    # From `hermes-engine` npm package
    # From `hermes-engine` npm package
    target_include_directories(
            ${PACKAGE_NAME}
            PRIVATE
            "${JS_RUNTIME_DIR}/android/include"
    )
  else()
    # Bundled Hermes from module `com.facebook.react:hermes-engine` or project `:ReactAndroid:hermes-engine`
    # Bundled Hermes from module `com.facebook.react:hermes-engine` or project `:ReactAndroid:hermes-engine`
    target_include_directories(
            ${PACKAGE_NAME}
            PRIVATE
            "hermes/API"
            "hermes/public"
    )
  endif()
else()
  string(APPEND CMAKE_CXX_FLAGS " -DJS_RUNTIME_JSC=1")
  find_library(
          JSEXECUTOR_LIB
          jscexecutor
          PATHS ${LIBRN_DIR}
          NO_CMAKE_FIND_ROOT_PATH
  )
  target_link_libraries(
          ${PACKAGE_NAME}
          ${JSEXECUTOR_LIB}
  )
endif()

if(${REACT_NATIVE_VERSION} LESS 66)
  file(
          TO_CMAKE_PATH
          "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp"
          INCLUDE_JSI_CPP
  )
endif()

add_library(
        ${PACKAGE_NAME}
          SHARED
          cpp-adapter.cpp
          ../cpp/JsiWorkletApi.cpp
          ../cpp/JsiWorkletContext.cpp
          ../cpp/base/JsiHostObject.cpp
          ../cpp/dispatch/DispatchQueue.cpp
          ../cpp/wrappers/JsiWrapper.cpp
)

target_include_directories(
        ${PACKAGE_NAME}
        PRIVATE
        ../cpp
        ../cpp/base
        ../cpp/decorators
        ../cpp/dispatch
        ../cpp/sharedvalues
        ../cpp/wrappers
        "${NODE_MODULES_DIR}/react-native/React"
        "${NODE_MODULES_DIR}/react-native/React/Base"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni"
        "${NODE_MODULES_DIR}/react-native/ReactCommon"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/hermes"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/turbomodule/core"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/react/nativemodule/core"
        "${NODE_MODULES_DIR}/hermes-engine/android/include"
        ${libfbjni_include_DIRS}
)

find_library(
        LOG_LIB
        log
)

message("-- LOG      : " ${LOG_LIB})

if(${REACT_NATIVE_VERSION} LESS 66)
  # JSI lib didn't exist on RN 0.65 and before. Simply omit it.
  set (JSI_LIB "")
else()
  # RN 0.66 distributes libjsi.so, can be used instead of compiling jsi.cpp manually.
  find_library(
          JSI_LIB
          jsi
          PATHS ${LIBRN_DIR}
          NO_CMAKE_FIND_ROOT_PATH
  )
endif()

message("-- JSI      : " ${JSI_LIB})

find_library(
        FBJNI_LIBRARY
        fbjni
        PATHS ${libfbjni_link_DIRS}
        NO_CMAKE_FIND_ROOT_PATH
)

message("-- FBJNI    : " ${FBJNI_LIBRARY})

find_library(
        REACT_LIB
        react_nativemodule_core
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

message("-- REACT    : " ${REACT_LIB})

target_link_libraries(
        ${PACKAGE_NAME}
        ${LOG_LIB}
        ${JSI_LIB}
        ${FBJNI_LIBRARY}
        ${REACT_LIB}
        -landroid
)
