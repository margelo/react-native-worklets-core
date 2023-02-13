# react-native-worklets

[Worklet](docs/WORKLETS.md) runner for React Native.

```js
const worklet = () => {
  'worklet'
  return Math.random()
}
```

## Installation

1. Install the library from npm:
    ```sh
    yarn add react-native-worklets
    ```
2. Add the babel plugin to your `babel.config.js`:
    ```js
    module.exports = {
      plugins: [
        ["react-native-worklets/plugin"],
        // ...
      ],
      // ...
    };
    ```
3. Restart Metro with clean cache:
    ```sh
    yarn start --reset-cache
    ```

## Usage

See [USAGE.md](docs/USAGE.md)

## Contributing

See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

## License

MIT

---

Made with [create-react-native-library](https://github.com/callstack/react-native-builder-bob)
