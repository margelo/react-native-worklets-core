export interface ISharedValue<T> {
  get value(): T;
  set value(v: T);
  addListener(listener: () => void): () => void;
}

export type ContextType = {
  [key: string]:
    | number
    | string
    | boolean
    | undefined
    | null
    | ISharedValue<any>
    | ContextType
    | ContextType[]
    | number[]
    | string[]
    | boolean[];
};

export interface IWorkletNativeApi {
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
  createWorklet: <C extends ContextType, T, A extends any[]>(
    context: C,
    worklet: (context: C, ...args: A) => T
  ) => (...args: A) => Promise<T>;
}
