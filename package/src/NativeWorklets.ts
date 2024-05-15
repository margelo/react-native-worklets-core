import type { TurboModule } from "react-native";
import { TurboModuleRegistry } from "react-native";
import type { IWorkletNativeApi } from "./types";
import type { UnsafeObject } from "react-native/Libraries/Types/CodegenTypes";
import { ModuleNotFoundError } from "./ModuleNotFoundError";

if (__DEV__) {
  console.log("Loading react-native-worklets-core...");
}

export interface Spec extends TurboModule {
  /**
   * Create a new instance of the Worklets API.
   * The returned {@linkcode UnsafeObject} is a `jsi::HostObject`.
   */
  createWorkletsApi(): UnsafeObject;
}

let module: Spec
try {
  // Try to find the CxxTurboModule.
  // CxxTurboModules can be autolinked on Android starting from react-native 0.74,
  // and are manually linked in WorkletsOnLoad.mm on iOS.
  module = TurboModuleRegistry.getEnforcing<Spec>("Worklets");
} catch (e) {
  // User didn't enable new arch, or the module does not exist.
  throw new ModuleNotFoundError(e)
}

/**
 * The Worklets API.
 * This object can be shared and accessed from multiple contexts,
 * however it is advised to not hold unnecessary references to it.
 */
export const Worklets = module.createWorkletsApi() as IWorkletNativeApi


if (__DEV__) {
  console.log("react-native-worklets-core loaded successfully!");
}
