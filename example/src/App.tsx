import { useEffect } from "react";
import { Worklets } from "react-native-worklets-core";

const App = () => {
  useEffect(() => {
    const workeltFunctionA = () => {
      "worklet";
    };

    Worklets.createRunInContextFn(() => {
      "worklet";
      function innerFct() {
        workeltFunctionA();
      }
      innerFct();
    });
  }, []);

  return null;
};

export default App;
