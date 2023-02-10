import { Worklets } from "react-native-worklets";
import { ExpectException, ExpectValue, getWorkletInfo } from "./utils";

export const worklet_tests = {
  check_is_not_worklet: () => {
    const fn = Worklets.createRunInContextFn((a: number) => {
      return a * 200;
    });
    return ExpectValue(typeof fn, "function");
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
    return ExpectValue(code, "function anonymous(a) {\n  return a;\n}");
  },
  check_share_object_with_js_function_fails_on_call: () => {
    const api = {
      someFunc: (a: number) => a + 100,
    };
    const f = () => {
      "worklet";
      return api.someFunc(100);
    };
    const w = Worklets.createRunInContextFn(f);
    return ExpectException(w);
  },
  check_js_promise_resolves: () => {
    const f = (a: number) => {
      "worklet";
      return Promise.resolve(a + 100);
    };
    return ExpectValue(f(100), 200);
  },
  check_c_promise_resolves_from_context: () => {
    const f = (a: number) => {
      "worklet";
      return a + 100;
    };
    const w = Worklets.createRunInContextFn(f);
    return ExpectValue(
      new Promise((resolve) => {
        w(100).then(resolve);
      }),
      200
    );
  },
  check_finally_is_called: () => {
    const f = (a: number) => {
      "worklet";
      return a + 100;
    };
    const w = Worklets.createRunInContextFn(f);
    return ExpectValue(
      new Promise<void>((resolve) => {
        w(100).finally(resolve);
      }),
      undefined
    );
  },
  check_then_with_empty_args: () => {
    const f = (a: number) => {
      "worklet";
      return a + 100;
    };
    const w = Worklets.createRunInContextFn(f);
    return ExpectValue(
      new Promise<void>((resolve) => {
        w(100).then().finally(resolve);
      }),
      undefined
    );
  },
};
