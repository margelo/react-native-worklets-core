/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * Generated with the TypeScript template
 * https://github.com/react-native-community/react-native-template-typescript
 *
 * @format
 */

import React, {useCallback, useMemo} from 'react';
import {
  Alert,
  Button,
  SafeAreaView,
  ScrollView,
  StatusBar,
  StyleSheet,
  Text,
  useColorScheme,
  View,
} from 'react-native';

import {Colors, Header} from 'react-native/Libraries/NewAppScreen';

import 'react-native-worklets';

interface ISharedValue<T> {
  get value(): T;
  set value(v: T);
  addListener(listener: () => void): () => void;
}

type ContextType = {
  [key: string]:
    | number
    | string
    | boolean
    | undefined
    | null
    | ISharedValue<any>
    | ContextType;
};
interface IWorkletNativeApi {
  installWorklet: <T extends (...args: any) => any>(
    worklet: T,
    contextName?: string,
  ) => void;
  createSharedValue: <T>(value: T) => ISharedValue<T>;
  runWorklet: <T extends (...args: any) => any>(
    worklet: T,
    contextName?: string,
  ) => (...args: Parameters<T>) => Promise<ReturnType<T>>;
  callbackToJavascript: <T extends (...args: any) => any>(worklet: T) => T;
  createWorklet: <C extends ContextType, T, A extends any[]>(
    context: C,
    worklet: (context: C, ...args: A) => T,
  ) => (...args: A) => Promise<T>;
}

declare global {
  var Worklets: IWorkletNativeApi;
}

const useWorklet = <D extends ContextType, T>(
  worklet: (ctx: D, ...args: any) => any,
  dependencies: D,
) =>
  useMemo(() => Worklets.createWorklet(dependencies, worklet), [dependencies]);

const App = () => {
  const factor = useMemo(() => 2.5, []);
  const callCount = useMemo(() => Worklets.createSharedValue(0), []);

  const calculateFactor = useWorklet(
    (ctx, a: number) => {
      ctx.callCount.value++;
      return a * ctx.factor;
    },
    {factor, callCount},
  );

  const [value, setValue] = React.useState<number>(0);
  const [isRunning, setIsRunning] = React.useState(false);
  const startWorklet = useCallback(() => {
    setIsRunning(true);
    calculateFactor(100)
      .then(b => {
        setIsRunning(false);
        setValue(b);
      })
      .catch(e => Alert.alert(e));
  }, []);

  const isDarkMode = useColorScheme() === 'dark';

  const backgroundStyle = {
    backgroundColor: isDarkMode ? Colors.darker : Colors.lighter,
  };

  return (
    <SafeAreaView style={backgroundStyle}>
      <StatusBar barStyle={isDarkMode ? 'light-content' : 'dark-content'} />
      <ScrollView
        contentInsetAdjustmentBehavior="automatic"
        style={backgroundStyle}>
        <Header />
        <View
          style={{
            backgroundColor: isDarkMode ? Colors.black : Colors.white,
          }}>
          <Text>Click here to run worklet:</Text>
          <Button title="Run" onPress={startWorklet} disabled={isRunning} />
          <Text>{`Result: ${value} (called ${callCount.value} times)`}</Text>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  sectionContainer: {
    marginTop: 32,
    paddingHorizontal: 24,
  },
  sectionTitle: {
    fontSize: 24,
    fontWeight: '600',
  },
  sectionDescription: {
    marginTop: 8,
    fontSize: 18,
    fontWeight: '400',
  },
  highlight: {
    fontWeight: '700',
  },
});

export default App;
