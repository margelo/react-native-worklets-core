import { Worklets } from "react-native-worklets";
import { ExpectException, ExpectValue, getWorkletInfo } from "./utils";

export const worklet_tests = {
  check_is_not_worklet: () => {
    return ExpectException(
      () =>
        Worklets.createRunInContextFn((a: number) => {
          return a * 200;
        }),
      "Exception in HostFunction: createRunInContextFn expects a worklet decorated function as its first parameter."
    );
  },
  check_worklet_closure: () => {
    const x = 100;
    const w = (a: number) => {
      "worklet";
      return a + x;
    };
    const { closure } = getWorkletInfo(w);
    return ExpectValue(closure, { x: 100 });
  },
  check_worklet_closure_shared_value: () => {
    const x = Worklets.createSharedValue(1000);
    const w = (a: number) => {
      "worklet";
      return a + x.value;
    };
    const { closure } = getWorkletInfo(w);
    return ExpectValue(closure, { x: { value: 1000 } });
  },
  check_worklet_code: () => {
    const w = (a: number) => {
      "worklet";
      return a;
    };
    const { code } = getWorkletInfo(w);
    return ExpectValue(code, "function _f(a){return a;}");
  },
};
