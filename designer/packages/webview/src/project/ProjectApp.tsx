/**
 * プロジェクトの設定画面(docs/designer/project-spec.md §6)。C++Builder のプロジェクトオプションに当たる。
 * フォーム(メインフォームと起動時に作るフォーム)・コード生成の全般・C++・C++ の上書き・Python・出力先の確認を並べる。
 */
import { formCodegenSettings, resolveTargets } from '@bethany-designer/codegen';
import {
  CPP_HEADER_EXTENSIONS,
  CPP_SOURCE_EXTENSIONS,
  l10n,
  matchesFormPattern,
  normalizePath,
  setAutoCreate,
  setMainForm,
  type BfprojDocument,
  type CommentLocale,
  type CppOverride,
  type Diagnostic,
} from '@bethany-designer/core';
import { useId, useState } from 'react';
import { enumOptions, SelectField, TextField } from '../components/inspector/fields.tsx';
import { postMessage } from '../vscode.ts';
import {
  addOverride,
  moveAutoCreate,
  moveOverride,
  parsePatterns,
  removeOverride,
  setCommentLocale,
  setCppField,
  setLanguage,
  setOverrideField,
  setOverrideForms,
  setPythonField,
  startupLists,
  textValue,
  type CppFormKey,
} from './projectModel.ts';
import { commitProject, useProjectStore } from './projectStore.ts';

type Path = readonly (string | number)[];

/** 起動時に作るフォームの一覧の先頭に出すメインフォーム(選べない)の値 */
const MAIN_FORM_OPTION = '*main*';

export function ProjectApp() {
  const status = useProjectStore((s) => s.status);
  const project = useProjectStore((s) => s.project);
  const diagnostics = useProjectStore((s) => s.diagnostics);
  const fileName = useProjectStore((s) => s.fileName);
  const error = useProjectStore((s) => s.error);

  if (status === 'loading') return <p>{l10n.t('Loading…')}</p>;
  const name = fileName.replace(/\.bfproj\.json$/, '');
  const openAsText = () => {
    postMessage({ type: 'openAsText' });
  };
  if (!project) {
    return (
      <main className="project-settings">
        <p>
          {l10n.t(
            'The settings cannot be shown because the file content is invalid. Fix it in the text editor.',
          )}
        </p>
        <ul>
          {diagnostics.map((d, i) => (
            <li key={i} className="field-error">
              {d.message}
            </li>
          ))}
        </ul>
        <button type="button" onClick={openAsText}>
          {l10n.t('Open as Text')}
        </button>
      </main>
    );
  }

  const errorAt = (path: Path) => messageAt(diagnostics, path);
  const blocked = diagnostics.some((d) => d.severity === 'error');
  return (
    <main className="project-settings">
      <header className="project-header">
        <h1>{l10n.t('Project Settings: {0}', name)}</h1>
        <div className="project-actions">
          <button
            type="button"
            className="primary"
            disabled={blocked}
            title={l10n.t('Generate the code of all the forms and the startup code')}
            onClick={() => {
              postMessage({ type: 'generateProject' });
            }}
          >
            {l10n.t('Generate Code for the Whole Project')}
          </button>
          <button
            type="button"
            disabled={blocked}
            onClick={() => {
              postMessage({ type: 'generateCode' });
            }}
          >
            {l10n.t('Generate Startup Code')}
          </button>
          <button type="button" onClick={openAsText}>
            {l10n.t('Open as Text')}
          </button>
        </div>
      </header>
      {error && (
        <div className="field-error" role="alert">
          {error}
        </div>
      )}
      <FormsSection project={project} errorAt={errorAt} />
      <GeneralSection project={project} />
      {project.codegen?.cpp && <CppSection project={project} name={name} errorAt={errorAt} />}
      {project.codegen?.cpp && <OverridesSection project={project} errorAt={errorAt} />}
      {project.codegen?.python && <PythonSection project={project} name={name} errorAt={errorAt} />}
      <PreviewSection project={project} />
    </main>
  );
}

function messageAt(diagnostics: readonly Diagnostic[], path: Path): string | undefined {
  const key = path.join('.');
  const found = diagnostics.filter((d) => d.path.join('.') === key);
  return found.length === 0 ? undefined : found.map((d) => d.message).join('\n');
}

