import {worklet_tests} from './worklet-tests';
import {sharedvalue_tests} from './sharedvalue-tests';
import {wrapper_tests} from './wrapper-tests';
import {Test} from './types';

export const Tests: {[key: string]: {[key: string]: Test}} = {
  Worklets: {...worklet_tests},
  SharedValues: {...sharedvalue_tests},
  WrapperTests: {...wrapper_tests},
};
