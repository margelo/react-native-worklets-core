import React, { useMemo, useState } from "react";
import { DevSettings, StyleSheet, Text, TouchableOpacity } from "react-native";
import { Worklets } from "react-native-worklets-core";

const context = Worklets.createContext("reload-stress");
const PARALLEL_CHAINS = 4;
const RELOAD_DELAY_MS = 300;

/**
 Keeps cross-context calls in flight (runAsync chains on a custom context with
 JS continuations, plus createRunOnJS callbacks from the worklet thread) and
 then reloads the JS bundle. Reloading while these jobs are queued must not
 crash; tap it repeatedly to exercise the reload path.
 */
export const ReloadStress = () => {
  const [calls, setCalls] = useState(0);
  const reportCall = useMemo(
    () => Worklets.createRunOnJS(() => setCalls((c) => c + 1)),
    []
  );

  const onPress = () => {
    const runChain = () => {
      context
        .runAsync(() => {
          "worklet";
          let sum = 0;
          for (let i = 0; i < 20000; i++) {
            sum += i;
          }
          reportCall();
          return sum;
        })
        .then(runChain);
    };
    for (let i = 0; i < PARALLEL_CHAINS; i++) {
      runChain();
    }
    setTimeout(() => DevSettings.reload(), RELOAD_DELAY_MS);
  };

  return (
    <TouchableOpacity style={styles.button} onPress={onPress}>
      <Text>Reload while busy ({calls} worklet calls)</Text>
    </TouchableOpacity>
  );
};

const styles = StyleSheet.create({
  button: {
    paddingVertical: 8,
    paddingHorizontal: 8,
  },
});
