import NativeSampleModule from "sample-native-module/js/NativeSampleModule";
import { Worklets, type IWorkletContext } from "react-native-worklets-core";
import { Platform } from "react-native";

export const native_contexts_tests = {
  // This test has to run first, so that the default context / RNWC API isn't initialized yet from the JS side
  call_custom_worklet_context_first: async () => {
    // Check if the native module is available
    if (NativeSampleModule == null) {
      if (Platform.OS === "android") {
        console.warn(
          "NativeSampleModule is only implemented on iOS currently."
        );
        return;
      } else {
        throw new Error(
          "NativeSampleModule is not available. Something went wrong with the native module setup."
        );
      }
    }

    // Note to library authors: We call on our custom native context before the Worklets API is initialized.
    // RNWC is a lazy-loaded turbo module, so it will only be available once we call it in JS once.
    // We call any property on the Worklets object to initialize it:
    Worklets.defaultContext;

    const context =
      NativeSampleModule.createCustomWorkletContext() as IWorkletContext;
    // Bug: this will never resolve (as it throws an error, which is swallowed [which is another bug])
    await context.runAsync(() => {
      "worklet";
      console.log("ctx: test: running worklet");
      return 100;
    });
  },
};
