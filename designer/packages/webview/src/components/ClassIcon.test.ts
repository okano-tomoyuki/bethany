import { getCatalog } from '@bethany-designer/core';
import { describe, expect, it } from 'vitest';
import { hasClassIcon } from './ClassIcon.tsx';

describe('ClassIcon', () => {
  it('カタログのすべてのクラスにアイコンがある', () => {
    const missing = Object.keys(getCatalog().classes).filter((name) => !hasClassIcon(name));
    expect(missing).toEqual([]);
  });
});
