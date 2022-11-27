// ReactNativeWorkletsModule.java

package com.rnworklets;

import android.util.Log;

import com.facebook.jni.HybridData;
import com.facebook.proguard.annotations.DoNotStrip;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;

import androidx.annotation.NonNull;

import java.lang.ref.WeakReference;

public class ReactNativeWorkletsModule extends ReactContextBaseJavaModule {

    public static final String NAME = "ReactNativeWorklets";


    @DoNotStrip
    private HybridData mHybridData;
    private WeakReference<ReactApplicationContext> mWeakContext;

    static {
        System.loadLibrary("worklets");
    }

    private native HybridData initHybrid(
            long jsContext,
            CallInvokerHolderImpl jsCallInvokerHolder
    );

    public ReactNativeWorkletsModule(ReactApplicationContext reactContext) {
        super(reactContext);
        this.mWeakContext = new WeakReference(reactContext);
    }

    @Override
    public String getName() {
        return NAME;
    }

    @ReactMethod(isBlockingSynchronousMethod = true)
    public boolean install() {
            try {
                installApi();
                return true;
            } catch (Exception exception) {
                Log.e(NAME, "Failed to initialize Worklets!", exception);
                return false;
            }
    }

    @NonNull
    @Override
    public void initialize() {
        super.initialize();

        ReactApplicationContext context = mWeakContext.get();

        CallInvokerHolderImpl holder = (CallInvokerHolderImpl)context
                .getCatalystInstance().getJSCallInvokerHolder();

        mHybridData = initHybrid(context.getJavaScriptContextHolder().get(), holder);

    }

    public void destroy() {
        mHybridData.resetNative();
    }

    public native void installApi();
}
