import { DependencyList, useMemo } from "react";

/**
 * Create a Worklet function that runs the given function on the JS context.
 * The returned function can be called from a worklet to hop back to the JS thread.
 *
 * @param callback The Worklet. Must be marked with the `'worklet'` directive.
 * @param dependencyList The React dependencies of this Worklet.
 * @returns A memoized Worklet
 */
export function useRunInJS<T extends (...args: any[]) => any>(
  callback: T,
  dependencyList: DependencyList
): (...args: Parameters<T>) => Promise<ReturnType<T>> {
  const worklet = useMemo(
    () => Worklets.createRunInJsFn(callback),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    dependencyList
  );

  return worklet;
}
