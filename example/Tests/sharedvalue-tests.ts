import { Worklets } from "react-native-worklets-core";
import { Expect, ExpectException, ExpectValue } from "./utils";

export const sharedvalue_tests = {
  get_set_numeric_value: () => {
    const sharedValue = Worklets.createSharedValue(100);
    sharedValue.value = 50;
    return ExpectValue(sharedValue.value, 50);
  },

  get_set_decimal_value: () => {
    const sharedValue = Worklets.createSharedValue(3.14159);
    return ExpectValue(sharedValue.value, 3.14159);
  },

  get_set_bool_value: () => {
    const sharedValue = Worklets.createSharedValue(true);
    sharedValue.value = false;
    return ExpectValue(sharedValue.value, false);
  },

  get_set_string_value: () => {
    const sharedValue = Worklets.createSharedValue("hello world");
    sharedValue.value = "hello worklet";
    return ExpectValue(sharedValue.value, "hello worklet");
  },

  get_set_object_value: () => {
    const sharedValue = Worklets.createSharedValue({ a: 100, b: 200 });
    sharedValue.value.a = 50;
    sharedValue.value.b = 100;
    return ExpectValue(sharedValue.value, { a: 50, b: 100 });
  },

  get_set_array_value: () => {
    const sharedValue = Worklets.createSharedValue([100, 50]);
    return ExpectValue(sharedValue.value[0], 100).then(() =>
      ExpectValue(sharedValue.value[1], 50)
    );
  },

  is_object: () => {
    const sharedValue = Worklets.createSharedValue({ a: 100, b: 200 });
    return ExpectValue(typeof sharedValue.value === "object", true);
  },

  object_keys: () => {
    const sharedValue = Worklets.createSharedValue({ a: 100, b: 200 });
    return ExpectValue(Object.keys(sharedValue.value), ["a", "b"]);
  },

  object_values: () => {
    const sharedValue = Worklets.createSharedValue({ a: 100, b: 200 });
    return ExpectValue(Object.values(sharedValue.value), [100, 200]);
  },

  box_number_to_string: () => {
    const sharedValue = Worklets.createSharedValue(100);
    // @ts-ignore
    sharedValue.value = "100";
    return ExpectValue(sharedValue.value, "100");
  },

  box_string_to_number: () => {
    const sharedValue = Worklets.createSharedValue("100");
    // @ts-ignore
    sharedValue.value = 100;
    return ExpectValue(sharedValue.value, 100);
  },

  box_string_to_array: () => {
    const sharedValue = Worklets.createSharedValue("100");
    // @ts-ignore
    sharedValue.value = [100, 200];
    return ExpectValue(sharedValue.value, [100, 200]);
  },

  box_string_to_object: () => {
    const sharedValue = Worklets.createSharedValue("100");
    // @ts-ignore
    sharedValue.value = { a: 100, b: 200 };
    return ExpectValue(sharedValue.value, { a: 100, b: 200 });
  },

  box_array_to_object: () => {
    const sharedValue = Worklets.createSharedValue([100, 200]);
    // @ts-ignore
    sharedValue.value = { a: 100, b: 200 };
    return ExpectValue(sharedValue.value, { a: 100, b: 200 });
  },

  box_object_to_array: () => {
    const sharedValue = Worklets.createSharedValue({ a: 100, b: 200 });
    // @ts-ignore
    sharedValue.value = [100.34, 200];
    return ExpectValue(sharedValue.value, [100.34, 200]);
  },

  array_object_keys: () => {
    return ExpectValue(
      Object.keys(Worklets.createSharedValue([50, 21, 32]).value),
      ["0", "1", "2"]
    );
  },

  array_object_values: () => {
    return ExpectValue(
      Object.values(Worklets.createSharedValue([50, 21, 32]).value),
      [50, 21, 32]
    );
  },

  array_spread: () => {
    const sharedValue = Worklets.createSharedValue([100, 200]);
    const p = [...sharedValue.value];
    return ExpectValue(p, [100, 200]);
  },

  array_destructure: () => {
    const sharedValue = Worklets.createSharedValue([100, 200]);
    const [first] = [...sharedValue.value];
    return ExpectValue(first, 100);
  },

  array_destructure_to_object: () => {
    const sharedValue = Worklets.createSharedValue([100, 200]);
    const p = { ...sharedValue.value };
    return ExpectValue(p, { 0: 100, 1: 200 });
  },

  array_length: () => {
    const sharedValue = Worklets.createSharedValue([100, 50]);
    return ExpectValue(sharedValue.value.length, 2);
  },

  object_value_destructure: () => {
    const sharedValue = Worklets.createSharedValue({ a: 100, b: 200 });
    const { a, b } = { ...sharedValue.value };
    return ExpectValue({ a, b }, { a: 100, b: 200 });
  },

  object_value_spread: () => {
    const sharedValue = Worklets.createSharedValue({ a: 100, b: 200 });
    const p = { ...sharedValue.value };
    return ExpectValue(p, { a: 100, b: 200 });
  },

  set_value_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue("hello world");
    const worklet = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      sharedValue.value = "hello worklet";
    });
    sharedValue.value = "hello worklet";
    return Expect(worklet(), () =>
      sharedValue.value === "hello worklet"
        ? undefined
        : `Expected ${"'hello worklet'"} but got ${sharedValue.value}`
    );
  },

  set_function_when_calling_function: () => {
    return ExpectException(() => {
      Worklets.createSharedValue(() => {}).value();
    });
  },

  add_listener: () => {
    const sharedValue = Worklets.createSharedValue(100);
    const didChange = Worklets.createSharedValue(false);
    const unsubscribe = sharedValue.addListener(() => (didChange.value = true));
    sharedValue.value = 50;
    unsubscribe();
    return ExpectValue(didChange.value, true);
  },

  add_listener_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue(100);
    const didChange = Worklets.createSharedValue(false);
    const w = Worklets.defaultContext.createRunAsync(function () {
      "worklet";
      const unsubscribe = sharedValue.addListener(
        () => (didChange.value = true)
      );
      sharedValue.value = 50;
      unsubscribe();
      return didChange.value;
    });
    return ExpectValue(w(), true);
  },

  set_object_property_to_undefined_after_being_an_object: () => {
    const sharedValue = Worklets.createSharedValue({ a: { b: 200 } });
    // @ts-ignore
    sharedValue.value.a = undefined;
    return ExpectValue(sharedValue.value, { a: undefined });
  },
};
