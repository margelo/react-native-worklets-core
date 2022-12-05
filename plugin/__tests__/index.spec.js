const { transform } = require("@babel/core");
const prettier = require("prettier");

const conf = {
  filename: "test.js",
  plugins: ["./plugin/index.js"],
};

describe("babel-plugin-preserve-jscontext-function-to-string", () => {
  it("should not decorate function when worklet key is not set in function", () => {
    const [, input, output] = transformCode(
      `var desc = { functions: {run: function run() {}}};
  test(desc);`,
      `var desc = {
    functions: {
      run: function run() {},
    },
  }
  test(desc)`
    );
    expect(output).toEqual(input);
  });

  it("should redeclare code when worklet key is set in function", () => {
    const [, output, expected] = transformCode(
      'var desc = { functions: {run: function run() {"worklet";}}};',
      `var desc = {
        functions: {
          run: (function () {
            const _run = function () {}
      
            _run._closure = {}
            _run.asString = "function run(){}"
            _run.__location =
              "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (3:9)"
            return _run
          })(),
        },
      };
    `
    );
    expect(expected).toEqual(output);
  });

  it("should not remove this keyword", () => {
    const [, output, expected] = transformCode(
      `var desc = { 
        functions: {
          run: function run() {"worklet"; this.stop();},
          stop: function stop () {"worklet";}
        }
      };`,
      `var desc = {
        functions: {
          run: (function () {
            const _run = function () {
              this.stop()
            }
      
            _run._closure = {}
            _run.asString = "function run(){this.stop();}"
            _run.__location =
              "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (3:9)"
            return _run
          })(),
          stop: (function () {
            const _stop = function () {}
      
            _stop._closure = {}
            _stop.asString = "function stop(){}"
            _stop.__location =
              "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (7:10)"
            return _stop
          })(),
        },
      };
    `
    );
    expect(output).toEqual(expected);
  });

  it("should redeclare code with dependencies", () => {
    const [, output, expected] = transformCode(
      `
      var abba = 100;
      var z = new JsValue<number>(100);
      function test() {
        "worklet";
        return abba+200+z.value;
      }`,
      `
      var abba = 100
      var z = new JsValue() < number > 100
      
      const test = (function () {
        const _test = function () {
          return abba + 200 + z.value
        }
      
        _test._closure = {
          abba,
          z,
        }
        _test.asString = "function test(){const{abba,z}=this;{return abba+200+z.value;}}"
        _test.__location =
          "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (3:0)"
        return _test
      })()
      `
    );
    expect(output).toEqual(expected);
  });

  it("should not add dependecy Infinity which is available in native", () => {
    const [, output, expected] = transformCode(
      `var a = new JsValue<number>(0);
       var desc = { functions: {run: function run() { "worklet"; if(a === Infinity) {      
       }; }}};`,
      `var a = new JsValue() < number > 0
      var desc = {
        functions: {
          run: (function () {
            const _run = function () {
              if (a === Infinity) {
              }
            }
      
            _run._closure = {
              a,
            }
            _run.asString = "function run(){const{a}=this;{if(a===Infinity){}}}"
            _run.__location =
              "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (4:9)"            
            return _run
          })(),
        },
      };`
    );
    expect(output).toEqual(expected);
  });

  it("should not add dependecy isNaN which is available in native", () => {
    const [, output, expected] = transformCode(
      `
    function run() { "worklet"; if(isNaN(0)){} };
    `,
      `const run = (function () {
        const _run = function () {
          if (isNaN(0)) {
          }
        }
      
        _run._closure = {}
        _run.asString = "function run(){if(isNaN(0)){}}"
        _run.__location =
          "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (1:0)"
        return _run
      })();`
    );
    expect(output).toEqual(expected);
  });

  it("should not add dependecies defined in the global object", () => {
    const [, output, expected] = transformCode(
      `
    function run() { "worklet"; if(setImmediate(() => {}, 0)){} };
    `,
      `const run = (function () {
        const _run = function () {
          if (setImmediate(() => {}, 0)) {
          }
        }
      
        _run._closure = {}
        _run.asString = "function run(){if(setImmediate(()=>{},0)){}}"
        _run.__location =
          "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (1:0)"
        return _run
      })();`
    );
    expect(output).toEqual(expected);
  });

  it("should support null coalescing operator", () => {
    const [, output, expected] = transformCode(
      `
    function run(obj) { "worklet"; return obj.foo ?? "default"; };
    `,
      `const run = (function () {
        const _run = function (obj) {
          return obj.foo ?? "default";
        }
      
        _run._closure = {}
        _run.asString = 'function run(obj){return obj.foo??"default";}'
        _run.__location =
          "/Users/christianfalch/repos/chrfalch/react-native-worklets/test.js (1:0)"
        return _run
      })();`
    );
    expect(output).toEqual(expected);
  });
});

function transformCode(input, expected) {
  const org = prettier.format(input, { semi: false, parser: "babel" });
  const { code } = transform(org, conf);
  const exp = expected
    ? prettier.format(expected, { semi: false, parser: "babel" })
    : "";
  const next = prettier.format(code, { semi: false, parser: "babel" });
  return [input, next, exp];
}