interface SectionProps {
  readonly project: BfprojDocument;
  readonly errorAt: (path: Path) => string | undefined;
}

function Section({
  title,
  description,
  children,
}: {
  readonly title: string;
  readonly description?: string;
  readonly children: React.ReactNode;
}) {
  return (
    <section className="settings-section">
      <h2>{title}</h2>
      {description && <p className="muted settings-description">{description}</p>}
      {children}
    </section>
  );
}

// ---- フォーム ------------------------------------------------------------------------

function FormsSection({ project, errorAt }: SectionProps) {
  const forms = project.forms ?? [];
  const { auto, available } = startupLists(project);
  const [selectedAuto, setSelectedAuto] = useState<string>();
  const [selectedAvailable, setSelectedAvailable] = useState<string>();
  const autoId = useId();
  const availableId = useId();

  return (
    <Section
      title={l10n.t('Forms')}
      description={l10n.t(
        'The main form is created first, then the auto-create forms in this order. Other forms are created by your code when needed.',
      )}
    >
      <SelectField
        label={l10n.t('Main form')}
        value={project.mainForm ?? ''}
        options={[
          ...(project.mainForm === undefined ? [{ value: '', label: l10n.t('(none)') }] : []),
          ...forms.map((f) => ({ value: f, label: f })),
        ]}
        error={errorAt(['mainForm'])}
        onChange={(value) => {
          if (value !== '') commitProject(setMainForm(project, value));
        }}
      />
      {forms.length === 0 ? (
        <p className="muted">
          {l10n.t('No forms. Add forms from the Forms view (Add to Project...).')}
        </p>
      ) : (
        <div className="startup-lists">
          <div className="startup-list">
            <label htmlFor={autoId}>{l10n.t('Auto-create forms')}</label>
            <select
              id={autoId}
              size={Math.max(6, forms.length)}
              value={selectedAuto ?? ''}
              onChange={(e) => {
                setSelectedAuto(e.target.value);
              }}
            >
              {project.mainForm !== undefined && (
                <option value={MAIN_FORM_OPTION} disabled>
                  {`★ ${normalizePath(project.mainForm)}`}
                </option>
              )}
              {auto.map((f) => (
                <option key={f} value={f}>
                  {f}
                </option>
              ))}
            </select>
          </div>
          <div className="startup-buttons">
            <button
              type="button"
              title={l10n.t('Move up')}
              disabled={selectedAuto === undefined || !auto.includes(selectedAuto)}
              onClick={() => {
                if (selectedAuto) commitProject(moveAutoCreate(project, selectedAuto, -1));
              }}
            >
              ↑
            </button>
            <button
              type="button"
              title={l10n.t('Move down')}
              disabled={selectedAuto === undefined || !auto.includes(selectedAuto)}
              onClick={() => {
                if (selectedAuto) commitProject(moveAutoCreate(project, selectedAuto, 1));
              }}
            >
              ↓
            </button>
            <button
              type="button"
              title={l10n.t("Don't Create at Startup")}
              disabled={selectedAuto === undefined || !auto.includes(selectedAuto)}
              onClick={() => {
                if (!selectedAuto) return;
                commitProject(setAutoCreate(project, selectedAuto, false));
                setSelectedAvailable(selectedAuto);
                setSelectedAuto(undefined);
              }}
            >
              ›
            </button>
            <button
              type="button"
              title={l10n.t('Create at Startup')}
              disabled={selectedAvailable === undefined || !available.includes(selectedAvailable)}
              onClick={() => {
                if (!selectedAvailable) return;
                commitProject(setAutoCreate(project, selectedAvailable, true));
                setSelectedAuto(selectedAvailable);
                setSelectedAvailable(undefined);
              }}
            >
              ‹
            </button>
          </div>
          <div className="startup-list">
            <label htmlFor={availableId}>{l10n.t('Available forms')}</label>
            <select
              id={availableId}
              size={Math.max(6, forms.length)}
              value={selectedAvailable ?? ''}
              onChange={(e) => {
                setSelectedAvailable(e.target.value);
              }}
            >
              {available.map((f) => (
                <option key={f} value={f}>
                  {f}
                </option>
              ))}
            </select>
          </div>
        </div>
      )}
    </Section>
  );
}

