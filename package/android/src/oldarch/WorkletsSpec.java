package com.worklets;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.Promise;

abstract class WorkletsSpec extends ReactContextBaseJavaModule {
  WorkletsSpec(ReactApplicationContext context) {
    super(context);
  }

  public abstract boolean install();
}
