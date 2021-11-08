import React from "react";
import { Worklets, ContextType } from "./types";

export const useWorklet = <D extends ContextType, T>(
  worklet: (ctx: D, ...args: any) => any,
  dependencies: D
) => {
  return React.useMemo(
    () => Worklets.createWorklet(dependencies, worklet),
    [dependencies, worklet]
  );
};
