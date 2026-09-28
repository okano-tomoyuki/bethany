/**
 * コード生成の設定の画面(docs/designer/editor-design.md §7。tk-designer ADR 0010)。
 * 生成する言語と、クラス名・出力先を指定して生成する。設定は DSL の codegen に保存する(Undo できる)。
 * 空欄は既定値(クラス名はフォームの名前、ファイル名は DSL のファイル名から決まる。dsl-spec.md §9)を表す。
 */
import { baseName } from '@no-vcl-designer/codegen';
import { hasErrors, l10n, type CodegenSettings, type CommentLocale } from '@no-vcl-designer/core';
import { generateCode } from '../editing.ts';
import { useDocumentStore, useUiStore } from '../store/stores.ts';
import { messagesAt } from './inspector/diagnostics.ts';
import { SelectField, TextField } from './inspector/fields.tsx';

type Target = 'cpp' | 'python';

export function CodegenView() {
  const doc = useDocumentStore((s) => s.document);
  const diagnostics = useDocumentStore((s) => s.diagnostics);
  const fileName = useDocumentStore((s) => s.fileName);
  const dispatch = useDocumentStore((s) => s.dispatch);
  const setView = useUiStore((s) => s.setView);
  if (!doc) return null;

  const codegen = doc.codegen ?? {};
  const base = baseName(fileName);
  const defaultClassName = `T${doc.form.name}`;
  const update = (next: CodegenSettings) => {
    dispatch({ type: 'setCodegen', codegen: next });
  };
  const toggle = (target: Target, enabled: boolean) => {
    update({ ...codegen, [target]: enabled ? {} : undefined });
  };
  /** 空欄は既定値(項目を書かない)。入力はその場で確定するため、エラーは返さない */
  const setField = (target: Target, key: string, text: string): string | undefined => {
    const value = text.trim();
    const entries = Object.entries(codegen[target] ?? {}).filter(([k]) => k !== key);
    if (value !== '') entries.push([key, value]);
    update({ ...codegen, [target]: Object.fromEntries(entries) });
    return undefined;
  };
  const errorAt = (target: Target, key: string) =>
    messagesAt(diagnostics, ['codegen', target, key]);

  const enabled = codegen.cpp !== undefined || codegen.python !== undefined;
  const blocked = hasErrors(diagnostics);

  return (
    <main className="codegen-view">
      <header className="codegen-header">
        <button
          type="button"
          onClick={() => {
            setView('design');
          }}
        >
          {l10n.t('← Back to Designer')}
        </button>
        <h2>{l10n.t('Generate Code')}</h2>
      </header>

      <p className="muted">
        {l10n.t(
          'The generated class inherits TForm. Empty fields use the defaults (the class name is T + the form name, and the file names are derived from the file name of the form). Paths are relative to the folder of the form file. The settings are saved in codegen of the form file.',
        )}
      </p>

      <section className="codegen-fields">
        <SelectField
          label={l10n.t('Comment language')}
          value={codegen.commentLocale ?? ''}
          options={[
            { value: '', label: l10n.t('Default ({0})', 'English') },
            { value: 'en', label: 'English' },
            // l10n-ignore(言語名はその言語で表示する)
            { value: 'ja', label: '日本語' },
          ]}
          error={messagesAt(diagnostics, ['codegen', 'commentLocale'])}
          onChange={(v) => {
            update({
              ...codegen,
              commentLocale: v === '' ? undefined : (v as CommentLocale),
            });
          }}
        />
      </section>

      <section className="codegen-target">
        <label className="codegen-toggle">
          <input
            type="checkbox"
            checked={codegen.cpp !== undefined}
            onChange={(e) => {
              toggle('cpp', e.target.checked);
            }}
          />
          {l10n.t('C++ (no_vcl.hpp)')}
        </label>
        {codegen.cpp && (
          <div className="codegen-fields">
            <TextField
              label={l10n.t('Class name')}
              value={codegen.cpp.className ?? ''}
              placeholder={defaultClassName}
              error={errorAt('cpp', 'className')}
              onCommit={(text) => setField('cpp', 'className', text)}
            />
            <TextField
              label={l10n.t('Header')}
              value={codegen.cpp.header ?? ''}
              placeholder={`${base}.hpp`}
              error={errorAt('cpp', 'header')}
              onCommit={(text) => setField('cpp', 'header', text)}
            />
            <TextField
              label={l10n.t('Source')}
              value={codegen.cpp.source ?? ''}
              placeholder={`${base}.cpp`}
              error={errorAt('cpp', 'source')}
              onCommit={(text) => setField('cpp', 'source', text)}
            />
          </div>
        )}
      </section>

      <section className="codegen-target">
        <label className="codegen-toggle">
          <input
            type="checkbox"
            checked={codegen.python !== undefined}
            onChange={(e) => {
              toggle('python', e.target.checked);
            }}
          />
          {l10n.t('Python (py/no_vcl.py)')}
        </label>
        {codegen.python && (
          <div className="codegen-fields">
            <TextField
              label={l10n.t('Class name')}
              value={codegen.python.className ?? ''}
              placeholder={defaultClassName}
              error={errorAt('python', 'className')}
              onCommit={(text) => setField('python', 'className', text)}
            />
            <TextField
              label={l10n.t('File')}
              value={codegen.python.file ?? ''}
              placeholder={`${base}.py`}
              error={errorAt('python', 'file')}
              onCommit={(text) => setField('python', 'file', text)}
            />
          </div>
        )}
      </section>

      <footer className="codegen-footer">
        <button
          type="button"
          className="primary"
          disabled={!enabled || blocked}
          onClick={generateCode}
        >
          {l10n.t('Generate')}
        </button>
        <span className="muted">
          {!enabled
            ? l10n.t('Select the languages to generate')
            : blocked
              ? l10n.t('Cannot generate because the form has validation errors')
              : l10n.t('Existing files are updated only in the marker regions')}
        </span>
      </footer>
    </main>
  );
}
