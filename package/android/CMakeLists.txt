cmake_minimum_required(VERSION 3.9.0)
project(ReactNativeWorklets)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(PACKAGE_NAME "react-native-worklets-core")

# Compile sources
add_library(
        ${PACKAGE_NAME}
        SHARED
        ../cpp/base/WKTJsiHostObject.cpp
        ../cpp/base/WKTRuntimeLifecycleMonitor.cpp
        ../cpp/dispatch/WKTDispatchQueue.cpp
        ../cpp/wrappers/WKTJsiPromiseWrapper.cpp
        ../cpp/wrappers/WKTJsiWrapper.cpp
        ../cpp/WKTJsiWorklet.cpp
        ../cpp/WKTJsiWorkletApi.cpp
        ../cpp/WKTJsiWorkletContext.cpp
)

# Add headers search paths
target_include_directories(${PACKAGE_NAME} PUBLIC ../cpp)

# Add Hermes (from com.facebook.react:hermes-android)
find_package(hermes-engine REQUIRED CONFIG)

# Link everything together
target_link_libraries(
        ${PACKAGE_NAME}
        ReactAndroid::jsi               # <-- JSI core
        react_codegen_RNWorkletsSpec    # <-- Generated Specs from CodeGen
        hermes-engine::libhermes        # <-- Hermes Prefab
)

# Add Hermes Debugger if enabled in Gradle CMake options
if(${HERMES_ENABLE_DEBUGGER})
  target_link_libraries(
    ${PACKAGE_NAME}
    ReactAndroid::hermes_executor
  )
endif()
