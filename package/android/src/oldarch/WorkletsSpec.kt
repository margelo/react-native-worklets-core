package com.margelo.worklets

import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.Promise

abstract class WorkletsSpec internal constructor(context: ReactApplicationContext) : ReactContextBaseJavaModule(context) {
  abstract fun install(): String?
}
