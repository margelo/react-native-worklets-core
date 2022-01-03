import React from 'react';
import {
  ActivityIndicator,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from 'react-native';

export type TestState = 'notrun' | 'running' | 'success' | 'failure';

type Props = {
  state: TestState;
  name: string;
  onRerun: () => void;
};

export const TestWrapper: React.FC<Props> = ({state, name, onRerun}) => {
  return (
    <View style={styles.container}>
      <View style={styles.symbolContainer}>
        {state === 'running' ? (
          <ActivityIndicator />
        ) : state === 'notrun' ? (
          <Text style={styles.checkmark}>◻</Text>
        ) : state === 'failure' ? (
          <Text style={styles.fail}>𐄂</Text>
        ) : (
          <Text style={styles.success}>☑</Text>
        )}
      </View>
      <Text style={styles.name}>{name}</Text>
      {state === 'failure' ? (
        <TouchableOpacity style={styles.button} onPress={onRerun}>
          <Text style={styles.buttonText}>rerun</Text>
        </TouchableOpacity>
      ) : null}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    height: 30,
  },
  symbolContainer: {
    width: 38,
    alignItems: 'center',
  },
  checkmark: {
    fontSize: 24,
  },
  success: {
    fontSize: 24,
    color: 'green',
  },
  fail: {
    fontSize: 24,
    color: 'red',
  },
  name: {
    flex: 1,
  },
  button: {
    paddingHorizontal: 8,
  },
  buttonText: {
    color: '#0000BB',
  },
});
