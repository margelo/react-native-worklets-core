const path = require("path");
const pak = require("../package.json");

module.exports = {
  dependencies: {
    [pak.name]: {
      root: path.join(__dirname, ".."),
    },
    "sample-native-module": {
      root: path.join(__dirname, "sample-native-module"),
    },
  },
};
