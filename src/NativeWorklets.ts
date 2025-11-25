import type { TurboModule } from "react-native";
import { TurboModuleRegistry } from "react-native";
import type { IWorkletNativeApi } from "./types";

export interface Spec extends TurboModule {
  install(): boolean;
}

const WorkletsInstaller = TurboModuleRegistry.getEnforcing<Spec>("Worklets");

console.log("Loading react-native-worklets-core...");

// @ts-expect-error it's an untyped JSI global.
if (global.Worklets == null) {
  if (
    WorkletsInstaller == null ||
    typeof WorkletsInstaller.install !== "function"
  ) {
    console.error(
      "Native Worklets Module cannot be found! Make sure you correctly " +
        "installed native dependencies and rebuilt your app."
    );
  } else {
    // Install the module
    const result = WorkletsInstaller.install();
    if (result !== true) {
      console.error(
        `Native Worklets Module failed to correctly install JSI Bindings! Result: ${result}`
      );
    } else {
      console.log("Worklets loaded successfully");
    }
  }
} else {
  console.log("react-native-worklets-core installed.");
}

// @ts-expect-error It's a global injected by JSI.
export const Worklets = global.Worklets as IWorkletNativeApi;
