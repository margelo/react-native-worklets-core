
cmake_minimum_required(VERSION 3.4.1)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 17)

set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
file (GLOB LIBFBJNI_INCLUDE_DIR "${BUILD_DIR}/fbjni-*-headers.jar/")
file (GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")

add_library(worklets
        SHARED
        cpp/worklets/JsiWorkletContext.cpp
        cpp/worklets/DispatchQueue.cpp
        cpp/worklets/JsiWrapper.cpp
        cpp/worklets/JsiArrayWrapper.cpp
        cpp/worklets/JsiWorkletApi.cpp
        jni/jni-adapter.cpp
)

target_include_directories(
        worklets
        PRIVATE
        cpp
        cpp/worklets
        cpp/jsi
        jni
        "${LIBFBJNI_INCLUDE_DIR}"
        ../node_modules/react-native/React
        ../node_modules/react-native/React/Base
        ../node_modules/react-native/ReactAndroid/src/main/jni
        ../node_modules/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni
        ../node_modules/react-native/ReactCommon
        ../node_modules/react-native/ReactCommon/jscallinvoker
        ../node_modules/react-native/ReactCommon/callinvoker
        ../node_modules/react-native/ReactCommon/jsi
        ../node_modules/react-native/ReactCommon/turbomodule/core
        ../node_modules/react-native/ReactCommon/react/nativemodule/core
)

if(${FOR_HERMES})
    file (GLOB LIBHERMES_DIR "${BUILD_DIR}/third-party-ndk/hermes/jni/${ANDROID_ABI}")
    # Use Hermes
    find_library(
            JS_ENGINE_LIB
            hermes
            PATHS ${LIBHERMES_DIR}
            NO_CMAKE_FIND_ROOT_PATH
    )
else()
    file (GLOB LIBJSC_DIR "${BUILD_DIR}/android-jsc*.aar/jni/${ANDROID_ABI}")
    # Use JSC
    find_library(
            JS_ENGINE_LIB
            jscexecutor
            PATHS ${LIBRN_DIR}
            NO_CMAKE_FIND_ROOT_PATH
    )
endif()

find_library(
        REACT_LIB
        react_nativemodule_core
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

find_library(
        JSI_LIB
        jsi
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

find_library(
        TURBOMODULES_LIB
        turbomodulejsijni
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)


find_library(
        FBJNI_LIB
        fbjni
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)


target_link_libraries(
        worklets 
        ${REACT_LIB}
        ${JS_ENGINE_LIB}
        ${JSI_LIB}
        ${FBJNI_LIB}
        ${TURBOMODULES_LIB}
        -landroid
)