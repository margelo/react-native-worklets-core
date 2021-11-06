import { IWorkletNativeApi } from "./src/types";

declare global {
  var Worklets: IWorkletNativeApi;
}

const { Worklets } = global;
export { Worklets };
export * from "./src/types";
