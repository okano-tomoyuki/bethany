import { describe, expect, it } from 'vitest';
import { stepZoom } from './uiStore.ts';

describe('stepZoom', () => {
  it('1 段階ずつ上げ下げし、範囲の端で止まる', () => {
    expect(stepZoom(1, 1)).toBe(1.25);
    expect(stepZoom(1, -1)).toBe(0.75);
    expect(stepZoom(2, 1)).toBe(2);
    expect(stepZoom(0.5, -1)).toBe(0.5);
  });

  it('段階の間の値からは、隣の段階に進む', () => {
    expect(stepZoom(1.1, 1)).toBe(1.25);
    expect(stepZoom(1.1, -1)).toBe(1);
  });
});
