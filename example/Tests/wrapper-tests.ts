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

const array_push: Test = () => {
  const array = Worklets.createSharedValue([100, 200]);
  array.value.push(300);
  return ExpectValue(array.value[2], 300);
};

const array_pop: Test = () => {
  const array = Worklets.createSharedValue([100, 200]);
  array.value.pop();
  return ExpectValue(array.value.length, 1);
};

const array_forEach: Test = () => {
  const array = Worklets.createSharedValue([100, 200]);
  let sum = 0;
  array.value.forEach(value => {
    sum += value;
  });
  return ExpectValue(sum, 300);
};

const array_filter: Test = () => {
  const array = Worklets.createSharedValue([100, 200]);
  const lessThan150 = array.value.filter(value => value < 150);
  return ExpectValue(lessThan150.length, 1);
};

const array_map: Test = () => {
  const array = Worklets.createSharedValue([100, 200]);
  const copy = array.value.map(value => value);
  return ExpectValue(copy, [100, 200]);
};

const array_iterator: Test = () => {
  const array = Worklets.createSharedValue([100, 200]);
  let sum = 0;
  for (const value of array.value) {
    sum += value;
  }
  return ExpectValue(sum, 300);
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
  array_push,
  array_pop,
  array_forEach,
  array_filter,
  array_map,
  array_iterator,
  convert_array: convert([123, 'abc']),
  convert_array_of_objects: convert([
    {x: 1, y: 2},
    {x: 5, y: 12},
  ]),
  convert_function_throws_exception,
};
