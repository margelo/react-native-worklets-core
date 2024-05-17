package com.margelo.worklets;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;
import com.facebook.react.TurboReactPackage;

import java.util.HashMap;
import java.util.Map;

// A dummy Java package just so that RN CLI finds react-native-worklets-core and autolinks it.
// The actual module is a C++ CxxTurboModule.
public class WorkletsPackage extends TurboReactPackage {
  @Nullable
  @Override
  public NativeModule getModule(@NonNull String name, @NonNull ReactApplicationContext reactContext) {
    // dummy
    return null;
  }

  @Override
  public ReactModuleInfoProvider getReactModuleInfoProvider() {
    return () -> {
      // dummy
      final Map<String, ReactModuleInfo> moduleInfos = new HashMap<>();
      return moduleInfos;
    };
  }
}
