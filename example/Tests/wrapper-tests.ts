import {Worklets} from 'react-native-worklets';
import {Test} from './types';
import {ExpectException, ExpectValue} from './utils';

const convert =
  <T>(value: T): Test =>
  () => {
    const valueToTest = Worklets.createSharedValue(value);
    return ExpectValue(valueToTest.value, value);
  };

const convert_function_throws_exception: Test = () => {
  return ExpectException(() => {
    return Promise.resolve(Worklets.createSharedValue(() => {}));
  }, 'Regular javascript functions cannot be shared.');
};

const array_is_array: Test = () => {
  return ExpectValue(Array.isArray(Worklets.createSharedValue([])), true);
};

const array_get: Test = () =>
  ExpectValue(Worklets.createSharedValue([100]).value[0], 100);

const array_set: Test = () => {
  const array = Worklets.createSharedValue([100, 200]);
  array.value[0] = 300;
  return ExpectValue(array.value[0], 300);
};

export const wrapper_tests = {
  convert_undefined: convert(undefined),
  convert_null: convert(null),
  convert_number: convert(123),
  convert_string: convert('abc'),
  convert_boolean: convert(true),
  convert_object: convert({a: 123, b: 'abc', child: {x: 5, y: 23}}),
  convert_object_with_children: convert({
    a: 123,
    b: 'abc',
    children: [
      {x: 5, y: 23},
      {x: 5, y: 23},
      {x: 5, y: 23},
    ],
  }),
  array_is_array,
  array_get,
  array_set,
  convert_array: convert([123, 'abc']),
  convert_array_of_objects: convert([
    {x: 1, y: 2},
    {x: 5, y: 12},
  ]),
  convert_function_throws_exception,
};
