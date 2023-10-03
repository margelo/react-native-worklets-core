const isThenable = <V>(value: V | Promise<V>) =>
  value instanceof Object &&
  "then" in value &&
  typeof value.then === "function";

export const Expect = <V>(
  value: V | Promise<V>,
  expected: (v: V) => string | undefined
) => {
  return new Promise<void>(async (resolve, reject) => {
    let resolvedValue: V;

    if (isThenable(value)) {
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
    let resolvedValue: V | undefined;
    if (isThenable(value)) {
      try {
        resolvedValue = await value;
      } catch (err) {
        console.log("ExpectValue, failed:", err);
        reject(new Error(`Expected ${expected}, got ${err}.`));
        return;
      }
    } else {
      resolvedValue = value as V;
    }
    console.log("ExpectValue, resolved:", resolvedValue);

    if (JSON.stringify(resolvedValue) !== JSON.stringify(expected)) {
      const message = `Expected ${JSON.stringify(
        expected
      )}, got ${JSON.stringify(resolvedValue)}.`;
      console.log("ExpectValue, failed:", message);
      reject(new Error(message));
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
      if (isThenable(value)) {
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
  return worklet.__initData
    ? // @ts-ignore
      { closure: worklet.__closure, code: worklet.__initData.code }
    : // @ts-ignore
      { closure: worklet.__closure, code: worklet.asString };
};
