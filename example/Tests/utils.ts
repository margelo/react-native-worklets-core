export const ExpectValue = <V>(value: V | Promise<V>, expected: V) => {
  return new Promise<void>(async (resolve, reject) => {
    let resolvedValue: V;

    if (value instanceof Promise) {
      resolvedValue = await value;
    } else {
      resolvedValue = value as any as V;
    }

    if (resolvedValue !== expected) {
      reject(
        new Error(
          `Expected ${expected}, got ${JSON.stringify(resolvedValue)}.`,
        ),
      );
    } else {
      resolve();
    }
  });
};

export const ExpectException = <T>(
  executor: (() => T) | (() => Promise<T>),
  expectedReason: string,
) => {
  return new Promise<void>(async (resolve, reject) => {
    try {
      const value = executor();
      if (value instanceof Promise) {
        await value;
        reject(new Error('Expected error but function succeeded.'));
      } else {
        reject(new Error('Expected error but function succeeded.'));
      }
    } catch (reason: any) {
      const resolvedReason =
        typeof reason === 'object' ? reason.message : reason;
      if (resolvedReason === expectedReason) {
        resolve();
      } else {
        reject(
          new Error(
            `Expected error message '${expectedReason}', got '${resolvedReason}'.`,
          ),
        );
      }
    }
  });
};
