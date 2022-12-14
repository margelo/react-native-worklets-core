import { useCallback, useEffect, useMemo, useState } from "react";
import type { TestInfo, TestState } from "./types";
import { useTests } from "./useTests";

/**
 * Creates a simple test runner for running tests on the device
 * @returns Test runner
 */
export const useTestRunner = () => {
  // Current test runner state
  const [activeTests, setActiveTests] = useState<{
    tests: TestInfo[];
    index: number;
    output: string[];
  }>();

  const { tests, categories } = useTests();

  /**
   * Runs a set of tests from start to end
   */
  const runTests = (testsToRun: TestInfo[]) => {
    // Setting state will trigger a rerender which in turn will start the first test
    setActiveTests({
      tests: testsToRun.map((t) => ({ ...t, state: "notrun" })),
      index: 0,
      output: ["Starting test runner..."],
    });
  };

  const runAllTests = useCallback(() => runTests(tests), [tests]);
  const runTest = useCallback((test: TestInfo) => {
    console.log("Running test: " + test.name);
    // Set the test state to running
    setActiveTests((p) => ({
      ...(p ?? {
        tests: [test],
        index: 0,
        output: [],
      }),
      tests: updateTestState(test, "running", p?.tests ?? [test]),
      output: addOutputStateLine(test.name + " running...", p?.output ?? []),
    }));

    // Start the test!
    setTimeout(
      () =>
        test
          .run()
          .then(() => {
            console.log("Test: " + test.name + " succeeded.");
            setTimeout(
              () =>
                setActiveTests((p) => ({
                  ...p!,
                  tests: updateTestState(test, "success", p!.tests),
                  output: removeOutputStateLine(p!.output),
                })),
              100
            );
          })
          .catch((err) => {
            console.log("Test: " + test.name + " failed.");
            setTimeout(
              () =>
                setActiveTests((p) => ({
                  ...p!,
                  tests: updateTestState(test, "failure", p!.tests),
                  output: addOutputStateLine(
                    "    " + (typeof err === "string" ? err : err.message),
                    updateOutputStateLastLine(test.name + " failed.", p!.output)
                  ),
                })),
              100
            );
          })
          .finally(() => {
            setTimeout(
              () =>
                setActiveTests((p) => {
                  if (p === undefined) {
                    return undefined;
                  }
                  // Finally we'll trigger a state update where we go to the next test.
                  if (p!.index !== p!.tests.length - 1) {
                    // Let's schedule the next test
                    return {
                      ...p,
                      index: p.index + 1,
                    };
                  } else {
                    return {
                      ...p,
                      index: -1,
                      output: addOutputStateLine(
                        "Done running tests",
                        p.output
                      ),
                    };
                  }
                }),
              100
            );
          }),
      100
    );
  }, []);

  // Test runner
  useEffect(() => {
    if (activeTests === undefined) {
      return;
    }

    if (
      activeTests &&
      activeTests.index > -1 &&
      activeTests.tests[activeTests.index]?.state === "notrun"
    ) {
      // Start the test
      console.log("Starting test with index", activeTests.index + 1);
      runTest(activeTests.tests[activeTests.index]!);
    }
  }, [activeTests, runTest]);

  return useMemo(
    () => ({
      runTests,
      runSingleTest: (test: TestInfo) => runTests([test]),
      runAllTests,
      isRunning: activeTests !== undefined && activeTests.index !== -1,
      tests: tests.map(
        (t) =>
          activeTests?.tests.find(
            (t2) => t2.name === t.name && t2.category === t.category
          ) ?? t
      ),
      categories,
      output: activeTests?.output ?? [],
    }),
    [activeTests, categories, runAllTests, tests]
  );
};

const updateTestState = (
  test: TestInfo,
  newState: TestState,
  tests: TestInfo[]
): TestInfo[] => {
  return tests.map((t) => ({
    ...t,
    state: t.name === test.name ? newState : t.state,
  }));
};

const updateOutputStateLastLine = (message: string, output: string[]) => {
  return [...output.slice(0, output.length - 1), message];
};

const addOutputStateLine = (message: string, output: string[]) => {
  return output.concat(message);
};

const removeOutputStateLine = (output: string[]) => {
  return output.slice(0, output.length - 1);
};
