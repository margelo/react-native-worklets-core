package com.margelo.worklets;

import android.util.Log;

public class JNIOnLoad {
    private static final String TAG = "Worklets";
    private static boolean isInitialized = false;

    /**
     * Initializes the C++ (JNI) part of react-native-worklets-core.
     * This function should be called in a `static` block everytime the Java part
     * interacts with the native C++ (JNI) part of react-native-worklets-core.
     * If react-native-worklets-core is already initialized, this function will do nothing.
     */
    public synchronized static void initializeNativeWorklets() {
        if (isInitialized) return;
        try {
            Log.i(TAG, "Loading react-native-worklets-core C++ library...");
            System.loadLibrary("rnworklets");
            Log.i(TAG, "Successfully loaded react-native-worklets-core C++ library!");
            isInitialized = true;
        } catch (Throwable e) {
            Log.e(TAG, "Failed to load react-native-worklets-core C++ library! Is it properly installed and linked?", e);
            throw e;
        }
    }
}
