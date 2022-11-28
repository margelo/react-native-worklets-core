"use strict";

const generate = require("@babel/generator").default;
// const hash = require('string-hash-64');
const traverse = require("@babel/traverse").default;
const parse = require("@babel/parser").parse;

function buildWorkletString(t, fun, closureVariables, name) {
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
            t.identifier("this")
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

  return generate(workletFunction, { compact: true }).code;
}

function processWorkletFunction(t, fun) {
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
      const parentNode = path.parent;

      if (
        parentNode.type === "MemberExpression" &&
        parentNode.object !== path.node
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
      let found = false;
      while (currentScope != null) {
        if (currentScope.bindings[name] != null) {
          found = true;
          break;
        }
        currentScope = currentScope.parent;
      }

      // Check if it exists on global object
      if (!global.hasOwnProperty(name) && !found) {
        closure.set(name, path.node);
      }
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

  const variables = Array.from(closure.values());
  const privateFunctionId = t.identifier("_" + functionName);

  console.log("Variables", variables);
  console.log("Location", fun.node.loc, state.file.opts.filename);

  // if we don't clone other modules won't process parts of newFun defined below
  // this is weird but couldn't find a better way to force transform helper to
  // process the function
  const clone = t.cloneNode(fun.node);
  const funExpression = t.functionExpression(null, clone.params, clone.body);

  const funString = buildWorkletString(t, fun, variables, functionName);
  // const workletHash = hash(funString);

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
          t.memberExpression(privateFunctionId, t.identifier("_scope"), false),
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
          t.memberExpression(privateFunctionId, t.identifier("_code"), false),
          t.stringLiteral(funString)
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

function processIfWorkletNode(t, p) {
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
          processWorkletFunction(t, fun);
        }
      }
    },
  });
}

module.exports = function ({ types: t }) {
  return {
    visitor: {
      "FunctionDeclaration|FunctionExpression|ArrowFunctionExpression": {
        exit(path) {
          processIfWorkletNode(t, path);
        },
      },
    },
  };
};
