import { useRef } from "react";
import type { ISharedValue } from "../types";
import { Worklets } from "../NativeWorklets";

/**
 * Create a Shared Value that persists between re-renders.
 * @param initialValue The initial value for this Shared Value
 * @returns The Shared Value instance
 * @example
 * ```ts
 * const counter = useSharedValue(42)
 * Worklets.defaultContext.runAsync(() => {
 *   'worklet'
 *   counter.value = 73
 * })
 * ```
 */
export function useSharedValue<T>(initialValue: T): ISharedValue<T> {
  const ref = useRef<ISharedValue<T>>();
  if (ref.current == null) {
    ref.current = Worklets.createSharedValue(initialValue);
  }
  return ref.current;
}
