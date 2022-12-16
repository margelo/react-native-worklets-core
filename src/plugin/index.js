"use strict";

const generate = require("@babel/generator").default;
const traverse = require("@babel/traverse").default;
const parse = require("@babel/parser").parse;

const globals = new Set();
const functionsToWorkletize = new Map();

function buildWorkletString(t, fun, closureVariables, name, state) {
  fun.traverse({
    enter(path) {
      t.removeComments(path.node);
    },
  });

  let workletFunction;
  if (closureVariables.length > 0) {
    workletFunction = t.functionExpression(
      t.identifier(name),
      fun.node.params,
      t.blockStatement([
        t.variableDeclaration("const", [
          t.variableDeclarator(
            t.objectPattern(
              closureVariables.map((variable) =>
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
        ]),
        fun.get("body").node,
      ])
    );
  } else {
    workletFunction = t.functionExpression(
      t.identifier(name),
      fun.node.params,
      fun.get("body").node
    );
  }

  return generate(workletFunction, {
    compact: true,
    sourceMaps: true,
    sourceFileName: state.file.opts.filename,
  });
}

function processWorkletFunction(t, fun, state) {
  if (!t.isFunctionParent(fun)) {
    return;
  }

  let functionName = "_f";

  if (fun.node.id) {
    functionName = fun.node.id.name;
  }

  const closure = new Map();
  const outputs = new Set();

  // We use copy because some of the plugins don't update bindings and
  // some even break them
  const astWorkletCopy = parse("\n(" + fun.toString() + "\n)");
  traverse(astWorkletCopy, {
    ReferencedIdentifier(path) {
      const name = path.node.name;

      // Check if it exists on global object or is a user provided global.
      if (global.hasOwnProperty(name) || globals.has(name)) {
        return;
      }

      const parentNode = path.parent;

      if (
        parentNode.type === "MemberExpression" &&
        parentNode.object !== path.node &&
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
    AssignmentExpression(path) {
      // test for <somethin>.value = <something> expressions
      const left = path.node.left;
      if (
        t.isMemberExpression(left) &&
        t.isIdentifier(left.object) &&
        t.isIdentifier(left.property, { name: "value" })
      ) {
        outputs.add(left.object.name);
      }
    },
  });

  fun.traverse({
    DirectiveLiteral(path) {
      if (path.node.value === "worklet" && path.getFunctionParent() === fun) {
        path.parentPath.remove();
      }
    },
  });

  // const codeObject = generate(fun.node, {
  //   sourceMaps: true,
  //   sourceFileName: state.file.opts.filename,
  // });

  const variables = Array.from(closure.values());
  const privateFunctionId = t.identifier("_" + functionName);

  // if we don't clone other modules won't process parts of newFun defined below
  // this is weird but couldn't find a better way to force transform helper to
  // process the function
  const clone = t.cloneNode(fun.node);
  const funExpression = t.functionExpression(null, clone.params, clone.body);

  const { code: funString } = buildWorkletString(
    t,
    fun,
    variables,
    functionName,
    state
  );

  // console.log(transformed.code, "\n\n", funString, "\n");

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

  // const workletHash = hash(funString);
  // console.log(JSON.stringify(props, null, 2));

  const newFun = t.functionExpression(
    fun.id,
    [],
    t.blockStatement([
      t.variableDeclaration("const", [
        t.variableDeclarator(privateFunctionId, funExpression),
      ]),
      t.expressionStatement(
        t.assignmentExpression(
          "=",
          t.memberExpression(
            privateFunctionId,
            t.identifier("_closure"),
            false
          ),
          t.objectExpression(
            variables.map((variable) =>
              t.objectProperty(
                t.identifier(variable.name),
                variable,
                false,
                true
              )
            )
          )
        )
      ),
      t.expressionStatement(
        t.assignmentExpression(
          "=",
          t.memberExpression(
            privateFunctionId,
            t.identifier("asString"),
            false
          ),
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
      t.returnStatement(privateFunctionId),
    ])
  );

  const replacement = t.callExpression(newFun, []);
  // we check if function needs to be assigned to variable declaration.
  // This is needed if function definition directly in a scope. Some other ways
  // where function definition can be used is for example with variable declaration:
  // const ggg = function foo() { }
  // ^ in such a case we don't need to definte variable for the function
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

function processIfWorkletNode(t, p, state) {
  const fun = p;

  fun.traverse({
    DirectiveLiteral(path) {
      const value = path.node.value;
      if (value === "worklet" && path.getFunctionParent() === fun) {
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

function processFunctionsToWorkletize(t, path, state) {
  const name =
    path.node.callee.type === "MemberExpression"
      ? path.node.callee.property.name
      : path.node.callee.name;

  const indexes = functionsToWorkletize.get(name);
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
        functionsToWorkletize.set(name, args);
      });
    },
    visitor: {
      "CallExpression": {
        exit(path, state) {
          processFunctionsToWorkletize(t, path, state);
        },
      },
      "FunctionDeclaration|FunctionExpression|ArrowFunctionExpression": {
        exit(path, state) {
          processIfWorkletNode(t, path, state);
        },
      },
    },
  };
};
