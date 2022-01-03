import {Worklets} from 'react-native-worklets';
import {Test} from './types';
import {ExpectException, ExpectValue} from './utils';

const run_on_js_thread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return ExpectValue(worklet.call(100), 100);
};

const run_on_js_thread_with_exception: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });
  return ExpectException(worklet.call, 'Test error');
};

const run_on_worklet_thread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return ExpectValue(worklet.callAsync(100), 100);
};

const run_on_worklet_thread_in_context: Test = () => {
  const context = Worklets.createWorkletContext('test');
  const worklet = Worklets.createWorklet((a: number) => a, undefined, context);
  return ExpectValue(worklet.callAsync(100), 100);
};

const run_on_worklet_thread_with_exception: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });
  return ExpectException(worklet.callAsync, 'Test error');
};

const run_on_worklet_thread_with_exception_in_context: Test = () => {
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

export const tests: {[key: string]: Test} = {
  run_on_js_thread,
  run_on_js_thread_with_exception,
  run_on_worklet_thread,
  run_on_worklet_thread_in_context,
  run_on_worklet_thread_with_exception,
  run_on_worklet_thread_with_exception_in_context,
};
