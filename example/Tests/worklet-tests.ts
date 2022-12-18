import { Worklets } from "react-native-worklets";
import { ExpectException, ExpectValue, getWorkletInfo } from "./utils";

export const worklet_tests = {
  check_is_not_worklet: () => {
    return ExpectException(() =>
      Worklets.createRunInContextFn((a: number) => {
        return a * 200;
      })
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
    return ExpectValue(code, "function anonymous(a){return a;}");
  },
  check_share_object_with_js_function_fails_on_call: () => {
    const api = {
      call: () => 100,
    };
    const f = () => {
      "worklet";
      return api.call();
    };
    const w = Worklets.createRunInContextFn(f);
    return ExpectException(w);
  },
};
