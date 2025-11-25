module.exports = {
  root: true,
  extends: ["@react-native", "prettier"],
  plugins: ["prettier"],
  rules: {
    "prettier/prettier": [
      "warn",
      {
        quoteProps: "consistent",
        singleQuote: false,
        tabWidth: 2,
        trailingComma: "es5",
        useTabs: false,
      },
    ],
  },
};
