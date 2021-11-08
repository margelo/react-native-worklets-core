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

import {Colors, Header} from 'react-native/Libraries/NewAppScreen';

import {Worklets, useWorklet} from 'react-native-worklets';

const App = () => {
  const factor = useMemo(() => 1.5, []);
  const values = useMemo(() => [1, 2, 3, 5], []);
  const callCount = useMemo(() => Worklets.createSharedValue(0), []);

  const calculateFactor = useWorklet(
    (ctx, a: number) => {
      ctx.callCount.value++;
      ctx.values.forEach(p => {
        a *= p;
      });
      return a * ctx.factor;
    },
    {factor, callCount, values},
  );

  const [value, setValue] = React.useState<number>(0);
  const [isRunning, setIsRunning] = React.useState(false);
  const startWorklet = useCallback(() => {
    setIsRunning(true);
    for (let i = 0; i < 100; i++) {
      calculateFactor(i)
        .then(b => {
          if (i === 99) {
            setIsRunning(false);
          }
          setValue(b);
        })
        .catch(e => Alert.alert(e));
    }
  }, [calculateFactor]);

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

export default App;
