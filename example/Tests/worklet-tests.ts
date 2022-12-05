import {Worklets} from 'react-native-worklets';
import {Expect, ExpectException, ExpectValue, getWorkletInfo} from './utils';

export const worklet_tests = {
  check_worklet_closure: () => {
    const x = 100;
    const w = (a: number) => {
      'worklet';
      return a + x;
    };
    const {closure} = getWorkletInfo(w);
    return ExpectValue(closure, {x: 100});
  },
  check_worklet_closure_shared_value: () => {
    const x = Worklets.createSharedValue(1000);
    const w = (a: number) => {
      'worklet';
      return a + x.value;
    };
    const {closure} = getWorkletInfo(w);
    return ExpectValue(closure, {x: {value: 1000}});
  },
  check_worklet_code: () => {
    const w = (a: number) => {
      'worklet';
      return a;
    };
    const {code} = getWorkletInfo(w);
    return ExpectValue(code, 'function _f(a){return a;}');
  },
  call_sync_on_js_thread: () => {
    const worklet = Worklets.createWorklet((a: number) => {
      'worklet';
      return a;
    });
    return ExpectValue(worklet.callInJSContext(100), 100);
  },

  call_sync_on_js_thread_with_error: () => {
    const worklet = Worklets.createWorklet(() => {
      'worklet';
      throw new Error('Test error');
    });
    return ExpectException(worklet.callInJSContext, 'Test error');
  },

  call_async_to_worklet_thread: () => {
    const x = 100;
    const worklet = Worklets.createWorklet((a: number) => {
      'worklet';
      return a + x;
    });
    return ExpectValue(worklet.callInWorkletContext(100), 200);
  },

  call_async_to_worklet_thread_context: () => {
    const context = Worklets.createWorkletContext('test');
    const worklet = Worklets.createWorklet((a: number) => {
      'worklet';
      return a;
    }, context);
    return ExpectValue(worklet.callInWorkletContext(100), 100);
  },

  call_async_to_worklet_thread_with_error: () => {
    const worklet = Worklets.createWorklet(() => {
      'worklet';
      throw new Error('Test error');
    });
    return ExpectException(worklet.callInWorkletContext, 'Test error');
  },

  call_async_to_worklet_thread_with_error_in_context: () => {
    const context = Worklets.createWorkletContext('test');
    const worklet = Worklets.createWorklet(() => {
      'worklet';
      throw new Error('Test error');
    }, context);
    return ExpectException(worklet.callInWorkletContext, 'Test error');
  },

  call_sync_to_js_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const workletB = Worklets.createWorklet(function (a: number) {
      'worklet';
      sharedValue.value = a;
    });

    const workletA = Worklets.createWorklet(function (a: number) {
      'worklet';
      workletB.callInJSContext(a);
    });
    return Expect(workletA.callInWorkletContext(100), () => {
      return sharedValue.value === 100
        ? undefined
        : `sharedValue.value to be 100, got ${sharedValue.value}`;
    });
  },

  call_sync_to_js_from_worklet_with_retval: () => {
    const workletB = Worklets.createWorklet(function (a: number) {
      'worklet';
      return a;
    });

    const workletA = Worklets.createWorklet(function (a: number) {
      'worklet';
      return workletB.callInJSContext(a);
    });
    return ExpectValue(workletA.callInWorkletContext(200), 200);
  },

  call_sync_to_js_from_worklet_with_error: () => {
    const callback = Worklets.createWorklet(() => {
      'worklet';
      throw new Error('Test error');
    });

    const workletA = Worklets.createWorklet(function () {
      'worklet';
      callback.callInJSContext();
    });
    return ExpectException(
      workletA.callInWorkletContext,
      'Exception in HostFunction: Test error',
    );
  },

  call_async_to_and_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const workletB = Worklets.createWorklet(function (b: number) {
      'worklet';
      sharedValue.value = b;
    });

    const workletA = Worklets.createWorklet(function (a: number) {
      'worklet';
      return workletB.callInWorkletContext(a);
    });

    return Expect(workletA.callInWorkletContext(100), () => {
      return sharedValue.value === 100
        ? undefined
        : `sharedValue.value to be 100, got ${sharedValue.value}`;
    });
  },

  call_async_to_and_from_worklet_with_return_value: () => {
    const workletB = Worklets.createWorklet(() => {
      'worklet';
      return 1000;
    });
    const workletA = Worklets.createWorklet(function () {
      'worklet';
      return workletB.callInWorkletContext();
    });
    return ExpectValue(workletA.callInWorkletContext(), 1000);
  },

  call_async_to_and_from_worklet_with_error: () => {
    const workletB = Worklets.createWorklet(() => {
      'worklet';
      throw Error('Test error');
    });
    const workletA = Worklets.createWorklet(function () {
      'worklet';
      return workletB.callInWorkletContext();
    });
    return ExpectException(workletA.callInWorkletContext, 'Test error');
  },
};
