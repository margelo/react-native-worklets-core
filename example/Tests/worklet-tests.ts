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
  check_nested_worklet_code: () => {
    const x = 5;
    const w = () => {
      "worklet";
      const nestedFn = () => {
        "worklet";
        return x + 1;
      };
      return nestedFn;
    };
    const { code } = getWorkletInfo(w);
    return ExpectValue(
      code,
      "function anonymous() {\n  const {\n    x\n  } = this._closure;\n  const nestedFn = function () {\n  const {\n    x\n  } = this._closure;\n    return x + 1;\n  };\n  return nestedFn;\n}"
    );
  },
  check_recursive_worklet_code: () => {
    const a = 1;
    function w(t: number): number {
      "worklet";
      if (t > 0) {
        return a + w(t - 1);
      } else {
        return 0;
      }
    }
    const { code } = getWorkletInfo(w);
    return ExpectValue(
      code,
      "function w(t) {\n  const w = this._recur;\n  const {\n    a\n  } = this._closure;\n  if (t > 0) {\n    return a + w(t - 1);\n  } else {\n    return 0;\n  }\n}"
    );
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
