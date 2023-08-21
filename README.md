<a href="https://margelo.io">
  <img src="./img/banner.svg" width="100%" />
</a>

# 🧵 react-native-worklets-core

A [Worklet](docs/WORKLETS.md) runner for React Native.

```js
const worklet = () => {
  'worklet'
  return Math.random()
}
```

> [!NOTE]
> In most cases, react-native-worklets-core shouldn't be used as a standalone dependency but rather as a peer-dependency for other modules such as [react-native-vision-camera](https://github.com/mrousavy/react-native-vision-camera), [react-native-wishlist](https://github.com/margelo/react-native-wishlist), or [react-native-skia](https://github.com/Shopify/react-native-skia).

## Installation

1. Install the library from npm:
    ```sh
    yarn add react-native-worklets-core
    ```
2. Add the babel plugin to your `babel.config.js`:
    ```js
    module.exports = {
      plugins: [
        ["react-native-worklets-core/plugin"],
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

## Credits

* Credits go to [Christian Falch](https://github.com/chrfalch) for building the initial version of this library. You're amazing! 🤩
* Credits go to [Software Mansion](https://swmansion.com) for introducing the concept of [Worklets](https://docs.swmansion.com/react-native-reanimated/docs/fundamentals/worklets) in [Reanimated v2](https://github.com/software-mansion/react-native-reanimated). This library is inspired by Reanimated v2 with a few structural changes to the Worklets architecture to make it more flexible for different use-cases, such as integrating it with other C++ libraries like VisionCamera and WishList.
