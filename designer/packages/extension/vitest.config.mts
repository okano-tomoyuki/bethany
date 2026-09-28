import { defineProject } from 'vitest/config';

export default defineProject({
  test: {
    name: 'extension',
    // vscode に依存しない処理だけをテストする(vscode のモジュールは拡張のホストの中にしか無い)
    include: ['src/**/*.test.ts'],
  },
});
