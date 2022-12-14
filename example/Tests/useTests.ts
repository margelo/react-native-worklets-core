import { useMemo } from "react";
import { Tests } from "./tests";
import type { TestInfo } from "./types";

export const useTests = () => {
  // All tests flattened
  const tests = useMemo(
    () =>
      Object.keys(Tests).reduce(
        (acc, category) =>
          acc.concat(
            Object.keys(Tests[category]!).map((testName) => ({
              run: Tests[category]![testName]!,
              name: testName,
              state: "notrun",
              category: category,
            }))
          ),
        [] as TestInfo[]
      ),
    []
  );

  const categories = useMemo(() => {
    const retVal = new Set<string>();
    tests.forEach((test) => retVal.add(test.category));
    return Array.from(retVal);
  }, [tests]);

  return useMemo(
    () => ({
      tests,
      categories,
    }),
    [categories, tests]
  );
};
