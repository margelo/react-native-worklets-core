import React, {useCallback, useMemo, useState} from 'react';
import {Button, ScrollView, StyleSheet, Text, View} from 'react-native';

import * as Tests from './Tests';
import {TestState, TestWrapper} from './Tests/TestWrapper';

const App = () => {
  const [output, setOutput] = useState<string[]>([]);
  const tests = useMemo((): {
    run: () => Promise<void>;
    name: string;
    state: TestState;
  }[] => {
    return Object.keys(Tests).map(t => ({
      // @ts-ignore
      run: Tests[t],
      name: t,
      state: 'notrun',
    }));
  }, []);

  const [running, setRunning] = useState(false);
  const runTests = useCallback(async () => {
    setRunning(true);
    setOutput(['Starting...']);
    for (let i = 0; i < tests.length; i++) {
      try {
        setOutput(p => p.concat(tests[i].name + ' running...'));
        await tests[i].run();
        tests[i].state = 'success';
        setOutput(p => [
          ...p.slice(0, p.length - 1),
          tests[i].name + ' succeeded.',
        ]);
      } catch (err: any) {
        tests[i].state = 'failure';
        setOutput(p => [
          ...p.slice(0, p.length - 1),
          tests[i].name + ' failed.',
        ]);
        setOutput(p => p.concat('    ' + err.message));
      }
    }
    setRunning(false);
    setOutput(p => p.concat('Done'));
  }, [tests]);

  return (
    <View style={styles.container}>
      <ScrollView
        contentInsetAdjustmentBehavior="automatic"
        contentContainerStyle={styles.testsContainer}>
        {tests.map(t => (
          <TestWrapper key={t.name} name={t.name} state={t.state} />
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

export default App;
