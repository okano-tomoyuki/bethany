import { describe, expect, it } from 'vitest';
import { excludeGlob } from './excludeGlob.ts';

describe('excludeGlob', () => {
  it('search.exclude と files.exclude の true のパターンをまとめる', () => {
    expect(
      excludeGlob(
        { '**/node_modules': true, '**/*.code-search': true, '**/dist': false },
        { '**/.git': true, '**/node_modules': true },
      ),
    ).toBe('{**/node_modules,**/*.code-search,**/.git}');
  });

  it('条件付き(when)と、中括弧・カンマを含むパターンは使わない', () => {
    expect(
      excludeGlob({ '**/*.js': { when: '$(basename).ts' }, '**/{a,b}': true, '**/out': true }),
    ).toBe('{**/out}');
  });

  it('除外が無ければ undefined', () => {
    expect(excludeGlob(undefined, {})).toBeUndefined();
  });
});
