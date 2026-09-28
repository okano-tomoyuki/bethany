/**
 * 親子の制約(docs/designer/dsl-spec.md §4.2): 子を置けるか(acceptsControls)、子に置けるクラス(childClasses)、
 * 置ける親のクラス(parentClasses)。意味の検証と編集コマンドで同じ判定を使う。
 */
import { findClass, isSubclassOf } from '../catalog/catalog.ts';
import { l10n } from '../l10n.ts';

export type ChildProblem = 'controls-not-allowed' | 'invalid-child-class' | 'invalid-parent-class';

/** parentClass のコントロールの子に childClass を置けないなら、その理由(カタログに無いクラスは調べない) */
export function childProblem(
  parentClass: string,
  childClass: string,
): { readonly code: ChildProblem; readonly message: string } | undefined {
  const parentInfo = findClass(parentClass);
  const info = findClass(childClass);
  if (!parentInfo || !info) return undefined;
  if (!parentInfo.acceptsControls)
    return {
      code: 'controls-not-allowed',
      message: l10n.t('{0} cannot have controls', parentClass),
    };
  if (parentInfo.childClasses && !parentInfo.childClasses.some((c) => isSubclassOf(childClass, c)))
    return {
      code: 'invalid-child-class',
      message: l10n.t(
        '{0} can only have {1} as controls',
        parentClass,
        parentInfo.childClasses.join(', '),
      ),
    };
  if (info.parentClasses && !info.parentClasses.some((c) => isSubclassOf(parentClass, c)))
    return {
      code: 'invalid-parent-class',
      message: l10n.t('{0} can only be placed in {1}', childClass, info.parentClasses.join(', ')),
    };
  return undefined;
}
