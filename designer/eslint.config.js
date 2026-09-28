import js from '@eslint/js';
import prettier from 'eslint-config-prettier';
import globals from 'globals';
import { defineConfig } from 'eslint/config';
import tseslint from 'typescript-eslint';

// core / codegen は VS Code・DOM・Node に依存しない(ADR 0035。拡張ホスト・Webview・CLI で共有する)
const pureModuleRestrictions = {
  patterns: [
    { group: ['vscode'], message: 'core / codegen では VS Code API を使わない' },
    {
      group: ['node:*', 'fs', 'path', 'os', 'child_process'],
      message: 'core / codegen では Node API を使わない',
    },
  ],
};

export default defineConfig(
  { ignores: ['**/dist/**', '**/coverage/**'] },
  js.configs.recommended,
  tseslint.configs.strictTypeChecked,
  {
    languageOptions: {
      parserOptions: {
        projectService: {
          allowDefaultProject: [
            '*.js',
            '*.ts',
            'packages/*/vitest.config.ts',
            'packages/*/build.mjs',
          ],
        },
        tsconfigRootDir: import.meta.dirname,
      },
    },
  },
  {
    files: ['packages/core/**', 'packages/codegen/**'],
    rules: { 'no-restricted-imports': ['error', pureModuleRestrictions] },
  },
  {
    files: ['packages/cli/**', 'tools/**', '*.js', '*.ts'],
    languageOptions: { globals: globals.node },
  },
  {
    // テストでは、見本の JSON を型を気にせず書き換えて異常系を作る
    files: ['**/*.test.ts'],
    rules: {
      '@typescript-eslint/no-explicit-any': 'off',
      '@typescript-eslint/no-unsafe-assignment': 'off',
      '@typescript-eslint/no-unsafe-member-access': 'off',
      '@typescript-eslint/no-unsafe-call': 'off',
      '@typescript-eslint/no-unsafe-return': 'off',
    },
  },
  {
    // 設定ファイル(JS)は型情報を持たないため、型情報を使うルールを外す
    files: ['**/*.js', '**/*.mjs'],
    extends: [tseslint.configs.disableTypeChecked],
  },
  prettier,
);
