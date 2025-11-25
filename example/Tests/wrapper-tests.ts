import { Worklets } from "react-native-worklets-core";
import { ExpectValue } from "./utils";

const convert =
  <T>(value: T): (() => Promise<void>) =>
  () => {
    const a = Worklets.createSharedValue(value).value;
    const b = value;
    return ExpectValue(a, b);
  };

export const wrapper_tests = {
  convert_undefined: convert(undefined),
  convert_null: convert(null),
  convert_number: convert(123),
  convert_string: convert("abc"),
  convert_boolean: convert(true),
  convert_object: convert({ a: 123, b: "abc", child: { x: 5, y: 23 } }),
  convert_object_with_children: convert({
    a: 123,
    b: "abc",
    children: [
      { x: 5, y: 23 },
      { x: 5, y: 23 },
      { x: 5, y: 23 },
    ],
  }),

  array_is_array: () => {
    return ExpectValue(
      Array.isArray(Worklets.createSharedValue([]).value),
      true
    );
  },

  array_instanceof_array: () => {
    return ExpectValue(
      Worklets.createSharedValue([]).value instanceof Array,
      true
    );
  },

  array_get: () => ExpectValue(Worklets.createSharedValue([100]).value[0], 100),

  array_set: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value[0] = 300;
    return ExpectValue(array.value[0], 300);
  },

  array_push: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.push(300);
    return ExpectValue(array.value[2], 300);
  },

  array_push_length: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.push(300);
    return ExpectValue(array.value.length, 3);
  },

  array_push_multiple: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.push(300, 400);
    return ExpectValue(array.value.length, 4);
  },

  array_pop: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.pop();
    return ExpectValue(array.value.length, 1);
  },

  array_pop_values: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.pop();
    return ExpectValue(array.value[0], 100);
  },

  array_shift: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.shift();
    return ExpectValue(array.value.length, 1);
  },

  array_shift_return: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const retVal = array.value.shift();
    return ExpectValue(retVal, 100);
  },

  array_shift_return_undefined: () => {
    const array = Worklets.createSharedValue([]);
    const retVal = array.value.shift();
    return ExpectValue(retVal, undefined);
  },

  array_shift_values: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.shift();
    return ExpectValue(array.value[0], 200);
  },

  array_unshift: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.unshift(300);
    return ExpectValue(array.value[0], 300);
  },

  array_unshift_length: () => {
    const array = Worklets.createSharedValue([100, 200]);
    array.value.unshift(300, 500);
    return ExpectValue(array.value.length, 4);
  },

  array_forEach: () => {
    const array = Worklets.createSharedValue([100, 200]);
    let sum = 0;
    array.value.forEach((value) => {
      sum += value;
    });
    return ExpectValue(sum, 300);
  },

  array_forEach_with_index_and_array_params: () => {
    const array = Worklets.createSharedValue([100, 200]);
    let sum = 0;
    array.value.forEach((_, index, arr) => {
      sum += arr[index] ?? 0;
    });
    return ExpectValue(sum, 300);
  },

  array_filter: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const lessThan150 = array.value.filter((value) => value < 150);
    return ExpectValue(lessThan150.length, 1);
  },

  array_filter_with_index_and_array_params: () => {
    const array = Worklets.createSharedValue([100, 200, 300]);
    const lessThan150 = array.value.filter((_, i, arr) => (arr[i] ?? 0) < 150);
    return ExpectValue(lessThan150.length, 1);
  },

  array_map: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const copy = array.value.map((value) => value);
    return ExpectValue(copy, [100, 200]);
  },

  array_map_with_index_and_arra_params: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const copy = array.value.map((_, i, arr) => arr[i]);
    return ExpectValue(copy, [100, 200]);
  },

  array_concat: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const next = [300, 400];
    return ExpectValue(array.value.concat(next).length, 4);
  },

  array_concat_multiple: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.concat(300, 400)[3], 400);
  },

  array_concat_not_mutate: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const next = array.value.concat([300, 400]);
    return ExpectValue(next === array.value, false);
  },

  array_find: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.find((p) => p === 100);
    return ExpectValue(result, 100);
  },

  array_find_with_index_and_array_params: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.find((_, i, arr) => arr[i] === 100);
    return ExpectValue(result, 100);
  },

  array_every: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.every((p) => p > 50);
    return ExpectValue(result, true);
  },

  array_every_index_array_params: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.every((_, i, arr) => (arr[i] ?? 0) > 50);
    return ExpectValue(result, true);
  },

  array_every_false: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.every((p) => p > 100);
    return ExpectValue(result, false);
  },

  array_some: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.some((p) => p < 200);
    return ExpectValue(result, true);
  },

  array_some_index_array_params: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.some((_, i, arr) => (arr[i] ?? 0) < 200);
    return ExpectValue(result, true);
  },

  array_some_false: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.some((p) => p > 300);
    return ExpectValue(result, false);
  },

  array_findIndex: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.findIndex((p) => p === 200);
    return ExpectValue(result, 1);
  },

  array_findIndex_with_index_array_params: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.findIndex((_, i, arr) => arr[i] === 200);
    return ExpectValue(result, 1);
  },

  array_findIndex_false: () => {
    const array = Worklets.createSharedValue([100, 200]);
    const result = array.value.findIndex((p) => p === 300);
    return ExpectValue(result, -1);
  },

  array_flat_length: () => {
    const array = Worklets.createSharedValue([
      100,
      200,
      [300, 400, [500, 600]],
    ]);
    const flattened = array.value.flat();
    return ExpectValue(flattened.length, 6);
  },

  array_flat_values: () => {
    const array = Worklets.createSharedValue([
      100,
      200,
      [300, 400, [500, 600]],
    ]);
    const flattened = array.value.flat();
    return ExpectValue(flattened[5], 600);
  },

  array_flat_with_depth: () => {
    const array = Worklets.createSharedValue([
      100,
      200,
      [300, 400, [500, 600]],
    ]);
    const flattened = array.value.flat(1);
    return ExpectValue(flattened.length, 4);
  },

  array_includes: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.includes(200), true);
  },

  array_includes_with_fromIndex_param: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.includes(100, 1), false);
  },

  array_includes_false: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.includes(900), false);
  },

  array_indexOf: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.indexOf(200), 1);
  },

  array_indexOf_with_fromIndex_param: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.indexOf(100, 1), -1);
  },

  array_indexOf_false: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.indexOf(900), -1);
  },

  array_join: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.join(), "100,200");
  },

  array_join_separator: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(array.value.join("+"), "100+200");
  },

  array_reduce: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(
      array.value.reduce((acc, cur, index) => {
        const retVal = { ...acc, [index]: cur };
        return retVal;
      }, {}),
      { 0: 100, 1: 200 }
    );
  },

  array_reduce_with_array_param: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(
      array.value.reduce((acc, _, index, a) => {
        const retVal = { ...acc, [index]: a[index] };
        return retVal;
      }, {}),
      { 0: 100, 1: 200 }
    );
  },

  array_reduce_without_initialvalue: () => {
    const array = Worklets.createSharedValue([100, 200]);
    return ExpectValue(
      // @ts-ignore
      array.value.reduce((acc, cur, index) => ({ ...acc, [index]: cur })),
      { 0: 100, 1: 200 }
    );
  },

  array_iterator: () => {
    const array = Worklets.createSharedValue([100, 200]);
    let sum = 0;
    for (const value of array.value) {
      sum += value;
    }
    return ExpectValue(sum, 300);
  },

  convert_array: convert([123, "abc"]),
  convert_array_of_objects: convert([
    { x: 1, y: 2 },
    { x: 5, y: 12 },
  ]),
};
