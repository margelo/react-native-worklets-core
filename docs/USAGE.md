# Usage

react-native-worklets can be used either as a standalone library to run Worklets on a background context, or as an integration with another third-party library to run code on a custom context.

## Standalone

To run any JS Function on a background Thread, convert it to a worklet. As an example, this is how you can compute the fibonacci sequence in JS, without blocking the main JS Thread:

```js
const fibonacci = (num: number): number => {
  'worklet'
  if (num <= 1) return 1
  return fibonacci(num - 1) + fibonacci(num - 2)
}

const worklet = Worklets.createRunInContextFn(fibonacci)
const result = await worklet(50)
console.log(`Fibonacci of 50 is ${result}`)
```

Use `createRunInJsFn` to call back to JS:

```js
const setAgeJS = Worklets.createRunInJsFn(setAge)

function something() {
  'worklet'
  setAgeJS(50)
}
```

## Integration

To integrate react-native-worklets in your library, first install the package:

### Dependency Installation

#### iOS:

1. Add `react-native-worklets` to your podspec:
    ```ruby
    s.dependency "react-native-worklets"
    ```

#### Android:

1. Add `react-native-worklets` to your `build.gradle`:
    ```groovy
    implementation project(":react-native-worklets")
    ```
2. Add `react-native-worklets` to your `CMakeLists.txt`:
    ```CMake
    find_package(react-native-worklets REQUIRED CONFIG)

    # ...

    target_link_libraries(
        ${PACKAGE_NAME}
        # ... other libs
        react-native-worklets::rnworklets
    )
    ```

Then, from C++ you can convert Functions to Worklets and call them on any Thread:

### Usage

1. Include the Headers:
    ```cpp
    #include <JsiWorkletContext.h>
    #include <JsiWorklet.h>
    ```
2. Create a Worklet Context:
    ```cpp
    auto jsCallInvoker = ... // <-- facebook::react::CallInvoker
    auto customThread = ... // <-- your custom Thread

    auto runOnJS = [jsCallInvoker](std::function<void()>&& f) {
        // Run on React JS Runtime
        jsCallInvoker->invokeAsync(std::move(f));
    };
    auto runOnWorklet = [customThread](std::function<void()>&& f) {
        // Run on your custom Thread
        customThread->invokeAsync(std::move(f));
    };

    _workletContext = std::make_shared<RNWorklet::JsiWorkletContext>("MyLibrary");
    _workletContext->initialize("MyLibrary",
                                jsRuntime,
                                runOnJS,
                                runOnWorklet);
    ```
3. When a user passes a `jsi::Function` with the `'worklet'` directive, you can convert that to a worklet:
   ```cpp
   auto runtime = ... // <-- source jsi::Runtime (main JS)
   auto func = ... // <-- JS with 'worklet'

   auto worklet = std::make_shared<RNWorklet::JsiWorklet>(runtime, func);
   auto workletInvoker = std::make_shared<RNWorklet::WorkletInvoker>(worklet);
   ```
4. Call it on your custom Thread:
   ```cpp
   _workletContext->invokeOnWorkletThread([=](RNWorklet::JsiWorkletContext*,
                                             jsi::Runtime& rt) {
      jsi::Value someValue(true);
      workletInvoker->call(rt,
                           jsi::Value::undefined(),
                           &someValue,
                           1);
   });
   ```

