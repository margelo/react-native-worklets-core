# Worklets

Worklets are small JavaScript functions that can be executed on a separate JavaScript Context.

## JavaScript Context

The JavaScript Context is the Runtime that runs your app's JavaScript. Usually there is only one JavaScript Context executing your app's JS bundle, which is called the "Main JS Thread" (`mqt_js`).

With react-native-worklets, a separate JavaScript Context can be created to offload some functions to a separate Thread.

This can be useful for libraries like Reanimated where Worklets are used as style updaters to run animations on the UI Thread, or VisionCamera to create Frame Processors which run on the Camera Thread. This comes with the benefit of being able to run JavaScript code in parallel.

## Usage

A Worklet can be created simply by adding the `'worklet'` directive to a function:

```js
const sayHello = () => {
  'worklet'
  console.log("Hello from the Worklet Thread!")
}
```

### Run on a background Thread

The function `sayHello` is now prepared to be executed on any Worklet context.
If you want to call `sayHello` on a default background Thread, build the worklet:

```js
const worklet = Worklets.createRunInContextFn(sayHello)
```

...and then call it:

```js
worklet()
```

### Parameters and Results

Worklets can take parameters and return results:

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

### Usage with integrations

Some third-party libraries integrate with react-native-worklets.

For example, [react-native-vision-camera](https://github.com/mrousavy/react-native-vision-camera) allows you to run any kind of realtime image processing inside a Worklet that gets called for every frame the camera sees.

For example, this runs a face detection algorithm at 60 FPS:

```jsx
const frameProcessor = useFrameProcessor((frame) => {
  'worklet'
  const result = detectFaces(frame)
  console.log(`Detected ${result.length} faces.`)
}, [])

return <Camera fps={60} frameProcessor={frameProcessor} />
```

The Camera library internally converts the Worklet and calls it, so the user only has to use the `'worklet'` directive.
