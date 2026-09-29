import { describe, expect, it } from 'vitest';
import { clientMetrics } from './insets.ts';

/**
 * 実物の LCL(Windows 11)で、150×150 のコンテナに alClient の子を置いたときの子の [Left, Top, Width, Height] から求めた値
 * (docs/adr/0048)。子の座標は子の座標の原点からのもの。
 */
function childOf(className: string, properties: Record<string, unknown>): number[] {
  const { origin, insets } = clientMetrics(className, properties);
  return [
    insets.left - origin.x,
    insets.top - origin.y,
    150 - insets.left - insets.right,
    150 - insets.top - insets.bottom,
  ];
}

describe('クライアント領域の余白(枠・縁・BorderWidth)', () => {
  it.each([
    [{}, [1, 1, 148, 148]],
    [{ BevelOuter: 'bvNone' }, [0, 0, 150, 150]],
    [{ BevelOuter: 'bvLowered', BevelInner: 'bvRaised', BevelWidth: 3 }, [6, 6, 138, 138]],
    [{ BevelOuter: 'bvSpace', BevelWidth: 2 }, [2, 2, 146, 146]],
    [{ BorderWidth: 5 }, [6, 6, 138, 138]],
    [{ BorderStyle: 'bsSingle' }, [1, 1, 144, 144]],
    [{ BorderStyle: 'bsSingle', BevelOuter: 'bvNone' }, [0, 0, 146, 146]],
    [
      { BorderStyle: 'bsSingle', BorderWidth: 4, BevelWidth: 2, BevelInner: 'bvLowered' },
      [8, 8, 130, 130],
    ],
  ])('TPanel %j', (properties, expected) => {
    expect(childOf('TPanel', properties)).toEqual(expected);
  });

  it('TScrollBox は枠(既定は bsSingle)の分だけ', () => {
    expect(childOf('TScrollBox', {})).toEqual([0, 0, 146, 146]);
    expect(childOf('TScrollBox', { BorderStyle: 'bsNone' })).toEqual([0, 0, 150, 150]);
  });

  it('フォームは BorderWidth の分だけ', () => {
    expect(clientMetrics('TForm', { BorderWidth: 6 }).insets).toEqual({
      left: 6,
      top: 6,
      right: 6,
      bottom: 6,
    });
  });
});
