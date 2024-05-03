type AnyFunc = (...args: any[]) => any;

type Workletize<TFunc extends () => any> = TFunc & {
  __closure: Record<string, unknown>;
  __initData: {
    code: string;
    location: string;
    __sourceMap: string;
  };
  __workletHash: number;
};
const EXPECTED_KEYS: (keyof Workletize<AnyFunc>)[] = [
  "__closure",
  "__initData",
  "__workletHash",
];

/**
 * Checks whether the given function is a Worklet or not.
 */
export function isWorklet<TFunc extends AnyFunc>(
  func: TFunc
): func is Workletize<TFunc> {
  const maybeWorklet = func as Partial<Workletize<TFunc>> & TFunc;
  if (typeof maybeWorklet.__workletHash !== "number") return false;

  if (
    maybeWorklet.__closure == null ||
    typeof maybeWorklet.__closure !== "object"
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
    let funcName = func.name;
    if (funcName.length === 0) {
      funcName = func.toString();
    }

    const expected = `[${EXPECTED_KEYS.join(", ")}]`;
    const received = `[${Object.keys(func).join(", ")}]`;
    super(
      `The function "${funcName}" is not a Worklet! \n` +
        `- Make sure the function "${funcName}" is decorated with the 'worklet' directive! \n` +
        `- Make sure react-native-worklets-core is installed properly! \n` +
        `- Make sure to add the react-native-worklets-core babel plugin to your babel.config.js! \n` +
        `- Make sure that no other plugin overrides the react-native-worklets-core babel plugin! \n` +
        `Expected "${funcName}" to contain ${expected}, but "${funcName}" only has these properties: ${received}`
    );
  }
}

/**
 * Ensures the given function is a Worklet, and throws an error if not.
 * @param func The function that should be a Worklet.
 * @returns The same function that was passed in.
 */
export function worklet<TFunc extends AnyFunc>(func: TFunc): Workletize<TFunc> {
  if (!isWorklet(func)) {
    throw new NotAWorkletError(func);
  }
  return func;
}
