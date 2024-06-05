const path = require("path");
const pak = require("../package.json");

module.exports = {
  presets: ["module:@react-native/babel-preset"],
  plugins: [
    [
      "module-resolver",
      {
        extensions: [".tsx", ".ts", ".js", ".json"],
        alias: {
          [pak.name]: path.join(__dirname, "..", pak.source),
          "sample-native-module": path.join(__dirname, "sample-native-module"),
        },
      },
    ],
    path.join(__dirname, "/../src/plugin"),
  ],
};
