import React, {useCallback, useMemo, useState} from 'react';
import {Button, ScrollView, StyleSheet, Text, View} from 'react-native';

import {Tests} from './Tests';
import {TestState, TestWrapper} from './Tests/TestWrapper';
import {Test} from './Tests/types';

type TestInfo = {
  run: Test;
  name: string;
  category: string;
  state: TestState;
};

const App = () => {
  const [output, setOutput] = useState<string[]>([]);
  const [running, setRunning] = useState(false);
  const [tests, setTests] = useState<TestInfo[]>(() =>
    Object.keys(Tests).reduce(
      (acc, category) =>
        acc.concat(
          Object.keys(Tests[category]).map(testName => ({
            run: Tests[category][testName],
            name: testName,
            state: 'notrun',
            category: category,
          })),
        ),
      [] as TestInfo[],
    ),
  );

  const categories = useMemo(() => {
    const retVal = new Set<string>();
    tests.forEach(test => retVal.add(test.category));
    return Array.from(retVal);
  }, [tests]);

  const runTest = useCallback(async (test: TestInfo) => {
    try {
      setOutput(p => addOutputStateLine(test.name + ' running...', p));
      setTests(p => updateTestState(test, 'running', p));
      await test.run();
      setTests(p => updateTestState(test, 'success', p));
      setOutput(p => removeOutputStateLine(p));
    } catch (err: any) {
      setTests(p => updateTestState(test, 'failure', p));
      setOutput(p => updateOutputStateLastLine(test.name + ' failed.', p));
      setOutput(p => addOutputStateLine('    ' + err.message, p));
    }
  }, []);

  const rerunTest = useCallback(
    async (test: TestInfo) => {
      setRunning(true);
      setOutput(['Starting...']);
      await runTest(test);
      setRunning(false);
      setOutput(p => addOutputStateLine('Done.', p));
    },
    [runTest],
  );

  const runTests = useCallback(async () => {
    setRunning(true);
    setTests(p => p.map(t => ({...t, state: 'notrun'})));
    setOutput(['Starting...']);
    for (let i = 0; i < tests.length; i++) {
      await runTest(tests[i]);
    }
    setRunning(false);
    setOutput(p => addOutputStateLine('Done.', p));
  }, [runTest, tests]);

  return (
    <View style={styles.container}>
      <View style={styles.tests}>
        <ScrollView
          contentInsetAdjustmentBehavior="automatic"
          contentContainerStyle={styles.testsScrollView}>
          {categories.map(category => (
            <React.Fragment key={category}>
              <Text style={styles.category}>{category}</Text>
              {tests
                .filter(t => t.category === category)
                .map(t => (
                  <TestWrapper
                    key={t.name}
                    name={t.name}
                    state={t.state}
                    onRerun={() => rerunTest(t)}
                  />
                ))}
            </React.Fragment>
          ))}
        </ScrollView>
      </View>
      <View style={styles.output}>
        <ScrollView>
          <Text style={styles.outputText}>{output.join('\n')}</Text>
        </ScrollView>
      </View>
      <View style={styles.buttons}>
        <Button
          title={running ? 'Running...' : 'Run All Tests'}
          disabled={running}
          onPress={runTests}
        />
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  tests: {
    paddingTop: 34,
    flex: 1,
  },
  testsScrollView: {
    paddingTop: 24,
    paddingBottom: 40,
  },
  output: {
    flex: 0.2,
    backgroundColor: '#EFEFEF',
  },
  outputText: {
    padding: 8,
  },
  buttons: {
    height: 100,
    backgroundColor: '#CCC',
    paddingBottom: 36,
    paddingTop: 14,
  },
  category: {
    fontWeight: 'bold',
    paddingTop: 8,
    paddingBottom: 4,
    paddingHorizontal: 8,
  },
});

const updateTestState = (
  test: TestInfo,
  newState: TestState,
  tests: TestInfo[],
): TestInfo[] => {
  return tests.map(t => ({
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

export default App;
