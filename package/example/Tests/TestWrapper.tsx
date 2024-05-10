import React from "react";
import {
  ActivityIndicator,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from "react-native";

export type TestState = "notrun" | "running" | "success" | "failure";

type Props = {
  state: TestState;
  name: string;
  onRun: () => void;
};

export const TestWrapper: React.FC<Props> = ({ state, name, onRun }) => {
  return (
    <View style={styles.container}>
      <View style={styles.symbolContainer}>
        {state === "running" ? (
          <ActivityIndicator />
        ) : state === "notrun" ? (
          <Text style={styles.checkmark}>◻</Text>
        ) : state === "failure" ? (
          <Text style={styles.fail}>𐄂</Text>
        ) : (
          <Text style={styles.success}>☑</Text>
        )}
      </View>
      <Text style={styles.name}>{name}</Text>
      <TouchableOpacity style={styles.button} onPress={onRun}>
        <Text style={styles.buttonText}>
          {state === "failure" ? "rerun" : "run"}
        </Text>
      </TouchableOpacity>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: "row",
    alignItems: "center",
    height: 24,
  },
  symbolContainer: {
    width: 38,
    alignItems: "center",
  },
  checkmark: {
    fontSize: 22,
  },
  success: {
    fontSize: 22,
    color: "green",
  },
  fail: {
    fontSize: 22,
    color: "red",
  },
  name: {
    flex: 1,
  },
  button: {
    paddingHorizontal: 8,
  },
  buttonText: {
    color: "#0000BB",
  },
});
