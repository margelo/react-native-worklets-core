import { AppRegistry, View } from "react-native";
import { name as appName } from "./app.json";
import { Worklets } from "react-native-worklets-core";

Worklets.createRunInContextFn(() => {
  "worklet";
})();

AppRegistry.registerComponent(appName, () => View);
