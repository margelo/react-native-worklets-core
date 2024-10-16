import type { TurboModule } from "react-native";
import { TurboModuleRegistry } from "react-native";
import type { IWorkletNativeApi } from "./types";
import { ModuleNotFoundError } from "./ModuleNotFoundError";

if (__DEV__) {
  console.log("Loading react-native-worklets-core...");
}

export interface Spec extends TurboModule {
  /**
   * Installs `WorkletsProxy` into this JS Runtime's `global`.
   * @returns A `string` error message if this function failed to run, or `undefined` if everything went successful.
   */
  install(): string | undefined;
}

let module: Spec;
try {
  module = TurboModuleRegistry.getEnforcing<Spec>("Worklets");
} catch (e) {
  // User didn't enable new arch, or the module does not exist.
  throw new ModuleNotFoundError(e);
}

const errorMessage = module.install();
if (errorMessage != null) {
  throw new Error(
    `Failed to install react-native-worklets-core: ${errorMessage}`
  );
}

/**
 * The Worklets API.
 * This object can be shared and accessed from multiple contexts,
 * however it is advised to not hold unnecessary references to it.
 */
// @ts-expect-error
export const Worklets = global.WorkletsProxy as IWorkletNativeApi;

if (__DEV__) {
  console.log("react-native-worklets-core loaded successfully!");
}
