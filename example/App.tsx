import React, {useCallback, useState} from 'react';
import {Button, ScrollView, StyleSheet, Text, View} from 'react-native';

import {Tests} from './Tests';
import {TestState, TestWrapper} from './Tests/TestWrapper';
import {Test} from './Tests/types';

const App = () => {
  const [output, setOutput] = useState<string[]>([]);
  const [running, setRunning] = useState(false);
  const [tests, setTests] = useState<TestInfo[]>(() =>
    Object.keys(Tests).map(t => ({
      run: Tests[t],
      name: t,
      state: 'notrun',
    })),
  );

  const runTest = useCallback(async (test: TestInfo) => {
    try {
      setOutput(p => addOutputStateLine(test.name + ' running...', p));
      setTests(p => updateTestState(test, 'running', p));
      await test.run();
      setTests(p => updateTestState(test, 'success', p));
      setOutput(p => updateOutputStateLastLine(test.name + ' succeeded.', p));
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
      <ScrollView
        contentInsetAdjustmentBehavior="automatic"
        contentContainerStyle={styles.testsContainer}>
        {tests.map(t => (
          <TestWrapper
            key={t.name}
            name={t.name}
            state={t.state}
            onRerun={() => rerunTest(t)}
          />
        ))}
      </ScrollView>
      <ScrollView contentContainerStyle={styles.output}>
        <Text style={styles.outputText}>{output.join('\n')}</Text>
      </ScrollView>
      <View style={styles.buttons}>
        <Button title="Run" disabled={running} onPress={runTests} />
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  testsContainer: {flex: 5},
  output: {
    flex: 1,
    padding: 8,
    backgroundColor: '#EFEFEF',
  },
  outputText: {flex: 1},
  buttons: {
    backgroundColor: '#CCC',
    paddingBottom: 36,
    paddingTop: 14,
  },
});

type TestInfo = {
  run: Test;
  name: string;
  state: TestState;
};

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

export default App;
