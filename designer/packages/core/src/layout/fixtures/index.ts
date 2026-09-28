/**
 * 配置の見本と、実物の LCL で記録した配置(tools/layout/record.mts で記録する)。
 * 見本を加えたら、ここにも加える。
 */
import type { LayoutRecord } from './record.ts';
import alignBasicText from './align-basic.nvform.json?raw';
import alignBasicRecord from './align-basic.lcl.json' with { type: 'json' };
import alignOrderText from './align-order.nvform.json?raw';
import alignOrderRecord from './align-order.lcl.json' with { type: 'json' };
import alignOrderConsistentText from './align-order-consistent.nvform.json?raw';
import alignOrderConsistentRecord from './align-order-consistent.lcl.json' with { type: 'json' };
import anchorsText from './anchors.nvform.json?raw';
import anchorsRecord from './anchors.lcl.json' with { type: 'json' };
import borderSpacingText from './border-spacing.nvform.json?raw';
import borderSpacingRecord from './border-spacing.lcl.json' with { type: 'json' };
import constraintsText from './constraints.nvform.json?raw';
import constraintsRecord from './constraints.lcl.json' with { type: 'json' };
import insetsText from './insets.nvform.json?raw';
import insetsRecord from './insets.lcl.json' with { type: 'json' };
import invisibleText from './invisible.nvform.json?raw';
import invisibleRecord from './invisible.lcl.json' with { type: 'json' };
import nestedText from './nested.nvform.json?raw';
import nestedRecord from './nested.lcl.json' with { type: 'json' };

export const FIXTURES: readonly { name: string; text: string; record: LayoutRecord }[] = [
  { name: 'align-basic', text: alignBasicText, record: alignBasicRecord },
  { name: 'align-order', text: alignOrderText, record: alignOrderRecord },
  {
    name: 'align-order-consistent',
    text: alignOrderConsistentText,
    record: alignOrderConsistentRecord,
  },
  { name: 'anchors', text: anchorsText, record: anchorsRecord },
  { name: 'border-spacing', text: borderSpacingText, record: borderSpacingRecord },
  { name: 'constraints', text: constraintsText, record: constraintsRecord },
  { name: 'insets', text: insetsText, record: insetsRecord },
  { name: 'invisible', text: invisibleText, record: invisibleRecord },
  { name: 'nested', text: nestedText, record: nestedRecord },
];
