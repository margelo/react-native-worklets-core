import { DependencyList, useMemo } from "react";

/**
 * Create a Worklet function that runs the given function on the JS context.
 * The returned function can be called from a worklet to hop back to the JS thread.
 *
 * @param callback The Worklet. Must be marked with the `'worklet'` directive.
 * @param dependencyList The React dependencies of this Worklet.
 * @returns A memoized Worklet
 */
export function useRunInJS<
  TResult,
  TArguments extends any[],
  T extends (...args: TArguments) => TResult
>(
  callback: T,
  dependencyList: DependencyList
): (...args: TArguments) => Promise<TResult> {
  const worklet = useMemo(
    () => Worklets.createRunInJsFn(callback),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    dependencyList
  );

  return worklet;
}
