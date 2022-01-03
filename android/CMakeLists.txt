
cmake_minimum_required(VERSION 3.4.1)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFOLLY_NO_CONFIG=1 -DFOLLY_HAVE_CLOCK_GETTIME=1 -DFOLLY_HAVE_MEMRCHR=1 -DFOLLY_USE_LIBCPP=1 -DFOLLY_MOBILE=1 -DON_ANDROID -DONANDROID -DFOR_HERMES=${FOR_HERMES}")

set (NODE_MODULES_DIR "../node_modules")

set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
file (GLOB LIBFBJNI_INCLUDE_DIR "${BUILD_DIR}/fbjni-*-headers.jar/")
file (GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")

add_library(worklets
        SHARED
        cpp/jsi/JsiHostObject.cpp
        cpp/wrappers/JsiWrapper.cpp
        cpp/worklets/JsiWorkletContext.cpp
        cpp/worklets/DispatchQueue.cpp
        cpp/worklets/JsiWorkletApi.cpp
        jni/jni-adapter.cpp
        jni/JniReactNativeWorkletsModule.cpp
)

target_include_directories(
        worklets
        PRIVATE
        cpp
        cpp/worklets
        cpp/jsi
        jni
        "${LIBFBJNI_INCLUDE_DIR}"
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
        LOG_LIB
        log
)

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


find_library(
        REACT_NATIVE_JNI_LIB
        reactnativejni
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

message(WARNING "CMakeLists.txt FOR_HERMES=${FOR_HERMES}")

target_link_libraries(
        worklets
        ${LOG_LIB}
        ${JSI_LIB}
        ${REACT_NATIVE_JNI_LIB}
        ${FBJNI_LIB}
        ${REACT_LIB}
        ${JS_ENGINE_LIB} # <-- Hermes or JSC
        ${TURBOMODULES_LIB}
        -landroid
)