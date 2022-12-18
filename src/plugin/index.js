// Simplified version of https://github.com/software-mansion/react-native-reanimated/blob/1ba563a61263a6a4802517bc9509d3d8cf5b5036/plugin.js

"use strict";

const generate = require("@babel/generator").default;
const traverse = require("@babel/traverse").default;
const { transformSync } = require("@babel/core");

const globals = new Set([]);
const functionArgsToWorkletize = new Map([]);

function buildWorkletString(t, fun, closureVariables, name, inputMap) {
  function prependClosureVariablesIfNecessary() {
    const closureDeclaration = t.variableDeclaration("const", [
      t.variableDeclarator(
        t.objectPattern(
          closureVariables
            // We shouldn't copy the worklets API to the closure -
            // it will be installed in each worklet runtime separately.
            // https://github.com/chrfalch/react-native-worklets/issues/24
            .filter((v) => v.name !== "Worklets")
            .map((variable) =>
              t.objectProperty(
                t.identifier(variable.name),
                t.identifier(variable.name),
                false,
                true
              )
            )
        ),
        t.identifier("jsThis")
      ),
    ]);

    function prependClosure(path) {
      if (closureVariables.length === 0 || path.parent.type !== "Program") {
        return;
      }

      path.node.body.body.unshift(closureDeclaration);
    }

    return {
      visitor: {
        "FunctionDeclaration|FunctionExpression|ArrowFunctionExpression|ObjectMethod":
          (path) => {
            prependClosure(path);
          },
      },
    };
  }

  const expression =
    fun.program.body.find(({ type }) => type === "FunctionDeclaration") ||
    fun.program.body.find(({ type }) => type === "ExpressionStatement")
      .expression;

  const workletFunction = t.functionExpression(
    t.identifier(name),
    expression.params,
    expression.body
  );

  const code = generate(workletFunction).code;

  const transformed = transformSync(code, {
    plugins: [prependClosureVariablesIfNecessary()],
    compact: true,
    sourceMaps: true,
    inputSourceMap: inputMap,
    ast: false,
    babelrc: false,
    configFile: false,
    comments: false,
  });

  return [transformed.code];
}

function makeWorkletName(t, fun) {
  if (t.isObjectMethod(fun)) {
    return fun.node.key.name;
  }
  if (t.isFunctionDeclaration(fun)) {
    return fun.node.id.name;
  }
  if (t.isFunctionExpression(fun) && t.isIdentifier(fun.node.id)) {
    return fun.node.id.name;
  }
  return "anonymous"; // fallback for ArrowFunctionExpression and unnamed FunctionExpression
}

function makeWorklet(t, fun, state) {
  // Returns a new FunctionExpression which is a workletized version of provided
  // FunctionDeclaration, FunctionExpression, ArrowFunctionExpression or ObjectMethod.

  const functionName = makeWorkletName(t, fun);

  const closure = new Map();

  // remove 'worklet'; directive before generating string
  fun.traverse({
    DirectiveLiteral(path) {
      if (path.node.value === "worklet" && path.getFunctionParent() === fun) {
        path.parentPath.remove();
      }
    },
  });

  // We use copy because some of the plugins don't update bindings and
  // some even break them

  const codeObject = generate(fun.node, {
    sourceMaps: true,
    sourceFileName: state.file.opts.filename,
  });

  // We need to add a newline at the end, because there could potentially be a
  // comment after the function that gets included here, and then the closing
  // bracket would become part of the comment thus resulting in an error, since
  // there is a missing closing bracket.
  const code =
    "(" + (t.isObjectMethod(fun) ? "function " : "") + codeObject.code + "\n)";

  const transformed = transformSync(code, {
    filename: state.file.opts.filename,
    presets: ["@babel/preset-typescript"],
    plugins: [
      "@babel/plugin-transform-shorthand-properties",
      "@babel/plugin-transform-arrow-functions",
      ["@babel/plugin-proposal-optional-chaining", { loose: true }],
      ["@babel/plugin-proposal-nullish-coalescing-operator", { loose: true }],
      ["@babel/plugin-transform-template-literals", { loose: true }],
    ],
    ast: true,
    babelrc: false,
    configFile: false,
    inputSourceMap: codeObject.map,
  });

  traverse(transformed.ast, {
    ReferencedIdentifier(path) {
      const name = path.node.name;
      if (
        global.hasOwnProperty(name) ||
        globals.has(name) ||
        (fun.node.id && fun.node.id.name === name)
      ) {
        return;
      }

      const parentNode = path.parent;

      if (
        parentNode.type === "MemberExpression" &&
        parentNode.property === path.node &&
        !parentNode.computed
      ) {
        return;
      }

      if (
        parentNode.type === "ObjectProperty" &&
        path.parentPath.parent.type === "ObjectExpression" &&
        path.node !== parentNode.value
      ) {
        return;
      }

      let currentScope = path.scope;

      while (currentScope != null) {
        if (currentScope.bindings[name] != null) {
          return;
        }
        currentScope = currentScope.parent;
      }
      closure.set(name, path.node);
    },
  });

  const variables = Array.from(closure.values());

  const privateFunctionId = t.identifier("_f");
  const clone = t.cloneNode(fun.node);
  let funExpression;
  if (clone.body.type === "BlockStatement") {
    funExpression = t.functionExpression(null, clone.params, clone.body);
  } else {
    funExpression = clone;
  }

  const [funString] = buildWorkletString(
    t,
    transformed.ast,
    variables,
    functionName,
    transformed.map
  );

  let location = state.file.opts.filename;
  if (state.opts && state.opts.relativeSourceLocation) {
    const path = require("path");
    location = path.relative(state.cwd, location);
  }

  const loc = fun && fun.node && fun.node.loc && fun.node.loc.start;
  if (loc) {
    const { line, column } = loc;
    if (typeof line === "number" && typeof column === "number") {
      location = `${location} (${line}:${column})`;
    }
  }

  const statements = [
    t.variableDeclaration("const", [
      t.variableDeclarator(privateFunctionId, funExpression),
    ]),
    t.expressionStatement(
      t.assignmentExpression(
        "=",
        t.memberExpression(privateFunctionId, t.identifier("_closure"), false),
        t.objectExpression(
          variables.map((variable) =>
            t.objectProperty(t.identifier(variable.name), variable, false, true)
          )
        )
      )
    ),
    t.expressionStatement(
      t.assignmentExpression(
        "=",
        t.memberExpression(privateFunctionId, t.identifier("asString"), false),
        t.stringLiteral(funString)
      )
    ),
    t.expressionStatement(
      t.assignmentExpression(
        "=",
        t.memberExpression(
          privateFunctionId,
          t.identifier("__location"),
          false
        ),
        t.stringLiteral(location)
      )
    ),
  ];

  statements.push(t.returnStatement(privateFunctionId));

  const newFun = t.functionExpression(fun.id, [], t.blockStatement(statements));

  return newFun;
}

