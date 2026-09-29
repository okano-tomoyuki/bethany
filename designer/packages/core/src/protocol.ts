/**
 * 拡張ホストと Webview の間で交わすメッセージの型(tk-designer ADR 0006。docs/designer/editor-design.md §2)。
 * 両者はこの型を介してのみ通信し、互いのコードを直接 import しない。
 */
import type { EditCommand } from './edit/commands.ts';

/** キャンバスの設定(拡張の設定 bethanyDesigner.canvas.*。editor-design.md §5.4) */
export interface CanvasSettings {
  /** LCL の default のフォントとして使う書体 */
  readonly fontFamily: string;
  /** その大きさ(ポイント) */
  readonly fontSize: number;
  /** 移動・大きさの変更で合わせる格子の間隔(ピクセル。0 なら合わせない) */
  readonly gridSize: number;
  /** フォームに格子の点を描くか */
  readonly showGrid: boolean;
}

export type ExtensionToWebviewMessage =
  /**
   * TextDocument の現在の内容。変更のたびに送る(テキストエディタでの編集や Undo/Redo を含む)。
   * fileName は DSL のファイル名(コード生成のクラス名・出力先の既定値に使う)
   */
  | {
      readonly type: 'document';
      readonly version: number;
      readonly text: string;
      readonly fileName: string;
    }
  /** edit の処理結果。document の送信より後に届く */
  | {
      readonly type: 'editResult';
      readonly requestId: number;
      readonly ok: boolean;
      readonly error?: string;
    }
  /** キャンバスの設定。ready の後と、設定が変わったときに送る */
  | { readonly type: 'settings'; readonly settings: CanvasSettings };

export type WebviewToExtensionMessage =
  | { readonly type: 'ready' }
  | { readonly type: 'edit'; readonly requestId: number; readonly command: EditCommand }
  /** コード生成(tk-designer ADR 0010)。生成・書き込み・結果の通知は拡張が行う */
  | { readonly type: 'generateCode' }
  /**
   * ハンドラの定義へ移動する(editor-design.md §7)。拡張はコードを生成し(足りない雛形を追記し)てから、定義を開く。
   * 直前に送った edit(ハンドラ名の設定)の適用の後に処理する
   */
  | { readonly type: 'goToHandler'; readonly handler: string };
