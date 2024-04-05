import React, { useEffect } from "react";
import {
  ScrollView,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from "react-native";

import { useTestRunner } from "../Tests";
import { TestWrapper } from "../Tests/TestWrapper";

import { Worklets } from "react-native-worklets-core";

// Invoke worklets one so it installs into the runtime
Worklets.defaultContext;

const testObject = global.TestObject;
// Pass a JS function on JS thread to worklets
testObject.setJsCallback(() => {
  console.log("JS Callback !!!");
});

const App = () => {
  const { tests, categories, output, runTests, runSingleTest } =
    useTestRunner();

  useEffect(() => {
    // In some worklet context call our JS function from the native side
    Worklets.createRunInContextFn(() => {
      "worklet";
      testObject.callJsCallback();
    })();
  }, []);

  return (
    <View style={styles.container}>
      <View style={styles.tests}>
        <ScrollView
          contentInsetAdjustmentBehavior="automatic"
          contentContainerStyle={styles.testsScrollView}
        >
          {categories.map((category) => (
            <React.Fragment key={category}>
              <TouchableOpacity
                onPress={() =>
                  runTests(tests.filter((t) => t.category === category))
                }
              >
                <Text style={styles.category}>{category}</Text>
              </TouchableOpacity>
              {tests
                .filter((t) => t.category === category)
                .map((t) => (
                  <TestWrapper
                    key={t.name}
                    name={t.name}
                    state={t.state}
                    onRun={() => runSingleTest(t)}
                  />
                ))}
            </React.Fragment>
          ))}
        </ScrollView>
      </View>
      <View style={styles.output}>
        <ScrollView>
          <Text style={styles.outputText}>{output.join("\n")}</Text>
        </ScrollView>
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
    borderTopColor: "#CCC",
    borderTopWidth: StyleSheet.hairlineWidth,
    flex: 0.2,
    backgroundColor: "#EFEFEF",
  },
  outputText: {
    padding: 8,
  },
  category: {
    paddingTop: 8,
    paddingBottom: 4,
    paddingHorizontal: 8,
    textDecorationLine: "underline",
  },
});

export default App;
