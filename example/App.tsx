import React, {useCallback, useMemo} from 'react';
import {
  Alert,
  Button,
  SafeAreaView,
  ScrollView,
  StatusBar,
  Text,
  useColorScheme,
  View,
} from 'react-native';

import {Colors} from 'react-native/Libraries/NewAppScreen';

import {Worklets} from 'react-native-worklets';

const App = () => {
  const factor = useMemo(() => 1.5, []);
  const values = useMemo(() => [1, 2, 3, 5], []);
  const callCount = useMemo(() => Worklets.createSharedValue(0), []);

  const logToConsole = useMemo(
    () =>
      Worklets.createWorklet({}, (_, message: string) => {
        setMessage(message);
      }),
    [],
  );

  const calculateFactor = useMemo(
    () =>
      Worklets.createWorklet(
        {factor, callCount, values, logToConsole},
        (ctx, a: number) => {
          ctx.callCount.value++;
          ctx.values.forEach(p => {
            a *= p;
          });
          return a * ctx.factor;
        },
      ),
    [callCount, factor, logToConsole, values],
  );

  const callBackToJS = useMemo(
    () =>
      Worklets.createWorklet({logToConsole}, ctx => {
        ctx.logToConsole.runOnJsThread('Hello from the worklet');
      }),
    [logToConsole],
  );

  const [calculationResult, setCalculationResult] = React.useState<number>(0);
  const [message, setMessage] = React.useState<string>('');
  const [isRunning, setIsRunning] = React.useState(false);

  const reset = useCallback(() => {
    setCalculationResult(0);
    setMessage('');
  }, []);

  const runWorkletInWorkletRuntime = useCallback(() => {
    setIsRunning(true);
    reset();
    for (let i = 0; i < 100; i++) {
      calculateFactor
        .runOnWorkletThread(i)
        .then(b => {
          if (i === 99) {
            setIsRunning(false);
          }
          setCalculationResult(b);
        })
        .catch(e => Alert.alert(e));
    }
  }, [calculateFactor, reset]);

  const runWorkletInMainRuntime = useCallback(() => {
    reset();
    setIsRunning(true);
    let b = 0;
    for (let i = 0; i < 100; i++) {
      b = calculateFactor.runOnJsThread(i);
    }
    setIsRunning(false);
    setCalculationResult(b);
  }, [calculateFactor, reset]);

  const runWorkletInWorkletRuntimeWithCallbackOnJsRuntime = useCallback(() => {
    reset();
    callBackToJS.runOnWorkletThread();
  }, [callBackToJS, reset]);

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
        <View>
          <Text>{`Result: ${calculationResult} (called ${callCount.value} times)`}</Text>
          <Text>{`Message: ${message}`}</Text>
        </View>
        <View>
          <Button
            title="Run worklet in Worklet runtime"
            onPress={runWorkletInWorkletRuntime}
            disabled={isRunning}
          />
        </View>
        <View>
          <Button
            title="Run worklet in main runtime"
            onPress={runWorkletInMainRuntime}
            disabled={isRunning}
          />
        </View>
        <View>
          <Button
            title="Run worklet in worklet runtime with callback on Js runtime"
            onPress={runWorkletInWorkletRuntimeWithCallbackOnJsRuntime}
            disabled={isRunning}
          />
        </View>
      </ScrollView>
    </SafeAreaView>
  );
};

export default App;
