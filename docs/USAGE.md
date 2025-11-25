# Usage

react-native-worklets-core can be used either as a standalone library to run Worklets on a background context, or as an integration with another third-party library to run code on a custom context.

## Standalone

To run any JS Function on a background Thread, convert it to a worklet. As an example, this is how you can compute the fibonacci sequence in JS, without blocking the main JS Thread:

```js
const fibonacci = (num: number): number => {
  'worklet'
  if (num <= 1) return num;
  let prev = 0, curr = 1;
  for (let i = 2; i <= num; i++) {
    let next = prev + curr;
    prev = curr;
    curr = next;
  }
  return curr;
}

const context = Worklets.defaultContext
const result = await context.runAsync(() => {
  'worklet'
  return fibonacci(50)
})
console.log(`Fibonacci of 50 is ${result}`)
```

Use `runOnJS` to call back to JS:

```js
const [age, setAge] = useState(30)

function something() {
  'worklet'
  Worklets.runOnJS(() => setAge(50))
}
```

### Memoize

Both the `runAsync` and `runOnJS` functions are convenience methods for `createRunAsync` and `createRunOnJS`. For frequent calls, prefer the `prepare...` equivalent instead to memoize the caller function.

### Hooks

Worklets also provides three utility hooks:

#### `useWorklet`

Uses a memoized Worklet that can be called from the JS context, or from within another Worklet context:

```ts
function App() {
  const worklet = useWorklet('default', () => {
    'worklet'
    console.log("hello from worklet!")
  }, [])

  worklet()
}
```

#### `useRunOnJS`

Uses a memoized callback to the JS context that can be called from within a Worklet context:

```ts
function App() {
  const sayHello = useRunOnJS(() => {
    console.log("hello from JS!")
  }, [])

  const worklet = useWorklet('default', () => {
    'worklet'
    console.log("hello from worklet!")
    sayHello()
  }, [sayHello])

  worklet()
}
```

#### `useSharedValue`

Uses a SharedValue instance that can be read from- and written to by both a JS context and a Worklet context at the same time:

```ts
function App() {
  const something = useSharedValue(5)
  const worklet = useWorklet('default', () => {
    'worklet'
    something.value = Math.random()
  }, [something])
}
```

The SharedValue will create a C++ based Proxy implementation for Arrays and Objects, so that any read- or write-operations on the Array/Object are thread-safe.

### Separate Contexts

You can also create specific contexts (Threads) to run Worklets on:

```js
const context1 = Worklets.createContext('my-new-thread-1')
context1.runAsync(() => {
  'worklet'
  console.log("Hello from context #1!")
})
```

...and even nest them without ever crossing the JavaScript Thread:

```js
const context1 = Worklets.createContext('my-new-thread-1')
const context2 = Worklets.createContext('my-new-thread-2')
context1.runAsync(() => {
  'worklet'
  console.log("Hello from context #1!")
  context2.runAsync(() => {
    'worklet'
    console.log("Hello from context #2!")
  })
})
```

## Integration

To integrate react-native-worklets-core in your library, first install the package:

### Dependency Installation

#### iOS:

1. Add `react-native-worklets-core` to your podspec:
    ```ruby
    s.dependency "react-native-worklets-core"
    ```

#### Android:

1. Add `react-native-worklets-core` to your `build.gradle`:
    ```groovy
    implementation project(":react-native-worklets-core")
    ```
2. Add `react-native-worklets-core` to your `CMakeLists.txt`:
    ```CMake
    find_package(react-native-worklets-core REQUIRED CONFIG)

    # ...

    target_link_libraries(
        ${PACKAGE_NAME}
        # ... other libs
        react-native-worklets-core::rnworklets
    )
    ```

Then, from C++ you can convert Functions to Worklets and call them on any Thread:

### Usage

1. Include the Headers:
    ```cpp
    #include <JsiWorkletContext.h>
    #include <JsiWorklet.h>
    ```
2. On the main JavaScript Thread (`mqt_js`), create your Worklet Context:
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

