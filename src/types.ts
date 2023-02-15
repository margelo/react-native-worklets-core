export interface ISharedValue<T> {
  get value(): T;
  set value(v: T);
  addListener(listener: () => void): () => void;
}

export interface IWorklet {
  /**
   * Returns the generated code for the worklet function.
   */
  readonly code: string;
  /**
   * Returns true for worklets.
   */
  readonly isWorklet: true;
}

/*
  Defines the interface for a worklet context. A worklet context is a javascript
  runtime that can be run in its own thread.
*/
export interface IWorkletContext {
  readonly name: string;
  /**
   * Adds an object to the worklet context. The object will be available in all worklets
   * on the global object by referencing to the propertyName
   * @param propertyName
   * @param propertyObject
   */
  addDecorator: <T>(propertyName: string, propertyObject: T) => void;
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
    | IWorklet;
};

export interface IWorkletNativeApi {
  /**
   * Creates a new worklet context with the given name. The name identifies the
   * name of the worklet runtime a worklet will be executed in when you call the
   * worklet.runOnWorkletThread();
   */
  createContext: (name: string) => IWorkletContext;
  /**
   * Creates a value that can be shared between runtimes
   */
  createSharedValue: <T>(value: T) => ISharedValue<T>;
  /**
   * Creates a function that will be executed in the worklet context. The function
   * will return a promise that will be resolved when the function has been
   * executed on the worklet thread.
   *
   * Used to create a function to call from the JS thread to the worklet thread.
   * @param worklet Decorated function that will be called in the context
   * @param context Context to call function in, or default context if not set.
   * @returns A function that will be called in the worklet context
   */
  createRunInContextFn: <C extends ContextType, T, A extends Array<unknown>>(
    fn: (this: C, ...args: A) => T,
    context?: IWorkletContext
  ) => (...args: A) => Promise<T>;
  /**
   * Creates a function that will be executed in the javascript context.
   *
   * Used to create a function to call back to the JS context from a worklet context.
   * @param worklet Decorated function that will be called in the JS context
   * @returns A function that will be called in the JS context
   */
  createRunInJsFn: <C extends ContextType, T, A extends Array<unknown>>(
    fn: (this: C, ...args: A) => T
  ) => (...args: A) => Promise<T>;

  /**
   * Get the default Worklet context.
   */
  defaultContext: IWorkletContext;
  /**
   * Get the current Worklet context, or `undefined` if called in main React JS context.
   */
  currentContext: IWorkletContext;
}
declare global {
  var Worklets: IWorkletNativeApi;
}

export const { Worklets } = globalThis;
