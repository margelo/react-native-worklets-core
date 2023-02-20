import { DependencyList, useCallback, useMemo } from "react";

/**
 * Create a Worklet function that persists between re-renders.
 * @param callback The Worklet. Must be marked with the `'worklet'` directive.
 * @param dependencyList The React dependencies of this Worklet.
 * @returns A memoized Worklet
 */
export function useWorklet<
  TResult,
  TArguments extends [],
  T extends (...args: TArguments) => TResult
>(
  callback: T,
  dependencyList: DependencyList
): (...args: TArguments) => Promise<TResult> {
  const worklet = useMemo(
    () =>
      Worklets.createRunInContextFn((...args: TArguments) => {
        "worklet";
        return callback(...args);
      }),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    dependencyList
  );

  // eslint-disable-next-line react-hooks/exhaustive-deps
  return useCallback(worklet, dependencyList);
}
