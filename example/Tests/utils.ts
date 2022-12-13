export const Expect = <V>(
  value: V | Promise<V>,
  expected: (v: V) => string | undefined
) => {
  return new Promise<void>(async (resolve, reject) => {
    let resolvedValue: V;

    if (value instanceof Promise) {
      resolvedValue = await value;
    } else {
      resolvedValue = value as any as V;
    }

    const resolvedExpected = expected(resolvedValue);

    if (resolvedExpected) {
      reject(new Error(`Expected ${resolvedExpected}.`));
    } else {
      resolve();
    }
  });
};

export const ExpectValue = <V, T>(value: V | Promise<V>, expected: T) => {
  return new Promise<void>(async (resolve, reject) => {
    let resolvedValue: V;

    if (value instanceof Promise) {
      resolvedValue = await value;
    } else {
      resolvedValue = value as any as V;
    }

    if (JSON.stringify(resolvedValue) !== JSON.stringify(expected)) {
      reject(
        new Error(
          `Expected ${JSON.stringify(expected)}, got ${JSON.stringify(
            resolvedValue
          )}.`
        )
      );
    } else {
      resolve();
    }
  });
};

export const ExpectException = <T>(
  executor: (() => T) | (() => Promise<T>),
  expectedReason?: string
) => {
  return new Promise<void>(async (resolve, reject) => {
    try {
      const value = executor();
      if (value instanceof Promise) {
        await value;
        reject(new Error("Expected error but function succeeded (Promise)."));
      } else {
        reject(new Error("Expected error but function succeeded."));
      }
    } catch (reason: any) {
      if (expectedReason) {
        const resolvedReason =
          typeof reason === "object" ? reason.message : reason;
        if (resolvedReason === expectedReason) {
          resolve();
        } else {
          const errorMessage = `Expected error message '${expectedReason.substr(
            0,
            80
          )}', got '${resolvedReason.substr(0, 80)}'.`;
          reject(new Error(errorMessage));
        }
      } else {
        resolve();
      }
    }
  });
};

export const getWorkletInfo = <T extends Array<unknown>, R>(
  worklet: (...args: T) => R
) => {
  // @ts-ignore
  return { closure: worklet._closure, code: worklet.asString };
};
