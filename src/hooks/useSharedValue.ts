import { useRef } from "react";
import type { ISharedValue } from "../types";

/**
 * Create a Shared Value that persists between re-renders.
 * @param initialValue The initial value for this Shared Value
 * @returns The Shared Value instance
 */
export function useSharedValue<T>(initialValue: T): ISharedValue<T> {
  const ref = useRef<ISharedValue<T>>();
  if (ref.current == null) {
    ref.current = Worklets.createSharedValue(initialValue);
  }
  return ref.current;
}
