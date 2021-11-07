// ReactNativeWorkletsModule.java

package com.rnworklets;

import com.facebook.jni.HybridData;
import com.facebook.proguard.annotations.DoNotStrip;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;

import androidx.annotation.NonNull;

public class ReactNativeWorkletsModule extends ReactContextBaseJavaModule {

    @DoNotStrip
    private HybridData mHybridData;

    private native HybridData initHybrid(
            long jsContext,
            CallInvokerHolderImpl jsCallInvokerHolder
    );

    static {
        System.loadLibrary("worklets");
    }

    private final ReactApplicationContext reactContext;

    public ReactNativeWorkletsModule(ReactApplicationContext reactContext) {
        super(reactContext);
        this.reactContext = reactContext;
    }

    @Override
    public String getName() {
        return "ReactNativeWorklets";
    }

    @NonNull
    @Override
    public void initialize() {
        super.initialize();

        CallInvokerHolderImpl holder = (CallInvokerHolderImpl)reactContext.getCatalystInstance().getJSCallInvokerHolder();

        mHybridData = initHybrid(reactContext.getJavaScriptContextHolder().get(), holder);

    }

    public void destroy() {
        mHybridData.resetNative();
    }
}
