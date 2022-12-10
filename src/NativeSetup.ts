import { NativeModules } from "react-native";

console.log("Loading Worklets...");

if (globalThis.Worklets == undefined || globalThis.Worklets == null) {
  // Initialize Worklets API
  const WorkletsModule = NativeModules.ReactNativeWorklets;

  if (WorkletsModule == null || typeof WorkletsModule.install !== "function") {
    console.error(
      "Native Worklets Module cannot be found! Make sure you correctly " +
        "installed native dependencies and rebuilt your app."
    );
  } else {
    const result = WorkletsModule.install();
    if (result !== true) {
      console.error(
        `Native Worklets Module failed to correctly install JSI Bindings! Result: ${result}`
      );
    } else {
      console.log("Worklets loaded successfully");
    }
  }
}
