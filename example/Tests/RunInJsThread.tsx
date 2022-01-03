import {Worklets} from 'react-native-worklets';
import {Test} from './types';

export const RunInJsThread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return new Promise<void>((resolve, reject) => {
    const r = worklet.call(100);
    if (r !== 100) {
      reject(new Error('Expected 100, got ' + r.toString()));
    }
    resolve();
  });
};

export const RunInJsThreadWithException: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });

  return new Promise<void>((resolve, reject) => {
    try {
      worklet.call();
      reject(new Error('Expected error'));
    } catch (reason) {
      if (reason === 'Test error') {
        resolve();
      } else {
        reject(
          new Error(
            "Expected error message 'Test error', got '" + reason + "'",
          ),
        );
      }
    }
  });
};
