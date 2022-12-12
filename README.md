# react-native-worklets

Worklet runner for React Native

## Installation

```sh
npm install react-native-worklets
```

Remember to add the babel plugin to your babel config:

```js
module.exports = {
  plugins: ["react-native-worklets/plugin"],
};
```

## Usage

```js
const a = (b: number) => {
  "worklet";
  return b + 1;
};

const worklet = Worklets.createRunOnWorkletFn(a);
const result = await worklet(100);
assert(result === 101);
```

## Contributing

See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

## License

MIT

---

Made with [create-react-native-library](https://github.com/callstack/react-native-builder-bob)
