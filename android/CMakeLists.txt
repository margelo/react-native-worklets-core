cmake_minimum_required(VERSION 3.4.1)

project("worklets")

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 17)

file(GLOB LIBRN_DIR "${PREBUILT_DIR}/${ANDROID_ABI}")

message("-- ABI     : " ${ANDROID_ABI})
message("-- BUILD   : " ${build_DIR})
message("-- PREBLD  : " ${PREBUILT_DIR})
message("-- HERMES  : " ${FOR_HERMES})

link_directories(../libs/android/${ANDROID_ABI}/)

if(${REACT_NATIVE_VERSION} LESS 66)
  file(
          TO_CMAKE_PATH
          "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp"
          INCLUDE_JSI_CPP
  )
endif()

add_library(worklets
            SHARED
            cpp-adapter.cpp
            ../cpp/base/JsiHostObject.cpp
            ../cpp/dispatch/DispatchQueue.cpp
            ../cpp/wrappers/JsiWrapper.cpp
)

target_include_directories(worklets
        PRIVATE
        ../cpp
        ../cpp/base
        ../cpp/dispatch
        ../cpp/decorators
        ../cpp/sharedvalues
        ../cpp/wrappers
        "${NODE_MODULES_DIR}/react-native/React"
        "${NODE_MODULES_DIR}/react-native/React/Base"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni"
        "${NODE_MODULES_DIR}/react-native/ReactCommon"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/turbomodule/core"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/react/nativemodule/core"
        "${NODE_MODULES_DIR}/hermes-engine/android/include"
)


find_library(
        LOG_LIB
        log
)

message("-- LOG     : " ${LOG_LIB})

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

message("-- JSI     : " ${JSI_LIB})

find_library(
        FBJNI_LIBRARY
        fbjni
        PATHS ${libfbjni_link_DIRS}
        NO_CMAKE_FIND_ROOT_PATH
)

message("-- FBJNI   : " ${FBJNI_LIBRARY})

target_link_libraries(
        worklets
        ${LOG_LIB}
        ${JSI_LIB}
        -landroid
)
