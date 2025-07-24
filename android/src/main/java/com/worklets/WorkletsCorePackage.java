package com.worklets;

import androidx.annotation.Nullable;

import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;
import com.facebook.react.TurboReactPackage;
import com.facebook.react.turbomodule.core.interfaces.TurboModule;

import java.util.HashMap;
import java.util.Map;

public class WorkletsCorePackage extends TurboReactPackage {

  @Nullable
  @Override
  public NativeModule getModule(String name, ReactApplicationContext reactContext) {
    if (name.equals(WorkletsModule.NAME)) {
      return new WorkletsModule(reactContext);
    } else {
      return null;
    }
  }

  @Override
  public ReactModuleInfoProvider getReactModuleInfoProvider() {
    return () -> {
      final Map<String, ReactModuleInfo> moduleInfos = new HashMap<>();
      Class<? extends NativeModule> moduleClass = WorkletsModule.class;
      ReactModule reactModule = moduleClass.getAnnotation(ReactModule.class);
      moduleInfos.put(
              reactModule.name(),
              new ReactModuleInfo(
                  reactModule.name(),
                  moduleClass.getName(),
                  true,
                  reactModule.needsEagerInit(),
                  reactModule.hasConstants(),
                  reactModule.isCxxModule(),
                  TurboModule.class.isAssignableFrom(moduleClass)));
      return moduleInfos;
    };
  }
}
