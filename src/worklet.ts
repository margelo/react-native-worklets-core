type AnyFunc = (...args: any[]) => any;

type Workletize<TFunc extends () => any> = TFunc & {
  __initData: {
    code: string;
    location: string;
    __sourceMap: string;
  };
  __workletHash: string;
};

/**
 * Checks whether the given function is a Worklet or not.
 */
export function isWorklet<TFunc extends AnyFunc>(
  func: TFunc
): func is Workletize<TFunc> {
  const maybeWorklet = func as Partial<Workletize<TFunc>> & TFunc;
  if (
    maybeWorklet.__workletHash == null ||
    typeof maybeWorklet.__workletHash !== "string"
  )
    return false;

  const initData = maybeWorklet.__initData;
  if (initData == null || typeof initData !== "object") return false;

  if (
    typeof initData.__sourceMap !== "string" ||
    typeof initData.code !== "string" ||
    typeof initData.location !== "string"
  )
    return false;

  return true;
}

class NotAWorkletError<TFunc extends AnyFunc> extends Error {
  constructor(func: TFunc) {
    super(
      `The given function ("${func.name}") is not a Worklet! ` +
        `- Make sure react-native-worklets-core is installed properly! \n` +
        `- Make sure to add the react-native-worklets-core babel plugin to your babel.config.js! \n` +
        `- Make sure that no other plugin overrides the react-native-worklets-core babel plugin! \n` +
        `- Make sure the function ${func.name} is decorated with the 'worklet' directive! \n` +
        `Expected ${func.name} to contain __workletHash and __initData, but ${
          func.name
        } has these properties: ${Object.keys(func)}`
    );
  }
}

/**
 * Ensures the given function is a Worklet, and throws an error if not.
 * @param func The function that should be a Worklet.
 * @returns The same function that was passed in.
 */
export function worklet<TFunc extends () => any>(
  func: TFunc
): Workletize<TFunc> {
  if (!isWorklet(func)) {
    throw new NotAWorkletError(func);
  }
  return func;
}
