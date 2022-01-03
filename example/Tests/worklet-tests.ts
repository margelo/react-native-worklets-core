import {Worklets} from 'react-native-worklets';
import {Test} from './types';
import {Expect, ExpectException, ExpectValue} from './utils';

const call_sync_on_js_thread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return ExpectValue(worklet.call(100), 100);
};

const call_sync_on_js_thread_with_error: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });
  return ExpectException(worklet.call, 'Test error');
};

const call_async_to_worklet_thread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return ExpectValue(worklet.callAsync(100), 100);
};

const call_async_to_worklet_thread_context: Test = () => {
  const context = Worklets.createWorkletContext('test');
  const worklet = Worklets.createWorklet((a: number) => a, undefined, context);
  return ExpectValue(worklet.callAsync(100), 100);
};

const call_async_to_worklet_thread_with_error: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });
  return ExpectException(worklet.callAsync, 'Test error');
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
  return ExpectException(worklet.callAsync, 'Test error');
};

const call_async_to_js_from_worklet: Test = () => {
  const sharedValue = Worklets.createSharedValue(0);
  const workletB = Worklets.createWorklet((a: number) => {
    sharedValue.value = a;
  });

  const workletA = Worklets.createWorklet(
    function (a: number) {
      this.callback.call(a);
    },
    {callback: workletB},
  );
  return Expect(workletA.callAsync(100), () => {
    return sharedValue.value === 100;
  });
};

export const worklet_tests: {[key: string]: Test} = {
  call_sync_on_js_thread,
  call_sync_on_js_thread_with_error,
  call_async_to_worklet_thread,
  call_async_to_worklet_thread_context,
  call_async_to_worklet_thread_with_error,
  call_async_to_worklet_thread_with_error_in_context,
  call_async_to_js_from_worklet,
};
