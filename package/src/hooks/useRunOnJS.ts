import { DependencyList, useMemo } from "react";
import { Worklets } from "../NativeWorklets";

/**
 * Create a Worklet function that runs the given function on the JS context.
 * The returned function can be called from a worklet to hop back to the JS thread.
 *
 * @param callback The JS code to run. This is **not** a `'worklet'`.
 * @param dependencyList The React dependencies of this Worklet.
 * @returns A memoized Worklet
 * @example
 * ```ts
 * const sayHello = useRunOnJS((name: string) => {
 *   console.log(`Hello ${name}, I am running on the JS Thread!`)
 * }, [])
 * Worklets.defaultContext.runAsync(() => {
 *   'worklet'
 *   sayHello('Marc')
 * })
 * ```
 */
export function useRunOnJS<T extends (...args: any[]) => any>(
  callback: T,
  dependencyList: DependencyList
): (...args: Parameters<T>) => Promise<ReturnType<T>> {
  const worklet = useMemo(
    () => Worklets.createRunOnJS(callback),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    dependencyList
  );

  return worklet;
}
