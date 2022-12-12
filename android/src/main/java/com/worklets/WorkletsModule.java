package com.worklets;

import android.util.Log;

import androidx.annotation.NonNull;

import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;

import java.lang.ref.WeakReference;

public class WorkletsModule extends com.worklets.WorkletsSpec {
  public static final String NAME = "Worklets";
  private final WeakReference<ReactApplicationContext> weakReactContext;

  WorkletsModule(ReactApplicationContext context) {
    super(context);
    this.weakReactContext = new WeakReference<>(context);
  }

  @Override
  @NonNull
  public String getName() {
    return NAME;
  }

  public static native boolean nativeInstall(long ptr);

  @ReactMethod
  public boolean install() {
    try {
      System.loadLibrary("worklets");
      ReactApplicationContext context = weakReactContext.get();
      if (context == null) {
        Log.e(NAME, "React Application Context was null!");
        return false;
      }

      CallInvokerHolderImpl holder = (CallInvokerHolderImpl) context.getCatalystInstance().getJSCallInvokerHolder();
      long ptrJsContextHolder = context.getJavaScriptContextHolder().get();
      return nativeInstall(ptrJsContextHolder);
    } catch (Exception exception) {
      Log.e(NAME, "Failed to initialize react-native-worklets!", exception);
      return false;
    }
  }
}