// ---- 全般 ----------------------------------------------------------------------------

function GeneralSection({ project }: { readonly project: BfprojDocument }) {
  const codegen = project.codegen ?? {};
  return (
    <Section title={l10n.t('Code Generation')}>
      <div className="field-row">
        <span className="field-label">{l10n.t('Languages')}</span>
        <div className="field-control language-checks">
          {(['cpp', 'python'] as const).map((language) => (
            <label key={language} className="prop-check">
              <input
                type="checkbox"
                checked={codegen[language] !== undefined}
                onChange={(e) => {
                  commitProject(setLanguage(project, language, e.target.checked));
                }}
              />
              {language === 'cpp' ? 'C++' : 'Python'}
            </label>
          ))}
        </div>
      </div>
      <SelectField
        label={l10n.t('Comment language')}
        value={codegen.commentLocale ?? ''}
        options={[
          { value: '', label: l10n.t('Default ({0})', 'English') },
          { value: 'en', label: 'English' },
          { value: 'ja', label: '日本語' }, // l10n-ignore
        ]}
        onChange={(value) => {
          commitProject(
            setCommentLocale(project, value === '' ? undefined : (value as CommentLocale)),
          );
        }}
      />
    </Section>
  );
}

// ---- C++ ---------------------------------------------------------------------------

/** C++ のフォームの形を決める項目(codegen.cpp と上書きで共通) */
function cppFormFields(): readonly {
  readonly key: CppFormKey;
  readonly label: string;
  readonly options?: readonly string[];
  readonly placeholder: string;
  /** 書かないときの値(上書きの「継承」の表示に使う。括弧は付けない) */
  readonly defaultLabel: string;
}[] {
  return [
    {
      key: 'namespace',
      label: l10n.t('Namespace'),
      placeholder: l10n.t('(none, e.g. app::ui)'),
      defaultLabel: l10n.t('none'),
    },
    {
      key: 'includeGuard',
      label: l10n.t('Include guard'),
      options: ['macro', 'pragma'],
      placeholder: 'macro',
      defaultLabel: 'macro',
    },
    {
      key: 'includeGuardPrefix',
      label: l10n.t('Include guard prefix'),
      placeholder: l10n.t('(none)'),
      defaultLabel: l10n.t('none'),
    },
    {
      key: 'headerExtension',
      label: l10n.t('Header extension'),
      options: CPP_HEADER_EXTENSIONS,
      placeholder: '.hpp',
      defaultLabel: '.hpp',
    },
    {
      key: 'sourceExtension',
      label: l10n.t('Source extension'),
      options: CPP_SOURCE_EXTENSIONS,
      placeholder: '.cpp',
      defaultLabel: '.cpp',
    },
    {
      key: 'headerDir',
      label: l10n.t('Header folder'),
      placeholder: l10n.t('(same folder as the form)'),
      defaultLabel: l10n.t('same folder as the form'),
    },
    {
      key: 'sourceDir',
      label: l10n.t('Source folder'),
      placeholder: l10n.t('(same folder as the form)'),
      defaultLabel: l10n.t('same folder as the form'),
    },
  ];
}

function CppSection({ project, name, errorAt }: SectionProps & { readonly name: string }) {
  const cpp = project.codegen?.cpp ?? {};
  return (
    <Section
      title="C++"
      description={l10n.t(
        'Folders are relative to the project file. With a header folder, add it to the include path of your build.',
      )}
    >
      <TextField
        label={l10n.t('Startup code')}
        value={cpp.main ?? ''}
        placeholder={`${name}.cpp`}
        error={errorAt(['codegen', 'cpp', 'main'])}
        onCommit={(text) => {
          commitProject(setCppField(project, 'main', textValue(text)));
          return undefined;
        }}
      />
      {cppFormFields().map((field) =>
        field.options ? (
          <SelectField
            key={field.key}
            label={field.label}
            value={cpp[field.key] ?? ''}
            options={enumOptions(field.options, field.placeholder)}
            error={errorAt(['codegen', 'cpp', field.key])}
            onChange={(value) => {
              commitProject(
                setCppField(project, field.key, value === '' ? undefined : (value as never)),
              );
            }}
          />
        ) : (
          <TextField
            key={field.key}
            label={field.label}
            value={cpp[field.key] ?? ''}
            placeholder={field.placeholder}
            error={errorAt(['codegen', 'cpp', field.key])}
            onCommit={(text) => {
              commitProject(setCppField(project, field.key, textValue(text) as never));
              return undefined;
            }}
          />
        ),
      )}
    </Section>
  );
}

