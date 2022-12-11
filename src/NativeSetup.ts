import { NativeModules } from "react-native";
import { addDecorators } from "./decorators";

console.log("Loading react-native-worklets...");

if (globalThis.Worklets == undefined || globalThis.Worklets == null) {
  // Initialize Worklets API
  const WorkletsModule = NativeModules.ReactNativeWorklets;

  if (WorkletsModule == null || typeof WorkletsModule.install !== "function") {
    console.error(
      "Native Worklets Module cannot be found! Make sure you correctly " +
        "installed native dependencies and rebuilt your app."
    );
  } else {
    // Install the module
    const result = WorkletsModule.install();
    if (result !== true) {
      console.error(
        `Native Worklets Module failed to correctly install JSI Bindings! Result: ${result}`
      );
    } else {
      console.log("Worklets loaded successfully");
      addDecorators();
    }
  }
} else {
  addDecorators();
}
