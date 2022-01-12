import {Worklets} from 'react-native-worklets';
import {Expect, ExpectException, ExpectValue} from './utils';

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
    const sharedValue = Worklets.createSharedValue('hello world');
    sharedValue.value = 'hello worklet';
    return ExpectValue(sharedValue.value, 'hello worklet');
  },

  get_set_object_value: () => {
    const sharedValue = Worklets.createSharedValue({a: 100, b: 200});
    sharedValue.value.a = 50;
    sharedValue.value.b = 100;
    return ExpectValue(sharedValue.value, {a: 50, b: 100});
  },

  get_set_array_value: () => {
    const sharedValue = Worklets.createSharedValue([100, 50]);
    return ExpectValue(sharedValue.value[0], 100).then(() =>
      ExpectValue(sharedValue.value[1], 50),
    );
  },

  box_number_to_string: () => {
    const sharedValue = Worklets.createSharedValue(100);
    // @ts-ignore
    sharedValue.value = '100';
    return ExpectValue(sharedValue.value, '100');
  },

  box_string_to_number: () => {
    const sharedValue = Worklets.createSharedValue('100');
    // @ts-ignore
    sharedValue.value = 100;
    return ExpectValue(sharedValue.value, 100);
  },

  box_string_to_array: () => {
    const sharedValue = Worklets.createSharedValue('100');
    // @ts-ignore
    sharedValue.value = [100, 200];
    return ExpectValue(sharedValue.value, [100, 200]);
  },

  box_string_to_object: () => {
    const sharedValue = Worklets.createSharedValue('100');
    // @ts-ignore
    sharedValue.value = {a: 100, b: 200};
    return ExpectValue(sharedValue.value, {a: 100, b: 200});
  },

  box_array_to_object: () => {
    const sharedValue = Worklets.createSharedValue([100, 200]);
    // @ts-ignore
    sharedValue.value = {a: 100, b: 200};
    return ExpectValue(sharedValue.value, {a: 100, b: 200});
  },

  box_object_to_array: () => {
    const sharedValue = Worklets.createSharedValue({a: 100, b: 200});
    // @ts-ignore
    sharedValue.value = [100.34, 200];
    return ExpectValue(sharedValue.value, [100.34, 200]);
  },

  array_destructure: () => {
    const sharedValue = Worklets.createSharedValue([100, 200]);
    sharedValue.value = [...sharedValue.value];
    return ExpectValue(sharedValue.value, [100, 200]);
  },

  array_destructure_to_object: () => {
    const sharedValue = Worklets.createSharedValue([100, 200]);
    sharedValue.value = {...sharedValue.value};
    return ExpectValue(sharedValue.value, {0: 100, 1: 200});
  },

  array_length: () => {
    const sharedValue = Worklets.createSharedValue([100, 50]);
    return ExpectValue(sharedValue.value.length, 2);
  },

  object_value_destructure: () => {
    const sharedValue = Worklets.createSharedValue({a: 100, b: 200});
    sharedValue.value = {...sharedValue.value};
    return ExpectValue(sharedValue.value, {a: 50, b: 100});
  },

  object_value_spread: () => {
    const sharedValue = Worklets.createSharedValue<Record<string, number>>({
      a: 100,
      b: 200,
    });
    sharedValue.value = {...sharedValue.value, c: 300};
    return ExpectValue(sharedValue.value, {a: 50, b: 100, c: 300});
  },

  set_value_from_worklet: () => {
    const sharedValue = Worklets.createSharedValue('hello world');
    const worklet = Worklets.createWorklet(
      function () {
        this.sharedValue.value = 'hello worklet';
      },
      {sharedValue},
    );
    sharedValue.value = 'hello worklet';
    return Expect(worklet.callInWorkletContext, () =>
      sharedValue.value === 'hello worklet'
        ? undefined
        : `Expected ${"'hello worklet'"} but got ${sharedValue.value}`,
    );
  },

  set_function_with_error: () => {
    return ExpectException(() => {
      Worklets.createSharedValue(() => {});
    }, 'Regular javascript functions cannot be shared.');
  },

  add_listener: () => {
    const sharedValue = Worklets.createSharedValue(100);
    const didChange = Worklets.createSharedValue(false);
    const unsubscribe = sharedValue.addListener(() => (didChange.value = true));
    sharedValue.value = 50;
    unsubscribe();
    console.log(didChange.value);
    return ExpectValue(didChange.value, true);
  },

  add_listener_from_worklet_should_fail: () => {
    const sharedValue = Worklets.createSharedValue(100);
    const worklet = Worklets.createWorklet(
      function () {
        this.sharedValue.addListener(() => {});
      },
      {sharedValue},
    );
    return ExpectException(
      worklet.callInWorkletContext,
      'addListener can only be called from the main Javascript context and not from a worklet.',
    );
  },
};
