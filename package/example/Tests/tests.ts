import { worklet_tests } from "./worklet-tests";
import { sharedvalue_tests } from "./sharedvalue-tests";
import { wrapper_tests } from "./wrapper-tests";
import { worklet_context_tests } from "./worklet-context-tests";
import { native_contexts_tests } from "./native-contexts-tests";

export const Tests: { [key: string]: { [key: string]: () => Promise<void> } } =
  {
    NativeContexts: { ...native_contexts_tests },
    Worklets: { ...worklet_tests },
    Contexts: { ...worklet_context_tests },
    SharedValues: { ...sharedvalue_tests },
    WrapperTests: { ...wrapper_tests },
  };
