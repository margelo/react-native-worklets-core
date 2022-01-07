import {Worklets} from 'react-native-worklets';
import {Test} from './types';
import {Expect, ExpectException, ExpectValue} from './utils';

const call_sync_on_js_thread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return ExpectValue(worklet.callInJSContext(100), 100);
};

const call_sync_on_js_thread_with_error: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });
  return ExpectException(worklet.callInJSContext, 'Test error');
};

const call_async_to_worklet_thread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return ExpectValue(worklet.callInWorkletContext(100), 100);
};

const call_async_to_worklet_thread_context: Test = () => {
  const context = Worklets.createWorkletContext('test');
  const worklet = Worklets.createWorklet((a: number) => a, undefined, context);
  return ExpectValue(worklet.callInWorkletContext(100), 100);
};

const call_async_to_worklet_thread_with_error: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });
  return ExpectException(worklet.callInWorkletContext, 'Test error');
};

const call_async_to_worklet_thread_with_error_in_context: Test = () => {
  const context = Worklets.createWorkletContext('test');
  const worklet = Worklets.createWorklet(
    () => {
      throw new Error('Test error');
    },
    undefined,
    context,
  );
  return ExpectException(worklet.callInWorkletContext, 'Test error');
};

const call_sync_to_js_from_worklet: Test = () => {
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
};

const call_sync_to_js_from_worklet_with_retval: Test = () => {
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
};

const call_sync_to_js_from_worklet_with_error: Test = () => {
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
};

const call_async_to_and_from_worklet: Test = () => {
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
};

const call_async_to_and_from_worklet_with_error: Test = () => {
  const workletB = Worklets.createWorklet(() => {
    throw Error('Test error');
  });
  const workletA = Worklets.createWorklet(
    function () {
      return this.callback.callInWorkletContext();
    },
    {callback: workletB},
  );
  return ExpectException(workletA.callInWorkletContext, 'Test error');
};

export const worklet_tests: {[key: string]: Test} = {
  call_sync_on_js_thread,
  call_sync_on_js_thread_with_error,
  call_async_to_worklet_thread,
  call_async_to_worklet_thread_context,
  call_async_to_worklet_thread_with_error,
  call_async_to_worklet_thread_with_error_in_context,
  call_sync_to_js_from_worklet,
  call_sync_to_js_from_worklet_with_error,
  call_sync_to_js_from_worklet_with_retval,
  call_async_to_and_from_worklet,
  call_async_to_and_from_worklet_with_error,
};
