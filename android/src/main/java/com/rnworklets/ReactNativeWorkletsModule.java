// ReactNativeWorkletsModule.java

package com.rnworklets;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;

import androidx.annotation.NonNull;

public class ReactNativeWorkletsModule extends ReactContextBaseJavaModule {

    private static native void initialize(long jsiRuntimePtr, CallInvokerHolderImpl jsCallInvokerHolder);
    private static native void destruct();

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

        ReactNativeWorkletsModule.initialize(
                this.getReactApplicationContext().getJavaScriptContextHolder().get(),
                holder
        );
    }

    @Override
    public void onCatalystInstanceDestroy() {
        ReactNativeWorkletsModule.destruct();
    }
}