// ---- C++ の上書き -----------------------------------------------------------------------

function OverridesSection({ project, errorAt }: SectionProps) {
  const overrides = project.codegen?.cpp?.overrides ?? [];
  const forms = project.forms ?? [];
  const firstPattern = () => {
    const first = forms[0];
    if (first === undefined) return '*.bfm.json';
    const dir = first.includes('/') ? first.slice(0, first.lastIndexOf('/')) : '';
    return dir === '' ? first : dir;
  };
  return (
    <Section
      title={l10n.t('C++ Settings for Some Forms')}
      description={l10n.t(
        'Each entry changes the C++ settings above for the forms that match its patterns (paths relative to the project file, *, ** and ?, or a folder). When several entries match, the later one wins.',
      )}
    >
      {overrides.map((override, index) => (
        <OverrideCard
          // 並べ替えで入力中の内容が別の要素に移らないよう、位置と内容で区別する
          key={`${String(index)}:${override.forms.join(',')}`}
          project={project}
          override={override}
          index={index}
          count={overrides.length}
          forms={forms}
          errorAt={errorAt}
        />
      ))}
      <button
        type="button"
        onClick={() => {
          commitProject(addOverride(project, firstPattern()));
        }}
      >
        {l10n.t('+ Add Settings for Some Forms')}
      </button>
    </Section>
  );
}

function OverrideCard({
  project,
  override,
  index,
  count,
  forms,
  errorAt,
}: SectionProps & {
  readonly override: CppOverride;
  readonly index: number;
  readonly count: number;
  readonly forms: readonly string[];
}) {
  const base = ['codegen', 'cpp', 'overrides', index] as const;
  const matched = forms.filter((f) => override.forms.some((p) => matchesFormPattern(p, f)));
  const cpp = project.codegen?.cpp ?? {};
  const patternErrors = override.forms
    .map((_, i) => errorAt([...base, 'forms', i]))
    .filter((e) => e !== undefined);
  return (
    <div className="override-card">
      <div className="override-header">
        <strong>{l10n.t('Entry {0}', index + 1)}</strong>
        <div className="override-buttons">
          <button
            type="button"
            title={l10n.t('Move up')}
            disabled={index === 0}
            onClick={() => {
              commitProject(moveOverride(project, index, -1));
            }}
          >
            ↑
          </button>
          <button
            type="button"
            title={l10n.t('Move down')}
            disabled={index === count - 1}
            onClick={() => {
              commitProject(moveOverride(project, index, 1));
            }}
          >
            ↓
          </button>
          <button
            type="button"
            title={l10n.t('Delete')}
            onClick={() => {
              commitProject(removeOverride(project, index));
            }}
          >
            ✕
          </button>
        </div>
      </div>
      <TextField
        label={l10n.t('Forms')}
        value={override.forms.join(', ')}
        placeholder="dialogs, **/*Dialog.bfm.json"
        error={patternErrors.length > 0 ? patternErrors.join('\n') : errorAt([...base, 'forms'])}
        onCommit={(text) => {
          const patterns = parsePatterns(text);
          if (patterns.length === 0) return l10n.t('Enter at least one pattern');
          commitProject(setOverrideForms(project, index, patterns));
          return undefined;
        }}
      />
      <p className="muted override-matched">
        {matched.length > 0
          ? l10n.t('Applies to: {0}', matched.join(', '))
          : l10n.t('Applies to no form')}
      </p>
      {cppFormFields().map((field) => {
        // 空欄・「継承」は上の C++ の設定(書いていなければ既定値)を使う
        const inherited = cpp[field.key] ?? field.defaultLabel;
        return field.options ? (
          <SelectField
            key={field.key}
            label={field.label}
            value={override[field.key] ?? ''}
            options={[
              { value: '', label: l10n.t('Inherit ({0})', inherited) },
              ...field.options.map((v) => ({ value: v, label: v })),
            ]}
            error={errorAt([...base, field.key])}
            onChange={(value) => {
              commitProject(
                setOverrideField(
                  project,
                  index,
                  field.key,
                  value === '' ? undefined : (value as never),
                ),
              );
            }}
          />
        ) : (
          <TextField
            key={field.key}
            label={field.label}
            value={override[field.key] ?? ''}
            placeholder={l10n.t('Inherit ({0})', inherited)}
            error={errorAt([...base, field.key])}
            onCommit={(text) => {
              commitProject(setOverrideField(project, index, field.key, textValue(text) as never));
              return undefined;
            }}
          />
        );
      })}
    </div>
  );
}

