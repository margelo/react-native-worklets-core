import { Worklets } from "react-native-worklets-core";
import { Expect, ExpectException, ExpectValue } from "./utils";

export const worklet_context_tests = {
  call_async_on_js_thread: () => {
    const worklet = (a: number) => {
      "worklet";
      return a;
    };
    return ExpectValue(worklet(100), 100);
  },

  call_async_on_js_thread_with_error: () => {
    const worklet = () => {
      "worklet";
      throw new Error("Test error");
    };
    return ExpectException(worklet, "Test error");
  },

  call_async_to_worklet_thread: () => {
    const x = 100;
    const f = (a: number) => {
      "worklet";
      return a + x;
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(100), 200);
  },

  call_async_to_worklet_thread_context: () => {
    const context = Worklets.createContext("test");
    const f = (a: number) => {
      "worklet";
      return a;
    };
    const w = context.createRunAsync(f);
    return ExpectValue(w(100), 100);
  },

  call_async_to_worklet_thread_with_error: () => {
    const f = () => {
      "worklet";
      throw new Error("Test error");
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectException(w, "Test error");
  },

  call_async_to_worklet_thread_with_error_in_context: () => {
    const context = Worklets.createContext("test");
    const f = () => {
      "worklet";
      throw new Error("Test error");
    };
    const w = context.createRunAsync(f);
    return ExpectException(w, "Test error");
  },

  call_async_to_worklet_thread_and_call_second_worklet: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const workletB = function (a: number) {
      "worklet";
      sharedValue.value = a;
    };

    const workletA = function (a: number) {
      "worklet";
      workletB(a);
    };

    const w = Worklets.defaultContext.createRunAsync(workletA);
    return Expect(w(100), () => {
      return sharedValue.value === 100
        ? undefined
        : `sharedValue.value to be 100, got ${sharedValue.value}`;
    });
  },

  call_async_to_js_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const setSharedValue = function (a: number) {
      "worklet";
      sharedValue.value = a;
    };

    const js1 = Worklets.createRunOnJS(setSharedValue);

    const w1 = function (a: number) {
      "worklet";
      return js1(a);
    };

    const w = Worklets.defaultContext.createRunAsync(w1);
    return Expect(w(100), () => {
      return sharedValue.value === 100
        ? undefined
        : `sharedValue.value to be 100, got ${sharedValue.value}`;
    });
  },

  call_async_to_js_from_worklet_with_retval: () => {
    const workletB = Worklets.createRunOnJS(function (a: number) {
      "worklet";
      return a;
    });

    const workletA = Worklets.defaultContext.createRunAsync((a: number) => {
      "worklet";
      return workletB(a);
    });
    return ExpectValue(workletA(200), 200);
  },

  call_async_to_js_from_worklet_with_error: () => {
    const callback = Worklets.createRunOnJS(() => {
      "worklet";
      throw new Error("Test error");
    });

    const workletA = Worklets.defaultContext.createRunAsync(() => {
      "worklet";
      return callback();
    });
    return ExpectException(workletA, "Test error");
  },

  call_decorated_js_function_from_worklet: () => {
    const adder = (a: number) => {
      "worklet";
      return a + a;
    };

    const w_square = Worklets.defaultContext.createRunAsync((a: number) => {
      "worklet";
      return Math.sqrt(adder(a));
    });

    return ExpectValue(w_square(32), 8);
  },

  call_async_to_and_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const workletB = function (b: number) {
      "worklet";
      sharedValue.value = b;
    };

    const workletA = Worklets.defaultContext.createRunAsync((a: number) => {
      "worklet";
      return workletB(a);
    });

    return Expect(workletA(100), () => {
      return sharedValue.value === 100
        ? undefined
        : `sharedValue.value to be 100, got ${sharedValue.value}`;
    });
  },

  call_async_to_and_from_worklet_with_return_value: () => {
    const workletB = () => {
      "worklet";
      return 1000;
    };
    const workletA = Worklets.defaultContext.createRunAsync(() => {
      "worklet";
      return workletB();
    });
    return ExpectValue(workletA(), 1000);
  },

  call_async_to_and_from_worklet_multiple_times_with_return_value: () => {
    const workletB = (): number => {
      "worklet";
      return 1000;
    };
    const workletA = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      let r = 0;
      for (let i = 0; i < 100; i++) {
        r += workletB();
      }
      return r;
    });
    return ExpectValue(workletA(), 100000);
  },

  call_async_to_and_from_worklet_with_error: () => {
    const workletB = () => {
      "worklet";
      throw Error("Test error");
    };
    const workletA = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      return workletB();
    });
    return ExpectException(workletA, "Test error");
  },

  call_worklet_to_worklet_without_wrapping_args: () => {
    const workletB = (a: { current: number }) => {
      "worklet";
      return a.current;
    };
    const workletA = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      return workletB({ current: 100 });
    });
    return ExpectValue(workletA(), 100);
  },

  fail_when_calling_a_regular_function_from_a_worklet: () => {
    const func = (a: number) => a;
    const worklet = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      return func(100);
    });
    return ExpectException(worklet);
  },
  call_worklet_with_this: () => {
    const obj = {
      a: 100,
      b: 100,
      f: function () {
        "worklet";
        return this.a + this.b;
      },
    };
    const sharedValue = Worklets.createSharedValue(obj);
    const worklet = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      return sharedValue.value.f();
    });
    return ExpectValue(worklet(), 200);
  },
  call_createRunOnJS_inside_worklet: () => {
    const fn = function (b: number) {
      "worklet";
      return b * 2;
    };
    const f = function (a: number) {
      "worklet";
      const wjs = Worklets.createRunOnJS(fn);
      return wjs(a);
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(100), 200);
  },
  call_worklet_in_same_context: () => {
    const workletInTest = Worklets.defaultContext.createRunAsync(function (
      a: number
    ) {
      "worklet";
      console.log("ctx: workletInTest: returning 100 + ", a);
      return 100 + a;
    });

    const worklet = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      console.log("ctx: worklet: calling workletInTest(100)");
      const a = workletInTest(100);
      console.log("ctx: worklet: ", a);
      return a;
    });
    console.log("ctx: Calling worklet()");
    return ExpectValue(worklet(), 200);
  },
  call_worklet_in_other_context: () => {
    const ctx = Worklets.createContext("test");
    function calcInCtx(a: number) {
      "worklet";
      return 100 + a;
    }
    calcInCtx.name = "calcInCtx";
    const workletInCtx = ctx.createRunAsync(calcInCtx);

    function calcInDefaultCtx() {
      "worklet";
      return workletInCtx(100);
    }
    calcInDefaultCtx.name = "calcInDefaultCtx";
    const worklet = Worklets.defaultContext.createRunAsync(calcInDefaultCtx);

    return ExpectValue(worklet(), 200);
  },
  call_createRunAsync_from_context: () => {
    const worklet = Worklets.defaultContext.createRunAsync(() => {
      "worklet";
      const workletInTest = Worklets.defaultContext.createRunAsync(
        (a: number) => {
          "worklet";
          return 100 + a;
        }
      );
      return workletInTest(100);
    });
    return ExpectValue(worklet(), 200);
  },
  call_createRunAsync_between_contexts: () => {
    const ctx = Worklets.createContext("test");

    const worklet = Worklets.defaultContext.createRunAsync(() => {
      "worklet";
      const workletInTest = ctx.createRunAsync((a: number) => {
        "worklet";
        return 100 + a;
      });
      return workletInTest(100);
    });
    return ExpectValue(worklet(), 200);
  },
  call_fire_and_forget: () => {
    const f = (a: number) => {
      "worklet";
      return a * 2;
    };
    let wf: any = Worklets.defaultContext.createRunAsync(f);
    wf(100);
    wf = undefined;
    return ExpectValue(true, true);
  },
  call_worklet_inside_worklet: () => {
    const f = (a: number) => {
      "worklet";
      return a * 2;
    };
    const fw = () => {
      "worklet";
      return f(100);
    };
    let wf = Worklets.defaultContext.createRunAsync(fw);
    return ExpectValue(wf(), 200);
  },
  call_run_async_directly: () => {
    const result = Worklets.defaultContext.runAsync(() => {
      "worklet";
      return 42;
    });
    return ExpectValue(result, 42);
  },
  call_run_on_js_directly: () => {
    const f = function (a: number) {
      "worklet";
      return Worklets.runOnJS(() => {
        "worklet";
        return a * 2;
      });
    };
    const w = Worklets.defaultContext.createRunAsync(f);
    return ExpectValue(w(100), 200);
  },
  call_run_async_and_run_on_js_directly: () => {
    const a = 150;
    const result = Worklets.defaultContext.runAsync(() => {
      "worklet";
      const b = a * 2;
      return Worklets.runOnJS(() => {
        "worklet";
        return b * 2;
      });
    });
    return ExpectValue(result, 600);
  },
  call_run_async_and_run_on_js_directly_other_context: () => {
    const a = 150;
    const context = Worklets.createContext("nested-context");
    const result = Worklets.defaultContext.runAsync(() => {
      "worklet";
      const b = a * 2;
      return context.runAsync(() => {
        "worklet";
        const c = b * 2;
        return Worklets.runOnJS(() => {
          "worklet";
          return c * 2;
        });
      });
    });
    return ExpectValue(result, 1200);
  },
  call_run_async_then_js_then_async: () => {
    const a = 150;
    const result = Worklets.defaultContext.runAsync(() => {
      "worklet";
      const b = a * 2;
      return Worklets.runOnJS(() => {
        "worklet";
        const c = b * 2;
        return Worklets.defaultContext.runAsync(() => {
          "worklet";
          return c * 2;
        });
      });
    });
    return ExpectValue(result, 1200);
  },
  call_run_async_then_js_then_async_custom_context: () => {
    const a = 150;
    const context = Worklets.createContext("nested-context-2");
    const result = context.runAsync(() => {
      "worklet";
      const b = a * 2;
      return Worklets.runOnJS(() => {
        "worklet";
        const c = b * 2;
        return context.runAsync(() => {
          "worklet";
          return c * 2;
        });
      });
    });
    return ExpectValue(result, 1200);
  },
};
