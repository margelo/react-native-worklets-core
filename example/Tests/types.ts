export type TestState = "notrun" | "running" | "success" | "failure";

export type TestInfo = {
  run: () => Promise<void>;
  name: string;
  category: string;
  state: TestState;
};
