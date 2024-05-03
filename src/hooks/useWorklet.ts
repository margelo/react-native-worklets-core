import { DependencyList, useMemo } from "react";
import type { IWorklet, IWorkletContext } from "../types";
import { worklet } from "../worklet";
import { Worklets } from "../NativeWorklets";

export function getWorkletDependencies(func: Function): DependencyList {
  if (__DEV__) {
    // In debug, perform runtime checks to ensure the given func is a safe worklet, and throw an error otherwise
    const workletFunc = worklet(func);
    return Object.values(workletFunc.__closure);
  }
  // in release, just cast and assume it's a worklet. if this crashes, the user saw it first in debug anyways.
  return Object.values((func as unknown as IWorklet<Function>).__closure);
}

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
  const ctx = context === "default" ? Worklets.defaultContext : context;

  return useMemo(
    () => ctx.createRunAsync(callback),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [...getWorkletDependencies(callback), ctx]
  );
}
