import { useMemo } from "react";
import type { IWorkletContext } from "../types";
import { Worklets } from "../NativeWorklets";
import { getWorkletDependencies } from "../worklet";

/**
 * Create a Worklet function that automatically memoizes itself using it's auto-captured closure.
 * The returned function can be called from both a Worklet context and the JS context, but will execute on a Worklet context.
 *
 * @worklet
 * @param context The context to run this Worklet in. Can be `default` to use the default background context, or a custom context.
 * @param callback The Worklet. Must be marked with the `'worklet'` directive.
 * @returns A memoized Worklet
 * ```ts
 * const sayHello = useWorklet('default', (name: string) => {
 *   'worklet'
 *   console.log(`Hello ${name}, I am running on the Worklet Thread!`)
 * })
 * sayHello()
 * ```
 */
export function useWorklet<T extends (...args: any[]) => any>(
  context: IWorkletContext | "default",
  callback: T
): (...args: Parameters<T>) => Promise<ReturnType<T>> {
  // Since `Worklets.defaultContext` always constructs a new jsi::Object,
  // we cannot use it in React's dependency-list as that'd trigger a new dependency each render.
  // Instead, we compare only it's name as a unique ID to re-trigger useMemo only when the name changes.
  const contextName = context === "default" ? context : context.name;

  // As a dependency for this use-memo we use all of the values captured inside the worklet,
  // as well as the unique context name.
  const dependencies = [...getWorkletDependencies(callback), contextName];

  return useMemo(
    () => {
      const actualContext =
        context === "default" ? Worklets.defaultContext : context;
      return actualContext.createRunAsync(callback);
    },
    // eslint-disable-next-line react-hooks/exhaustive-deps
    dependencies
  );
}
