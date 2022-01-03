export interface ISharedValue<T> {
  get value(): T;
  set value(v: T);
  addListener(listener: () => void): () => void;
}

export interface IWorklet<A extends any[], T extends (...args: A) => any> {
  /**
   * Calls the worklet on the worklet thread.
   */
  callAsync: OmitThisParameter<(...args: A) => Promise<ReturnType<T>>>;
  /**
   * Calls the worklet on the same thread as the caller
   */
  call: OmitThisParameter<(...args: A) => ReturnType<T>>;
  /**
   * Returns the context of the worklet
   */
  readonly context: IWorkletContext;
  /**
   * Returns true for worklets.
   */
  readonly isWorklet: true;
}

export interface IWorkletContext {
  readonly name: string;
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
    | boolean[]
    | IWorklet<any, any>;
};

export interface IWorkletNativeApi {
  /**
   * Creates a new worklet context with the given name. The name identifies the
   * name of the worklet runtime a worklet will be executed in when you call the
   * worklet.runOnWorkletThread();
   */
  createWorkletContext: (name: string) => IWorkletContext;
  /**
   * Creates a value that can be shared between runtimes
   */
  createSharedValue: <T>(value: T) => ISharedValue<T>;
  /**
   * Creates a worklet that can be executed on either then main thread or on
   * the worklet thread in the context given by the context name (or empty to run
   * in the default context)
   * @param worklet Function to use to create the worklet
   * @param closure Values that will be copied to the worklet closure
   * @param context Worklet context to run the worklet in. Optional.
   * @param returns an @see(IWorklet) object
   */
  createWorklet: <C extends ContextType, T, A extends any[]>(
    worklet: (this: C, ...args: A) => T,
    closure?: C,
    context?: IWorkletContext
  ) => IWorklet<A, (...args: A) => T>;
}
declare global {
  var Worklets: IWorkletNativeApi;
}

export const { Worklets } = global;