function processWorkletFunction(t, fun, state) {
  // Replaces FunctionDeclaration, FunctionExpression or ArrowFunctionExpression
  // with a workletized version of itself.

  if (!t.isFunctionParent(fun)) {
    return;
  }

  const newFun = makeWorklet(t, fun, state);

  const replacement = t.callExpression(newFun, []);

  // we check if function needs to be assigned to variable declaration.
  // This is needed if function definition directly in a scope. Some other ways
  // where function definition can be used is for example with variable declaration:
  // const ggg = function foo() { }
  // ^ in such a case we don't need to define variable for the function
  const needDeclaration =
    t.isScopable(fun.parent) || t.isExportNamedDeclaration(fun.parent);
  fun.replaceWith(
    fun.node.id && needDeclaration
      ? t.variableDeclaration("const", [
          t.variableDeclarator(fun.node.id, replacement),
        ])
      : replacement
  );
}

function processIfWorkletNode(t, fun, state) {
  fun.traverse({
    DirectiveLiteral(path) {
      const value = path.node.value;
      if (value === "worklet" && path.getFunctionParent() === fun) {
        // make sure "worklet" is listed among directives for the fun
        // this is necessary as because of some bug, babel will attempt to
        // process replaced function if it is nested inside another function
        const directives = fun.node.body.directives;
        if (
          directives &&
          directives.length > 0 &&
          directives.some(
            (directive) =>
              t.isDirectiveLiteral(directive.value) &&
              directive.value.value === "worklet"
          )
        ) {
          processWorkletFunction(t, fun, state);
        }
      }
    },
  });
}

function processWorklets(t, path, state) {
  const name =
    path.node.callee.type === "MemberExpression"
      ? path.node.callee.property.name
      : path.node.callee.name;

  const indexes = functionArgsToWorkletize.get(name);
  if (Array.isArray(indexes)) {
    indexes.forEach((index) => {
      processWorkletFunction(t, path.get(`arguments.${index}`), state);
    });
  }
}

module.exports = function ({ types: t }) {
  return {
    pre() {
      // Extra globals.
      this.opts?.globals?.forEach((name) => {
        globals.add(name);
      });
      // Function arguments that will be automatically transformed to worklets.
      // The format is [{ name: functionName, args: [argumentIndex1, argumentIndex2, ...]}, ...]
      // For example, [{ name: 'useWorklet', args: [0] }] will transform the first argument of functions called useWorklet
      // to a worklet automatically without needed to add the "worklet" directive.
      this.opts?.functionsToWorkletize?.forEach(({ name, args }) => {
        functionArgsToWorkletize.set(name, args);
      });
    },
    visitor: {
      "CallExpression": {
        enter(path, state) {
          processWorklets(t, path, state);
        },
      },
      "FunctionDeclaration|FunctionExpression|ArrowFunctionExpression": {
        enter(path, state) {
          processIfWorkletNode(t, path, state);
        },
      },
    },
  };
};
