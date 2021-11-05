interface ISharedValue<T> {
  get value(): T;
  set value(v: T);
  addListener(listener: () => void): () => void;
}

interface IWorkletNativeApi {
  installWorklet: <T extends (...args: any) => any>(
    worklet: T,
    contextName?: string
  ) => void;
  createSharedValue: <T>(value: T) => ISharedValue<T>;
  runWorklet: <T extends (...args: any) => any>(
    worklet: T,
    contextName?: string
  ) => (...args: Parameters<T>) => Promise<ReturnType<T>>;
  callbackToJavascript: <T extends (...args: any) => any>(worklet: T) => T;
}

declare module Worklets {
  interface Global {
    Worklets: IWorkletNativeApi;
  }
}
