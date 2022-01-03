import {Worklets} from 'react-native-worklets';
import {Test} from './types';

export const RunInWorkletThread: Test = () => {
  const worklet = Worklets.createWorklet((a: number) => a);
  return new Promise<void>((resolve, reject) => {
    return worklet
      .callAsync(100)
      .then(r => {
        if (r !== 100) {
          reject(new Error('Expected 100, got ' + r.toString()));
        }
        resolve();
      })
      .catch(reject);
  });
};

export const RunInWorkletThreadInContext: Test = () => {
  const context = Worklets.createWorkletContext('test');
  const worklet = Worklets.createWorklet((a: number) => a, undefined, context);
  return new Promise<void>((resolve, reject) => {
    return worklet
      .callAsync(100)
      .then(r => {
        if (r !== 100) {
          reject(new Error('Expected 100, got ' + r.toString()));
        }
        resolve();
      })
      .catch(reject);
  });
};

export const RunInWorkletThreadWithException: Test = () => {
  const worklet = Worklets.createWorklet(() => {
    throw new Error('Test error');
  });
  return new Promise<void>((resolve, reject) => {
    return worklet
      .callAsync()
      .then(() => {
        reject('Expected error');
      })
      .catch((reason: string) => {
        if (reason === 'Test error') {
          resolve();
        } else {
          reject(
            new Error(
              "Expected error message 'Test error', got '" + reason + "'",
            ),
          );
        }
      });
  });
};

export const RunInWorkletThreadWithExceptionInContext: Test = () => {
  const context = Worklets.createWorkletContext('test');
  const worklet = Worklets.createWorklet(
    () => {
      throw new Error('Test error');
    },
    undefined,
    context,
  );
  return new Promise<void>((resolve, reject) => {
    return worklet
      .callAsync()
      .then(() => {
        reject('Expected error');
      })
      .catch((reason: string) => {
        if (reason === 'Test error') {
          resolve();
        } else {
          reject(
            new Error(
              "Expected error message 'Test error', got '" + reason + "'",
            ),
          );
        }
      });
  });
};
