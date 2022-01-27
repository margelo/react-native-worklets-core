import {Worklets} from 'react-native-worklets';
import {Expect, ExpectException, ExpectValue} from './utils';

export const worklet_tests = {
  call_sync_on_js_thread: () => {
    const worklet = Worklets.createWorklet((a: number) => a);
    return ExpectValue(worklet.callInJSContext(100), 100);
  },

  call_sync_on_js_thread_with_error: () => {
    const worklet = Worklets.createWorklet(() => {
      throw new Error('Test error');
    });
    return ExpectException(worklet.callInJSContext, 'Test error');
  },

  call_async_to_worklet_thread: () => {
    const worklet = Worklets.createWorklet((a: number) => a);
    return ExpectValue(worklet.callInWorkletContext(100), 100);
  },

  call_async_to_worklet_thread_context: () => {
    const context = Worklets.createWorkletContext('test');
    const worklet = Worklets.createWorklet(
      (a: number) => a,
      undefined,
      context,
    );
    return ExpectValue(worklet.callInWorkletContext(100), 100);
  },

  call_async_to_worklet_thread_with_error: () => {
    const worklet = Worklets.createWorklet(() => {
      throw new Error('Test error');
    });
    return ExpectException(worklet.callInWorkletContext, 'Test error');
  },

  call_async_to_worklet_thread_with_error_in_context: () => {
    const context = Worklets.createWorkletContext('test');
    const worklet = Worklets.createWorklet(
      () => {
        throw new Error('Test error');
      },
      undefined,
      context,
    );
    return ExpectException(worklet.callInWorkletContext, 'Test error');
  },

  call_sync_to_js_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const workletB = Worklets.createWorklet(
      function (a: number) {
        this.sharedValue.value = a;
      },
      {sharedValue},
    );

    const workletA = Worklets.createWorklet(
      function (a: number) {
        this.callback.callInJSContext(a);
      },
      {callback: workletB},
    );
    return Expect(workletA.callInWorkletContext(100), () => {
      return sharedValue.value === 100
        ? undefined
        : `sharedValue.value to be 100, got ${sharedValue.value}`;
    });
  },

  call_sync_to_js_from_worklet_with_retval: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const workletB = Worklets.createWorklet(
      function (a: number) {
        return a;
      },
      {sharedValue},
    );

    const workletA = Worklets.createWorklet(
      function (a: number) {
        return this.callback.callInJSContext(a);
      },
      {callback: workletB},
    );
    return ExpectValue(workletA.callInWorkletContext(200), 200);
  },

  call_sync_to_js_from_worklet_with_error: () => {
    const callback = Worklets.createWorklet(() => {
      throw new Error('Test error');
    });

    const workletA = Worklets.createWorklet(
      function () {
        this.callback.callInJSContext();
      },
      {callback},
    );
    return ExpectException(workletA.callInWorkletContext, 'Test error');
  },

  call_async_to_and_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue(0);
    const workletB = Worklets.createWorklet(
      function (b: number) {
        this.sharedValue.value = b;
      },
      {sharedValue},
    );

    const workletA = Worklets.createWorklet(
      function (a: number) {
        return this.callback.callInWorkletContext(a);
      },
      {callback: workletB},
    );
    return Expect(workletA.callInWorkletContext(100), () => {
      return sharedValue.value === 100
        ? undefined
        : `sharedValue.value to be 100, got ${sharedValue.value}`;
    });
  },

  call_async_to_and_from_worklet_with_return_value: () => {
    const workletB = Worklets.createWorklet(() => 1000);
    const workletA = Worklets.createWorklet(
      function () {
        return this.workletB.callInWorkletContext();
      },
      {workletB},
    );
    return ExpectValue(workletA.callInWorkletContext(), 1000);
  },

  call_async_to_and_from_worklet_with_error: () => {
    const workletB = Worklets.createWorklet(() => {
      throw Error('Test error');
    });
    const workletA = Worklets.createWorklet(
      function () {
        return this.workletB.callInWorkletContext();
      },
      {workletB},
    );
    return ExpectException(workletA.callInWorkletContext, 'Test error');
  },
};
