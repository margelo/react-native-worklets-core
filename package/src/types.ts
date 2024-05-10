/**
 * Represents a value that can be shared between multiple contexts.
 *
 * Getting and setting the `value` is thread-safe.
 *
 * Objects and Arrays are wrapped as custom C++ Proxies instead of copied by value.
 */
export interface ISharedValue<T> {
  get value(): T;
  set value(v: T);
  addListener(listener: () => void): () => void;
}

/**
 * Represents the given function as a Worklet.
 *
 * Use the `worklet(...)` function to safely perform such casts with runtime checking.
 */
export type IWorklet<TFunc extends Function> = TFunc & {
  /**
   * Contains an object of all captured values inside this Worklet.
   *
   * Primitives will be captured by (deep-)copy, and `SharedValues`, `HostObjects`
   * or `HostFunctions` will be captured by reference.
   */
  __closure: Record<string, unknown>;
  /**
   * Contains data that will be used to initialize a Worklet natively.
   */
  __initData: {
    /**
     * Contains the full JS code of the transpiled function with proper closure unwrapping.
     */
    code: string;
    /**
     * Contains the location of the function in the JS sources.
     */
    location: string;
    /**
     * Contains a source-code map of the Worklet to properly resolve stacktraces.
     */
    sourceMap: string;
  };
  /**
   * Holds a unique compile-time hash for the code and closure of this Worklet.
   */
  __workletHash: number;
};

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
  /**
   * Creates a function that can be executed asynchronously on the Worklet context.
   *
   * The resulting function is memoized, so this is merely just a bit more efficient than {@linkcode runAsync}.
   * @worklet
   * @param worklet The worklet to run on this Context. It needs to be decorated with the `'worklet'` directive.
   * @returns A function that can be called to execute the Worklet function on this context.
   * @example
   * ```ts
   * const context = Worklets.createContext("myContext")
   * const func = context.createRunAsync((name) => `hello ${name}!`)
   * const first = await func("marc")
   * const second = await func("christian")
   * ```
   */
  createRunAsync: <TArgs extends unknown[], TReturn>(
    worklet: (...args: TArgs) => TReturn
  ) => (...args: TArgs) => Promise<TReturn>;
  /**
   * Runs the given Function asynchronously on this Worklet context.
   * @worklet
   * @param worklet The worklet to run on this Context. It needs to be decorated with the `'worklet'` directive.
   * @returns A Promise that resolves once the Worklet function has completed executing.
   * @example
   * ```ts
   * const context = Worklets.createContext("myContext")
   * const string = await context.runAsync(() => "hello!")
   * ```
   */
  runAsync: <T>(worklet: () => T) => Promise<T>;
}

export interface IWorkletNativeApi {
  /**
   * Creates a new worklet context with the given name. The name identifies the
   * name of the worklet runtime a worklet will be executed in when you call the
   * worklet.runOnWorkletThread();
   */
  createContext: (name: string) => IWorkletContext;
  /**
   * Creates a value that can be shared between runtimes.
   *
   * Arrays and Objects are wrapped in C++ Proxies instead of copied by value.
   * Array and Objects reads and writes are thread-safe.
   */
  createSharedValue: <T>(value: T) => ISharedValue<T>;

  /**
   * @deprecated This API has been deprecated, use {@linkcode IWorkletContext.createRunAsync()} instead
   */
  createRunInContextFn: never;
  /**
   * @deprecated This API has been deprecated, use {@linkcode createRunOnJS()} instead
   */
  createRunInJsFn: never;

  /**
   * Creates a function that can be executed asynchronously on the default React-JS context.
   *
   * The resulting function is memoized, so this is merely just a bit more efficient than {@linkcode runOnJS}.
   * @param func The function to run on the default React-JS context.
   * @returns A function that can be called to execute the function on the default React-JS .
   * @example
   * ```ts
   * const [user, setUser] = useState("marc")
   *
   * const context = Worklets.defaultContext
   * const callback = Worklets.createRunOnJS(setUser)
   * context.runAsync(() => {
   *   const name = "christian"
   *   callback(name)
   * })
   * ```
   */
  createRunOnJS: <TArgs extends unknown[], TReturn>(
    func: (...args: TArgs) => TReturn
  ) => (...args: TArgs) => Promise<TReturn>;
  /**
   * Runs the given Function asynchronously on the default React-JS context.
   * @param func The function to run on the default React-JS context.
   * @returns A Promise that resolves once the function has completed executing.
   * @example
   * ```ts
   * const [user, setUser] = useState("marc")
   *
   * const context = Worklets.defaultContext
   * context.runAsync(() => {
   *   const name = "christian"
   *   Worklets.runOnJS(() => setUser(name))
   * })
   * ```
   */
  runOnJS: <T>(func: () => T) => Promise<T>;

  /**
   * Returns a unique identifier for the Thread this method is called on.
   *
   * Thread-IDs start at 0 and use `thread_local` storage to store their IDs
   * which are incremented everytime a new Thread calls `getCurrentThreadId()`.
   */
  getCurrentThreadId(): number;
  /**
   * Get the default Worklet context.
   */
  defaultContext: IWorkletContext;
  /**
   * Get the current Worklet context, or `undefined` if called in main React JS context.
   */
  currentContext: IWorkletContext;
  /**
   * Returns true if jsi/cpp believes that the passed value is an array.
   */
  __jsi_is_array: <T>(value: T) => boolean;
  /**
   * Returns true if jsi/cpp believes that the passed value is an object.
   */
  __jsi_is_object: <T>(value: T) => boolean;
}
