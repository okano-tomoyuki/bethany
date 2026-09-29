/**
 * マーカー区間の書き出しと、既存のファイルへのマージ(docs/designer/codegen-design.md §2・§5。tk-designer から流用)。
 *
 *   // <bethany-designer:begin id="beth_CreateComponents">
 *   ...(生成したコード)
 *   // <bethany-designer:end id="beth_CreateComponents" hash="1a2b3c4d">
 *
 * - 区間の中身は毎回置き換える。hash が中身と合わなければ、利用者が手で編集したものとして知らせる(M2 / M3)。
 * - マーカーの欠落・重複・入れ子・対応の誤りがあれば、何も書き込まずにエラーにする(M4)。
 * - ハンドラの雛形は、まだ定義されていないものだけを 1行マーカー <bethany-designer:handler-stubs> の直後に追記する(M1)。
 * - 後の版で加えた区間は、既存のファイルに無ければ ifMissing に従って加える(末尾か、以前の生成物の決まった行の置き換え)。
 *   組(group)の区間は、どれか 1 つでも加えられなければ、組のどれも加えずに警告にする。
 */
import { l10n } from '@bethany-designer/core';
import { regionHash } from './hash.ts';

export interface Region {
  readonly id: string;
  /** 区間の中身(字下げ済み。末尾の改行なし) */
  readonly content: string;
  /** マーカー行の字下げの深さ */
  readonly indent: number;
  /**
   * 既存のファイルに無いとき(後の版で加えた区間)の扱い。無ければエラー。
   * - 'append': 末尾に加える(ファイルの末尾に置く区間)
   * - 関数: 以前の生成物の中で、区間に置き換える行の範囲 [start, end) を返す(start === end なら、その位置に空行とともに挿入する)。
   *   見つからなければ undefined
   */
  readonly ifMissing?: 'append' | ((lines: readonly string[]) => LineRange | undefined);
  /** ifMissing で加えるときの組の名前(組のどれかを加えられなければ、組のどれも加えない。警告に出す) */
  readonly group?: string;
}

export interface LineRange {
  readonly start: number;
  readonly end: number;
}

export interface HandlerStub {
  readonly name: string;
  /** 字下げ済みのコード(末尾の改行なし) */
  readonly code: string;
}

export interface GeneratedCode {
  readonly regions: readonly Region[];
  readonly stubs: readonly HandlerStub[];
  /** 新規ファイルの内容。rendered(id) はマーカーつきの区間 */
  readonly scaffold: (rendered: (id: string) => string) => string;
}

export interface LanguageSyntax {
  /** 行コメントの開始("#" / "//") */
  readonly comment: string;
  readonly indentUnit: string;
  /** 既存のファイルにハンドラ name が定義済みか */
  readonly hasHandler: (text: string, name: string) => boolean;
  /** stubs マーカーがないときの追記位置(行番号。この行の前に挿入する) */
  readonly fallbackStubLine: (lines: readonly string[]) => number;
  /** 末尾に区間を加えるときに前に置く空行の数(Python は 2 行、C++ は 1 行) */
  readonly blankLinesBeforeAppended: number;
}

export type MergeResult =
  | {
      readonly ok: true;
      readonly text: string;
      /** 手で編集されていた(上書きした)区間の id */
      readonly modifiedRegions: readonly string[];
      /** 追記したハンドラの雛形 */
      readonly addedStubs: readonly string[];
      /** 加えられなかった区間の組など、書き込みは行ったが知らせること */
      readonly warnings?: readonly string[];
    }
  | { readonly ok: false; readonly error: string };

const STUBS_MARKER = '<bethany-designer:handler-stubs>';

