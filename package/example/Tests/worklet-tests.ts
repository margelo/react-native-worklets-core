import { worklet, Worklets } from "react-native-worklets-core";
import { ExpectException, ExpectValue, getWorkletInfo } from "./utils";

export const worklet_tests = {
  check_is_not_worklet: () => {
    const fn = Worklets.defaultContext.createRunAsync((a: number) => {
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
  call_nested_worklet: () => {
    const rootWorklet = () => {
      "worklet";
      const cl = { x: 100 };
      const nestedWorklet = (c: number) => {
        "worklet";
        return c + cl.x;
      };
      const nestedWorkletFn =
        Worklets.defaultContext.createRunAsync(nestedWorklet);
      return nestedWorkletFn(100);
    };
    const fw = () => {
      "worklet";
      return rootWorklet();
    };
    let wf = Worklets.defaultContext.createRunAsync(fw);
    return ExpectValue(wf(), 200);
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
    const w = Worklets.defaultContext.createRunAsync(f);
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
    const w = Worklets.defaultContext.createRunAsync(f);
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
    const w = Worklets.defaultContext.createRunAsync(f);
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
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(
      new Promise<void>((resolve) => {
        w(100).then().finally(resolve);
      }),
      undefined
    );
  },
  check_pure_array_is_passed_as_array: () => {
    const array = [0, 1, 2, 3, 4];
    const f = () => {
      "worklet";
      return Array.isArray(array);
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(), true);
  },
  check_thread_id_exists: () => {
    const threadId = Worklets.getCurrentThreadId();
    return ExpectValue(Number.isSafeInteger(threadId), true);
  },
  check_thread_id_consecutive_calls_are_equal: () => {
    const first = Worklets.getCurrentThreadId();
    const second = Worklets.getCurrentThreadId();
    return ExpectValue(first, second);
  },
  check_thread_id_consecutive_calls_in_worklet_are_equal: async () => {
    const [first, second] = await Worklets.defaultContext.runAsync(() => {
      "worklet";
      const firstId = Worklets.getCurrentThreadId();
      const secondId = Worklets.getCurrentThreadId();
      return [firstId, secondId];
    });
    return ExpectValue(first, second);
  },
  check_thread_ids_are_different: async () => {
    const jsThreadId = Worklets.getCurrentThreadId();
    const workletThreadId = await Worklets.defaultContext.runAsync(() => {
      "worklet";
      return Worklets.getCurrentThreadId();
    });
    return await ExpectValue(jsThreadId !== workletThreadId, true);
  },
  check_pure_array_is_passed_as_jsi_array: () => {
    const array = [0, 1, 2, 3, 4];
    const f = () => {
      "worklet";
      return Worklets.__jsi_is_array(array);
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(), true);
  },
  check_pure_array_inside_object_is_passed_as_jsi_array: () => {
    const obj = { a: [0, 1, 2, 3, 4] };
    const f = () => {
      "worklet";
      return Worklets.__jsi_is_array(obj.a);
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(), true);
  },
  check_pure_array_nested_argument_is_passed_as_jsi_array: () => {
    const obj = { a: [0, 1, 2, 3, 4] };
    const f = (t: typeof obj) => {
      "worklet";
      return Worklets.__jsi_is_array(t.a);
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(obj), true);
  },
  check_shared_value_array_is_not_passed_as_jsi_array: () => {
    const obj = Worklets.createSharedValue([0, 1, 2, 3, 4]);
    const f = () => {
      "worklet";
      return Worklets.__jsi_is_array(obj.value);
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(), false);
  },
  check_shared_value_nested_array_is_not_passed_as_jsi_array: () => {
    const obj = Worklets.createSharedValue({ a: [0, 1, 2, 3, 4] });
    const f = () => {
      "worklet";
      return Worklets.__jsi_is_array(obj.value.a);
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(), false);
  },
  check_called_directly: () => {
    const result = Worklets.defaultContext.runAsync(() => {
      "worklet";
      return 42;
    });
    return ExpectValue(result, 42);
  },
  check_worklet_checker_works: () => {
    const func = () => {
      "worklet";
      return 42;
    };
    const same = worklet(func);
    return ExpectValue(same, func);
  },
  check_worklet_checker_throws_invalid_worklet: () => {
    const func = () => {
      return "not a worklet";
    };
    return ExpectException(() => {
      worklet(func);
    });
  },
};
