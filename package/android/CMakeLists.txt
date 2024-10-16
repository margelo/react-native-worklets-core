cmake_minimum_required(VERSION 3.9.0)
project(ReactNativeWorklets)

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

        # Android-specific C++ sources
        src/main/cpp/JNIOnLoad.cpp
        src/main/cpp/JWorklets.cpp
)

# Add headers search paths
target_include_directories(
        ${PACKAGE_NAME}
        PUBLIC
        ../cpp
        ../cpp/base
        ../cpp/decorators
        ../cpp/dispatch
        ../cpp/sharedvalues
        ../cpp/wrappers

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
