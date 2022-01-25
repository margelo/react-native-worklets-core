# react-native-worklets

## Notes:

Implements a simple API around calling javascript functions in separate Javascript threads/engines with value sharing. When a function is transferred to another JS runtime, the `toString` method of the function is called to get its source code. There is currently a bug in Hermes on iOS that prevents this from working correctly in debug mode.

The API for creating a worklet (API is up for discussion):

```ts
const sharedValue = Worklets.createSharedValue(0);
const worklet = Worklets.createWorklet(
  function (a: number) {
    this.sharedValue.value = a;
  },
  {sharedValue},
);
await worklet.callInWorkletContext(200)
assert(sharedValue.value === 200);
```

Sharing values between JS runtimes is done by using shared values. The goal of the project is to provide JS primitives, objects and arrays that are shareable without copying data.

## Example project
The example project is just a suite of tests to run that runs through all of the expected behaviour and tracks errors and failures.

## Reanimated
We're currently not using the worklet syntax when defining our worklets, but this can easily be added to the implementation and is done just to reduce the complexity of the project.
