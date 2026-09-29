/**
 * プロパティの欄の文字入力と DSL の値の変換(docs/designer/editor-design.md §6)。
 * 空欄は「値を書かない(既定値)」を表す。
 */
import { getCatalog } from '../catalog/catalog.ts';
import type { PropertyType } from '../catalog/types.ts';
import type { PropertyValue } from '../dsl/schema.ts';
import { l10n } from '../l10n.ts';

export type InputResult =
  | { readonly ok: true; readonly value: PropertyValue | undefined }
  | { readonly ok: false; readonly error: string };

const COLOR_PATTERN = /^#[0-9A-Fa-f]{6}$/;

/** TextToShortCut が解釈できるキーの名前(LCL の ShortCutToText と同じ表記) */
const SHORTCUT_KEYS: ReadonlySet<string> = new Set([
  'BkSp',
  'Tab',
  'Enter',
  'Esc',
  'Space',
  'PgUp',
  'PgDn',
  'End',
  'Home',
  'Left',
  'Up',
  'Right',
  'Down',
  'Ins',
  'Del',
]);
const SHORTCUT_MODIFIERS = ['Shift', 'Ctrl', 'Alt'] as const;

/** 文字入力を型に合う値にする。空欄は undefined(既定値に戻す) */
export function parseInput(type: PropertyType, text: string): InputResult {
  const ok = (value: PropertyValue | undefined): InputResult => ({ ok: true, value });
  const error = (message: string): InputResult => ({ ok: false, error: message });
  if (text === '' && type.kind !== 'strings') return ok(undefined);
  const trimmed = text.trim();
  const catalog = getCatalog();

  switch (type.kind) {
    case 'int':
      return /^[-+]?\d+$/.test(trimmed)
        ? ok(Number.parseInt(trimmed, 10))
        : error(l10n.t('Enter an integer'));
    case 'float': {
      const value = Number(trimmed);
      return trimmed !== '' && Number.isFinite(value) ? ok(value) : error(l10n.t('Enter a number'));
    }
    case 'bool':
      if (trimmed === 'true') return ok(true);
      if (trimmed === 'false') return ok(false);
      return error(l10n.t('Enter true or false'));
    case 'string':
      return ok(text);
    case 'char':
      return text.length === 1 ? ok(text) : error(l10n.t('Enter a single character'));
    case 'enum': {
      const items = catalog.enums[type.enum] ?? [];
      return items.includes(trimmed)
        ? ok(trimmed)
        : error(l10n.t('Enter one of {0}', items.join(', ')));
    }
    case 'alias': {
      const constants = catalog.constants[type.alias] ?? {};
      if (type.alias === 'TShortCut') {
        const normalized = normalizeShortCut(trimmed);
        return normalized === undefined
          ? error(l10n.t('Enter a shortcut such as Ctrl+S, Shift+F5 or Ctrl+Alt+Del'))
          : ok(normalized);
      }
      if (Object.hasOwn(constants, trimmed)) return ok(trimmed);
      if (type.alias === 'TColor' && COLOR_PATTERN.test(trimmed)) return ok(trimmed.toUpperCase());
      return error(
        type.alias === 'TColor'
          ? l10n.t('Enter a color constant ({0}) or #RRGGBB', Object.keys(constants).join(', '))
          : l10n.t('Enter one of {0}', Object.keys(constants).join(', ')),
      );
    }
    case 'flags':
    case 'set': {
      const items =
        (type.kind === 'flags' ? catalog.flags[type.flags] : catalog.enums[type.enum]) ?? [];
      const names = trimmed
        .replace(/^\[|\]$/g, '')
        .split(',')
        .map((s) => s.trim())
        .filter((s) => s !== '');
      const unknown = names.find((n) => !items.includes(n));
      if (unknown !== undefined)
        return error(l10n.t('{0} is not one of {1}', unknown, items.join(', ')));
      // 並びはカタログの順にする(同じ集合なら同じ値)
      return ok(items.filter((i) => names.includes(i)));
    }
    case 'strings':
      return ok(text === '' ? [] : text.replace(/\r\n/g, '\n').split('\n'));
    case 'ref':
      return ok(trimmed);
    case 'object':
      return error(l10n.t('Set the properties inside {0}', type.class));
    case 'collection':
      return error(l10n.t('Set the properties inside {0}', type.item));
  }
}

/** 値を欄に表示する文字列にする(parseInput の逆。未設定は空欄) */
export function formatValue(type: PropertyType, value: unknown): string {
  if (value === undefined || value === null) return '';
  switch (type.kind) {
    case 'flags':
    case 'set':
      return Array.isArray(value) ? `[${value.join(',')}]` : '';
    case 'strings':
      return Array.isArray(value) ? value.join('\n') : '';
    case 'object':
    case 'collection':
      return '';
    default:
      return typeof value === 'string' ? value : JSON.stringify(value);
  }
}

/** ショートカットの表記を LCL の形(Ctrl+Shift+S のような修飾キーの順ではなく Shift+Ctrl+Alt+キー)に揃える */
export function normalizeShortCut(text: string): string | undefined {
  const parts = text.split('+').map((p) => p.trim());
  const key = parts.pop();
  if (key === undefined || key === '') return undefined;
  const modifiers = new Set<string>();
  for (const part of parts) {
    const modifier = SHORTCUT_MODIFIERS.find((m) => m.toLowerCase() === part.toLowerCase());
    if (!modifier || modifiers.has(modifier)) return undefined;
    modifiers.add(modifier);
  }
  const normalizedKey = normalizeKey(key);
  if (normalizedKey === undefined) return undefined;
  return [...SHORTCUT_MODIFIERS.filter((m) => modifiers.has(m)), normalizedKey].join('+');
}

function normalizeKey(key: string): string | undefined {
  if (/^[A-Za-z0-9]$/.test(key)) return key.toUpperCase();
  const fn = /^[Ff](\d{1,2})$/.exec(key);
  if (fn) {
    const n = Number(fn[1]);
    return n >= 1 && n <= 24 ? `F${String(n)}` : undefined;
  }
  return [...SHORTCUT_KEYS].find((k) => k.toLowerCase() === key.toLowerCase());
}
