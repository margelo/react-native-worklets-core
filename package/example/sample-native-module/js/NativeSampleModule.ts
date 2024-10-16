import { type TurboModule, TurboModuleRegistry } from "react-native";
import type { UnsafeObject } from "react-native/Libraries/Types/CodegenTypes";

export interface Spec extends TurboModule {
  createCustomWorkletContext(): UnsafeObject;
}

export default TurboModuleRegistry.get<Spec>("NativeSampleModule");