// ---- Python ------------------------------------------------------------------------

function PythonSection({ project, name, errorAt }: SectionProps & { readonly name: string }) {
  const python = project.codegen?.python ?? {};
  return (
    <Section
      title="Python"
      description={l10n.t(
        'The startup code imports the form modules by their paths from its folder, so put the module folder in the folder of the startup code or below.',
      )}
    >
      <TextField
        label={l10n.t('Startup code')}
        value={python.main ?? ''}
        placeholder={`${name}.py`}
        error={errorAt(['codegen', 'python', 'main'])}
        onCommit={(text) => {
          commitProject(setPythonField(project, 'main', textValue(text)));
          return undefined;
        }}
      />
      <TextField
        label={l10n.t('Module folder')}
        value={python.moduleDir ?? ''}
        placeholder={l10n.t('(same folder as the form)')}
        error={errorAt(['codegen', 'python', 'moduleDir'])}
        onCommit={(text) => {
          commitProject(setPythonField(project, 'moduleDir', textValue(text)));
          return undefined;
        }}
      />
    </Section>
  );
}

// ---- 出力先の確認 --------------------------------------------------------------------

function PreviewSection({ project }: { readonly project: BfprojDocument }) {
  const forms = project.forms ?? [];
  if (forms.length === 0 || (!project.codegen?.cpp && !project.codegen?.python)) return null;
  const rows = forms.map((formPath) => {
    const path = normalizePath(formPath);
    const dir = path.includes('/') ? path.slice(0, path.lastIndexOf('/') + 1) : '';
    const file = path.slice(dir.length);
    const settings = formCodegenSettings([{ doc: project, formPath: path }]);
    // 出力先はファイル名から決まる(クラス名は使わないので、フォームの名前はファイル名で代える)
    const name = file.replace(/\.bfm\.json$/, '');
    const targets = resolveTargets(
      { formatVersion: 1, form: { name, class: 'TForm' } },
      file,
      settings,
    );
    const inProject = (p: string | undefined) => (p === undefined ? '' : normalizePath(dir + p));
    return {
      form: path,
      namespace: settings.cpp?.namespace.join('::') ?? '',
      header: inProject(targets.cpp?.header),
      source: inProject(targets.cpp?.source),
      guard: targets.cpp ? (targets.cpp.includeGuard ?? '#pragma once') : '',
      python: inProject(targets.python?.file),
    };
  });
  const cpp = project.codegen.cpp !== undefined;
  const python = project.codegen.python !== undefined;
  return (
    <Section
      title={l10n.t('Output Files')}
      description={l10n.t('Where the code of each form is generated with the settings above.')}
    >
      <div className="preview-scroll">
        <table className="preview-table">
          <thead>
            <tr>
              <th>{l10n.t('Form')}</th>
              {cpp && <th>{l10n.t('Namespace')}</th>}
              {cpp && <th>{l10n.t('Header')}</th>}
              {cpp && <th>{l10n.t('Source')}</th>}
              {cpp && <th>{l10n.t('Include guard')}</th>}
              {python && <th>Python</th>}
            </tr>
          </thead>
          <tbody>
            {rows.map((row) => (
              <tr key={row.form}>
                <td>{row.form}</td>
                {cpp && <td>{row.namespace}</td>}
                {cpp && <td>{row.header}</td>}
                {cpp && <td>{row.source}</td>}
                {cpp && <td>{row.guard}</td>}
                {python && <td>{row.python}</td>}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </Section>
  );
}
