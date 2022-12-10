# react-native-worklets

## Notes:

Implements a simple API around calling javascript functions in separate Javascript threads/engines with value sharing. When a function is transferred to another JS runtime, the `asString` method of the function is called to get its source code.

```ts
const worklet = Worklets.createRunInContextFn(function (a: number) {
  "worklet";
  return Math.sqrt(a);
});
// Will run in the default context' thread / runtime
const a = await worklet(64);
assert(a === 8);
```

It is also possible to create multiple contexts:

```ts
const context = Worklets.createContext("test");
const worklet = Worklets.createRunInContextFn(function (a: number) {
  "worklet";
  return Math.sqrt(a);
}, context);

// Will run in the test-context' thread / runtime
const a = await worklet(64);
assert(a === 8);
```

If you want finer control over how contexts are setup, or want to integrate the library in your library, the C++ code implements a simple API for creating contexts and running functions in them.

## Shared Values

Sharing values between JS runtimes is done by using shared values. The goal of the project is to provide JS primitives, objects and arrays that are shareable without copying data.

## Example project

The example project is just a suite of tests to run that runs through all of the expected behaviour and tracks errors and failures.

## Reanimated

TODO: Compatibility with Reanimated