export function renderRegion(region: Region, syntax: LanguageSyntax): string {
  const indent = syntax.indentUnit.repeat(region.indent);
  const hash = regionHash(region.content);
  return [
    `${indent}${syntax.comment} <bethany-designer:begin id="${region.id}">`,
    // 中身が空の区間(名前空間を使わないときの名前空間の区間など)は、マーカーの行だけにする
    ...(region.content === '' ? [] : [region.content]),
    `${indent}${syntax.comment} <bethany-designer:end id="${region.id}" hash="${hash}">`,
  ].join('\n');
}

/** 新規ファイルを作る */
export function createFile(generated: GeneratedCode, syntax: LanguageSyntax): string {
  const byId = new Map(generated.regions.map((r) => [r.id, r]));
  return generated.scaffold((id) => {
    const region = byId.get(id);
    if (!region) throw new Error(`unknown region: ${id}`);
    return renderRegion(region, syntax);
  });
}

interface ParsedRegion {
  readonly id: string;
  readonly begin: number;
  readonly end: number;
  readonly hash: string | undefined;
}

const BEGIN = /^\s*(?:#|\/\/)\s*<bethany-designer:begin id="([^"]+)">\s*$/;
const END = /^\s*(?:#|\/\/)\s*<bethany-designer:end id="([^"]+)"(?: hash="([0-9a-f]*)")?>\s*$/;

function parseRegions(lines: readonly string[]): ParsedRegion[] | string {
  const regions: ParsedRegion[] = [];
  let open: { id: string; begin: number } | undefined;
  for (const [i, line] of lines.entries()) {
    const begin = BEGIN.exec(line);
    if (begin) {
      if (open)
        return l10n.t(
          'Line {0}: region "{2}" starts before region "{1}" is closed',
          i + 1,
          open.id,
          begin[1] ?? '',
        );
      open = { id: begin[1] ?? '', begin: i };
      continue;
    }
    const end = END.exec(line);
    if (end) {
      if (!open || open.id !== end[1]) {
        return l10n.t(
          'Line {0}: the end marker of region "{1}" has no matching begin marker',
          i + 1,
          end[1] ?? '',
        );
      }
      if (regions.some((r) => r.id === open?.id))
        return l10n.t('Region "{0}" is duplicated', open.id);
      regions.push({ id: open.id, begin: open.begin, end: i, hash: end[2] });
      open = undefined;
    }
  }
  if (open) return l10n.t('Region "{0}" has no end marker', open.id);
  return regions;
}

/**
 * text をマーカー区間の中身と外側に分ける。マーカーが壊れていれば undefined。
 * outside は区間(マーカー行を含む)を空行にしたもので、行番号は元のまま。
 */
export function splitRegions(
  text: string,
): { readonly inside: string; readonly outside: readonly string[] } | undefined {
  const lines = text.split(/\r?\n/);
  const parsed = parseRegions(lines);
  if (typeof parsed === 'string') return undefined;
  const outside = [...lines];
  const inside: string[] = [];
  for (const p of parsed) {
    inside.push(...lines.slice(p.begin + 1, p.end));
    outside.fill('', p.begin, p.end + 1);
  }
  return { inside: inside.join('\n'), outside };
}

/** 既存のファイルのマーカー区間を置き換え、足りないハンドラの雛形を追記する */
export function mergeFile(
  existing: string,
  generated: GeneratedCode,
  syntax: LanguageSyntax,
): MergeResult {
  const eol = existing.includes('\r\n') ? '\r\n' : '\n';
  const original = existing.split(/\r?\n/);
  const before = parseRegions(original);
  if (typeof before === 'string')
    return {
      ok: false,
      error: l10n.t('Nothing was written because the markers are broken: {0}', before),
    };

  const absent = generated.regions.filter((r) => !before.some((p) => p.id === r.id));
  const missing = absent.filter((r) => r.ifMissing === undefined);
  if (missing.length > 0) {
    return {
      ok: false,
      error: l10n.t(
        'Nothing was written because marker regions were not found: {0}',
        missing.map((r) => r.id).join(', '),
      ),
    };
  }

  // 後の版で加えた区間を先に加えてから、すべての区間を置き換える
  const { lines, skippedGroups } = addMissingRegions(original, absent, syntax);
  const parsed = parseRegions(lines);
  if (typeof parsed === 'string') return { ok: false, error: parsed };
  const warnings = skippedGroups.map((group) =>
    l10n.t(
      'The regions for {0} were not added because the file has been edited where they go, so the settings for them are not applied to this file',
      group,
    ),
  );

  const modifiedRegions = parsed
    .filter((p) => generated.regions.some((r) => r.id === p.id))
    .filter((p) => p.hash !== regionHash(lines.slice(p.begin + 1, p.end).join('\n')))
    .map((p) => p.id);

  // 後ろの区間から置き換えると、前の区間の行番号がずれない
  const output = [...lines];
  for (const p of [...parsed].sort((a, b) => b.begin - a.begin)) {
    const region = generated.regions.find((r) => r.id === p.id);
    if (!region) continue; // 未知の区間(新しいバージョンの生成物など)はそのまま残す
    output.splice(p.begin, p.end - p.begin + 1, ...renderRegion(region, syntax).split('\n'));
  }

  // まだ定義されていないハンドラの雛形を追記する(削除はしない)
  const current = output.join('\n');
  const newStubs = generated.stubs.filter((s) => !syntax.hasHandler(current, s.name));
  if (newStubs.length > 0) {
    const markerLine = output.findIndex((l) => l.includes(STUBS_MARKER));
    const insertAt = markerLine >= 0 ? markerLine + 1 : syntax.fallbackStubLine(output);
    const stubLines = newStubs.flatMap((s) => ['', ...s.code.split('\n')]);
    output.splice(insertAt, 0, ...stubLines);
  }

  return {
    ok: true,
    text: output.join(eol),
    modifiedRegions,
    addedStubs: newStubs.map((s) => s.name),
    ...(warnings.length > 0 && { warnings }),
  };
}

/**
 * 既存のファイルに無い区間を ifMissing に従って加える。組のどれかの位置が見つからなければ、その組は加えない。
 * 置き換え・挿入は後ろの行から行い、末尾に加えるものは最後に加える(末尾の空行の前に、言語の決まった数の空行を挟む)。
 */
function addMissingRegions(
  original: readonly string[],
  absent: readonly Region[],
  syntax: LanguageSyntax,
): { readonly lines: string[]; readonly skippedGroups: readonly string[] } {
  const placed = absent.map((region) => ({
    region,
    range:
      typeof region.ifMissing === 'function' ? region.ifMissing(original) : ('append' as const),
  }));
  const skippedGroups = [
    ...new Set(
      placed.flatMap((p) => (p.range === undefined && p.region.group ? [p.region.group] : [])),
    ),
  ];
  const usable = placed.filter(
    (p) =>
      p.range !== undefined &&
      !(p.region.group !== undefined && skippedGroups.includes(p.region.group)),
  );

  const lines = [...original];
  const ranged = usable.flatMap((p) =>
    p.range !== undefined && p.range !== 'append' ? [{ region: p.region, range: p.range }] : [],
  );
  for (const { region, range } of ranged.sort((a, b) => b.range.start - a.range.start)) {
    const rendered = renderRegion(region, syntax).split('\n');
    lines.splice(
      range.start,
      range.end - range.start,
      ...(range.start === range.end ? [...rendered, ''] : rendered),
    );
  }
  const appended = usable.filter((p) => p.range === 'append').map((p) => p.region);
  if (appended.length > 0) {
    while (lines.length > 0 && lines[lines.length - 1]?.trim() === '') lines.pop();
    const gap = Array<string>(syntax.blankLinesBeforeAppended).fill('');
    lines.push(...appended.flatMap((r) => [...gap, ...renderRegion(r, syntax).split('\n')]), '');
  }
  return { lines, skippedGroups };
}
