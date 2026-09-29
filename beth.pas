library beth;

{$macro on}
{$ifdef WINDOWS}
  {$define BETH_CALL := stdcall}
{$else}
  {$define BETH_CALL := cdecl}
{$endif}

{ エクスポート関数は「LCLでそのメンバが公開(public/published)されるクラス」の名前で1本ずつ用意する。
  兄弟クラスがそれぞれ公開していて重複する場合(Text, Checked)だけ、宣言元の共通祖先の名前で
  1本にまとめ、protected hack(下の TXxxAccess)経由でアクセスする。
  階層の対応は docs/class-hierarchy.md を参照。 }

uses
  Classes,
  Interfaces,
  Forms,
  Controls,
  StdCtrls,
  ExtCtrls,
  ComCtrls,
  ToolWin,
  CheckLst,
  Buttons,
  Spin,
  MaskEdit,
  Grids,
  AVL_Tree,
  Menus,
  LCLProc,
  SysUtils,
  Graphics,
  Dialogs,
  ImgList,
  CustomTimer,
  ActnList,
  Clipbrd
  {$ifdef LCLwin32}
  , Windows, InterfaceBase, WSControls
  {$endif};

type
  { Data は登録時に渡された利用者データをそのまま返す(C 側で状態を持ち回るため) }
  TBethCallback = procedure(Sender: Pointer; Data: Pointer); BETH_CALL;

  { 公開関数の中で起きた例外を呼び出し側へ知らせる(docs/adr/0031)。文字列は UTF-8 で、呼び出しの間だけ有効。 }
  TBethErrorCallback = procedure(ClassName: PChar; Message: PChar); BETH_CALL;

  { Cのプレーンな関数ポインタ(beth_callback_t)を
    LCLのTNotifyEvent(オブジェクトメソッド)へ橋渡しする }
  TCallbackBridge = class(TComponent)
  private
    FCallback: TBethCallback;
    FData: Pointer;
  public
    procedure DoClick(Sender: TObject);
    property Callback: TBethCallback read FCallback write FCallback;
    property Data: Pointer read FData write FData;
  end;

  { 書き換え可能な引数(Pascal の var 引数)を 1 つ持つイベント用のコールバック。Value はその値を指すポインタで、
    コールバックの中で書き換えると呼び出し元に反映される。
      OnClose      (TCloseEvent)     : TCloseAction の序数(caNone=0, caHide, caFree, caMinimize)
      OnCloseQuery (TCloseQueryEvent): CanClose(0 = False、0 以外 = True。渡すときの True は -1) }
  TBethVarCallback = procedure(Sender: Pointer; Value: PInteger; Data: Pointer); BETH_CALL;

  TVarCallbackBridge = class(TComponent)
  private
    FCallback: TBethVarCallback;
    FData: Pointer;
  public
    procedure DoClose(Sender: TObject; var CloseAction: TCloseAction);
    procedure DoCloseQuery(Sender: TObject; var CanClose: Boolean);
  end;

  { OnKeyDown/OnKeyUp(TKeyEvent)用。Key(キーコード)は書き換え可能(var 引数)。0 にすると LCL に渡さない。
    Shift は TShiftStateEnum の各値をビットとして表した LongWord(beth_ss* のビット和、ShiftStateToInt 参照)。 }
  TBethKeyCallback = procedure(Sender: Pointer; Key: PInteger; Shift: LongWord; Data: Pointer); BETH_CALL;

  TKeyCallbackBridge = class(TComponent)
  private
    FCallback: TBethKeyCallback;
    FData: Pointer;
  public
    procedure DoKey(Sender: TObject; var Key: Word; Shift: TShiftState);
  end;

  { OnKeyPress(TKeyPressEvent)用。Key は文字コード、書き換え可能。0 にすると LCL に渡さない。 }
  TBethKeyPressCallback = procedure(Sender: Pointer; Key: PInteger; Data: Pointer); BETH_CALL;

  TKeyPressCallbackBridge = class(TComponent)
  private
    FCallback: TBethKeyPressCallback;
    FData: Pointer;
  public
    procedure DoKeyPress(Sender: TObject; var Key: char);
  end;

  { OnMouseDown/OnMouseUp(TMouseEvent)用。Button は TMouseButton の序数(beth_mb*)、Shift は上記と同じ。 }
  TBethMouseCallback = procedure(Sender: Pointer; Button, Shift, X, Y: Integer; Data: Pointer); BETH_CALL;

  TMouseCallbackBridge = class(TComponent)
  private
    FCallback: TBethMouseCallback;
    FData: Pointer;
  public
    procedure DoMouse(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
  end;

  { OnMouseMove(TMouseMoveEvent)用。 }
  TBethMouseMoveCallback = procedure(Sender: Pointer; Shift, X, Y: Integer; Data: Pointer); BETH_CALL;

  TMouseMoveCallbackBridge = class(TComponent)
  private
    FCallback: TBethMouseMoveCallback;
    FData: Pointer;
  public
    procedure DoMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
  end;

  { OnMouseWheel(TMouseWheelEvent)用。Handled は書き換え可能(0 以外 = True)。
    True にすると、ホイール操作をこのハンドラで処理済みとして扱う(既定のスクロール等が起きなくなる)。 }
  TBethMouseWheelCallback = procedure(Sender: Pointer; Shift, WheelDelta, X, Y: Integer; Handled: PInteger; Data: Pointer); BETH_CALL;

  TMouseWheelCallbackBridge = class(TComponent)
  private
    FCallback: TBethMouseWheelCallback;
    FData: Pointer;
  public
    procedure DoMouseWheel(Sender: TObject; Shift: TShiftState; WheelDelta: Integer; MousePos: TPoint; var Handled: Boolean);
  end;

  { protected メンバへアクセスするための派生クラス(protected hack)。
    同一ユニット内で宣言した派生クラス経由なら、基底の protected メンバに触れられる。 }
  { OnSelectionChange(TSelectionChangeEvent)用。User は利用者の操作による変更か(0 = False、0 以外 = True)。 }
  TBethBoolCallback = procedure(Sender: Pointer; Value: LongBool; Data: Pointer); BETH_CALL;

  TBoolCallbackBridge = class(TComponent)
  private
    FCallback: TBethBoolCallback;
    FData: Pointer;
  public
    procedure DoSelectionChange(Sender: TObject; User: Boolean);
  end;

  { Application.OnException(TExceptionEvent)用。例外のクラス名とメッセージを渡す(docs/adr/0043)。 }
  TBethExceptionCallback = procedure(Sender: Pointer; ClassName, Message: PChar; Data: Pointer); BETH_CALL;

  TExceptionCallbackBridge = class(TComponent)
  private
    FCallback: TBethExceptionCallback;
    FData: Pointer;
  public
    procedure DoException(Sender: TObject; E: Exception);
  end;

  TControlAccess = class(TControl);
  TButtonControlAccess = class(TButtonControl);
  TWinControlAccess = class(TWinControl);
  TCustomEditAccess = class(TCustomEdit);
  TCustomMemoAccess = class(TCustomMemo);
  TCustomLabelAccess = class(TCustomLabel);
  TCustomListBoxAccess = class(TCustomListBox);
  TCustomComboBoxAccess = class(TCustomComboBox);
  TCustomCheckBoxAccess = class(TCustomCheckBox);

  { *_Create で生成したすべてのコンポーネントの破棄を受け取り、C/C++ 側へ通知する。
    Owner による連鎖破棄など、呼び出し側が知らないところで起きる破棄も検知できる。 }
  TFreeNotifier = class(TComponent)
  protected
    procedure Notification(AComponent: TComponent; Operation: TOperation); override;
  end;

var
  GFreeNotifier: TFreeNotifier;
  GFreeCallback: TBethCallback = nil;
  GFreeData: Pointer = nil;
  { TComponent ではない項目(TTreeNode・TListItem・TListColumn・THeaderSection・TCoolBand。FreeNotification が無い)の破棄通知。
    WatchItem で付けた観察者が、項目の破棄(TPersistent.Destroy の ooFree)で呼ぶ(NotifyItemFreed)。 }
  GItemFreeCallback: TBethCallback = nil;
  GItemFreeData: Pointer = nil;
  { DLL の切り離し中は True。LCL の終了処理で起きるイベント(フォームの OnDestroy・OnHide 等)を呼び出し側へ送らない。 }
  GDetaching: Boolean = False;

  { 例外の受け渡し(docs/adr/0031)。
    - 公開関数の中で送出された例外は、その関数の except で捕まえ、GErrorCallback で呼び出し側(beth_c.cpp)へ
      クラス名とメッセージを知らせる(FPC の例外は C/C++ の関数をまたいで伝わらないため、公開関数の外へは出さない)。
    - 呼び出し側のイベントのコールバックの中で起きた例外(C++ の throw)は、呼び出し側が SetCallbackError で知らせ、
      コールバックから戻った後にブリッジが CheckCallbackError で EBethCallbackError として送出し直す。
      メッセージループの中なら LCL の Application.HandleException が処理し(VCL と同じくメッセージボックス)、
      公開関数の中(MenuItem の Click 等)なら、その関数の except で呼び出し側へ知らせる。 }
  GErrorCallback: TBethErrorCallback = nil;

threadvar
  GCallbackErrorPending: Boolean;
  GCallbackErrorClass: AnsiString;
  GCallbackErrorMessage: AnsiString;

type
  { 呼び出し側のコールバックで起きた例外。呼び出し側へ知らせるときは、元のクラス名(OriginalClassName)を使う。 }
  EBethCallbackError = class(Exception)
  private
    FOriginalClassName: AnsiString;
  public
    property OriginalClassName: AnsiString read FOriginalClassName;
  end;

{ 公開関数の except から呼ぶ。捕まえた例外(ExceptObject)のクラス名とメッセージを呼び出し側へ知らせる。 }
procedure ReportException;
var
  E: TObject;
  C, M: AnsiString;
begin
  E := ExceptObject;
  if E is EBethCallbackError then
    C := EBethCallbackError(E).OriginalClassName
  else if E <> nil then
    C := E.ClassName
  else
    C := '';
  if E is Exception then
    M := Exception(E).Message
  else
    M := '';
  if Assigned(GErrorCallback) then
    GErrorCallback(PChar(C), PChar(M));
end;

{ ブリッジが呼び出し側のコールバックから戻った後に呼ぶ。コールバックの中で例外が起きていれば送出し直す。 }
procedure CheckCallbackError;
var
  E: EBethCallbackError;
begin
  if not GCallbackErrorPending then
    Exit;
  GCallbackErrorPending := False;
  E := EBethCallbackError.Create(GCallbackErrorMessage);
  E.FOriginalClassName := GCallbackErrorClass;
  raise E;
end;

procedure TCallbackBridge.DoClick(Sender: TObject);
begin
  if Assigned(FCallback) and not GDetaching then
    FCallback(Pointer(Sender), FData);
  CheckCallbackError;
end;

procedure TVarCallbackBridge.DoClose(Sender: TObject; var CloseAction: TCloseAction);
var
  A: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  A := Ord(CloseAction);
  FCallback(Pointer(Sender), @A, FData);
  { 範囲外の値が書き込まれた場合は、既定の動作のままにする }
  if (A >= Ord(Low(TCloseAction))) and (A <= Ord(High(TCloseAction))) then
    CloseAction := TCloseAction(A);
  CheckCallbackError;
end;

procedure TVarCallbackBridge.DoCloseQuery(Sender: TObject; var CanClose: Boolean);
var
  A: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if CanClose then A := -1 else A := 0;
  FCallback(Pointer(Sender), @A, FData);
  CanClose := A <> 0;
  CheckCallbackError;
end;

{ TShiftState(集合型)を LongWord のビット集合(beth_ss* と対応)に変換する。
  ループで変換することで、集合の実際のバイトサイズ(packset ディレクティブの効果)に依存しない。 }
function ShiftStateToInt(const S: TShiftState): LongWord;
var
  I: TShiftStateEnum;
begin
  Result := 0;
  for I := Low(TShiftStateEnum) to High(TShiftStateEnum) do
    if I in S then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToShiftState(V: LongWord): TShiftState;
var
  I: TShiftStateEnum;
begin
  Result := [];
  for I := Low(TShiftStateEnum) to High(TShiftStateEnum) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

procedure TKeyCallbackBridge.DoKey(Sender: TObject; var Key: Word; Shift: TShiftState);
var
  K: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  K := Key;
  FCallback(Pointer(Sender), @K, ShiftStateToInt(Shift), FData);
  Key := Word(K and $FFFF);
  CheckCallbackError;
end;

procedure TKeyPressCallbackBridge.DoKeyPress(Sender: TObject; var Key: char);
var
  K: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  K := Ord(Key);
  FCallback(Pointer(Sender), @K, FData);
  Key := Chr(K and $FF);
  CheckCallbackError;
end;

procedure TMouseCallbackBridge.DoMouse(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), Ord(Button), ShiftStateToInt(Shift), X, Y, FData);
  CheckCallbackError;
end;

procedure TMouseMoveCallbackBridge.DoMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), ShiftStateToInt(Shift), X, Y, FData);
  CheckCallbackError;
end;

procedure TMouseWheelCallbackBridge.DoMouseWheel(Sender: TObject; Shift: TShiftState; WheelDelta: Integer; MousePos: TPoint; var Handled: Boolean);
var
  H: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if Handled then H := -1 else H := 0;
  FCallback(Pointer(Sender), ShiftStateToInt(Shift), WheelDelta, MousePos.X, MousePos.Y, @H, FData);
  Handled := H <> 0;
  CheckCallbackError;
end;

{ ブリッジの取得(*_SetOnXxx は必ずこれを通す)。
  Current はイベントに現在設定されているメソッドの Data(= メソッドの持ち主のオブジェクト)。
  それが Owner の持つこのライブラリのブリッジなら、新しく作らずにコールバックを差し替えて再利用する
  (同じイベントに何度登録してもブリッジが蓄積しない)。
  Cb に nil を渡すとハンドラの解除になる。ブリッジ自体は残して何もしないようにする
  (解除がコールバックの実行中に行われても、実行中のブリッジを破棄しないため)。 }
function MethodData(const M: TNotifyEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TCloseEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TCloseQueryEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TTabChangingEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TKeyEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TKeyPressEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TMouseEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TMouseMoveEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TMouseWheelEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function KeyBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethKeyCallback; Data: Pointer): TKeyCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TKeyCallbackBridge) and (TKeyCallbackBridge(Current).Owner = Owner) then
    Result := TKeyCallbackBridge(Current)
  else
    Result := TKeyCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function KeyPressBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethKeyPressCallback; Data: Pointer): TKeyPressCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TKeyPressCallbackBridge) and (TKeyPressCallbackBridge(Current).Owner = Owner) then
    Result := TKeyPressCallbackBridge(Current)
  else
    Result := TKeyPressCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MouseBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethMouseCallback; Data: Pointer): TMouseCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TMouseCallbackBridge) and (TMouseCallbackBridge(Current).Owner = Owner) then
    Result := TMouseCallbackBridge(Current)
  else
    Result := TMouseCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MouseMoveBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethMouseMoveCallback; Data: Pointer): TMouseMoveCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TMouseMoveCallbackBridge) and (TMouseMoveCallbackBridge(Current).Owner = Owner) then
    Result := TMouseMoveCallbackBridge(Current)
  else
    Result := TMouseMoveCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MouseWheelBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethMouseWheelCallback; Data: Pointer): TMouseWheelCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TMouseWheelCallbackBridge) and (TMouseWheelCallbackBridge(Current).Owner = Owner) then
    Result := TMouseWheelCallbackBridge(Current)
  else
    Result := TMouseWheelCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function VarBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethVarCallback; Data: Pointer): TVarCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TVarCallbackBridge) and (TVarCallbackBridge(Current).Owner = Owner) then
    Result := TVarCallbackBridge(Current)
  else
    Result := TVarCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

procedure TBoolCallbackBridge.DoSelectionChange(Sender: TObject; User: Boolean);
begin
  if Assigned(FCallback) and not GDetaching then
    FCallback(Pointer(Sender), User, FData);
  CheckCallbackError;
end;

procedure TExceptionCallbackBridge.DoException(Sender: TObject; E: Exception);
var
  C, M: AnsiString;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  { ハンドラから送出された例外(DLL が送出し直したもの)は、元の例外のクラス名を渡す(docs/adr/0031) }
  if E is EBethCallbackError then
    C := EBethCallbackError(E).OriginalClassName
  else
    C := E.ClassName;
  M := E.Message;
  FCallback(Pointer(Sender), PChar(C), PChar(M), FData);
  CheckCallbackError;
end;

function BoolBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethBoolCallback; Data: Pointer): TBoolCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TBoolCallbackBridge) and (TBoolCallbackBridge(Current).Owner = Owner) then
    Result := TBoolCallbackBridge(Current)
  else
    Result := TBoolCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function ExceptionBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethExceptionCallback; Data: Pointer): TExceptionCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TExceptionCallbackBridge) and (TExceptionCallbackBridge(Current).Owner = Owner) then
    Result := TExceptionCallbackBridge(Current)
  else
    Result := TExceptionCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MethodData(const M: TSelectionChangeEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TIdleEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TExceptionEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

procedure TFreeNotifier.Notification(AComponent: TComponent; Operation: TOperation);
begin
  inherited Notification(AComponent, Operation);
  if (Operation = opRemove) and Assigned(GFreeCallback) then
    GFreeCallback(Pointer(AComponent), GFreeData);
end;

function BridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethCallback; Data: Pointer): TCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TCallbackBridge) and (TCallbackBridge(Current).Owner = Owner) then
    Result := TCallbackBridge(Current)
  else
    Result := TCallbackBridge.Create(Owner);
  Result.Callback := Cb;
  Result.Data := Data;
end;

{ 文字列を返す関数は必ずこれを通す。
  プロパティの読み出し結果(Caption 等)は一時的な AnsiString で、PChar にキャストしてそのまま返すと、
  関数を抜けた時点で参照カウントが 0 になって解放され、呼び出し側は解放済みのメモリを指すことになる。
  スレッドごとのバッファに保持してから返すことで、同じスレッドで次に文字列を返す関数を呼ぶまで有効にする。 }
threadvar
  GReturnStr: AnsiString;

function ReturnStr(const S: AnsiString): PChar;
begin
  GReturnStr := S;
  Result := PChar(GReturnStr);
end;

{ 生成したコンポーネントを破棄通知の対象に登録して返す。*_Create は必ずこれを通す。 }
function Watch(C: TComponent): Pointer;
begin
  C.FreeNotification(GFreeNotifier);
  Result := Pointer(C);
end;

{ DLL の切り離し(プロセス終了時の FreeLibrary / ExitProcess)では、この後の LCL の終了処理で
  Application とそれが所有するフォームが破棄される。その時点では呼び出し側(C++ のレジストリや
  C のコールバックが参照するデータ)が既に破棄されている可能性があるため、通知を止めておく。
  フックはユニットの終了処理より前に呼ばれる。 }
procedure DetachHook(DllParam: PtrInt);
begin
  GDetaching := True;
  GFreeCallback := nil;
  GFreeData := nil;
  GItemFreeCallback := nil;
  GItemFreeData := nil;
  GErrorCallback := nil;
end;

{ FreeNotify }

procedure FreeNotify_SetCallback(Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    GFreeCallback := Cb;
    GFreeData := Data;
  except
    ReportException;
  end;
end;

{ 例外の受け渡し(docs/adr/0031)。この 2 つは例外を送出しないため、try/except で包まない。 }

procedure Error_SetCallback(Cb: TBethErrorCallback); BETH_CALL;
begin
  GErrorCallback := Cb;
end;

{ 呼び出し側のイベントのコールバックの中で起きた例外を知らせる。コールバックから戻った後に、DLL 側で送出し直す。 }
procedure SetCallbackError(ClassName: PChar; Message: PChar); BETH_CALL;
begin
  GCallbackErrorPending := True;
  GCallbackErrorClass := ClassName;
  GCallbackErrorMessage := Message;
end;

{ TComponent }

procedure TComponent_Destroy(Obj: Pointer); BETH_CALL;
begin
  try
    TComponent(Obj).Free;
  except
    ReportException;
  end;
end;

{ 所有しているコンポーネントをすべて破棄する(自身は残る)。 }
procedure TComponent_DestroyComponents(Obj: Pointer); BETH_CALL;
begin
  try
    TComponent(Obj).DestroyComponents;
  except
    ReportException;
  end;
end;

{ TControl }

function TControl_GetParent(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TControl(Obj).Parent);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TControl_SetParent(Obj: Pointer; ParentObj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Parent := TWinControl(ParentObj);
  except
    ReportException;
  end;
end;

function TControl_GetLeft(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControl(Obj).Left;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetLeft(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).Left := Value;
  except
    ReportException;
  end;
end;

function TControl_GetTop(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControl(Obj).Top;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetTop(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).Top := Value;
  except
    ReportException;
  end;
end;

function TControl_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControl(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).Width := Value;
  except
    ReportException;
  end;
end;

function TControl_GetHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControl(Obj).Height;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).Height := Value;
  except
    ReportException;
  end;
end;

function TControl_GetVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControl(Obj).Visible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControl_SetVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControl(Obj).Visible := Value;
  except
    ReportException;
  end;
end;

function TControl_GetEnabled(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControl(Obj).Enabled;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControl_SetEnabled(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControl(Obj).Enabled := Value;
  except
    ReportException;
  end;
end;

function TControl_GetCaption(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TControl(Obj).Caption);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TControl_SetCaption(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TControl(Obj).Caption := Value;
  except
    ReportException;
  end;
end;

{ Align は TControl の public。TAlign の序数(alNone=0, alTop, alBottom, alLeft, alRight, alClient, alCustom)で受け渡す。
  既定値はクラスごとに異なる(TControl は alNone、TStatusBar は alBottom、TSplitter は alLeft)。 }
function TControl_GetAlign(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TControl(Obj).Align);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetAlign(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    { TCustomCoolBar は Align の Setter を reintroduce で差し替え(非仮想)、alLeft/alRight なら Vertical も切り替えるため、
      TControl の Setter を経由せずにその Setter を呼ぶ。 }
    if TObject(Obj) is TCustomCoolBar then
      TCustomCoolBar(Obj).Align := TAlign(Value)
    else
      TControl(Obj).Align := TAlign(Value);
  except
    ReportException;
  end;
end;

{ AutoSize は TControl の public(docs/adr/0029)。LCL では Align と同じく、配置はフォームの表示まで行われないことがある。 }
function TControl_GetAutoSize(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControl(Obj).AutoSize;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControl_SetAutoSize(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControl(Obj).AutoSize := Value;
  except
    ReportException;
  end;
end;

{ Text は TControl で protected。TCustomEdit と TCustomComboBox がそれぞれ公開している。 }
function TControl_GetText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TControlAccess(Obj).Text);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TControl_SetText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TControlAccess(Obj).Text := Value;
  except
    ReportException;
  end;
end;

procedure TControl_Show(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Show;
  except
    ReportException;
  end;
end;

procedure TControl_Hide(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Hide;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnClick(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).OnClick := @BridgeFor(TControl(Obj), MethodData(TControl(Obj).OnClick), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnDblClick(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControlAccess(Obj).OnDblClick := @BridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnDblClick), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnResize(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).OnResize := @BridgeFor(TControl(Obj), MethodData(TControl(Obj).OnResize), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnMouseDown(Obj: Pointer; Cb: TBethMouseCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControlAccess(Obj).OnMouseDown := @MouseBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseDown), Cb, Data).DoMouse;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnMouseUp(Obj: Pointer; Cb: TBethMouseCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControlAccess(Obj).OnMouseUp := @MouseBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseUp), Cb, Data).DoMouse;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnMouseMove(Obj: Pointer; Cb: TBethMouseMoveCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControlAccess(Obj).OnMouseMove := @MouseMoveBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseMove), Cb, Data).DoMouseMove;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnMouseEnter(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControlAccess(Obj).OnMouseEnter := @BridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseEnter), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnMouseLeave(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControlAccess(Obj).OnMouseLeave := @BridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseLeave), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TControl_SetOnMouseWheel(Obj: Pointer; Cb: TBethMouseWheelCallback; Data: Pointer); BETH_CALL;
begin
  try
    TControlAccess(Obj).OnMouseWheel := @MouseWheelBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseWheel), Cb, Data).DoMouseWheel;
  except
    ReportException;
  end;
end;

{ TWinControl }

procedure TWinControl_SetOnKeyDown(Obj: Pointer; Cb: TBethKeyCallback; Data: Pointer); BETH_CALL;
begin
  try
    TWinControl(Obj).OnKeyDown := @KeyBridgeFor(TWinControl(Obj), MethodData(TWinControl(Obj).OnKeyDown), Cb, Data).DoKey;
  except
    ReportException;
  end;
end;

procedure TWinControl_SetOnKeyUp(Obj: Pointer; Cb: TBethKeyCallback; Data: Pointer); BETH_CALL;
begin
  try
    TWinControl(Obj).OnKeyUp := @KeyBridgeFor(TWinControl(Obj), MethodData(TWinControl(Obj).OnKeyUp), Cb, Data).DoKey;
  except
    ReportException;
  end;
end;

procedure TWinControl_SetOnKeyPress(Obj: Pointer; Cb: TBethKeyPressCallback; Data: Pointer); BETH_CALL;
begin
  try
    TWinControl(Obj).OnKeyPress := @KeyPressBridgeFor(TWinControl(Obj), MethodData(TWinControl(Obj).OnKeyPress), Cb, Data).DoKeyPress;
  except
    ReportException;
  end;
end;

{ TCustomForm / TForm }

function TForm_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TForm.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomForm_Show(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).Show;
  except
    ReportException;
  end;
end;

procedure TCustomForm_Hide(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).Hide;
  except
    ReportException;
  end;
end;

function TCustomForm_ShowModal(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomForm(Obj).ShowModal;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomForm_Close(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).Close;
  except
    ReportException;
  end;
end;

{ 保留中のメッセージを処理し終えてから破棄する(Application.ReleaseComponent)。
  フォーム自身やその子のイベントハンドラの中からでも安全に呼べる。 }
procedure TCustomForm_Release(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).Release;
  except
    ReportException;
  end;
end;

procedure TCustomForm_SetOnClose(Obj: Pointer; Cb: TBethVarCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnClose := @VarBridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnClose), Cb, Data).DoClose;
  except
    ReportException;
  end;
end;

procedure TCustomForm_SetOnCloseQuery(Obj: Pointer; Cb: TBethVarCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnCloseQuery := @VarBridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnCloseQuery), Cb, Data).DoCloseQuery;
  except
    ReportException;
  end;
end;

procedure TCustomForm_SetOnShow(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnShow := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnShow), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TCustomForm_SetOnHide(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnHide := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnHide), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TCustomForm_SetOnActivate(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnActivate := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnActivate), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TCustomForm_SetOnDeactivate(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnDeactivate := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnDeactivate), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ 破棄の最初(BeforeDestruction)で呼ばれる。子コントロールはまだ生きており、破棄通知はこの後に来る。 }
procedure TCustomForm_SetOnDestroy(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnDestroy := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnDestroy), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TApplication

  Application は LCL(Forms ユニット)のグローバル変数で、DLL の読み込み時に生成・初期化済み。
  LCL の TApplication は FCL の TCustomApplication から派生するが、C++Builder に合わせ
  TComponent 直下のクラスとして扱い、関数名も TApplication_* にそろえる。 }

function GetApplication: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(Application);
  except
    Result := nil;
    ReportException;
  end;
end;

{ MainForm を設定できるのは LCL では CreateForm の中だけ(UpdateMainForm は CreateForm が
  生成中のフォームにしか効かない)ため、クラスを渡せない C/C++ 側向けに素の TForm を
  CreateForm で生成して返す。最初に生成したフォームが MainForm になる。 }
function TApplication_CreateForm(Obj: Pointer): Pointer; BETH_CALL;
var
  F: TForm;
begin
  try
    TApplication(Obj).CreateForm(TForm, F);
    Result := Watch(F);
  except
    Result := nil;
    ReportException;
  end;
end;

function TApplication_GetMainForm(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TApplication(Obj).MainForm);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TApplication_Run(Obj: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).Run;
  except
    ReportException;
  end;
end;

procedure TApplication_ProcessMessages(Obj: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).ProcessMessages;
  except
    ReportException;
  end;
end;

procedure TApplication_Terminate(Obj: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).Terminate;
  except
    ReportException;
  end;
end;

function TApplication_GetTerminated(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TApplication(Obj).Terminated;
  except
    Result := False;
    ReportException;
  end;
end;

function TApplication_GetTitle(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TApplication(Obj).Title);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TApplication_SetTitle(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TApplication(Obj).Title := Value;
  except
    ReportException;
  end;
end;

function TApplication_GetShowMainForm(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TApplication(Obj).ShowMainForm;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TApplication_SetShowMainForm(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TApplication(Obj).ShowMainForm := Value;
  except
    ReportException;
  end;
end;

{ TPanel / TGroupBox / TLabel }

function TPanel_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TPanel.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TGroupBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TGroupBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TLabel_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TLabel.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TButtonControl / TButton / TCheckBox / TRadioButton }

{ Checked は TButtonControl で protected。TCheckBox と TRadioButton がそれぞれ公開している。 }
function TButtonControl_GetChecked(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TButtonControlAccess(Obj).Checked;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TButtonControl_SetChecked(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TButtonControlAccess(Obj).Checked := Value;
  except
    ReportException;
  end;
end;

function TButton_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TButton.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCheckBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TCheckBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TRadioButton_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TRadioButton.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TCustomEdit / TEdit }

function TCustomEdit_GetMaxLength(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomEdit(Obj).MaxLength;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetMaxLength(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomEdit(Obj).MaxLength := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetReadOnly(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomEdit(Obj).ReadOnly;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomEdit_SetReadOnly(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomEdit(Obj).ReadOnly := Value;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomEdit(Obj).OnChange := @BridgeFor(TCustomEdit(Obj), MethodData(TCustomEdit(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

function TEdit_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TEdit.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TCustomMemo / TMemo }

{ Lines(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomMemo_GetLines(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomMemo(Obj).Lines);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomMemo_GetScrollBars(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomMemo(Obj).ScrollBars);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomMemo_SetScrollBars(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomMemo(Obj).ScrollBars := TScrollStyle(Value);
  except
    ReportException;
  end;
end;

function TMemo_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TMemo.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TCustomComboBox / TComboBox }

function TCustomComboBox_GetItemIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomComboBox(Obj).ItemIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomComboBox_SetItemIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomComboBox(Obj).ItemIndex := Value;
  except
    ReportException;
  end;
end;

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomComboBox_GetItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomComboBox(Obj).Items);
  except
    Result := nil;
    ReportException;
  end;
end;

function TComboBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TComboBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ OnChange は TCustomComboBox では protected で、公開しているのは TComboBox だけ。 }
procedure TComboBox_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TComboBox(Obj).OnChange := @BridgeFor(TComboBox(Obj), MethodData(TComboBox(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TCustomListBox / TListBox }

function TCustomListBox_GetItemIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomListBox(Obj).ItemIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomListBox_SetItemIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomListBox(Obj).ItemIndex := Value;
  except
    ReportException;
  end;
end;

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomListBox_GetItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomListBox(Obj).Items);
  except
    Result := nil;
    ReportException;
  end;
end;

function TListBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TListBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TCustomTimer / TTimer }

function TCustomTimer_GetInterval(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTimer(Obj).Interval;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTimer_SetInterval(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTimer(Obj).Interval := Value;
  except
    ReportException;
  end;
end;

function TCustomTimer_GetEnabled(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomTimer(Obj).Enabled;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomTimer_SetEnabled(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomTimer(Obj).Enabled := Value;
  except
    ReportException;
  end;
end;

procedure TCustomTimer_SetOnTimer(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomTimer(Obj).OnTimer := @BridgeFor(TCustomTimer(Obj), MethodData(TCustomTimer(Obj).OnTimer), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

function TTimer_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TTimer.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TPaintBox }

function TPaintBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TPaintBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TPaintBox_GetCanvas(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPaintBox(Obj).Canvas);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TPaintBox_SetOnPaint(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TPaintBox(Obj).OnPaint := @BridgeFor(TPaintBox(Obj), MethodData(TPaintBox(Obj).OnPaint), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TCanvas }
{ Canvasはコントロール(TPaintBox等)が内部で保持するオブジェクトであり、
  独自のCreate/Destroyは持たない。取得元のコントロールが破棄されれば
  一緒に破棄される。 }

procedure TCanvas_MoveTo(Obj: Pointer; X, Y: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).MoveTo(X, Y);
  except
    ReportException;
  end;
end;

procedure TCanvas_LineTo(Obj: Pointer; X, Y: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).LineTo(X, Y);
  except
    ReportException;
  end;
end;

procedure TCanvas_Rectangle(Obj: Pointer; X1, Y1, X2, Y2: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).Rectangle(X1, Y1, X2, Y2);
  except
    ReportException;
  end;
end;

procedure TCanvas_Ellipse(Obj: Pointer; X1, Y1, X2, Y2: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).Ellipse(X1, Y1, X2, Y2);
  except
    ReportException;
  end;
end;

procedure TCanvas_TextOut(Obj: Pointer; X, Y: Integer; Text: PChar); BETH_CALL;
begin
  try
    TCanvas(Obj).TextOut(X, Y, Text);
  except
    ReportException;
  end;
end;

function TCanvas_GetPen(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCanvas(Obj).Pen);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCanvas_GetBrush(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCanvas(Obj).Brush);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCanvas_GetFont(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCanvas(Obj).Font);
  except
    Result := nil;
    ReportException;
  end;
end;

{ グラフィック(TGraphic の派生のハンドル)を描く(docs/adr/0029)。Graphic が nil なら何もしない。 }
procedure TCanvas_Draw(Obj: Pointer; X, Y: Integer; Graphic: Pointer); BETH_CALL;
begin
  try
    if Graphic <> nil then
      TCanvas(Obj).Draw(X, Y, TGraphic(Graphic));
  except
    ReportException;
  end;
end;

procedure TCanvas_StretchDraw(Obj: Pointer; X1, Y1, X2, Y2: Integer; Graphic: Pointer); BETH_CALL;
var
  R: TRect;
begin
  try
    if Graphic = nil then
      Exit;
    { Rect(...) は Win32 では Windows ユニットの型名に隠されるため、フィールドで組み立てる。 }
    R.Left := X1;
    R.Top := Y1;
    R.Right := X2;
    R.Bottom := Y2;
    TCanvas(Obj).StretchDraw(R, TGraphic(Graphic));
  except
    ReportException;
  end;
end;

{ Brush で塗りつぶす(枠は描かない)。 }
procedure TCanvas_FillRect(Obj: Pointer; X1, Y1, X2, Y2: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).FillRect(X1, Y1, X2, Y2);
  except
    ReportException;
  end;
end;

function TCanvas_GetPixels(Obj: Pointer; X, Y: Integer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TCanvas(Obj).Pixels[X, Y]);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCanvas_SetPixels(Obj: Pointer; X, Y: Integer; Value: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).Pixels[X, Y] := TColor(Value);
  except
    ReportException;
  end;
end;

{ TPen / TBrush / TFont (いずれも非所有) }

function TPen_GetColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TPen(Obj).Color);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TPen_SetColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TPen(Obj).Color := TColor(Value);
  except
    ReportException;
  end;
end;

function TPen_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TPen(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TPen_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TPen(Obj).Width := Value;
  except
    ReportException;
  end;
end;

function TBrush_GetColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TBrush(Obj).Color);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TBrush_SetColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TBrush(Obj).Color := TColor(Value);
  except
    ReportException;
  end;
end;

function TFont_GetName(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TFont(Obj).Name);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TFont_SetName(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TFont(Obj).Name := Value;
  except
    ReportException;
  end;
end;

function TFont_GetSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFont(Obj).Size;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFont_SetSize(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFont(Obj).Size := Value;
  except
    ReportException;
  end;
end;

function TFont_GetColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TFont(Obj).Color);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFont_SetColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFont(Obj).Color := TColor(Value);
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 1 で挙げたコントロール。既存クラスの部分列として追加する。 }

{ TScrollBox: TScrollingWinControl(実装済み)の直接の派生で、追加のメンバは無い。 }
function TScrollBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TScrollBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TToggleBox: TCustomCheckBox(実装済み)の直接の派生で、追加のメンバは無い(Checked を共有)。 }
function TToggleBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TToggleBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TBevel }

function TBevel_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TBevel.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TBevel_GetShape(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TBevel(Obj).Shape);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TBevel_SetShape(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TBevel(Obj).Shape := TBevelShape(Value);
  except
    ReportException;
  end;
end;

function TBevel_GetStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TBevel(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TBevel_SetStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TBevel(Obj).Style := TBevelStyle(Value);
  except
    ReportException;
  end;
end;

{ TShape: Pen/Brush は TCustomShape が所有する実体で、TCanvas の Pen/Brush と同じく非所有のハンドルとして返す。 }

function TShape_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TShape.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomShape_GetShape(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomShape(Obj).Shape);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomShape_SetShape(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomShape(Obj).Shape := TShapeType(Value);
  except
    ReportException;
  end;
end;

function TCustomShape_GetPen(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomShape(Obj).Pen);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomShape_GetBrush(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomShape(Obj).Brush);
  except
    Result := nil;
    ReportException;
  end;
end;

{ TStaticText }

function TStaticText_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TStaticText.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomStaticText_GetBorderStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomStaticText(Obj).BorderStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomStaticText_SetBorderStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomStaticText(Obj).BorderStyle := TStaticBorderStyle(Value);
  except
    ReportException;
  end;
end;

{ TStatusBar: LCL に中間の TCustomStatusBar は無く、TWinControl の直接の派生。
  Panels(TCollection)は今回未対応。SimpleText/SimplePanel のみ。 }

{$ifdef LCLwin32}
var
  GStatusBarHeightWarmedUp: Boolean = False;

{ LCL の Win32 実装は、ステータスバーの推奨の高さを最初の GetPreferredSize で一度だけ測り、
  ユニット内の変数にキャッシュする(win32wscomctrls.pp の InitializePreferredStatusBarHeight)。
  その際の計測用ウィンドウは WS_CHILD で、親に WidgetSet.AppHandle を使い、DLL(IsLibrary)では
  AppHandle が作られないため Screen.ActiveForm で代用する。Application.Run 開始前は
  ActiveForm も nil のため親が 0 になり、Win32 エラー 1406 で失敗する(ADR 0015)。
  そこで最初の TStatusBar の生成時に、使い捨ての隠しウィンドウを AppHandle に一時的に設定して
  計測を済ませておく。AppHandle はフォームの所有関係にも使われるため、計測後すぐに 0 に戻す。 }
procedure WarmUpStatusBarHeight(StatusBar: TStatusBar);
var
  Dummy: HWND;
  W, H: Integer;
begin
  if GStatusBarHeightWarmedUp or (WidgetSet.AppHandle <> 0) then
    Exit;
  Dummy := CreateWindowExW(0, 'STATIC', nil, WS_POPUP, 0, 0, 0, 0, 0, 0, HInstance, nil);
  if Dummy = 0 then
    Exit;
  WidgetSet.AppHandle := Dummy;
  try
    W := 0;
    H := 0;
    TWSWinControlClass(StatusBar.WidgetSetClass).GetPreferredSize(StatusBar, W, H, False);
  finally
    WidgetSet.AppHandle := 0;
    DestroyWindow(Dummy);
  end;
  GStatusBarHeightWarmedUp := True;
end;
{$endif}

function TStatusBar_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TStatusBar.Create(TComponent(Owner)));
    {$ifdef LCLwin32}
    WarmUpStatusBarHeight(TStatusBar(Result));
    {$endif}
  except
    Result := nil;
    ReportException;
  end;
end;

function TStatusBar_GetSimpleText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStatusBar(Obj).SimpleText);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TStatusBar_SetSimpleText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TStatusBar(Obj).SimpleText := Value;
  except
    ReportException;
  end;
end;

function TStatusBar_GetSimplePanel(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TStatusBar(Obj).SimplePanel;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TStatusBar_SetSimplePanel(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TStatusBar(Obj).SimplePanel := Value;
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 1、2 バッチ目(範囲・数値系のコントロール)。
  いずれも ComCtrls のネイティブコントロール。ADR 0015 の TStatusBar の問題(推奨の高さの計測が
  Application.Run 開始前に失敗する)は TStatusBar 固有の処理によるもので、これらには関係しない。 }

{ TScrollBar }

function TScrollBar_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TScrollBar.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomScrollBar_GetKind(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomScrollBar(Obj).Kind);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetKind(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).Kind := TScrollBarKind(Value);
  except
    ReportException;
  end;
end;

function TCustomScrollBar_GetMin(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomScrollBar(Obj).Min;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetMin(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).Min := Value;
  except
    ReportException;
  end;
end;

function TCustomScrollBar_GetMax(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomScrollBar(Obj).Max;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetMax(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).Max := Value;
  except
    ReportException;
  end;
end;

function TCustomScrollBar_GetPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomScrollBar(Obj).Position;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).Position := Value;
  except
    ReportException;
  end;
end;

function TCustomScrollBar_GetPageSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomScrollBar(Obj).PageSize;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetPageSize(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).PageSize := Value;
  except
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).OnChange := @BridgeFor(TCustomScrollBar(Obj), MethodData(TCustomScrollBar(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TTrackBar }

function TTrackBar_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TTrackBar.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomTrackBar_GetMin(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).Min;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetMin(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).Min := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetMax(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).Max;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetMax(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).Max := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).Position;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).Position := Value;
  except
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).OnChange := @BridgeFor(TCustomTrackBar(Obj), MethodData(TCustomTrackBar(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TProgressBar: 表示専用で、対応するイベントは無い。 }

function TProgressBar_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TProgressBar.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomProgressBar_GetMin(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomProgressBar(Obj).Min;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetMin(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).Min := Value;
  except
    ReportException;
  end;
end;

function TCustomProgressBar_GetMax(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomProgressBar(Obj).Max;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetMax(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).Max := Value;
  except
    ReportException;
  end;
end;

function TCustomProgressBar_GetPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomProgressBar(Obj).Position;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).Position := Value;
  except
    ReportException;
  end;
end;

{ TUpDown: Min/Max/Position/Increment/Associate は TCustomUpDown では protected で、
  唯一の具象クラス TUpDown が published にしている(TCheckBox の Checked と同じ形)ため、
  関数名は TUpDown_* にし、TUpDown(Obj) で直接アクセスする(protected hack は不要)。
  Associate は対象の TWinControl(TEdit 等)への参照で、TControl.Parent と同じくハンドルで表す。
  OnClick/OnChanging は独自のシグネチャ(ボタン方向・ユーザー操作かどうかを渡す)のため今回は未対応。 }

function TUpDown_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TUpDown.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TUpDown_GetMin(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TUpDown(Obj).Min;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TUpDown_SetMin(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TUpDown(Obj).Min := Value;
  except
    ReportException;
  end;
end;

function TUpDown_GetMax(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TUpDown(Obj).Max;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TUpDown_SetMax(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TUpDown(Obj).Max := Value;
  except
    ReportException;
  end;
end;

function TUpDown_GetPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TUpDown(Obj).Position;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TUpDown_SetPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TUpDown(Obj).Position := Value;
  except
    ReportException;
  end;
end;

function TUpDown_GetIncrement(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TUpDown(Obj).Increment;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TUpDown_SetIncrement(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TUpDown(Obj).Increment := Value;
  except
    ReportException;
  end;
end;

function TUpDown_GetAssociate(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TUpDown(Obj).Associate);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TUpDown_SetAssociate(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TUpDown(Obj).Associate := TWinControl(Value);
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 1、4 バッチ目(Items を持つグループ・リスト系のコントロール)。 }

{ TRadioGroup: OnClick は TCustomRadioGroup 自身のフィールド(TControl.OnClick とは別)なので、
  専用のブリッジで登録する(TComboBox の OnChange と同じ理由)。 }

function TRadioGroup_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TRadioGroup.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomRadioGroup_GetItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomRadioGroup(Obj).Items);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomRadioGroup_GetItemIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomRadioGroup(Obj).ItemIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomRadioGroup_SetItemIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomRadioGroup(Obj).ItemIndex := Value;
  except
    ReportException;
  end;
end;

procedure TCustomRadioGroup_SetOnClick(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomRadioGroup(Obj).OnClick := @BridgeFor(TCustomRadioGroup(Obj), MethodData(TCustomRadioGroup(Obj).OnClick), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TCheckGroup: Checked はインデックス付きプロパティ。値は他のインデックス付きアクセスと同様、
  引数に Index を追加して表す(TCustomDrawGrid_GetColWidths 等と同じ形)。 }

function TCheckGroup_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TCheckGroup.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomCheckGroup_GetItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomCheckGroup(Obj).Items);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomCheckGroup_GetChecked(Obj: Pointer; Index: Integer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCheckGroup(Obj).Checked[Index];
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCheckGroup_SetChecked(Obj: Pointer; Index: Integer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCheckGroup(Obj).Checked[Index] := Value;
  except
    ReportException;
  end;
end;

{ TCheckListBox: Items は基底 TCustomListBox のものをそのまま使う(beth_TCustomListBox_GetItems で
  共通)。Checked はインデックス付き。OnClickCheck は Sender のみの TNotifyEvent。 }

function TCheckListBox_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TCheckListBox.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomCheckListBox_GetChecked(Obj: Pointer; Index: Integer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCheckListBox(Obj).Checked[Index];
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCheckListBox_SetChecked(Obj: Pointer; Index: Integer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCheckListBox(Obj).Checked[Index] := Value;
  except
    ReportException;
  end;
end;

procedure TCustomCheckListBox_SetOnClickCheck(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomCheckListBox(Obj).OnClickCheck := @BridgeFor(TCustomCheckListBox(Obj), MethodData(TCustomCheckListBox(Obj).OnClickCheck), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 1、5 バッチ目(ボタンの派生)。
  Down/GroupIndex/Flat/AllowAllUp(TCustomSpeedButton)、Kind(TCustomBitBtn)はいずれも public のため
  protected hack は不要。Caption/OnClick は TControl から共有する。Glyph(ビットマップ)は未対応。 }

function TSpeedButton_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TSpeedButton.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomSpeedButton_GetDown(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).Down;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetDown(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).Down := Value;
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetGroupIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).GroupIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetGroupIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).GroupIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetFlat(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).Flat;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetFlat(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).Flat := Value;
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetAllowAllUp(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).AllowAllUp;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetAllowAllUp(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).AllowAllUp := Value;
  except
    ReportException;
  end;
end;

function TBitBtn_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TBitBtn.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomBitBtn_GetKind(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomBitBtn(Obj).Kind);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetKind(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).Kind := TBitBtnKind(Value);
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 1、6 バッチ目(数値・書式付き Edit)。
  TCustomSpinEdit(整数)は TCustomFloatSpinEdit(実数)の派生で、Value/MinValue/MaxValue/Increment を
  Integer で再宣言して Double 版を隠す。C++ 側でも同じ隠蔽を再現するため、関数名を宣言元のクラスごとに
  分ける(TCustomFloatSpinEdit_* は Double、TCustomSpinEdit_* は Integer)。 }

function TFloatSpinEdit_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TFloatSpinEdit.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomFloatSpinEdit_GetValue(Obj: Pointer): Double; BETH_CALL;
begin
  try
    Result := TCustomFloatSpinEdit(Obj).Value;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomFloatSpinEdit_SetValue(Obj: Pointer; Value: Double); BETH_CALL;
begin
  try
    TCustomFloatSpinEdit(Obj).Value := Value;
  except
    ReportException;
  end;
end;

function TCustomFloatSpinEdit_GetMinValue(Obj: Pointer): Double; BETH_CALL;
begin
  try
    Result := TCustomFloatSpinEdit(Obj).MinValue;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomFloatSpinEdit_SetMinValue(Obj: Pointer; Value: Double); BETH_CALL;
begin
  try
    TCustomFloatSpinEdit(Obj).MinValue := Value;
  except
    ReportException;
  end;
end;

function TCustomFloatSpinEdit_GetMaxValue(Obj: Pointer): Double; BETH_CALL;
begin
  try
    Result := TCustomFloatSpinEdit(Obj).MaxValue;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomFloatSpinEdit_SetMaxValue(Obj: Pointer; Value: Double); BETH_CALL;
begin
  try
    TCustomFloatSpinEdit(Obj).MaxValue := Value;
  except
    ReportException;
  end;
end;

function TCustomFloatSpinEdit_GetIncrement(Obj: Pointer): Double; BETH_CALL;
begin
  try
    Result := TCustomFloatSpinEdit(Obj).Increment;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomFloatSpinEdit_SetIncrement(Obj: Pointer; Value: Double); BETH_CALL;
begin
  try
    TCustomFloatSpinEdit(Obj).Increment := Value;
  except
    ReportException;
  end;
end;

function TCustomFloatSpinEdit_GetDecimalPlaces(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomFloatSpinEdit(Obj).DecimalPlaces;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomFloatSpinEdit_SetDecimalPlaces(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomFloatSpinEdit(Obj).DecimalPlaces := Value;
  except
    ReportException;
  end;
end;

function TSpinEdit_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TSpinEdit.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomSpinEdit_GetValue(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpinEdit(Obj).Value;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpinEdit_SetValue(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpinEdit(Obj).Value := Value;
  except
    ReportException;
  end;
end;

function TCustomSpinEdit_GetMinValue(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpinEdit(Obj).MinValue;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpinEdit_SetMinValue(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpinEdit(Obj).MinValue := Value;
  except
    ReportException;
  end;
end;

function TCustomSpinEdit_GetMaxValue(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpinEdit(Obj).MaxValue;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpinEdit_SetMaxValue(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpinEdit(Obj).MaxValue := Value;
  except
    ReportException;
  end;
end;

function TCustomSpinEdit_GetIncrement(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpinEdit(Obj).Increment;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpinEdit_SetIncrement(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpinEdit(Obj).Increment := Value;
  except
    ReportException;
  end;
end;

{ TMaskEdit: EditMask は TCustomMaskEdit では protected だが、唯一の具象クラス TMaskEdit が
  published にしているため、TMaskEdit(Obj) で直接アクセスする(TUpDown と同じ形)。 }

function TMaskEdit_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TMaskEdit.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TMaskEdit_GetEditMask(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TMaskEdit(Obj).EditMask);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TMaskEdit_SetEditMask(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TMaskEdit(Obj).EditMask := Value;
  except
    ReportException;
  end;
end;

{ TCustomLabeledEdit(docs/adr/0028)。EditLabel(TBoundLabel)は、LCL が生成時に内部で作る子コンポーネント
  (Owner は LabeledEdit 自身。LabeledEdit と一緒に破棄される)。返すときに Watch し、C++ 側は WrapExisting でラップする
  (ADR 0017 と同じ形)。ラベルの Parent と位置は、LabeledEdit の Parent・LabelPosition・LabelSpacing に合わせて LCL が決める。 }

function TLabeledEdit_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TLabeledEdit.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomLabeledEdit_GetEditLabel(Obj: Pointer): Pointer; BETH_CALL;
var
  L: TBoundLabel;
begin
  try
    L := TCustomLabeledEdit(Obj).EditLabel;
    if L = nil then
      Result := nil
    else
      Result := Watch(L);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomLabeledEdit_GetLabelPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomLabeledEdit(Obj).LabelPosition);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomLabeledEdit_SetLabelPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomLabeledEdit(Obj).LabelPosition := TLabelPosition(Value);
  except
    ReportException;
  end;
end;

function TCustomLabeledEdit_GetLabelSpacing(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomLabeledEdit(Obj).LabelSpacing;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomLabeledEdit_SetLabelSpacing(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomLabeledEdit(Obj).LabelSpacing := Value;
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 1、7 バッチ目(最後のバッチ)。
  Tabs/TabIndex/OnChange は TCustomTabControl では protected だが、唯一の具象クラス TTabControl が
  独自のフィールドで再宣言して published にしているため、TTabControl(Obj) で直接アクセスする
  (TUpDown と同じ形)。TPageControl/TTabSheet(所有ページの生成・破棄)は今回見送る。 }

function TTabControl_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TTabControl.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ Tabs(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TTabControl_GetTabs(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TTabControl(Obj).Tabs);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTabControl_GetTabIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTabControl(Obj).TabIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TTabControl_SetTabIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TTabControl(Obj).TabIndex := Value;
  except
    ReportException;
  end;
end;

procedure TTabControl_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTabControl(Obj).OnChange := @BridgeFor(TTabControl(Obj), MethodData(TTabControl(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TSplitter: 隣の Align 済みコントロール(同じ Align を持つ直前のコントロール)の幅・高さをドラッグで変える。
  Align=alLeft/alRight なら縦のバー、alTop/alBottom なら横のバーになる(既定は alLeft)。
  メンバはすべて TCustomSplitter の public。OnCanResize/OnCanOffset(var 引数 2 つの独自のイベント形)は今回見送る。 }

function TSplitter_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TSplitter.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomSplitter_GetAutoSnap(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomSplitter(Obj).AutoSnap;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomSplitter_SetAutoSnap(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomSplitter(Obj).AutoSnap := Value;
  except
    ReportException;
  end;
end;

function TCustomSplitter_GetBeveled(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomSplitter(Obj).Beveled;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomSplitter_SetBeveled(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomSplitter(Obj).Beveled := Value;
  except
    ReportException;
  end;
end;

function TCustomSplitter_GetMinSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSplitter(Obj).MinSize;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSplitter_SetMinSize(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSplitter(Obj).MinSize := Value;
  except
    ReportException;
  end;
end;

{ TAnchorKind の序数(akTop=0, akLeft, akRight, akBottom)。 }
function TCustomSplitter_GetResizeAnchor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomSplitter(Obj).ResizeAnchor);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSplitter_SetResizeAnchor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSplitter(Obj).ResizeAnchor := TAnchorKind(Value);
  except
    ReportException;
  end;
end;

{ TResizeStyle の序数(rsLine=0, rsNone, rsPattern, rsUpdate)。 }
function TCustomSplitter_GetResizeStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomSplitter(Obj).ResizeStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSplitter_SetResizeStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSplitter(Obj).ResizeStyle := TResizeStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomSplitter_GetSplitterPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSplitter(Obj).GetSplitterPosition;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSplitter_SetSplitterPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSplitter(Obj).SetSplitterPosition(Value);
  except
    ReportException;
  end;
end;

procedure TCustomSplitter_SetOnMoved(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomSplitter(Obj).OnMoved := @BridgeFor(TCustomSplitter(Obj), MethodData(TCustomSplitter(Obj).OnMoved), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 5(メニュー)。
  TMenu.Items(ルートの TMenuItem)は LCL が TMenu の中で生成し、*_Create を経由しない。
  このようにコンポーネントを返す関数のうち、LCL が内部で生成したものを返しうるもの(TMenu_GetItems・
  TMenuItem_GetItem・TMenuItem_GetParent)は、返す前に Watch で破棄通知の対象に登録する
  (FreeNotification は同じ相手に何度呼んでも 1 回分しか登録されない)。
  これにより、C++ 側は初めて受け取ったハンドルにラッパーを後から作っても、破棄通知で寿命を合わせられる。 }

{ ShortCut: Key は仮想キーコード、Shift は beth_ss* のビット集合。VCL と同じ値(scShift=$2000 等)を返す。 }
function ShortCut_Make(Key: Integer; Shift: LongWord): Integer; BETH_CALL;
begin
  try
    Result := Menus.ShortCut(Word(Key), IntToShiftState(Shift));
  except
    Result := 0;
    ReportException;
  end;
end;

function ShortCut_FromText(Text: PChar): Integer; BETH_CALL;
begin
  try
    Result := TextToShortCut(Text);
  except
    Result := 0;
    ReportException;
  end;
end;

function ShortCut_ToText(Value: Integer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(ShortCutToText(TShortCut(Value)));
  except
    Result := '';
    ReportException;
  end;
end;

{ TMenuItem }

function TMenuItem_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TMenuItem.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TMenuItem_GetCaption(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TMenuItem(Obj).Caption);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TMenuItem_SetCaption(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TMenuItem(Obj).Caption := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetChecked(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).Checked;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TMenuItem_SetChecked(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TMenuItem(Obj).Checked := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetEnabled(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).Enabled;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TMenuItem_SetEnabled(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TMenuItem(Obj).Enabled := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).Visible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TMenuItem_SetVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TMenuItem(Obj).Visible := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetAutoCheck(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).AutoCheck;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TMenuItem_SetAutoCheck(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TMenuItem(Obj).AutoCheck := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetRadioItem(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).RadioItem;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TMenuItem_SetRadioItem(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TMenuItem(Obj).RadioItem := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetGroupIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).GroupIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TMenuItem_SetGroupIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TMenuItem(Obj).GroupIndex := Byte(Value);
  except
    ReportException;
  end;
end;

function TMenuItem_GetDefault(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).Default;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TMenuItem_SetDefault(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TMenuItem(Obj).Default := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetShortCut(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).ShortCut;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TMenuItem_SetShortCut(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TMenuItem(Obj).ShortCut := TShortCut(Value);
  except
    ReportException;
  end;
end;

function TMenuItem_GetHint(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TMenuItem(Obj).Hint);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TMenuItem_SetHint(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TMenuItem(Obj).Hint := Value;
  except
    ReportException;
  end;
end;

procedure TMenuItem_SetOnClick(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).OnClick := @BridgeFor(TMenuItem(Obj), MethodData(TMenuItem(Obj).OnClick), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

function TMenuItem_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TMenuItem_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TMenuItem(Obj).Items[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

{ 親の TMenuItem(TMenu のルートの Items 直下の項目なら、そのルート)。どこにも追加されていなければ nil。 }
function TMenuItem_GetParent(Obj: Pointer): Pointer; BETH_CALL;
var
  P: TMenuItem;
begin
  try
    P := TMenuItem(Obj).Parent;
    if P = nil then
      Result := nil
    else
      Result := Watch(P);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TMenuItem_Add(Obj: Pointer; Item: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Add(TMenuItem(Item));
  except
    ReportException;
  end;
end;

procedure TMenuItem_Insert(Obj: Pointer; Index: Integer; Item: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Insert(Index, TMenuItem(Item));
  except
    ReportException;
  end;
end;

{ Delete/Remove は子から外すだけで破棄しない(VCL と同じ。破棄は Owner に任せるか、明示的に行う)。
  Clear はすべての子を破棄する。 }
procedure TMenuItem_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure TMenuItem_Remove(Obj: Pointer; Item: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Remove(TMenuItem(Item));
  except
    ReportException;
  end;
end;

procedure TMenuItem_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Clear;
  except
    ReportException;
  end;
end;

function TMenuItem_IndexOf(Obj: Pointer; Item: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).IndexOf(TMenuItem(Item));
  except
    Result := 0;
    ReportException;
  end;
end;

{ 区切り線(Caption が '-' の項目)を末尾に追加する。追加される項目は LCL が内部で生成する(Owner はこの項目)。 }
procedure TMenuItem_AddSeparator(Obj: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).AddSeparator;
  except
    ReportException;
  end;
end;

function TMenuItem_IsLine(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).IsLine;
  except
    Result := False;
    ReportException;
  end;
end;

{ 利用者が項目を選んだときと同じ処理(AutoCheck の反映と OnClick)を行う。 }
procedure TMenuItem_Click(Obj: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Click;
  except
    ReportException;
  end;
end;

{ TMenu / TMainMenu / TPopupMenu }

function TMenu_GetItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TMenu(Obj).Items);
  except
    Result := nil;
    ReportException;
  end;
end;

function TMainMenu_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TMainMenu.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TPopupMenu_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TPopupMenu.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ X, Y はスクリーン座標。Win32 ではメニューが閉じるまで戻らない。 }
procedure TPopupMenu_Popup(Obj: Pointer; X, Y: Integer); BETH_CALL;
begin
  try
    TPopupMenu(Obj).PopUp(X, Y);
  except
    ReportException;
  end;
end;

function TPopupMenu_GetAutoPopup(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TPopupMenu(Obj).AutoPopup;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TPopupMenu_SetAutoPopup(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TPopupMenu(Obj).AutoPopup := Value;
  except
    ReportException;
  end;
end;

{ 右クリック等でメニューを開いたコントロール(OnPopup の中で、どのコントロールから開かれたかを知るのに使う)。 }
function TPopupMenu_GetPopupComponent(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPopupMenu(Obj).PopupComponent);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TPopupMenu_SetPopupComponent(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TPopupMenu(Obj).PopupComponent := TComponent(Value);
  except
    ReportException;
  end;
end;

procedure TPopupMenu_SetOnPopup(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TPopupMenu(Obj).OnPopup := @BridgeFor(TPopupMenu(Obj), MethodData(TPopupMenu(Obj).OnPopup), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TPopupMenu_SetOnClose(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TPopupMenu(Obj).OnClose := @BridgeFor(TPopupMenu(Obj), MethodData(TPopupMenu(Obj).OnClose), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TCustomForm.Menu(public。TForm が published)と TControl.PopupMenu(public)。 }

function TCustomForm_GetMenu(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomForm(Obj).Menu);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomForm_SetMenu(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).Menu := TMainMenu(Value);
  except
    ReportException;
  end;
end;

function TControl_GetPopupMenu(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TControl(Obj).PopupMenu);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TControl_SetPopupMenu(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).PopupMenu := TPopupMenu(Value);
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 2、1 バッチ目(TPageControl + TTabSheet)。
  TPageControl は TTabControl と同じ TCustomTabControl の派生。TabIndex/OnChange は TCustomTabControl の protected を
  TPageControl が published にしているため、TPageControl(Obj) で直接アクセスする(関数名も TPageControl_*)。
  AddTabSheet が生成するページは LCL の内部で生成される(Owner はページコントロール)ため、ページを返す関数は
  TMenu_GetItems と同じく Watch してから返す(docs/adr/0017-... を参照)。 }

function TPageControl_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TPageControl.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function WatchOrNil(C: TComponent): Pointer;
begin
  if C = nil then
    Result := nil
  else
    Result := Watch(C);
end;

function TPageControl_GetActivePage(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchOrNil(TPageControl(Obj).ActivePage);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TPageControl_SetActivePage(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TPageControl(Obj).ActivePage := TTabSheet(Value);
  except
    ReportException;
  end;
end;

function TPageControl_GetActivePageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TPageControl(Obj).ActivePageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TPageControl_SetActivePageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TPageControl(Obj).ActivePageIndex := Value;
  except
    ReportException;
  end;
end;

function TPageControl_GetPage(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchOrNil(TPageControl(Obj).Pages[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomTabControl_GetPageCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTabControl(Obj).PageCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TPageControl_AddTabSheet(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TPageControl(Obj).AddTabSheet);
  except
    Result := nil;
    ReportException;
  end;
end;

{ すべてのページを外して破棄する。LCL の TNBPages.Delete は Application.ReleaseComponent を使うため、
  破棄は遅延され、次にメッセージを処理したとき(または Owner の破棄時)に行われる。 }
procedure TPageControl_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TPageControl(Obj).Clear;
  except
    ReportException;
  end;
end;

procedure TPageControl_SelectNextPage(Obj: Pointer; GoForward: LongBool); BETH_CALL;
begin
  try
    TPageControl(Obj).SelectNextPage(GoForward);
  except
    ReportException;
  end;
end;

function TPageControl_GetTabIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TPageControl(Obj).TabIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TPageControl_SetTabIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TPageControl(Obj).TabIndex := Value;
  except
    ReportException;
  end;
end;

procedure TPageControl_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TPageControl(Obj).OnChange := @BridgeFor(TPageControl(Obj), MethodData(TPageControl(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ OnChanging(TTabChangingEvent: Sender, var AllowChange)は OnCloseQuery と同じ形のため、同じブリッジ(DoCloseQuery)を使う。 }
procedure TCustomTabControl_SetOnChanging(Obj: Pointer; Cb: TBethVarCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomTabControl(Obj).OnChanging := @VarBridgeFor(TComponent(Obj), MethodData(TCustomTabControl(Obj).OnChanging), Cb, Data).DoCloseQuery;
  except
    ReportException;
  end;
end;

function TCustomTabControl_GetMultiLine(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomTabControl(Obj).MultiLine;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomTabControl_SetMultiLine(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomTabControl(Obj).MultiLine := Value;
  except
    ReportException;
  end;
end;

function TCustomTabControl_GetShowTabs(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomTabControl(Obj).ShowTabs;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomTabControl_SetShowTabs(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomTabControl(Obj).ShowTabs := Value;
  except
    ReportException;
  end;
end;

{ TTabPosition の序数(tpTop=0, tpBottom, tpLeft, tpRight)。 }
function TCustomTabControl_GetTabPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomTabControl(Obj).TabPosition);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTabControl_SetTabPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTabControl(Obj).TabPosition := TTabPosition(Value);
  except
    ReportException;
  end;
end;

{ TTabSheet(TCustomPage の派生)。タブの文字列は Caption(TControl_SetCaption)。 }

function TTabSheet_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TTabSheet.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TTabSheet_GetPageControl(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TTabSheet(Obj).PageControl);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TTabSheet_SetPageControl(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TTabSheet(Obj).PageControl := TPageControl(Value);
  except
    ReportException;
  end;
end;

function TTabSheet_GetTabIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTabSheet(Obj).TabIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCustomPage_GetPageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomPage(Obj).PageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPage_SetPageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPage(Obj).PageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomPage_GetTabVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomPage(Obj).TabVisible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomPage_SetTabVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomPage(Obj).TabVisible := Value;
  except
    ReportException;
  end;
end;

procedure TCustomPage_SetOnShow(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomPage(Obj).OnShow := @BridgeFor(TCustomPage(Obj), MethodData(TCustomPage(Obj).OnShow), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TCustomPage_SetOnHide(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomPage(Obj).OnHide := @BridgeFor(TCustomPage(Obj), MethodData(TCustomPage(Obj).OnHide), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 2、2 バッチ目(TTreeView)。docs/adr/0019-... を参照。
  TTreeNode・TTreeNodes は TComponent ではなく TPersistent で、FreeNotification が使えない。
  ノードの破棄は、ハンドルを C 側へ渡すときに付ける観察者(WatchItem)で通知する(docs/adr/0026 で ADR 0019 の方式を置き換えた)。 }

type
  { TComponent ではない項目(ツリービューのノード・リストビューの項目や列)を 1 つ受け取るイベント用。
    ツリービューの OnChange/OnExpanded/OnCollapsed/OnDeletion、リストビューの OnDeletion/OnItemChecked/OnColumnClick、
    ヘッダーコントロールの OnSectionClick/OnSectionResize/OnSectionSeparatorDblClick。
    イベントの型ごとに引数の型が異なるため、メソッドを分ける(コールバックの形は同じ)。 }
  TBethItemCallback = procedure(Sender: Pointer; Item: Pointer; Data: Pointer); BETH_CALL;

  TItemCallbackBridge = class(TComponent)
  private
    FCallback: TBethItemCallback;
    FData: Pointer;
  public
    procedure DoNode(Sender: TObject; Node: TTreeNode);
    procedure DoListItem(Sender: TObject; Item: TListItem);
    procedure DoColumn(Sender: TObject; Column: TListColumn);
    procedure DoSection(HeaderControl: TCustomHeaderControl; Section: THeaderSection);
  end;

  { 項目と整数(または真偽値)を 1 つずつ受け取るイベント用。リストビューの OnSelectItem(Selected)・OnChange(TItemChange の序数)。 }
  TBethItemIntCallback = procedure(Sender: Pointer; Item: Pointer; Value: Integer; Data: Pointer); BETH_CALL;

  TItemIntCallbackBridge = class(TComponent)
  private
    FCallback: TBethItemIntCallback;
    FData: Pointer;
  public
    procedure DoSelectItem(Sender: TObject; Item: TListItem; Selected: Boolean);
    procedure DoItemChange(Sender: TObject; Item: TListItem; Change: TItemChange);
  end;

  { OnChanging/OnExpanding/OnCollapsing(Sender, Node, var Allow)用。Allow は書き換え可能(0 = False)。 }
  TBethItemAllowCallback = procedure(Sender: Pointer; Node: Pointer; Allow: PInteger; Data: Pointer); BETH_CALL;

  TItemAllowCallbackBridge = class(TComponent)
  private
    FCallback: TBethItemAllowCallback;
    FData: Pointer;
  public
    procedure DoNodeAllow(Sender: TObject; Node: TTreeNode; var Allow: Boolean);
  end;

{ TComponent ではない項目(TTreeNode・TListItem・TListColumn・THeaderSection・TCoolBand。いずれも TPersistent)の破棄通知。
  項目のハンドルを C 側へ渡すとき(関数の戻り値・イベントの引数)に WatchItem で FPC の TPersistent の観察者を付け、
  TPersistent.Destroy が送る ooFree で項目の破棄を通知する(コンポーネントの Watch と FreeNotification と同じ考え方)。
  ooFree は破棄の最後(派生クラスのデストラクタの処理がすべて終わった後)に、コレクションの Clear・破棄の途中でも必ず送られる。
  破棄の最初(TCustomTreeView.Delete 等)で通知すると、その後の破棄の処理から呼ばれたイベント(OnCollapsing 等)で
  C++ 側が同じ項目を再びラップし、その登録が残ってしまう(docs/adr/0026)。 }

type
  { 参照カウントをしない TComponent の IInterface 実装を使う(観察者の一覧に入れても解放されない)。 }
  TItemFreeObserver = class(TComponent, IFPObserver)
  public
    procedure FPOObservedChanged(ASender: TObject; Operation: TFPObservedOperation; Data: Pointer);
  end;

var
  GItemFreeObserver: TItemFreeObserver = nil;
  GObservedItems: TAVLTree = nil;  // 観察者を付けた項目(同じ項目に二重に付けないため。ポインタで比較する)

procedure NotifyItemFreed(Item: TObject);
begin
  if Assigned(GItemFreeCallback) then
    GItemFreeCallback(Pointer(Item), GItemFreeData);
end;

procedure TItemFreeObserver.FPOObservedChanged(ASender: TObject; Operation: TFPObservedOperation; Data: Pointer);
var
  Node: TAVLTreeNode;
begin
  if Operation = ooFree then
  begin
    Node := GObservedItems.Find(ASender);
    if Node <> nil then
      GObservedItems.Delete(Node);
    NotifyItemFreed(ASender);
  end;
end;

function WatchItem(Item: TPersistent): Pointer;
begin
  Result := Pointer(Item);
  if Item = nil then
    Exit;
  if GItemFreeObserver = nil then
  begin
    GItemFreeObserver := TItemFreeObserver.Create(nil);
    GObservedItems := TAVLTree.Create;  // 既定の比較はポインタの大小
  end;
  if GObservedItems.Find(Item) = nil then
  begin
    Item.FPOAttachObserver(GItemFreeObserver);
    GObservedItems.Add(Item);
  end;
end;

procedure TItemCallbackBridge.DoNode(Sender: TObject; Node: TTreeNode);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), WatchItem(Node), FData);
  CheckCallbackError;
end;

procedure TItemCallbackBridge.DoListItem(Sender: TObject; Item: TListItem);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), WatchItem(Item), FData);
  CheckCallbackError;
end;

procedure TItemCallbackBridge.DoColumn(Sender: TObject; Column: TListColumn);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), WatchItem(Column), FData);
  CheckCallbackError;
end;

procedure TItemCallbackBridge.DoSection(HeaderControl: TCustomHeaderControl; Section: THeaderSection);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(HeaderControl), WatchItem(Section), FData);
  CheckCallbackError;
end;

procedure TItemIntCallbackBridge.DoSelectItem(Sender: TObject; Item: TListItem; Selected: Boolean);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if Selected then
    FCallback(Pointer(Sender), WatchItem(Item), -1, FData)
  else
    FCallback(Pointer(Sender), WatchItem(Item), 0, FData);
  CheckCallbackError;
end;

procedure TItemIntCallbackBridge.DoItemChange(Sender: TObject; Item: TListItem; Change: TItemChange);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), WatchItem(Item), Ord(Change), FData);
  CheckCallbackError;
end;

procedure TItemAllowCallbackBridge.DoNodeAllow(Sender: TObject; Node: TTreeNode; var Allow: Boolean);
var
  A: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if Allow then A := -1 else A := 0;
  FCallback(Pointer(Sender), WatchItem(Node), @A, FData);
  Allow := A <> 0;
  CheckCallbackError;
end;

{ BridgeFor と同じく、同じイベントに何度登録してもブリッジを再利用する。
  イベントの型(TTVChangedEvent・TTVExpandedEvent 等)は構造が同じ別名の型のため、MethodData の多重定義ではなく
  呼び出し側で TMethod(...).Data を渡す。 }
function ItemBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethItemCallback; Data: Pointer): TItemCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TItemCallbackBridge) and (TItemCallbackBridge(Current).Owner = Owner) then
    Result := TItemCallbackBridge(Current)
  else
    Result := TItemCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function ItemAllowBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethItemAllowCallback; Data: Pointer): TItemAllowCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TItemAllowCallbackBridge) and (TItemAllowCallbackBridge(Current).Owner = Owner) then
    Result := TItemAllowCallbackBridge(Current)
  else
    Result := TItemAllowCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function ItemIntBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethItemIntCallback; Data: Pointer): TItemIntCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TItemIntCallbackBridge) and (TItemIntCallbackBridge(Current).Owner = Owner) then
    Result := TItemIntCallbackBridge(Current)
  else
    Result := TItemIntCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

procedure ItemFree_SetCallback(Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    GItemFreeCallback := Cb;
    GItemFreeData := Data;
  except
    ReportException;
  end;
end;

{ TTreeView / TCustomTreeView }

function TTreeView_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TTreeView.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ Items(TTreeNodes)はツリービューが所有する非所有のハンドル(TCanvas と同じく、ツリービューと寿命が一致する)。 }
function TCustomTreeView_GetItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomTreeView(Obj).Items);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomTreeView_GetSelected(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCustomTreeView(Obj).Selected);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomTreeView_SetSelected(Obj: Pointer; Node: Pointer); BETH_CALL;
begin
  try
    TCustomTreeView(Obj).Selected := TTreeNode(Node);
  except
    ReportException;
  end;
end;

procedure TCustomTreeView_FullExpand(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomTreeView(Obj).FullExpand;
  except
    ReportException;
  end;
end;

procedure TCustomTreeView_FullCollapse(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomTreeView(Obj).FullCollapse;
  except
    ReportException;
  end;
end;

function TCustomTreeView_AlphaSort(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomTreeView(Obj).AlphaSort;
  except
    Result := False;
    ReportException;
  end;
end;

{ X, Y はツリービューのクライアント座標。そこにノードが無ければ nil。 }
function TCustomTreeView_GetNodeAt(Obj: Pointer; X, Y: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCustomTreeView(Obj).GetNodeAt(X, Y));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TCustomTreeView の protected を TTreeView が published にしているプロパティ。 }

function TTreeView_GetReadOnly(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeView(Obj).ReadOnly;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeView_SetReadOnly(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeView(Obj).ReadOnly := Value;
  except
    ReportException;
  end;
end;

function TTreeView_GetShowLines(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeView(Obj).ShowLines;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeView_SetShowLines(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeView(Obj).ShowLines := Value;
  except
    ReportException;
  end;
end;

function TTreeView_GetShowRoot(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeView(Obj).ShowRoot;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeView_SetShowRoot(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeView(Obj).ShowRoot := Value;
  except
    ReportException;
  end;
end;

function TTreeView_GetShowButtons(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeView(Obj).ShowButtons;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeView_SetShowButtons(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeView(Obj).ShowButtons := Value;
  except
    ReportException;
  end;
end;

function TTreeView_GetAutoExpand(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeView(Obj).AutoExpand;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeView_SetAutoExpand(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeView(Obj).AutoExpand := Value;
  except
    ReportException;
  end;
end;

function TTreeView_GetHideSelection(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeView(Obj).HideSelection;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeView_SetHideSelection(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeView(Obj).HideSelection := Value;
  except
    ReportException;
  end;
end;

function TTreeView_GetRowSelect(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeView(Obj).RowSelect;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeView_SetRowSelect(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeView(Obj).RowSelect := Value;
  except
    ReportException;
  end;
end;

procedure TTreeView_SetOnChange(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTreeView(Obj).OnChange := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnChange).Data, Cb, Data).DoNode;
  except
    ReportException;
  end;
end;

procedure TTreeView_SetOnExpanded(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTreeView(Obj).OnExpanded := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnExpanded).Data, Cb, Data).DoNode;
  except
    ReportException;
  end;
end;

procedure TTreeView_SetOnCollapsed(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTreeView(Obj).OnCollapsed := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnCollapsed).Data, Cb, Data).DoNode;
  except
    ReportException;
  end;
end;

procedure TTreeView_SetOnDeletion(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTreeView(Obj).OnDeletion := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnDeletion).Data, Cb, Data).DoNode;
  except
    ReportException;
  end;
end;

procedure TTreeView_SetOnChanging(Obj: Pointer; Cb: TBethItemAllowCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTreeView(Obj).OnChanging := @ItemAllowBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnChanging).Data, Cb, Data).DoNodeAllow;
  except
    ReportException;
  end;
end;

procedure TTreeView_SetOnExpanding(Obj: Pointer; Cb: TBethItemAllowCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTreeView(Obj).OnExpanding := @ItemAllowBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnExpanding).Data, Cb, Data).DoNodeAllow;
  except
    ReportException;
  end;
end;

procedure TTreeView_SetOnCollapsing(Obj: Pointer; Cb: TBethItemAllowCallback; Data: Pointer); BETH_CALL;
begin
  try
    TTreeView(Obj).OnCollapsing := @ItemAllowBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnCollapsing).Data, Cb, Data).DoNodeAllow;
  except
    ReportException;
  end;
end;

{ TTreeNodes。Sibling/Parent に nil を渡すと最上位のノードになる(LCL と同じ)。 }

function TTreeNodes_Add(Obj: Pointer; Sibling: Pointer; Text: PChar): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).Add(TTreeNode(Sibling), Text));
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNodes_AddFirst(Obj: Pointer; Sibling: Pointer; Text: PChar): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).AddFirst(TTreeNode(Sibling), Text));
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNodes_AddChild(Obj: Pointer; Parent: Pointer; Text: PChar): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).AddChild(TTreeNode(Parent), Text));
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNodes_AddChildFirst(Obj: Pointer; Parent: Pointer; Text: PChar): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).AddChildFirst(TTreeNode(Parent), Text));
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNodes_Insert(Obj: Pointer; NextNode: Pointer; Text: PChar): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).Insert(TTreeNode(NextNode), Text));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TTreeNodes_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TTreeNodes(Obj).Clear;
  except
    ReportException;
  end;
end;

procedure TTreeNodes_Delete(Obj: Pointer; Node: Pointer); BETH_CALL;
begin
  try
    TTreeNodes(Obj).Delete(TTreeNode(Node));
  except
    ReportException;
  end;
end;

{ すべてのノード(子孫を含む)の数。GetItem の Index は、上から順に数えた位置(AbsoluteIndex)。 }
function TTreeNodes_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNodes(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TTreeNodes_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).Item[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNodes_GetFirstNode(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).GetFirstNode);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNodes_FindNodeWithText(Obj: Pointer; Text: PChar): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNodes(Obj).FindNodeWithText(Text));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TTreeNodes_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TTreeNodes(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TTreeNodes_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TTreeNodes(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ TTreeNode }

function TTreeNode_GetText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TTreeNode(Obj).Text);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TTreeNode_SetText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TTreeNode(Obj).Text := Value;
  except
    ReportException;
  end;
end;

function TTreeNode_GetExpanded(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).Expanded;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeNode_SetExpanded(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeNode(Obj).Expanded := Value;
  except
    ReportException;
  end;
end;

function TTreeNode_GetSelected(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).Selected;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeNode_SetSelected(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeNode(Obj).Selected := Value;
  except
    ReportException;
  end;
end;

function TTreeNode_GetHasChildren(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).HasChildren;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TTreeNode_SetHasChildren(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TTreeNode(Obj).HasChildren := Value;
  except
    ReportException;
  end;
end;

{ 利用者データ(LCL は解釈しない)。 }
function TTreeNode_GetData(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).Data;
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TTreeNode_SetData(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TTreeNode(Obj).Data := Value;
  except
    ReportException;
  end;
end;

function TTreeNode_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TTreeNode_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).Items[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).Index;
  except
    Result := 0;
    ReportException;
  end;
end;

function TTreeNode_GetLevel(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).Level;
  except
    Result := 0;
    ReportException;
  end;
end;

function TTreeNode_GetAbsoluteIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).AbsoluteIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

function TTreeNode_GetParent(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).Parent);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_GetTreeView(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TTreeNode(Obj).TreeView);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_GetFirstChild(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).GetFirstChild);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_GetLastChild(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).GetLastChild);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_GetNextSibling(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).GetNextSibling);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_GetPrevSibling(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).GetPrevSibling);
  except
    Result := nil;
    ReportException;
  end;
end;

{ 上から順(子孫を含む)の次/前のノード。 }
function TTreeNode_GetNext(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).GetNext);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_GetPrev(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TTreeNode(Obj).GetPrev);
  except
    Result := nil;
    ReportException;
  end;
end;

function TTreeNode_IndexOf(Obj: Pointer; Node: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).IndexOf(TTreeNode(Node));
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TTreeNode_Expand(Obj: Pointer; Recurse: LongBool); BETH_CALL;
begin
  try
    TTreeNode(Obj).Expand(Recurse);
  except
    ReportException;
  end;
end;

procedure TTreeNode_Collapse(Obj: Pointer; Recurse: LongBool); BETH_CALL;
begin
  try
    TTreeNode(Obj).Collapse(Recurse);
  except
    ReportException;
  end;
end;

{ このノード(と子孫)を削除する。削除されたノードごとに OnDeletion と破棄通知が呼ばれる。 }
procedure TTreeNode_Delete(Obj: Pointer); BETH_CALL;
begin
  try
    TTreeNode(Obj).Delete;
  except
    ReportException;
  end;
end;

procedure TTreeNode_DeleteChildren(Obj: Pointer); BETH_CALL;
begin
  try
    TTreeNode(Obj).DeleteChildren;
  except
    ReportException;
  end;
end;

procedure TTreeNode_MakeVisible(Obj: Pointer); BETH_CALL;
begin
  try
    TTreeNode(Obj).MakeVisible;
  except
    ReportException;
  end;
end;

{ Mode は TNodeAttachMode の序数(naAdd=0, naAddFirst, naAddChild, naAddChildFirst, naInsert, naInsertBehind)。 }
procedure TTreeNode_MoveTo(Obj: Pointer; Destination: Pointer; Mode: Integer); BETH_CALL;
begin
  try
    TTreeNode(Obj).MoveTo(TTreeNode(Destination), TNodeAttachMode(Mode));
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 2、3 バッチ目(TListView)。docs/adr/0020-... を参照。
  項目(TListItem。TPersistent)と列(TListColumn。TCollectionItem)は TTreeNode と同じく、ハンドルを C 側へ渡すときに付ける
  観察者(WatchItem)で破棄を通知する(docs/adr/0026 で ADR 0020 の方式を置き換えた)。
  TCustomListView.Destroy は inherited Destroy(=破棄通知)の後で項目を破棄するため、リストビュー自身のラッパーが
  delete された後にも項目の破棄通知が届く(項目のレジストリはリストビューのラッパーに依存しないので問題ない)。 }

{ TListView / TCustomListView }

function TListView_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TListView.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ Items(TListItems)と Columns(TListColumns)は、リストビューが所有する非所有のハンドル(リストビューと寿命が一致する)。 }
function TCustomListView_GetItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomListView(Obj).Items);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomListView_GetSelected(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCustomListView(Obj).Selected);
  except
    Result := nil;
    ReportException;
  end;
end;

{ LCL の TCustomListView.SetSelection は、ウィンドウハンドルが無いと(フォームの表示前・表示されていないページの上)
  内部の FSelected を覚えるだけで項目の選択状態を変えず、GetSelection は項目の状態から選択を探し直すため nil を返す
  (フォームのコンストラクタで Selected を設定しても効かない)。VCL と同じくハンドルの有無によらず選択されるよう、
  ハンドルが無いときは項目の Selected も設定する(MultiSelect でなければ他の項目の選択を外す)。 }
procedure SelectListItem(LV: TCustomListView; Item: TListItem);
var
  I: Integer;
begin
  LV.Selected := Item;
  if LV.HandleAllocated then
    Exit;
  if not LV.MultiSelect then
    for I := 0 to LV.Items.Count - 1 do
      if LV.Items[I] <> Item then
        LV.Items[I].Selected := False;
  if Item <> nil then
    Item.Selected := True;
end;

procedure TCustomListView_SetSelected(Obj: Pointer; Item: Pointer); BETH_CALL;
begin
  try
    SelectListItem(TCustomListView(Obj), TListItem(Item));
  except
    ReportException;
  end;
end;

function TCustomListView_GetItemIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomListView(Obj).ItemIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

{ -1 で選択を外す。範囲外の値は無視する。 }
procedure TCustomListView_SetItemIndex(Obj: Pointer; Value: Integer); BETH_CALL;
var
  LV: TCustomListView;
begin
  try
    LV := TCustomListView(Obj);
    if Value = -1 then
      SelectListItem(LV, nil)
    else if (Value >= 0) and (Value < LV.Items.Count) then
      SelectListItem(LV, LV.Items[Value]);
  except
    ReportException;
  end;
end;

function TCustomListView_GetSelCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomListView(Obj).SelCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCustomListView_GetCheckboxes(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListView(Obj).Checkboxes;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListView_SetCheckboxes(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListView(Obj).Checkboxes := Value;
  except
    ReportException;
  end;
end;

function TCustomListView_GetGridLines(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListView(Obj).GridLines;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListView_SetGridLines(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListView(Obj).GridLines := Value;
  except
    ReportException;
  end;
end;

function TCustomListView_GetMultiSelect(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListView(Obj).MultiSelect;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListView_SetMultiSelect(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListView(Obj).MultiSelect := Value;
  except
    ReportException;
  end;
end;

function TCustomListView_GetReadOnly(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListView(Obj).ReadOnly;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListView_SetReadOnly(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListView(Obj).ReadOnly := Value;
  except
    ReportException;
  end;
end;

function TCustomListView_GetRowSelect(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListView(Obj).RowSelect;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListView_SetRowSelect(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListView(Obj).RowSelect := Value;
  except
    ReportException;
  end;
end;

procedure TCustomListView_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomListView(Obj).Clear;
  except
    ReportException;
  end;
end;

procedure TCustomListView_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomListView(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TCustomListView_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomListView(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ X, Y はクライアント座標。そこに項目が無ければ nil。 }
function TCustomListView_GetItemAt(Obj: Pointer; X, Y: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCustomListView(Obj).GetItemAt(X, Y));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomListView_ClearSelection(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomListView(Obj).ClearSelection;
  except
    ReportException;
  end;
end;

procedure TCustomListView_SelectAll(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomListView(Obj).SelectAll;
  except
    ReportException;
  end;
end;

{ TCustomListView の protected を TListView が published にしているメンバ。 }

function TListView_GetColumns(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TListView(Obj).Columns);
  except
    Result := nil;
    ReportException;
  end;
end;

{ TViewStyle の序数(vsIcon=0, vsSmallIcon, vsList, vsReport)。 }
function TListView_GetViewStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TListView(Obj).ViewStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListView_SetViewStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListView(Obj).ViewStyle := TViewStyle(Value);
  except
    ReportException;
  end;
end;

function TListView_GetHideSelection(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TListView(Obj).HideSelection;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TListView_SetHideSelection(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TListView(Obj).HideSelection := Value;
  except
    ReportException;
  end;
end;

{ TSortType の序数(stNone=0, stData, stText, stBoth)。 }
function TListView_GetSortType(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TListView(Obj).SortType);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListView_SetSortType(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListView(Obj).SortType := TSortType(Value);
  except
    ReportException;
  end;
end;

{ 並べ替えに使う列の位置(0 が Caption の列)。既定の -1 のままでは、SortType を設定しても並べ替えない(LCL の Sort)。 }
function TListView_GetSortColumn(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListView(Obj).SortColumn;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListView_SetSortColumn(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListView(Obj).SortColumn := Value;
  except
    ReportException;
  end;
end;

{ TSortDirection の序数(sdAscending=0, sdDescending)。 }
function TListView_GetSortDirection(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TListView(Obj).SortDirection);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListView_SetSortDirection(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListView(Obj).SortDirection := TSortDirection(Value);
  except
    ReportException;
  end;
end;

procedure TListView_SetOnSelectItem(Obj: Pointer; Cb: TBethItemIntCallback; Data: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).OnSelectItem := @ItemIntBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnSelectItem).Data, Cb, Data).DoSelectItem;
  except
    ReportException;
  end;
end;

procedure TListView_SetOnChange(Obj: Pointer; Cb: TBethItemIntCallback; Data: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).OnChange := @ItemIntBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnChange).Data, Cb, Data).DoItemChange;
  except
    ReportException;
  end;
end;

procedure TListView_SetOnDeletion(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).OnDeletion := @ItemBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnDeletion).Data, Cb, Data).DoListItem;
  except
    ReportException;
  end;
end;

procedure TListView_SetOnItemChecked(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).OnItemChecked := @ItemBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnItemChecked).Data, Cb, Data).DoListItem;
  except
    ReportException;
  end;
end;

procedure TListView_SetOnColumnClick(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).OnColumnClick := @ItemBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnColumnClick).Data, Cb, Data).DoColumn;
  except
    ReportException;
  end;
end;

{ TListItems }

function TListItems_Add(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TListItems(Obj).Add);
  except
    Result := nil;
    ReportException;
  end;
end;

function TListItems_Insert(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TListItems(Obj).Insert(Index));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TListItems_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    TListItems(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure TListItems_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TListItems(Obj).Clear;
  except
    ReportException;
  end;
end;

function TListItems_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListItems(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TListItems_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TListItems(Obj).Item[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

function TListItems_IndexOf(Obj: Pointer; Item: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListItems(Obj).IndexOf(TListItem(Item));
  except
    Result := 0;
    ReportException;
  end;
end;

{ StartIndex の次(Inclusive なら StartIndex から)から Caption を探す。Partial なら前方一致、Wrap なら末尾から先頭へ続けて探す。 }
function TListItems_FindCaption(Obj: Pointer; StartIndex: Integer; Value: PChar; Partial, Inclusive, Wrap: LongBool): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TListItems(Obj).FindCaption(StartIndex, Value, Partial, Inclusive, Wrap));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TListItems_Exchange(Obj: Pointer; Index1, Index2: Integer); BETH_CALL;
begin
  try
    TListItems(Obj).Exchange(Index1, Index2);
  except
    ReportException;
  end;
end;

procedure TListItems_Move(Obj: Pointer; FromIndex, ToIndex: Integer); BETH_CALL;
begin
  try
    TListItems(Obj).Move(FromIndex, ToIndex);
  except
    ReportException;
  end;
end;

procedure TListItems_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TListItems(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TListItems_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TListItems(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ TListItem }

function TListItem_GetCaption(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TListItem(Obj).Caption);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TListItem_SetCaption(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TListItem(Obj).Caption := Value;
  except
    ReportException;
  end;
end;

function TListItem_GetChecked(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TListItem(Obj).Checked;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TListItem_SetChecked(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TListItem(Obj).Checked := Value;
  except
    ReportException;
  end;
end;

function TListItem_GetSelected(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TListItem(Obj).Selected;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TListItem_SetSelected(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TListItem(Obj).Selected := Value;
  except
    ReportException;
  end;
end;

function TListItem_GetFocused(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TListItem(Obj).Focused;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TListItem_SetFocused(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TListItem(Obj).Focused := Value;
  except
    ReportException;
  end;
end;

function TListItem_GetData(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := TListItem(Obj).Data;
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TListItem_SetData(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TListItem(Obj).Data := Value;
  except
    ReportException;
  end;
end;

function TListItem_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListItem(Obj).Index;
  except
    Result := 0;
    ReportException;
  end;
end;

function TListItem_GetListView(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TListItem(Obj).ListView);
  except
    Result := nil;
    ReportException;
  end;
end;

{ SubItems(2 列目以降の文字列)。 }

{ SubItems(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TListItem_GetSubItems(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TListItem(Obj).SubItems);
  except
    Result := nil;
    ReportException;
  end;
end;

{ この項目を削除する。OnDeletion と項目の破棄通知が呼ばれる。 }
procedure TListItem_Delete(Obj: Pointer); BETH_CALL;
begin
  try
    TListItem(Obj).Delete;
  except
    ReportException;
  end;
end;

procedure TListItem_MakeVisible(Obj: Pointer; PartialOK: LongBool); BETH_CALL;
begin
  try
    TListItem(Obj).MakeVisible(PartialOK);
  except
    ReportException;
  end;
end;

{ TListColumns / TListColumn }

function TListColumns_Add(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TListColumns(Obj).Add);
  except
    Result := nil;
    ReportException;
  end;
end;

function TListColumns_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListColumns(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TListColumns_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TListColumns(Obj).Items[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TListColumns_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    TListColumns(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure TListColumns_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TListColumns(Obj).Clear;
  except
    ReportException;
  end;
end;

function TListColumn_GetCaption(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TListColumn(Obj).Caption);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TListColumn_SetCaption(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TListColumn(Obj).Caption := Value;
  except
    ReportException;
  end;
end;

function TListColumn_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListColumn(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListColumn_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListColumn(Obj).Width := Value;
  except
    ReportException;
  end;
end;

{ TAlignment の序数(taLeftJustify=0, taRightJustify, taCenter)。 }
function TListColumn_GetAlignment(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TListColumn(Obj).Alignment);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListColumn_SetAlignment(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListColumn(Obj).Alignment := TAlignment(Value);
  except
    ReportException;
  end;
end;

function TListColumn_GetAutoSize(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TListColumn(Obj).AutoSize;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TListColumn_SetAutoSize(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TListColumn(Obj).AutoSize := Value;
  except
    ReportException;
  end;
end;

function TListColumn_GetVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TListColumn(Obj).Visible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TListColumn_SetVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TListColumn(Obj).Visible := Value;
  except
    ReportException;
  end;
end;

{ 列の並び順。書き換えると列が移動する。 }
function TListColumn_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListColumn(Obj).Index;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListColumn_SetIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListColumn(Obj).Index := Value;
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 2、4 バッチ目(TDrawGrid・TStringGrid)。docs/adr/0021-... を参照。
  セルは LCL でもオブジェクトではなく、(列, 行)の位置で指定するため、TTreeNode・TListItem のような寿命管理は要らない。
  主なメンバは TCustomGrid の protected を TCustomDrawGrid が public にしているため、関数名は TCustomDrawGrid_* にする。 }

type
  { OnSelection(Sender, ACol, ARow)・OnHeaderClick(Sender, IsColumn, Index)用。整数 2 つを渡す。 }
  TBethCellCallback = procedure(Sender: Pointer; A, B: Integer; Data: Pointer); BETH_CALL;

  TCellCallbackBridge = class(TComponent)
  private
    FCallback: TBethCellCallback;
    FData: Pointer;
  public
    procedure DoSelection(Sender: TObject; aCol, aRow: Integer);
    procedure DoHeaderClick(Sender: TObject; IsColumn: Boolean; Index: Integer);
  end;

  { OnSelectCell(Sender, ACol, ARow, var CanSelect)用。CanSelect は書き換え可能(0 = False)。 }
  TBethCellAllowCallback = procedure(Sender: Pointer; ACol, ARow: Integer; Allow: PInteger; Data: Pointer); BETH_CALL;

  TCellAllowCallbackBridge = class(TComponent)
  private
    FCallback: TBethCellAllowCallback;
    FData: Pointer;
  public
    procedure DoSelectCell(Sender: TObject; aCol, aRow: Integer; var CanSelect: Boolean);
  end;

  { OnDrawCell(Sender, ACol, ARow, Rect, State)用。State は TGridDrawState のビット集合(beth_gd*)。 }
  TBethDrawCellCallback = procedure(Sender: Pointer; ACol, ARow, Left, Top, Right, Bottom: Integer; State: LongWord; Data: Pointer); BETH_CALL;

  TDrawCellCallbackBridge = class(TComponent)
  private
    FCallback: TBethDrawCellCallback;
    FData: Pointer;
  public
    procedure DoDrawCell(Sender: TObject; aCol, aRow: Integer; aRect: TRect; aState: TGridDrawState);
  end;

procedure TCellCallbackBridge.DoSelection(Sender: TObject; aCol, aRow: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), aCol, aRow, FData);
  CheckCallbackError;
end;

procedure TCellCallbackBridge.DoHeaderClick(Sender: TObject; IsColumn: Boolean; Index: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if IsColumn then
    FCallback(Pointer(Sender), -1, Index, FData)
  else
    FCallback(Pointer(Sender), 0, Index, FData);
  CheckCallbackError;
end;

procedure TCellAllowCallbackBridge.DoSelectCell(Sender: TObject; aCol, aRow: Integer; var CanSelect: Boolean);
var
  A: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if CanSelect then A := -1 else A := 0;
  FCallback(Pointer(Sender), aCol, aRow, @A, FData);
  CanSelect := A <> 0;
  CheckCallbackError;
end;

procedure TDrawCellCallbackBridge.DoDrawCell(Sender: TObject; aCol, aRow: Integer; aRect: TRect; aState: TGridDrawState);
var
  S: LongWord;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  { TGridDrawState は名前の無い列挙型の集合のため、要素ごとに判定してビットに変換する。 }
  S := 0;
  if gdSelected in aState then S := S or $01;
  if gdFocused in aState then S := S or $02;
  if gdFixed in aState then S := S or $04;
  if gdHot in aState then S := S or $08;
  if gdPushed in aState then S := S or $10;
  if gdRowHighlight in aState then S := S or $20;
  FCallback(Pointer(Sender), aCol, aRow, aRect.Left, aRect.Top, aRect.Right, aRect.Bottom, S, FData);
  CheckCallbackError;
end;

function CellBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethCellCallback; Data: Pointer): TCellCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TCellCallbackBridge) and (TCellCallbackBridge(Current).Owner = Owner) then
    Result := TCellCallbackBridge(Current)
  else
    Result := TCellCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function CellAllowBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethCellAllowCallback; Data: Pointer): TCellAllowCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TCellAllowCallbackBridge) and (TCellAllowCallbackBridge(Current).Owner = Owner) then
    Result := TCellAllowCallbackBridge(Current)
  else
    Result := TCellAllowCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function DrawCellBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethDrawCellCallback; Data: Pointer): TDrawCellCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TDrawCellCallbackBridge) and (TDrawCellCallbackBridge(Current).Owner = Owner) then
    Result := TDrawCellCallbackBridge(Current)
  else
    Result := TDrawCellCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

{ TGridOptions(集合型)と LongWord のビット集合(beth_go* と対応。ビットの位置は TGridOption の序数)の変換。 }
function GridOptionsToInt(const O: TGridOptions): LongWord;
var
  I: TGridOption;
begin
  Result := 0;
  for I := Low(TGridOption) to High(TGridOption) do
    if I in O then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToGridOptions(V: LongWord): TGridOptions;
var
  I: TGridOption;
begin
  Result := [];
  for I := Low(TGridOption) to High(TGridOption) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

{ TDrawGrid / TStringGrid }

function TDrawGrid_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TDrawGrid.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TStringGrid_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TStringGrid.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TCustomGrid の public。 }

procedure TCustomGrid_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomGrid(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TCustomGrid_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomGrid(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ すべての行・列を削除する(ColCount・RowCount が 0 になる)。セルの文字列だけを消すのは TCustomStringGrid_Clean。 }
procedure TCustomGrid_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomGrid(Obj).Clear;
  except
    ReportException;
  end;
end;

{ セルのクライアント座標での矩形。 }
procedure TCustomGrid_CellRect(Obj: Pointer; ACol, ARow: Integer; Left, Top, Right, Bottom: PInteger); BETH_CALL;
var
  R: TRect;
begin
  try
    R := TCustomGrid(Obj).CellRect(ACol, ARow);
    Left^ := R.Left;
    Top^ := R.Top;
    Right^ := R.Right;
    Bottom^ := R.Bottom;
  except
    ReportException;
  end;
end;

{ クライアント座標 X, Y にあるセル。セルの外なら -1。 }
procedure TCustomGrid_MouseToCell(Obj: Pointer; X, Y: Integer; ACol, ARow: PInteger); BETH_CALL;
var
  C, R: Longint;
begin
  try
    TCustomGrid(Obj).MouseToCell(X, Y, C, R);
    ACol^ := C;
    ARow^ := R;
  except
    ReportException;
  end;
end;

{ TCustomDrawGrid の public(LCL では TCustomGrid の protected を公開したもの)。 }

function TCustomDrawGrid_GetColCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).ColCount;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetColCount(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).ColCount := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetRowCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).RowCount;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetRowCount(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).RowCount := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetFixedCols(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).FixedCols;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetFixedCols(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).FixedCols := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetFixedRows(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).FixedRows;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetFixedRows(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).FixedRows := Value;
  except
    ReportException;
  end;
end;

{ 現在のセル(フォーカスのあるセル)の列・行。 }
function TCustomDrawGrid_GetCol(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).Col;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetCol(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).Col := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetRow(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).Row;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetRow(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).Row := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetDefaultColWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).DefaultColWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetDefaultColWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).DefaultColWidth := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetDefaultRowHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).DefaultRowHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetDefaultRowHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).DefaultRowHeight := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetColWidths(Obj: Pointer; ACol: Integer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).ColWidths[ACol];
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetColWidths(Obj: Pointer; ACol, Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).ColWidths[ACol] := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetRowHeights(Obj: Pointer; ARow: Integer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).RowHeights[ARow];
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetRowHeights(Obj: Pointer; ARow, Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).RowHeights[ARow] := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetOptions(Obj: Pointer): LongWord; BETH_CALL;
begin
  try
    Result := GridOptionsToInt(TCustomDrawGrid(Obj).Options);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetOptions(Obj: Pointer; Value: LongWord); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).Options := IntToGridOptions(Value);
  except
    ReportException;
  end;
end;

{ 選択範囲(Left/Right が列、Top/Bottom が行。単一のセルなら Left = Right、Top = Bottom)。 }
procedure TCustomDrawGrid_GetSelection(Obj: Pointer; Left, Top, Right, Bottom: PInteger); BETH_CALL;
var
  R: TGridRect;
begin
  try
    R := TCustomDrawGrid(Obj).Selection;
    Left^ := R.Left;
    Top^ := R.Top;
    Right^ := R.Right;
    Bottom^ := R.Bottom;
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetSelection(Obj: Pointer; Left, Top, Right, Bottom: Integer); BETH_CALL;
var
  R: TGridRect;
begin
  try
    { Rect(...) は Win32 では Windows ユニットの型名(TRect の別名)に隠されるため、フィールドで組み立てる。 }
    R.Left := Left;
    R.Top := Top;
    R.Right := Right;
    R.Bottom := Bottom;
    TCustomDrawGrid(Obj).Selection := R;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetLeftCol(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).LeftCol;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetLeftCol(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).LeftCol := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetTopRow(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).TopRow;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetTopRow(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).TopRow := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetDefaultDrawing(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).DefaultDrawing;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetDefaultDrawing(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).DefaultDrawing := Value;
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetFixedColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).FixedColor;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetFixedColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).FixedColor := TColor(Value);
  except
    ReportException;
  end;
end;

function TCustomDrawGrid_GetEditorMode(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomDrawGrid(Obj).EditorMode;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetEditorMode(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).EditorMode := Value;
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_InsertColRow(Obj: Pointer; IsColumn: LongBool; Index: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).InsertColRow(IsColumn, Index);
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_DeleteColRow(Obj: Pointer; IsColumn: LongBool; Index: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).DeleteColRow(IsColumn, Index);
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_MoveColRow(Obj: Pointer; IsColumn: LongBool; FromIndex, ToIndex: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).MoveColRow(IsColumn, FromIndex, ToIndex);
  except
    ReportException;
  end;
end;

{ IsColumn が True なら、列 Index の値で行を並べ替える(固定行は除く)。False なら行 Index の値で列を並べ替える。 }
procedure TCustomDrawGrid_SortColRow(Obj: Pointer; IsColumn: LongBool; Index: Integer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).SortColRow(IsColumn, Index);
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetOnDrawCell(Obj: Pointer; Cb: TBethDrawCellCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).OnDrawCell := @DrawCellBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnDrawCell).Data, Cb, Data).DoDrawCell;
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetOnSelectCell(Obj: Pointer; Cb: TBethCellAllowCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).OnSelectCell := @CellAllowBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnSelectCell).Data, Cb, Data).DoSelectCell;
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetOnSelection(Obj: Pointer; Cb: TBethCellCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).OnSelection := @CellBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnSelection).Data, Cb, Data).DoSelection;
  except
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetOnHeaderClick(Obj: Pointer; Cb: TBethCellCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomDrawGrid(Obj).OnHeaderClick := @CellBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnHeaderClick).Data, Cb, Data).DoHeaderClick;
  except
    ReportException;
  end;
end;

{ TCustomStringGrid の public。 }

function TCustomStringGrid_GetCells(Obj: Pointer; ACol, ARow: Integer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TCustomStringGrid(Obj).Cells[ACol, ARow]);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TCustomStringGrid_SetCells(Obj: Pointer; ACol, ARow: Integer; Value: PChar); BETH_CALL;
begin
  try
    TCustomStringGrid(Obj).Cells[ACol, ARow] := Value;
  except
    ReportException;
  end;
end;

{ すべてのセルの文字列を消す(行・列の数は変わらない)。 }
procedure TCustomStringGrid_Clean(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomStringGrid(Obj).Clean;
  except
    ReportException;
  end;
end;

procedure TCustomStringGrid_AutoSizeColumns(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomStringGrid(Obj).AutoSizeColumns;
  except
    ReportException;
  end;
end;

procedure TCustomStringGrid_AutoSizeColumn(Obj: Pointer; ACol: Integer); BETH_CALL;
begin
  try
    TCustomStringGrid(Obj).AutoSizeColumn(ACol);
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 2、5 バッチ目(THeaderControl)。docs/adr/0024-... を参照。
  セクション(THeaderSection。TCollectionItem)は TListColumn と同じく、ハンドルを C 側へ渡すときに付ける観察者(WatchItem)で
  破棄を通知する(docs/adr/0026 で ADR 0024 の方式を置き換えた)。 }

type
  { OnSectionTrack(HeaderControl, Section, Width, State)用。State は TSectionTrackState の序数。 }
  TBethSectionTrackCallback = procedure(Sender: Pointer; Section: Pointer; Width: Integer; State: Integer; Data: Pointer); BETH_CALL;

  TSectionTrackCallbackBridge = class(TComponent)
  private
    FCallback: TBethSectionTrackCallback;
    FData: Pointer;
  public
    procedure DoTrack(HeaderControl: TCustomHeaderControl; Section: THeaderSection; Width: Integer; State: TSectionTrackState);
  end;

  { OnSectionDrag(Sender, FromSection, ToSection, var AllowDrag)用。Allow は書き換え可能(0 = False)。 }
  TBethSectionDragCallback = procedure(Sender: Pointer; FromSection: Pointer; ToSection: Pointer; Allow: PInteger; Data: Pointer); BETH_CALL;

  TSectionDragCallbackBridge = class(TComponent)
  private
    FCallback: TBethSectionDragCallback;
    FData: Pointer;
  public
    procedure DoDrag(Sender: TObject; FromSection, ToSection: THeaderSection; var AllowDrag: Boolean);
  end;

procedure TSectionTrackCallbackBridge.DoTrack(HeaderControl: TCustomHeaderControl; Section: THeaderSection; Width: Integer; State: TSectionTrackState);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(HeaderControl), WatchItem(Section), Width, Ord(State), FData);
  CheckCallbackError;
end;

procedure TSectionDragCallbackBridge.DoDrag(Sender: TObject; FromSection, ToSection: THeaderSection; var AllowDrag: Boolean);
var
  A: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if AllowDrag then A := -1 else A := 0;
  FCallback(Pointer(Sender), WatchItem(FromSection), WatchItem(ToSection), @A, FData);
  AllowDrag := A <> 0;
  CheckCallbackError;
end;

{ THeaderControl / TCustomHeaderControl }

function THeaderControl_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(THeaderControl.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ Sections(THeaderSections)は、ヘッダーコントロールが所有する非所有のハンドル(ヘッダーコントロールと寿命が一致する)。 }
function TCustomHeaderControl_GetSections(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomHeaderControl(Obj).Sections);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomHeaderControl_GetDragReorder(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomHeaderControl(Obj).DragReorder;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetDragReorder(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomHeaderControl(Obj).DragReorder := Value;
  except
    ReportException;
  end;
end;

{ Win32 では Windows ユニットの型名 Point(TPoint の別名)が Types.Point を隠すため、TPoint はフィールドで組み立てる。 }
function TCustomHeaderControl_GetSectionAt(Obj: Pointer; X, Y: Integer): Integer; BETH_CALL;
var
  P: TPoint;
begin
  try
    P.X := X;
    P.Y := Y;
    Result := TCustomHeaderControl(Obj).GetSectionAt(P);
  except
    Result := 0;
    ReportException;
  end;
end;

function TCustomHeaderControl_GetSectionFromOriginalIndex(Obj: Pointer; OriginalIndex: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCustomHeaderControl(Obj).SectionFromOriginalIndex[OriginalIndex]);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetOnSectionClick(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    THeaderControl(Obj).OnSectionClick := @ItemBridgeFor(THeaderControl(Obj), TMethod(THeaderControl(Obj).OnSectionClick).Data, Cb, Data).DoSection;
  except
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetOnSectionResize(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    THeaderControl(Obj).OnSectionResize := @ItemBridgeFor(THeaderControl(Obj), TMethod(THeaderControl(Obj).OnSectionResize).Data, Cb, Data).DoSection;
  except
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetOnSectionSeparatorDblClick(Obj: Pointer; Cb: TBethItemCallback; Data: Pointer); BETH_CALL;
begin
  try
    THeaderControl(Obj).OnSectionSeparatorDblClick := @ItemBridgeFor(THeaderControl(Obj), TMethod(THeaderControl(Obj).OnSectionSeparatorDblClick).Data, Cb, Data).DoSection;
  except
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetOnSectionTrack(Obj: Pointer; Cb: TBethSectionTrackCallback; Data: Pointer); BETH_CALL;
var
  HC: THeaderControl;
  Bridge: TSectionTrackCallbackBridge;
begin
  try
    HC := THeaderControl(Obj);
    if (TMethod(HC.OnSectionTrack).Data <> nil) and (TObject(TMethod(HC.OnSectionTrack).Data) is TSectionTrackCallbackBridge)
       and (TSectionTrackCallbackBridge(TMethod(HC.OnSectionTrack).Data).Owner = HC) then
      Bridge := TSectionTrackCallbackBridge(TMethod(HC.OnSectionTrack).Data)
    else
      Bridge := TSectionTrackCallbackBridge.Create(HC);
    Bridge.FCallback := Cb;
    Bridge.FData := Data;
    HC.OnSectionTrack := @Bridge.DoTrack;
  except
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetOnSectionDrag(Obj: Pointer; Cb: TBethSectionDragCallback; Data: Pointer); BETH_CALL;
var
  HC: THeaderControl;
  Bridge: TSectionDragCallbackBridge;
begin
  try
    HC := THeaderControl(Obj);
    if (TMethod(HC.OnSectionDrag).Data <> nil) and (TObject(TMethod(HC.OnSectionDrag).Data) is TSectionDragCallbackBridge)
       and (TSectionDragCallbackBridge(TMethod(HC.OnSectionDrag).Data).Owner = HC) then
      Bridge := TSectionDragCallbackBridge(TMethod(HC.OnSectionDrag).Data)
    else
      Bridge := TSectionDragCallbackBridge.Create(HC);
    Bridge.FCallback := Cb;
    Bridge.FData := Data;
    HC.OnSectionDrag := @Bridge.DoDrag;
  except
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetOnSectionEndDrag(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    THeaderControl(Obj).OnSectionEndDrag := @BridgeFor(THeaderControl(Obj), MethodData(THeaderControl(Obj).OnSectionEndDrag), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ THeaderSections(TCollection)。セクションを返す関数は WatchItem してから返す。 }

function THeaderSections_Add(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(THeaderSections(Obj).Add);
  except
    Result := nil;
    ReportException;
  end;
end;

function THeaderSections_Insert(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(THeaderSections(Obj).Insert(Index));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure THeaderSections_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    THeaderSections(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure THeaderSections_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    THeaderSections(Obj).Clear;
  except
    ReportException;
  end;
end;

function THeaderSections_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSections(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function THeaderSections_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(THeaderSections(Obj).Items[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure THeaderSections_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    THeaderSections(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure THeaderSections_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    THeaderSections(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ THeaderSection }

function THeaderSection_GetText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(THeaderSection(Obj).Text);
  except
    Result := '';
    ReportException;
  end;
end;

procedure THeaderSection_SetText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    THeaderSection(Obj).Text := Value;
  except
    ReportException;
  end;
end;

function THeaderSection_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure THeaderSection_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    THeaderSection(Obj).Width := Value;
  except
    ReportException;
  end;
end;

function THeaderSection_GetMinWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).MinWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure THeaderSection_SetMinWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    THeaderSection(Obj).MinWidth := Value;
  except
    ReportException;
  end;
end;

function THeaderSection_GetMaxWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).MaxWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure THeaderSection_SetMaxWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    THeaderSection(Obj).MaxWidth := Value;
  except
    ReportException;
  end;
end;

{ TAlignment の序数(taLeftJustify = 0, taRightJustify, taCenter)。 }
function THeaderSection_GetAlignment(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(THeaderSection(Obj).Alignment);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure THeaderSection_SetAlignment(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    THeaderSection(Obj).Alignment := TAlignment(Value);
  except
    ReportException;
  end;
end;

function THeaderSection_GetVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).Visible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure THeaderSection_SetVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    THeaderSection(Obj).Visible := Value;
  except
    ReportException;
  end;
end;

{ TCollectionItem.Index。書き換えるとセクションが移動する。 }
function THeaderSection_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).Index;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure THeaderSection_SetIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    THeaderSection(Obj).Index := Value;
  except
    ReportException;
  end;
end;

function THeaderSection_GetLeft(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).Left;
  except
    Result := 0;
    ReportException;
  end;
end;

function THeaderSection_GetRight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).Right;
  except
    Result := 0;
    ReportException;
  end;
end;

function THeaderSection_GetOriginalIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).OriginalIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 2、6 バッチ目(TToolBar・TToolButton)。docs/adr/0025-... を参照。
  TToolButton は TGraphicControl(TComponent)なので、他のコントロールと同じく Owner と FreeNotification で寿命を管理する。
  ツールバーへの追加は Parent にツールバーを設定することで行う(LCL の TToolButton.SetParent がツールバーに登録する)。 }

{ TToolWindow。EdgeBorders は TEdgeBorder の序数をビットの位置とするビット集合(ebLeft = 1, ebTop = 2, ...)。 }

function TToolWindow_GetEdgeBorders(Obj: Pointer): LongWord; BETH_CALL;
var
  B: TEdgeBorder;
begin
  try
    Result := 0;
    for B := Low(TEdgeBorder) to High(TEdgeBorder) do
      if B in TToolWindow(Obj).EdgeBorders then
        Result := Result or (LongWord(1) shl Ord(B));
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolWindow_SetEdgeBorders(Obj: Pointer; Value: LongWord); BETH_CALL;
var
  B: TEdgeBorder;
  S: TEdgeBorders;
begin
  try
    S := [];
    for B := Low(TEdgeBorder) to High(TEdgeBorder) do
      if (Value and (LongWord(1) shl Ord(B))) <> 0 then
        Include(S, B);
    TToolWindow(Obj).EdgeBorders := S;
  except
    ReportException;
  end;
end;

{ TEdgeStyle の序数(esNone = 0, esRaised, esLowered)。 }
function TToolWindow_GetEdgeInner(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TToolWindow(Obj).EdgeInner);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolWindow_SetEdgeInner(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolWindow(Obj).EdgeInner := TEdgeStyle(Value);
  except
    ReportException;
  end;
end;

function TToolWindow_GetEdgeOuter(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TToolWindow(Obj).EdgeOuter);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolWindow_SetEdgeOuter(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolWindow(Obj).EdgeOuter := TEdgeStyle(Value);
  except
    ReportException;
  end;
end;

procedure TToolWindow_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TToolWindow(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TToolWindow_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TToolWindow(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ TToolBar }

function TToolBar_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TToolBar.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TToolBar_GetButtonCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).ButtonCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TToolBar_GetButton(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchOrNil(TToolBar(Obj).Buttons[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

function TToolBar_GetRowCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).RowCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TToolBar_GetButtonHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).ButtonHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolBar_SetButtonHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolBar(Obj).ButtonHeight := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetButtonWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).ButtonWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolBar_SetButtonWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolBar(Obj).ButtonWidth := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetDropDownWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).DropDownWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolBar_SetDropDownWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolBar(Obj).DropDownWidth := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetIndent(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).Indent;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolBar_SetIndent(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolBar(Obj).Indent := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetFlat(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).Flat;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolBar_SetFlat(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolBar(Obj).Flat := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetList(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).List;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolBar_SetList(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolBar(Obj).List := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetShowCaptions(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).ShowCaptions;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolBar_SetShowCaptions(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolBar(Obj).ShowCaptions := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetTransparent(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).Transparent;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolBar_SetTransparent(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolBar(Obj).Transparent := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetWrapable(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolBar(Obj).Wrapable;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolBar_SetWrapable(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolBar(Obj).Wrapable := Value;
  except
    ReportException;
  end;
end;

procedure TToolBar_SetButtonSize(Obj: Pointer; NewButtonWidth, NewButtonHeight: Integer); BETH_CALL;
begin
  try
    TToolBar(Obj).SetButtonSize(NewButtonWidth, NewButtonHeight);
  except
    ReportException;
  end;
end;

{ TToolButton }

function TToolButton_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TToolButton.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TToolButton_GetAllowAllUp(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).AllowAllUp;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetAllowAllUp(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolButton(Obj).AllowAllUp := Value;
  except
    ReportException;
  end;
end;

function TToolButton_GetDown(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).Down;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetDown(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolButton(Obj).Down := Value;
  except
    ReportException;
  end;
end;

function TToolButton_GetGrouped(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).Grouped;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetGrouped(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolButton(Obj).Grouped := Value;
  except
    ReportException;
  end;
end;

function TToolButton_GetIndeterminate(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).Indeterminate;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetIndeterminate(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolButton(Obj).Indeterminate := Value;
  except
    ReportException;
  end;
end;

function TToolButton_GetMarked(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).Marked;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetMarked(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolButton(Obj).Marked := Value;
  except
    ReportException;
  end;
end;

function TToolButton_GetShowCaption(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).ShowCaption;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetShowCaption(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolButton(Obj).ShowCaption := Value;
  except
    ReportException;
  end;
end;

function TToolButton_GetWrap(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).Wrap;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetWrap(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TToolButton(Obj).Wrap := Value;
  except
    ReportException;
  end;
end;

{ TToolButtonStyle の序数(tbsButton = 0, tbsCheck, tbsDropDown, tbsSeparator, tbsDivider, tbsButtonDrop)。 }
function TToolButton_GetStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TToolButton(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolButton_SetStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolButton(Obj).Style := TToolButtonStyle(Value);
  except
    ReportException;
  end;
end;

function TToolButton_GetDropdownMenu(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchOrNil(TToolButton(Obj).DropdownMenu);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TToolButton_SetDropdownMenu(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TToolButton(Obj).DropdownMenu := TPopupMenu(Value);
  except
    ReportException;
  end;
end;

function TToolButton_GetMenuItem(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchOrNil(TToolButton(Obj).MenuItem);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TToolButton_SetMenuItem(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TToolButton(Obj).MenuItem := TMenuItem(Value);
  except
    ReportException;
  end;
end;

{ ツールバーの中での位置(ツールバーに置かれていなければ -1)。 }
function TToolButton_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).Index;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolButton_Click(Obj: Pointer); BETH_CALL;
begin
  try
    TToolButton(Obj).Click;
  except
    ReportException;
  end;
end;

procedure TToolButton_ArrowClick(Obj: Pointer); BETH_CALL;
begin
  try
    TToolButton(Obj).ArrowClick;
  except
    ReportException;
  end;
end;

function TToolButton_PointInArrow(Obj: Pointer; X, Y: Integer): LongBool; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).PointInArrow(X, Y);
  except
    Result := False;
    ReportException;
  end;
end;

procedure TToolButton_SetOnArrowClick(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TToolButton(Obj).OnArrowClick := @BridgeFor(TToolButton(Obj), MethodData(TToolButton(Obj).OnArrowClick), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ docs/component-coverage.md の Tier 2、7 バッチ目(TCoolBar)。docs/adr/0026-... を参照。
  バンド(TCoolBand。TCollectionItem)は、コントロールの Parent をクールバーにしたとき(InsertControl)に LCL が内部で生成し、
  コントロールを外したとき(RemoveControl)にも内部で削除されるため、この DLL の関数を通らずに生成・破棄される。
  破棄は他の項目と同じく、ハンドルを C 側へ返すときに付ける観察者(WatchItem)で通知する(どの経路で破棄されても届く)。 }

{ TCoolBar / TCustomCoolBar。既定の Align は alTop。 }

function TCoolBar_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TCoolBar.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ Bands(TCoolBands)は、クールバーが所有する非所有のハンドル(クールバーと寿命が一致する)。 }
function TCustomCoolBar_GetBands(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomCoolBar(Obj).Bands);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomCoolBar_AutosizeBands(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).AutosizeBands;
  except
    ReportException;
  end;
end;

procedure TCustomCoolBar_MouseToBandPos(Obj: Pointer; X, Y: Integer; ABand: PInteger; AGrabber: PInteger); BETH_CALL;
var
  B: Integer;
  G: Boolean;
begin
  try
    TCustomCoolBar(Obj).MouseToBandPos(X, Y, B, G);
    ABand^ := B;
    if G then AGrabber^ := -1 else AGrabber^ := 0;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetFixedSize(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).FixedSize;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetFixedSize(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).FixedSize := Value;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetFixedOrder(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).FixedOrder;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetFixedOrder(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).FixedOrder := Value;
  except
    ReportException;
  end;
end;

{ TGrabStyle の序数(gsSimple = 0, gsDouble, gsHorLines, gsVerLines, gsGripper, gsButton)。 }
function TCustomCoolBar_GetGrabStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomCoolBar(Obj).GrabStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetGrabStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).GrabStyle := TGrabStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetGrabWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).GrabWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetGrabWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).GrabWidth := Value;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetHorizontalSpacing(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).HorizontalSpacing;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetHorizontalSpacing(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).HorizontalSpacing := Value;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetVerticalSpacing(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).VerticalSpacing;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetVerticalSpacing(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).VerticalSpacing := Value;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetShowText(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).ShowText;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetShowText(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).ShowText := Value;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetThemed(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).Themed;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetThemed(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).Themed := Value;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetVertical(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCoolBar(Obj).Vertical;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetVertical(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).Vertical := Value;
  except
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).OnChange := @BridgeFor(TCustomCoolBar(Obj), MethodData(TCustomCoolBar(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TCoolBands(TCollection)。バンドを返す関数は WatchItem してから返す。 }

function TCoolBands_Add(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCoolBands(Obj).Add);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCoolBands_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBands(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCoolBands_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCoolBands(Obj).Items[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCoolBands_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    TCoolBands(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure TCoolBands_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TCoolBands(Obj).Clear;
  except
    ReportException;
  end;
end;

procedure TCoolBands_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCoolBands(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TCoolBands_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCoolBands(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

function TCoolBands_FindBand(Obj: Pointer; AControl: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TCoolBands(Obj).FindBand(TControl(AControl)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCoolBands_FindBandIndex(Obj: Pointer; AControl: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBands(Obj).FindBandIndex(TControl(AControl));
  except
    Result := 0;
    ReportException;
  end;
end;

{ TCoolBand }

function TCoolBand_GetText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TCoolBand(Obj).Text);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TCoolBand_SetText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TCoolBand(Obj).Text := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCoolBand_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCoolBand(Obj).Width := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetMinWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).MinWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCoolBand_SetMinWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCoolBand(Obj).MinWidth := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetMinHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).MinHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCoolBand_SetMinHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCoolBand(Obj).MinHeight := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetBreak(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Break;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCoolBand_SetBreak(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCoolBand(Obj).Break := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Visible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCoolBand_SetVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCoolBand(Obj).Visible := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetFixedSize(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).FixedSize;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCoolBand_SetFixedSize(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCoolBand(Obj).FixedSize := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetFixedBackground(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).FixedBackground;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCoolBand_SetFixedBackground(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCoolBand(Obj).FixedBackground := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetHorizontalOnly(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).HorizontalOnly;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCoolBand_SetHorizontalOnly(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCoolBand(Obj).HorizontalOnly := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TCoolBand(Obj).Color);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCoolBand_SetColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCoolBand(Obj).Color := TColor(Value);
  except
    ReportException;
  end;
end;

function TCoolBand_GetParentColor(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).ParentColor;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCoolBand_SetParentColor(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCoolBand(Obj).ParentColor := Value;
  except
    ReportException;
  end;
end;

{ TCollectionItem.Index。書き換えるとバンドが移動する。 }
function TCoolBand_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Index;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCoolBand_SetIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCoolBand(Obj).Index := Value;
  except
    ReportException;
  end;
end;

{ バンドに置くコントロール。設定するとそのコントロールの Parent がクールバーになり、Align は alNone になる。 }
function TCoolBand_GetControl(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchOrNil(TCoolBand(Obj).Control);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCoolBand_SetControl(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCoolBand(Obj).Control := TControl(Value);
  except
    ReportException;
  end;
end;

function TCoolBand_GetLeft(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Left;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCoolBand_GetTop(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Top;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCoolBand_GetRight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Right;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCoolBand_GetHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).Height;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCoolBand_AutosizeWidth(Obj: Pointer); BETH_CALL;
begin
  try
    TCoolBand(Obj).AutosizeWidth;
  except
    ReportException;
  end;
end;

{ TStrings(docs/adr/0027)。ハンドルはコントロールの Items・Lines・Tabs 等(TCustomListBox_GetItems 等)から得る。
  ハンドルは所有者の持ち物で、LCL がウィンドウの生成・破棄のときに差し替えることがあるため、呼び出し側は保存しない。 }

function TStrings_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TStrings(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TStrings_GetStrings(Obj: Pointer; Index: Integer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStrings(Obj).Strings[Index]);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TStrings_SetStrings(Obj: Pointer; Index: Integer; Value: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).Strings[Index] := Value;
  except
    ReportException;
  end;
end;

{ Objects[Index] は利用者データ(C 側のポインタ)として扱う。LCL は解釈しない(TStringList は所有しない)。 }
function TStrings_GetObjects(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TStrings(Obj).Objects[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TStrings_SetObjects(Obj: Pointer; Index: Integer; Value: Pointer); BETH_CALL;
begin
  try
    TStrings(Obj).Objects[Index] := TObject(Value);
  except
    ReportException;
  end;
end;

function TStrings_Add(Obj: Pointer; S: PChar): Integer; BETH_CALL;
begin
  try
    Result := TStrings(Obj).Add(S);
  except
    Result := 0;
    ReportException;
  end;
end;

function TStrings_AddObject(Obj: Pointer; S: PChar; AObject: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TStrings(Obj).AddObject(S, TObject(AObject));
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TStrings_Insert(Obj: Pointer; Index: Integer; S: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).Insert(Index, S);
  except
    ReportException;
  end;
end;

procedure TStrings_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    TStrings(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure TStrings_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TStrings(Obj).Clear;
  except
    ReportException;
  end;
end;

function TStrings_IndexOf(Obj: Pointer; S: PChar): Integer; BETH_CALL;
begin
  try
    Result := TStrings(Obj).IndexOf(S);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TStrings_Exchange(Obj: Pointer; Index1, Index2: Integer); BETH_CALL;
begin
  try
    TStrings(Obj).Exchange(Index1, Index2);
  except
    ReportException;
  end;
end;

procedure TStrings_Move(Obj: Pointer; CurIndex, NewIndex: Integer); BETH_CALL;
begin
  try
    TStrings(Obj).Move(CurIndex, NewIndex);
  except
    ReportException;
  end;
end;

procedure TStrings_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TStrings(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TStrings_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TStrings(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ すべての行を改行でつないだ文字列。設定すると改行で分けて置き換える。 }
function TStrings_GetText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStrings(Obj).Text);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TStrings_SetText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).Text := Value;
  except
    ReportException;
  end;
end;

function TStrings_GetCommaText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStrings(Obj).CommaText);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TStrings_SetCommaText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).CommaText := Value;
  except
    ReportException;
  end;
end;

procedure TStrings_Assign(Obj: Pointer; Source: Pointer); BETH_CALL;
begin
  try
    TStrings(Obj).Assign(TStrings(Source));
  except
    ReportException;
  end;
end;

procedure TStrings_AddStrings(Obj: Pointer; Source: Pointer); BETH_CALL;
begin
  try
    TStrings(Obj).AddStrings(TStrings(Source));
  except
    ReportException;
  end;
end;

{ 名前=値 の形の行(Names・Values・ValueFromIndex・IndexOfName)。区切りは LCL の既定の '='。 }

function TStrings_GetNames(Obj: Pointer; Index: Integer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStrings(Obj).Names[Index]);
  except
    Result := '';
    ReportException;
  end;
end;

function TStrings_GetValues(Obj: Pointer; Name: PChar): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStrings(Obj).Values[Name]);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TStrings_SetValues(Obj: Pointer; Name: PChar; Value: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).Values[Name] := Value;
  except
    ReportException;
  end;
end;

function TStrings_GetValueFromIndex(Obj: Pointer; Index: Integer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStrings(Obj).ValueFromIndex[Index]);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TStrings_SetValueFromIndex(Obj: Pointer; Index: Integer; Value: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).ValueFromIndex[Index] := Value;
  except
    ReportException;
  end;
end;

function TStrings_IndexOfName(Obj: Pointer; Name: PChar): Integer; BETH_CALL;
begin
  try
    Result := TStrings(Obj).IndexOfName(Name);
  except
    Result := 0;
    ReportException;
  end;
end;

{ 任意の区切り文字の文字列(Delimiter・StrictDelimiter・DelimitedText)。 }

function TStrings_GetDelimiter(Obj: Pointer): AnsiChar; BETH_CALL;
begin
  try
    Result := TStrings(Obj).Delimiter;
  except
    Result := #0;
    ReportException;
  end;
end;

procedure TStrings_SetDelimiter(Obj: Pointer; Value: AnsiChar); BETH_CALL;
begin
  try
    TStrings(Obj).Delimiter := Value;
  except
    ReportException;
  end;
end;

function TStrings_GetStrictDelimiter(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TStrings(Obj).StrictDelimiter;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TStrings_SetStrictDelimiter(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TStrings(Obj).StrictDelimiter := Value;
  except
    ReportException;
  end;
end;

function TStrings_GetDelimitedText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStrings(Obj).DelimitedText);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TStrings_SetDelimitedText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).DelimitedText := Value;
  except
    ReportException;
  end;
end;

{ ファイル名・内容とも UTF-8 のまま扱う(LCL は文字列を UTF-8 として扱う)。 }

procedure TStrings_LoadFromFile(Obj: Pointer; FileName: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).LoadFromFile(FileName);
  except
    ReportException;
  end;
end;

procedure TStrings_SaveToFile(Obj: Pointer; FileName: PChar); BETH_CALL;
begin
  try
    TStrings(Obj).SaveToFile(FileName);
  except
    ReportException;
  end;
end;

{ TStringList(docs/adr/0028)。利用者が生成し、TStringList_Destroy で破棄する(TComponent ではなく、Owner も破棄通知も無い)。
  TStrings の操作は TStrings_* を使う。 }

function TStringList_Create: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TStringList.Create);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TStringList_Destroy(Obj: Pointer); BETH_CALL;
begin
  try
    TStringList(Obj).Free;
  except
    ReportException;
  end;
end;

procedure TStringList_Sort(Obj: Pointer); BETH_CALL;
begin
  try
    TStringList(Obj).Sort;
  except
    ReportException;
  end;
end;

{ ソートされた一覧から S を二分探索する。見つからなければ、S を挿入すべき位置を Index に入れて False を返す。
  Sorted でない一覧に使うと例外になる(LCL の仕様)。 }
function TStringList_Find(Obj: Pointer; S: PChar; Index: PInteger): LongBool; BETH_CALL;
var
  I: Integer;
begin
  try
    Result := TStringList(Obj).Find(S, I);
    if Index <> nil then
      Index^ := I;
  except
    Result := False;
    ReportException;
  end;
end;

function TStringList_GetSorted(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TStringList(Obj).Sorted;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TStringList_SetSorted(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TStringList(Obj).Sorted := Value;
  except
    ReportException;
  end;
end;

function TStringList_GetDuplicates(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TStringList(Obj).Duplicates);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TStringList_SetDuplicates(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TStringList(Obj).Duplicates := TDuplicates(Value);
  except
    ReportException;
  end;
end;

function TStringList_GetCaseSensitive(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TStringList(Obj).CaseSensitive;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TStringList_SetCaseSensitive(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TStringList(Obj).CaseSensitive := Value;
  except
    ReportException;
  end;
end;

{ ---------------- グラフィックス(docs/adr/0029) ----------------
  TGraphic の派生(TBitmap・TPortableNetworkGraphic・TJPEGImage)は TPersistent で、TComponent ではない。
  *_Create で生成したものは利用者の持ち物で、TGraphic_Destroy で破棄する(TStringList と同じ)。
  TPicture.Graphic・TCustomBitBtn.Glyph 等が返すハンドルは所有者の持ち物で、LCL が差し替える
  (TPicture は LoadFromFile・Bitmap の参照・Graphic への代入のたびに中身のオブジェクトを作り直す)ため、保存してはならない。 }

procedure TGraphic_Destroy(Obj: Pointer); BETH_CALL;
begin
  try
    TGraphic(Obj).Free;
  except
    ReportException;
  end;
end;

function TGraphic_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TGraphic(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TGraphic_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TGraphic(Obj).Width := Value;
  except
    ReportException;
  end;
end;

function TGraphic_GetHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TGraphic(Obj).Height;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TGraphic_SetHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TGraphic(Obj).Height := Value;
  except
    ReportException;
  end;
end;

function TGraphic_GetEmpty(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TGraphic(Obj).Empty;
  except
    Result := False;
    ReportException;
  end;
end;

function TGraphic_GetTransparent(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TGraphic(Obj).Transparent;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TGraphic_SetTransparent(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TGraphic(Obj).Transparent := Value;
  except
    ReportException;
  end;
end;

{ ファイル名は UTF-8。形式はクラスで決まる(TBitmap に PNG のファイルを読むと例外になる)。
  拡張子から形式を選ぶのは TPicture_LoadFromFile のほう。 }
procedure TGraphic_LoadFromFile(Obj: Pointer; FileName: PChar); BETH_CALL;
begin
  try
    TGraphic(Obj).LoadFromFile(FileName);
  except
    ReportException;
  end;
end;

procedure TGraphic_SaveToFile(Obj: Pointer; FileName: PChar); BETH_CALL;
begin
  try
    TGraphic(Obj).SaveToFile(FileName);
  except
    ReportException;
  end;
end;

{ Source はグラフィックか TPicture のハンドル。nil なら Clear と同じ。 }
procedure TGraphic_Assign(Obj: Pointer; Source: Pointer); BETH_CALL;
begin
  try
    if Source = nil then
      TGraphic(Obj).Clear
    else
      TGraphic(Obj).Assign(TPersistent(Source));
  except
    ReportException;
  end;
end;

procedure TGraphic_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TGraphic(Obj).Clear;
  except
    ReportException;
  end;
end;

{ TRasterImage の public。Canvas はグラフィックが所有し、初めて参照したときに作られる。 }
function TRasterImage_GetCanvas(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TRasterImage(Obj).Canvas);
  except
    Result := nil;
    ReportException;
  end;
end;

{ TPixelFormat の序数(pfDevice=0, pf1bit, pf4bit, pf8bit, pf15bit, pf16bit, pf24bit, pf32bit, pfCustom)。 }
function TRasterImage_GetPixelFormat(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TRasterImage(Obj).PixelFormat);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TRasterImage_SetPixelFormat(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TRasterImage(Obj).PixelFormat := TPixelFormat(Value);
  except
    ReportException;
  end;
end;

function TRasterImage_GetTransparentColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TRasterImage(Obj).TransparentColor);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TRasterImage_SetTransparentColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TRasterImage(Obj).TransparentColor := TColor(Value);
  except
    ReportException;
  end;
end;

{ TTransparentMode の序数(tmAuto=0, tmFixed)。 }
function TRasterImage_GetTransparentMode(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TRasterImage(Obj).TransparentMode);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TRasterImage_SetTransparentMode(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TRasterImage(Obj).TransparentMode := TTransparentMode(Value);
  except
    ReportException;
  end;
end;

procedure TCustomBitmap_SetSize(Obj: Pointer; AWidth, AHeight: Integer); BETH_CALL;
begin
  try
    TCustomBitmap(Obj).SetSize(AWidth, AHeight);
  except
    ReportException;
  end;
end;

{ TBitmap は Win32 では Windows ユニットの構造体(BITMAP)に隠されるため、Graphics.TBitmap と書く。 }
function TBitmap_Create: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(Graphics.TBitmap.Create);
  except
    Result := nil;
    ReportException;
  end;
end;

function TPortableNetworkGraphic_Create: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPortableNetworkGraphic.Create);
  except
    Result := nil;
    ReportException;
  end;
end;

function TJPEGImage_Create: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TJPEGImage.Create);
  except
    Result := nil;
    ReportException;
  end;
end;

{ 保存するときの品質(1〜100。既定は 75)。 }
function TJPEGImage_GetCompressionQuality(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TJPEGImage(Obj).CompressionQuality;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TJPEGImage_SetCompressionQuality(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TJPEGImage(Obj).CompressionQuality := TJPEGQualityRange(Value);
  except
    ReportException;
  end;
end;

{ TPicture。TCustomImage.Picture は画像コントロールが所有する(生成時に作られ、差し替わらない)。
  TPicture_Create で生成したものは利用者の持ち物で、TPicture_Destroy で破棄する。 }

function TPicture_Create: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPicture.Create);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TPicture_Destroy(Obj: Pointer); BETH_CALL;
begin
  try
    TPicture(Obj).Free;
  except
    ReportException;
  end;
end;

{ 空なら nil。 }
function TPicture_GetGraphic(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPicture(Obj).Graphic);
  except
    Result := nil;
    ReportException;
  end;
end;

{ Value と同じクラスのグラフィックを作って内容を写す(Value はそのまま呼び出し側の持ち物)。nil なら空にする。 }
procedure TPicture_SetGraphic(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TPicture(Obj).Graphic := TGraphic(Value);
  except
    ReportException;
  end;
end;

{ 中身がそのクラスでなければ、そのクラスに変換する(中身のオブジェクトが作り直される。空なら空のものを作る)。 }
function TPicture_GetBitmap(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPicture(Obj).Bitmap);
  except
    Result := nil;
    ReportException;
  end;
end;

function TPicture_GetPNG(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPicture(Obj).PNG);
  except
    Result := nil;
    ReportException;
  end;
end;

function TPicture_GetJpeg(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPicture(Obj).Jpeg);
  except
    Result := nil;
    ReportException;
  end;
end;

function TPicture_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TPicture(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

function TPicture_GetHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TPicture(Obj).Height;
  except
    Result := 0;
    ReportException;
  end;
end;

{ 拡張子から形式(クラス)を選んで読み込む。ファイル名は UTF-8。 }
procedure TPicture_LoadFromFile(Obj: Pointer; FileName: PChar); BETH_CALL;
begin
  try
    TPicture(Obj).LoadFromFile(FileName);
  except
    ReportException;
  end;
end;

procedure TPicture_SaveToFile(Obj: Pointer; FileName: PChar); BETH_CALL;
begin
  try
    TPicture(Obj).SaveToFile(FileName);
  except
    ReportException;
  end;
end;

{ Source は TPicture かグラフィックのハンドル。nil なら空にする。 }
procedure TPicture_Assign(Obj: Pointer; Source: Pointer); BETH_CALL;
begin
  try
    TPicture(Obj).Assign(TPersistent(Source));
  except
    ReportException;
  end;
end;

procedure TPicture_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TPicture(Obj).Clear;
  except
    ReportException;
  end;
end;

{ TImage(TCustomImage)。 }

function TImage_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TImage.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomImage_GetPicture(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomImage(Obj).Picture);
  except
    Result := nil;
    ReportException;
  end;
end;

{ Value(TPicture)の内容を写す。 }
procedure TCustomImage_SetPicture(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomImage(Obj).Picture := TPicture(Value);
  except
    ReportException;
  end;
end;

{ Picture が空なら、コントロールの大きさの TBitmap を作ってからその Canvas を返す。
  中身がビットマップの類でない(アイコン等)なら、コントロール自身の Canvas を返す。 }
function TCustomImage_GetCanvas(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomImage(Obj).Canvas);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomImage_GetHasGraphic(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).HasGraphic;
  except
    Result := False;
    ReportException;
  end;
end;

function TCustomImage_GetCenter(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).Center;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomImage_SetCenter(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomImage(Obj).Center := Value;
  except
    ReportException;
  end;
end;

function TCustomImage_GetStretch(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).Stretch;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomImage_SetStretch(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomImage(Obj).Stretch := Value;
  except
    ReportException;
  end;
end;

function TCustomImage_GetStretchOutEnabled(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).StretchOutEnabled;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomImage_SetStretchOutEnabled(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomImage(Obj).StretchOutEnabled := Value;
  except
    ReportException;
  end;
end;

function TCustomImage_GetStretchInEnabled(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).StretchInEnabled;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomImage_SetStretchInEnabled(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomImage(Obj).StretchInEnabled := Value;
  except
    ReportException;
  end;
end;

function TCustomImage_GetProportional(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).Proportional;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomImage_SetProportional(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomImage(Obj).Proportional := Value;
  except
    ReportException;
  end;
end;

function TCustomImage_GetTransparent(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).Transparent;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomImage_SetTransparent(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomImage(Obj).Transparent := Value;
  except
    ReportException;
  end;
end;

{ Picture(またはその中身)が変わったときに呼ばれる。 }
procedure TCustomImage_SetOnPictureChanged(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomImage(Obj).OnPictureChanged := @BridgeFor(TCustomImage(Obj), MethodData(TCustomImage(Obj).OnPictureChanged), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TCustomBitBtn・TCustomSpeedButton の Glyph。ボタンが所有する TBitmap(差し替わらない)を返す。
  Set は Value の内容を写す(nil なら空にする)。NumGlyphs は、横に並べた状態別の画像の数(1〜4)。
  Layout は TButtonLayout の序数(blGlyphLeft=0, blGlyphRight, blGlyphTop, blGlyphBottom)。
  Margin は端から画像までの距離(-1 なら画像と文字列を中央に置く)、Spacing は画像と文字列の間隔。 }

function TCustomBitBtn_GetGlyph(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomBitBtn(Obj).Glyph);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetGlyph(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).Glyph := Graphics.TBitmap(Value);
  except
    ReportException;
  end;
end;

function TCustomBitBtn_GetNumGlyphs(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomBitBtn(Obj).NumGlyphs;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetNumGlyphs(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).NumGlyphs := Value;
  except
    ReportException;
  end;
end;

function TCustomBitBtn_GetLayout(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomBitBtn(Obj).Layout);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetLayout(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).Layout := TButtonLayout(Value);
  except
    ReportException;
  end;
end;

function TCustomBitBtn_GetMargin(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomBitBtn(Obj).Margin;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetMargin(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).Margin := Value;
  except
    ReportException;
  end;
end;

function TCustomBitBtn_GetSpacing(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomBitBtn(Obj).Spacing;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetSpacing(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).Spacing := Value;
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetGlyph(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomSpeedButton(Obj).Glyph);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetGlyph(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).Glyph := Graphics.TBitmap(Value);
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetNumGlyphs(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).NumGlyphs;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetNumGlyphs(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).NumGlyphs := Value;
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetLayout(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomSpeedButton(Obj).Layout);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetLayout(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).Layout := TButtonLayout(Value);
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetMargin(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).Margin;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetMargin(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).Margin := Value;
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetSpacing(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).Spacing;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetSpacing(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).Spacing := Value;
  except
    ReportException;
  end;
end;

{ ---------------- TImageList(docs/adr/0030) ----------------
  TCustomImageList は TComponent(TLCLComponent)なので、生成・破棄は他のコンポーネントと同じ(Watch して返し、Owner に任せてよい)。
  画像を受け取る関数は、画像を写して加える(渡したグラフィックは呼び出し側の持ち物のまま)。
  Add・Insert 等は、画像を Width・Height の大きさに伸縮して 1 つとして加える(VCL と違い、幅が Width の倍数でも分けない)。
  横に並んだ複数の画像を分けて加えるのは AddSliced。 }

function TImageList_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TImageList.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomImageList_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomImageList(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomImageList_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Width := Value;
  except
    ReportException;
  end;
end;

function TCustomImageList_GetHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomImageList(Obj).Height;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomImageList_SetHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Height := Value;
  except
    ReportException;
  end;
end;

function TCustomImageList_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomImageList(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCustomImageList_GetMasked(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomImageList(Obj).Masked;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomImageList_SetMasked(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Masked := Value;
  except
    ReportException;
  end;
end;

function TCustomImageList_GetBkColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TCustomImageList(Obj).BkColor);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomImageList_SetBkColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).BkColor := TColor(Value);
  except
    ReportException;
  end;
end;

{ TDrawingStyle の序数(dsFocus=0, dsSelected, dsNormal, dsTransparent)。 }
function TCustomImageList_GetDrawingStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomImageList(Obj).DrawingStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomImageList_SetDrawingStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).DrawingStyle := TDrawingStyle(Value);
  except
    ReportException;
  end;
end;

{ Image・Mask は TCustomBitmap の派生(TBitmap・TPortableNetworkGraphic・TJPEGImage)のハンドル。Mask は nil でよい。
  加えた最初の画像の位置を返す。 }
function TCustomImageList_Add(Obj: Pointer; Image, Mask: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomImageList(Obj).Add(TCustomBitmap(Image), TCustomBitmap(Mask));
  except
    Result := 0;
    ReportException;
  end;
end;

{ Image を横 AHorizontalCount・縦 AVerticalCount に分けて、それぞれを画像として加える。加えた最初の画像の位置を返す。 }
function TCustomImageList_AddSliced(Obj: Pointer; Image: Pointer; AHorizontalCount, AVerticalCount: Integer): Integer; BETH_CALL;
begin
  try
    Result := TCustomImageList(Obj).AddSliced(TCustomBitmap(Image), AHorizontalCount, AVerticalCount);
  except
    Result := 0;
    ReportException;
  end;
end;

{ MaskColor の画素を透明として加える。Image は TBitmap のハンドル。 }
function TCustomImageList_AddMasked(Obj: Pointer; Image: Pointer; MaskColor: Integer): Integer; BETH_CALL;
begin
  try
    Result := TCustomImageList(Obj).AddMasked(Graphics.TBitmap(Image), TColor(MaskColor));
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomImageList_Insert(Obj: Pointer; Index: Integer; Image, Mask: Pointer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Insert(Index, TCustomBitmap(Image), TCustomBitmap(Mask));
  except
    ReportException;
  end;
end;

procedure TCustomImageList_Replace(Obj: Pointer; Index: Integer; Image, Mask: Pointer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Replace(Index, TCustomBitmap(Image), TCustomBitmap(Mask));
  except
    ReportException;
  end;
end;

procedure TCustomImageList_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure TCustomImageList_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Clear;
  except
    ReportException;
  end;
end;

procedure TCustomImageList_Move(Obj: Pointer; CurIndex, NewIndex: Integer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Move(CurIndex, NewIndex);
  except
    ReportException;
  end;
end;

{ Index 番目の画像を Image(TCustomBitmap の派生)に写す。 }
procedure TCustomImageList_GetBitmap(Obj: Pointer; Index: Integer; Image: Pointer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).GetBitmap(Index, TCustomBitmap(Image));
  except
    ReportException;
  end;
end;

procedure TCustomImageList_Draw(Obj: Pointer; Canvas: Pointer; X, Y, Index: Integer; Enabled: LongBool); BETH_CALL;
begin
  try
    TCustomImageList(Obj).Draw(TCanvas(Canvas), X, Y, Index, Boolean(Enabled));
  except
    ReportException;
  end;
end;

procedure TCustomImageList_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TCustomImageList_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ Clear・Delete・Move・BkColor の変更で呼ばれる(LCL の仕様で、Add・Insert 等では呼ばれない。BeginUpdate の間は EndUpdate まで遅れる)。 }
procedure TCustomImageList_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomImageList(Obj).OnChange := @BridgeFor(TCustomImageList(Obj), MethodData(TCustomImageList(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ 各コントロール・項目の Images・ImageIndex・Bitmap(docs/adr/0030)。
  Images は TCustomImageList のハンドル(nil なら画像リストを外す)。LCL は画像リストの破棄を FreeNotification で受け、
  コントロールの Images を nil に戻す。ImageIndex は画像リストでの位置(-1 なら無し)。
  Bitmap は所有者が持つ TBitmap(差し替わらない)を返し、Set は内容を写す。 }

function TCustomImage_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomImage(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomImage_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomImage(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TCustomImage_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomImage(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomImage_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomImage(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomBitBtn_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomBitBtn(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TCustomBitBtn_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomBitBtn(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomBitBtn_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomBitBtn(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomSpeedButton(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TCustomSpeedButton_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomSpeedButton(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomSpeedButton_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomSpeedButton(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomTabControl_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomTabControl(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomTabControl_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomTabControl(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TCustomPage_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomPage(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPage_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPage(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomTreeView_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomTreeView(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomTreeView_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomTreeView(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TCustomTreeView_GetStateImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomTreeView(Obj).StateImages);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomTreeView_SetStateImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomTreeView(Obj).StateImages := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TTreeNode_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TTreeNode_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TTreeNode(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TTreeNode_GetSelectedIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).SelectedIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TTreeNode_SetSelectedIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TTreeNode(Obj).SelectedIndex := Value;
  except
    ReportException;
  end;
end;

function TTreeNode_GetStateIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).StateIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TTreeNode_SetStateIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TTreeNode(Obj).StateIndex := Value;
  except
    ReportException;
  end;
end;

function TTreeNode_GetOverlayIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TTreeNode(Obj).OverlayIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TTreeNode_SetOverlayIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TTreeNode(Obj).OverlayIndex := Value;
  except
    ReportException;
  end;
end;

function TListView_GetLargeImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TListView(Obj).LargeImages);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TListView_SetLargeImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).LargeImages := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TListView_GetSmallImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TListView(Obj).SmallImages);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TListView_SetSmallImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).SmallImages := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TListView_GetStateImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TListView(Obj).StateImages);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TListView_SetStateImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TListView(Obj).StateImages := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TListItem_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListItem(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListItem_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListItem(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TListItem_GetStateIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListItem(Obj).StateIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListItem_SetStateIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListItem(Obj).StateIndex := Value;
  except
    ReportException;
  end;
end;

function TListColumn_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TListColumn(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TListColumn_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TListColumn(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TToolBar_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TToolBar(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TToolBar_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TToolBar(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TToolBar_GetHotImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TToolBar(Obj).HotImages);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TToolBar_SetHotImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TToolBar(Obj).HotImages := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TToolBar_GetDisabledImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TToolBar(Obj).DisabledImages);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TToolBar_SetDisabledImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TToolBar(Obj).DisabledImages := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TToolButton_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TToolButton(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TToolButton_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TToolButton(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomHeaderControl_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomHeaderControl(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomHeaderControl_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomHeaderControl(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function THeaderSection_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := THeaderSection(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure THeaderSection_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    THeaderSection(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomCoolBar(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TCustomCoolBar_GetBitmap(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomCoolBar(Obj).Bitmap);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomCoolBar_SetBitmap(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomCoolBar(Obj).Bitmap := Graphics.TBitmap(Value);
  except
    ReportException;
  end;
end;

function TCoolBand_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCoolBand(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCoolBand_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCoolBand(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCoolBand_GetBitmap(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCoolBand(Obj).Bitmap);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCoolBand_SetBitmap(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCoolBand(Obj).Bitmap := Graphics.TBitmap(Value);
  except
    ReportException;
  end;
end;

function TMenu_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TMenu(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TMenu_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TMenu(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TMenuItem_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TMenuItem(Obj).ImageIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TMenuItem_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TMenuItem(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TMenuItem_GetSubMenuImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TMenuItem(Obj).SubMenuImages);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TMenuItem_SetSubMenuImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).SubMenuImages := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TMenuItem_GetBitmap(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TMenuItem(Obj).Bitmap);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TMenuItem_SetBitmap(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Bitmap := Graphics.TBitmap(Value);
  except
    ReportException;
  end;
end;

{ TControl の Color・Font と TFont の Style・Assign(ダイアログの結果を適用する先。docs/adr/0033) }

function TControl_GetColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TControl(Obj).Color);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).Color := TColor(Value);
  except
    ReportException;
  end;
end;

{ コントロールが所有する TFont(コントロールと寿命が一致し、差し替わらない)。 }
function TControl_GetFont(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TControl(Obj).Font);
  except
    Result := nil;
    ReportException;
  end;
end;

{ Value の内容を写す(LCL の SetFont と同じ)。nil なら何もしない。 }
procedure TControl_SetFont(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    if Value <> nil then
      TControl(Obj).Font := TFont(Value);
  except
    ReportException;
  end;
end;

{ TFontStyles(集合型)を LongWord のビット集合(fsBold = 1 shl 0 等)に変換する。 }
function FontStylesToInt(const S: TFontStyles): LongWord;
var
  I: TFontStyle;
begin
  Result := 0;
  for I := Low(TFontStyle) to High(TFontStyle) do
    if I in S then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToFontStyles(V: LongWord): TFontStyles;
var
  I: TFontStyle;
begin
  Result := [];
  for I := Low(TFontStyle) to High(TFontStyle) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

function TFont_GetStyle(Obj: Pointer): LongWord; BETH_CALL;
begin
  try
    Result := FontStylesToInt(TFont(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFont_SetStyle(Obj: Pointer; Value: LongWord); BETH_CALL;
begin
  try
    TFont(Obj).Style := IntToFontStyles(Value);
  except
    ReportException;
  end;
end;

{ Source の内容(Name・Size・Color・Style 等)を写す。nil なら何もしない。 }
procedure TFont_Assign(Obj: Pointer; Source: Pointer); BETH_CALL;
begin
  try
    if Source <> nil then
      TFont(Obj).Assign(TFont(Source));
  except
    ReportException;
  end;
end;

{ TCommonDialog(docs/adr/0033) }

{ ダイアログを表示する。TFindDialog・TReplaceDialog 以外は閉じるまで戻らず、OK で閉じたら True を返す。 }
function TCommonDialog_Execute(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCommonDialog(Obj).Execute;
  except
    Result := False;
    ReportException;
  end;
end;

function TCommonDialog_GetTitle(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TCommonDialog(Obj).Title);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCommonDialog_SetTitle(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TCommonDialog(Obj).Title := Value;
  except
    ReportException;
  end;
end;

procedure TCommonDialog_SetOnShow(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCommonDialog(Obj).OnShow := @BridgeFor(TCommonDialog(Obj), MethodData(TCommonDialog(Obj).OnShow), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TCommonDialog_SetOnClose(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCommonDialog(Obj).OnClose := @BridgeFor(TCommonDialog(Obj), MethodData(TCommonDialog(Obj).OnClose), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ OK で閉じようとしたとき(CanClose を False にすると閉じない)。Value は TBethVarCallback の CanClose と同じ。 }
procedure TCommonDialog_SetOnCanClose(Obj: Pointer; Cb: TBethVarCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCommonDialog(Obj).OnCanClose := @VarBridgeFor(TCommonDialog(Obj), MethodData(TCommonDialog(Obj).OnCanClose), Cb, Data).DoCloseQuery;
  except
    ReportException;
  end;
end;

{ TFileDialog }

function TFileDialog_GetFileName(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TFileDialog(Obj).FileName);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TFileDialog_SetFileName(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TFileDialog(Obj).FileName := Value;
  except
    ReportException;
  end;
end;

function TFileDialog_GetFilter(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TFileDialog(Obj).Filter);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TFileDialog_SetFilter(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TFileDialog(Obj).Filter := Value;
  except
    ReportException;
  end;
end;

function TFileDialog_GetFilterIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFileDialog(Obj).FilterIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFileDialog_SetFilterIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFileDialog(Obj).FilterIndex := Value;
  except
    ReportException;
  end;
end;

function TFileDialog_GetInitialDir(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TFileDialog(Obj).InitialDir);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TFileDialog_SetInitialDir(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TFileDialog(Obj).InitialDir := Value;
  except
    ReportException;
  end;
end;

function TFileDialog_GetDefaultExt(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TFileDialog(Obj).DefaultExt);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TFileDialog_SetDefaultExt(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TFileDialog(Obj).DefaultExt := Value;
  except
    ReportException;
  end;
end;

{ 選択したファイルの一覧(ofAllowMultiSelect のとき複数)。ダイアログが所有する TStrings で、差し替わらない。 }
function TFileDialog_GetFiles(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TFileDialog(Obj).Files);
  except
    Result := nil;
    ReportException;
  end;
end;

{ TOpenDialog / TSaveDialog / TSelectDirectoryDialog }

function OpenOptionsToInt(const O: TOpenOptions): LongWord;
var
  I: TOpenOption;
begin
  Result := 0;
  for I := Low(TOpenOption) to High(TOpenOption) do
    if I in O then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToOpenOptions(V: LongWord): TOpenOptions;
var
  I: TOpenOption;
begin
  Result := [];
  for I := Low(TOpenOption) to High(TOpenOption) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

function TOpenDialog_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TOpenDialog.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TOpenDialog_GetOptions(Obj: Pointer): LongWord; BETH_CALL;
begin
  try
    Result := OpenOptionsToInt(TOpenDialog(Obj).Options);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TOpenDialog_SetOptions(Obj: Pointer; Value: LongWord); BETH_CALL;
begin
  try
    TOpenDialog(Obj).Options := IntToOpenOptions(Value);
  except
    ReportException;
  end;
end;

function TSaveDialog_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TSaveDialog.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TSelectDirectoryDialog_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TSelectDirectoryDialog.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TColorDialog }

function ColorDialogOptionsToInt(const O: TColorDialogOptions): LongWord;
var
  I: TColorDialogOption;
begin
  Result := 0;
  for I := Low(TColorDialogOption) to High(TColorDialogOption) do
    if I in O then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToColorDialogOptions(V: LongWord): TColorDialogOptions;
var
  I: TColorDialogOption;
begin
  Result := [];
  for I := Low(TColorDialogOption) to High(TColorDialogOption) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

function TColorDialog_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TColorDialog.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TColorDialog_GetColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Integer(TColorDialog(Obj).Color);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TColorDialog_SetColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TColorDialog(Obj).Color := TColor(Value);
  except
    ReportException;
  end;
end;

{ 作成した色("ColorA=FFFFFF" のような 名前=値 の行)。ダイアログが所有する TStrings で、差し替わらない。 }
function TColorDialog_GetCustomColors(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TColorDialog(Obj).CustomColors);
  except
    Result := nil;
    ReportException;
  end;
end;

function TColorDialog_GetOptions(Obj: Pointer): LongWord; BETH_CALL;
begin
  try
    Result := ColorDialogOptionsToInt(TColorDialog(Obj).Options);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TColorDialog_SetOptions(Obj: Pointer; Value: LongWord); BETH_CALL;
begin
  try
    TColorDialog(Obj).Options := IntToColorDialogOptions(Value);
  except
    ReportException;
  end;
end;

{ TFontDialog }

function FontDialogOptionsToInt(const O: TFontDialogOptions): LongWord;
var
  I: TFontDialogOption;
begin
  Result := 0;
  for I := Low(TFontDialogOption) to High(TFontDialogOption) do
    if I in O then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToFontDialogOptions(V: LongWord): TFontDialogOptions;
var
  I: TFontDialogOption;
begin
  Result := [];
  for I := Low(TFontDialogOption) to High(TFontDialogOption) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

function TFontDialog_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TFontDialog.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ ダイアログが所有する TFont(ダイアログと寿命が一致し、差し替わらない)。 }
function TFontDialog_GetFont(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TFontDialog(Obj).Font);
  except
    Result := nil;
    ReportException;
  end;
end;

{ Value の内容を写す(LCL の SetFont と同じ)。nil なら何もしない。 }
procedure TFontDialog_SetFont(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    if Value <> nil then
      TFontDialog(Obj).Font := TFont(Value);
  except
    ReportException;
  end;
end;

function TFontDialog_GetMinFontSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFontDialog(Obj).MinFontSize;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFontDialog_SetMinFontSize(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFontDialog(Obj).MinFontSize := Value;
  except
    ReportException;
  end;
end;

function TFontDialog_GetMaxFontSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFontDialog(Obj).MaxFontSize;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFontDialog_SetMaxFontSize(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFontDialog(Obj).MaxFontSize := Value;
  except
    ReportException;
  end;
end;

function TFontDialog_GetOptions(Obj: Pointer): LongWord; BETH_CALL;
begin
  try
    Result := FontDialogOptionsToInt(TFontDialog(Obj).Options);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFontDialog_SetOptions(Obj: Pointer; Value: LongWord); BETH_CALL;
begin
  try
    TFontDialog(Obj).Options := IntToFontDialogOptions(Value);
  except
    ReportException;
  end;
end;

{ TFindDialog / TReplaceDialog }

type
  { ReplaceText・OnReplace は TFindDialog の protected で、TReplaceDialog が published にしている。 }
  TFindDialogAccess = class(TFindDialog);

function FindOptionsToInt(const O: TFindOptions): LongWord;
var
  I: TFindOption;
begin
  Result := 0;
  for I := Low(TFindOption) to High(TFindOption) do
    if I in O then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToFindOptions(V: LongWord): TFindOptions;
var
  I: TFindOption;
begin
  Result := [];
  for I := Low(TFindOption) to High(TFindOption) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

function TFindDialog_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TFindDialog.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TReplaceDialog_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TReplaceDialog.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

function TFindDialog_GetFindText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TFindDialog(Obj).FindText);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TFindDialog_SetFindText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TFindDialog(Obj).FindText := Value;
  except
    ReportException;
  end;
end;

function TFindDialog_GetReplaceText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TFindDialogAccess(Obj).ReplaceText);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TFindDialog_SetReplaceText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TFindDialogAccess(Obj).ReplaceText := Value;
  except
    ReportException;
  end;
end;

function TFindDialog_GetOptions(Obj: Pointer): LongWord; BETH_CALL;
begin
  try
    Result := FindOptionsToInt(TFindDialog(Obj).Options);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFindDialog_SetOptions(Obj: Pointer; Value: LongWord); BETH_CALL;
begin
  try
    TFindDialog(Obj).Options := IntToFindOptions(Value);
  except
    ReportException;
  end;
end;

function TFindDialog_GetLeft(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFindDialog(Obj).Left;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFindDialog_SetLeft(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFindDialog(Obj).Left := Value;
  except
    ReportException;
  end;
end;

function TFindDialog_GetTop(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFindDialog(Obj).Top;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFindDialog_SetTop(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFindDialog(Obj).Top := Value;
  except
    ReportException;
  end;
end;

{ 表示中のダイアログを閉じる。 }
procedure TFindDialog_CloseDialog(Obj: Pointer); BETH_CALL;
begin
  try
    TFindDialog(Obj).CloseDialog;
  except
    ReportException;
  end;
end;

procedure TFindDialog_SetOnFind(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TFindDialog(Obj).OnFind := @BridgeFor(TFindDialog(Obj), MethodData(TFindDialog(Obj).OnFind), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TFindDialog_SetOnReplace(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TFindDialogAccess(Obj).OnReplace := @BridgeFor(TFindDialog(Obj), MethodData(TFindDialogAccess(Obj).OnReplace), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ デザイナーで設定する共通のプロパティ(docs/adr/0034) }

{ TComponent }

function TComponent_GetTag(Obj: Pointer): PtrInt; BETH_CALL;
begin
  try
    Result := TComponent(Obj).Tag;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TComponent_SetTag(Obj: Pointer; Value: PtrInt); BETH_CALL;
begin
  try
    TComponent(Obj).Tag := Value;
  except
    ReportException;
  end;
end;

{ TControl }

{ TAnchors(集合型)を LongWord のビット集合(akTop = 1 shl 0 等)に変換する。 }
function AnchorsToInt(const A: TAnchors): LongWord;
var
  I: TAnchorKind;
begin
  Result := 0;
  for I := Low(TAnchorKind) to High(TAnchorKind) do
    if I in A then
      Result := Result or (LongWord(1) shl Ord(I));
end;

function IntToAnchors(V: LongWord): TAnchors;
var
  I: TAnchorKind;
begin
  Result := [];
  for I := Low(TAnchorKind) to High(TAnchorKind) do
    if (V and (LongWord(1) shl Ord(I))) <> 0 then
      Include(Result, I);
end;

function TControl_GetAnchors(Obj: Pointer): LongWord; BETH_CALL;
begin
  try
    Result := AnchorsToInt(TControl(Obj).Anchors);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetAnchors(Obj: Pointer; Value: LongWord); BETH_CALL;
begin
  try
    TControl(Obj).Anchors := IntToAnchors(Value);
  except
    ReportException;
  end;
end;

{ コントロールが所有する TControlBorderSpacing(コントロールと寿命が一致し、差し替わらない)。 }
function TControl_GetBorderSpacing(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TControl(Obj).BorderSpacing);
  except
    Result := nil;
    ReportException;
  end;
end;

{ Value の内容を写す(LCL の SetBorderSpacing と同じ)。nil なら何もしない。 }
procedure TControl_SetBorderSpacing(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    if Value <> nil then
      TControl(Obj).BorderSpacing := TControlBorderSpacing(Value);
  except
    ReportException;
  end;
end;

{ コントロールが所有する TSizeConstraints(コントロールと寿命が一致し、差し替わらない)。 }
function TControl_GetConstraints(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TControl(Obj).Constraints);
  except
    Result := nil;
    ReportException;
  end;
end;

{ Value の内容を写す(LCL の SetConstraints と同じ)。nil なら何もしない。 }
procedure TControl_SetConstraints(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    if Value <> nil then
      TControl(Obj).Constraints := TSizeConstraints(Value);
  except
    ReportException;
  end;
end;

function TControl_GetHint(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TControl(Obj).Hint);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TControl_SetHint(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TControl(Obj).Hint := Value;
  except
    ReportException;
  end;
end;

function TControl_GetShowHint(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControl(Obj).ShowHint;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControl_SetShowHint(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControl(Obj).ShowHint := Value;
  except
    ReportException;
  end;
end;

function TControl_GetCursor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControl(Obj).Cursor;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetCursor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).Cursor := TCursor(Value);
  except
    ReportException;
  end;
end;

{ ParentColor・ParentFont・ParentShowHint は TControl の protected で、ほとんどの具象クラスが published にしている。 }

function TControl_GetParentColor(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControlAccess(Obj).ParentColor;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControl_SetParentColor(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControlAccess(Obj).ParentColor := Value;
  except
    ReportException;
  end;
end;

function TControl_GetParentFont(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControlAccess(Obj).ParentFont;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControl_SetParentFont(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControlAccess(Obj).ParentFont := Value;
  except
    ReportException;
  end;
end;

function TControl_GetParentShowHint(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControlAccess(Obj).ParentShowHint;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControl_SetParentShowHint(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControlAccess(Obj).ParentShowHint := Value;
  except
    ReportException;
  end;
end;

{ TWinControl }

function TWinControl_GetTabOrder(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TWinControl(Obj).TabOrder;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TWinControl_SetTabOrder(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TWinControl(Obj).TabOrder := TTabOrder(Value);
  except
    ReportException;
  end;
end;

function TWinControl_GetTabStop(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TWinControl(Obj).TabStop;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TWinControl_SetTabStop(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TWinControl(Obj).TabStop := Value;
  except
    ReportException;
  end;
end;

{ TSizeConstraints(0 は制限なし) }

function TSizeConstraints_GetMinWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TSizeConstraints(Obj).MinWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TSizeConstraints_SetMinWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TSizeConstraints(Obj).MinWidth := Value;
  except
    ReportException;
  end;
end;

function TSizeConstraints_GetMinHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TSizeConstraints(Obj).MinHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TSizeConstraints_SetMinHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TSizeConstraints(Obj).MinHeight := Value;
  except
    ReportException;
  end;
end;

function TSizeConstraints_GetMaxWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TSizeConstraints(Obj).MaxWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TSizeConstraints_SetMaxWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TSizeConstraints(Obj).MaxWidth := Value;
  except
    ReportException;
  end;
end;

function TSizeConstraints_GetMaxHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TSizeConstraints(Obj).MaxHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TSizeConstraints_SetMaxHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TSizeConstraints(Obj).MaxHeight := Value;
  except
    ReportException;
  end;
end;

{ TControlBorderSpacing }

function TControlBorderSpacing_GetLeft(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlBorderSpacing(Obj).Left;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlBorderSpacing_SetLeft(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlBorderSpacing(Obj).Left := Value;
  except
    ReportException;
  end;
end;

function TControlBorderSpacing_GetTop(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlBorderSpacing(Obj).Top;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlBorderSpacing_SetTop(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlBorderSpacing(Obj).Top := Value;
  except
    ReportException;
  end;
end;

function TControlBorderSpacing_GetRight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlBorderSpacing(Obj).Right;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlBorderSpacing_SetRight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlBorderSpacing(Obj).Right := Value;
  except
    ReportException;
  end;
end;

function TControlBorderSpacing_GetBottom(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlBorderSpacing(Obj).Bottom;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlBorderSpacing_SetBottom(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlBorderSpacing(Obj).Bottom := Value;
  except
    ReportException;
  end;
end;

function TControlBorderSpacing_GetAround(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlBorderSpacing(Obj).Around;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlBorderSpacing_SetAround(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlBorderSpacing(Obj).Around := Value;
  except
    ReportException;
  end;
end;

function TControlBorderSpacing_GetInnerBorder(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlBorderSpacing(Obj).InnerBorder;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlBorderSpacing_SetInnerBorder(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlBorderSpacing(Obj).InnerBorder := Value;
  except
    ReportException;
  end;
end;

{ ---------------- ModalResult・メッセージのダイアログ・フォームの表示・既定のボタン(docs/adr/0041) ---------------- }

function TCustomForm_GetModalResult(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomForm(Obj).ModalResult;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomForm_SetModalResult(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomForm(Obj).ModalResult := Value;
  except
    ReportException;
  end;
end;

function TCustomForm_GetBorderStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomForm(Obj).BorderStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomForm_SetBorderStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomForm(Obj).BorderStyle := TFormBorderStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomForm_GetPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomForm(Obj).Position);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomForm_SetPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomForm(Obj).Position := TPosition(Value);
  except
    ReportException;
  end;
end;

function TCustomForm_GetWindowState(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomForm(Obj).WindowState);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomForm_SetWindowState(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomForm(Obj).WindowState := TWindowState(Value);
  except
    ReportException;
  end;
end;

function TCustomForm_GetBorderIcons(Obj: Pointer): LongWord; BETH_CALL;
var
  I: TBorderIcon;
begin
  try
    Result := 0;
    for I := Low(TBorderIcon) to High(TBorderIcon) do
      if I in TCustomForm(Obj).BorderIcons then
        Result := Result or (LongWord(1) shl Ord(I));
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomForm_SetBorderIcons(Obj: Pointer; Value: LongWord); BETH_CALL;
var
  I: TBorderIcon;
  Icons: TBorderIcons;
begin
  try
    Icons := [];
    for I := Low(TBorderIcon) to High(TBorderIcon) do
      if (Value and (LongWord(1) shl Ord(I))) <> 0 then
        Include(Icons, I);
    TCustomForm(Obj).BorderIcons := Icons;
  except
    ReportException;
  end;
end;

function TCustomForm_GetFormStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomForm(Obj).FormStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomForm_SetFormStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomForm(Obj).FormStyle := TFormStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomForm_GetKeyPreview(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomForm(Obj).KeyPreview;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomForm_SetKeyPreview(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomForm(Obj).KeyPreview := Value;
  except
    ReportException;
  end;
end;

function TCustomForm_GetActiveControl(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomForm(Obj).ActiveControl);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomForm_SetActiveControl(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).ActiveControl := TWinControl(Value);
  except
    ReportException;
  end;
end;

function TCustomButton_GetModalResult(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomButton(Obj).ModalResult;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomButton_SetModalResult(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomButton(Obj).ModalResult := Value;
  except
    ReportException;
  end;
end;

function TCustomButton_GetDefault(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomButton(Obj).Default;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomButton_SetDefault(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomButton(Obj).Default := Value;
  except
    ReportException;
  end;
end;

function TCustomButton_GetCancel(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomButton(Obj).Cancel;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomButton_SetCancel(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomButton(Obj).Cancel := Value;
  except
    ReportException;
  end;
end;

{ メッセージのダイアログ。Buttons は TMsgDlgBtn の順のビット集合。Caption が空なら題名を付けない形(LCL の既定の題名)を使う。 }

function IntToMsgDlgButtons(V: LongWord): TMsgDlgButtons;
var
  B: TMsgDlgBtn;
begin
  Result := [];
  for B := Low(TMsgDlgBtn) to High(TMsgDlgBtn) do
    if (V and (LongWord(1) shl Ord(B))) <> 0 then
      Include(Result, B);
end;

procedure Dialogs_ShowMessage(Msg: PChar); BETH_CALL;
begin
  try
    ShowMessage(Msg);
  except
    ReportException;
  end;
end;

function Dialogs_MessageDlg(Caption, Msg: PChar; DlgType: Integer; Buttons: LongWord;
  HelpCtx: Integer): Integer; BETH_CALL;
begin
  try
    if (Caption = nil) or (Caption^ = #0) then
      Result := MessageDlg(Msg, TMsgDlgType(DlgType), IntToMsgDlgButtons(Buttons), HelpCtx)
    else
      Result := MessageDlg(Caption, Msg, TMsgDlgType(DlgType), IntToMsgDlgButtons(Buttons), HelpCtx);
  except
    Result := 0;
    ReportException;
  end;
end;

function Dialogs_InputBox(Caption, Prompt, DefaultValue: PChar): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(InputBox(Caption, Prompt, DefaultValue));
  except
    Result := '';
    ReportException;
  end;
end;

function Dialogs_PasswordBox(Caption, Prompt: PChar): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(PasswordBox(Caption, Prompt));
  except
    Result := '';
    ReportException;
  end;
end;

{ InputQuery。OK なら Ok^ を真にし、入力された文字列を返す(取りやめなら Value のまま)。 }
function Dialogs_InputQuery(Caption, Prompt, Value: PChar; Ok: PLongBool): PChar; BETH_CALL;
var
  S: AnsiString;
begin
  try
    S := Value;
    Ok^ := InputQuery(Caption, Prompt, S);
    Result := ReturnStr(S);
  except
    Ok^ := False;
    Result := '';
    ReportException;
  end;
end;

{ Flags と戻り値は Windows の MB_…・ID… の値(LCL の TApplication.MessageBox と同じ)。 }
function TApplication_MessageBox(Obj: Pointer; Text, Caption: PChar; Flags: Integer): Integer; BETH_CALL;
begin
  try
    Result := TApplication(Obj).MessageBox(Text, Caption, Flags);
  except
    Result := 0;
    ReportException;
  end;
end;

{ ---------------- フォーカス・表示の更新・テキストの編集・TMemo・TLabel(docs/adr/0042) ---------------- }

{ 選択の置き換え・全選択・クリップボード・元に戻す、は LCL ではウィンドウのハンドルが無いと何もしない。
  VCL と同じく表示の前でも使えるよう、先にハンドルを作る(HandleNeeded。親のハンドルも作られる)。 }

function TControl_GetClientWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControl(Obj).ClientWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetClientWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).ClientWidth := Value;
  except
    ReportException;
  end;
end;

function TControl_GetClientHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControl(Obj).ClientHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControl_SetClientHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControl(Obj).ClientHeight := Value;
  except
    ReportException;
  end;
end;

procedure TControl_Invalidate(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Invalidate;
  except
    ReportException;
  end;
end;

procedure TControl_Repaint(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Repaint;
  except
    ReportException;
  end;
end;

procedure TControl_Refresh(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Refresh;
  except
    ReportException;
  end;
end;

procedure TControl_Update(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Update;
  except
    ReportException;
  end;
end;

procedure TControl_BringToFront(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).BringToFront;
  except
    ReportException;
  end;
end;

procedure TControl_SendToBack(Obj: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).SendToBack;
  except
    ReportException;
  end;
end;

procedure TControl_SetBounds(Obj: Pointer; ALeft, ATop, AWidth, AHeight: Integer); BETH_CALL;
begin
  try
    TControl(Obj).SetBounds(ALeft, ATop, AWidth, AHeight);
  except
    ReportException;
  end;
end;

procedure TWinControl_SetFocus(Obj: Pointer); BETH_CALL;
begin
  try
    TWinControlAccess(Obj).SetFocus;
  except
    ReportException;
  end;
end;

function TWinControl_CanFocus(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TWinControlAccess(Obj).CanFocus;
  except
    Result := False;
    ReportException;
  end;
end;

function TWinControl_Focused(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TWinControlAccess(Obj).Focused;
  except
    Result := False;
    ReportException;
  end;
end;

function TCustomEdit_GetSelStart(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomEditAccess(Obj).SelStart;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetSelStart(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).SelStart := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetSelLength(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomEditAccess(Obj).SelLength;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetSelLength(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).SelLength := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetSelText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TCustomEditAccess(Obj).SelText);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TCustomEdit_SetSelText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HandleNeeded;
    TCustomEditAccess(Obj).SelText := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetModified(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomEditAccess(Obj).Modified;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomEdit_SetModified(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).Modified := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetCanUndo(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomEditAccess(Obj).CanUndo;
  except
    Result := False;
    ReportException;
  end;
end;

function TCustomEdit_GetPasswordChar(Obj: Pointer): AnsiChar; BETH_CALL;
begin
  try
    Result := AnsiChar(TCustomEditAccess(Obj).PasswordChar);
  except
    Result := #0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetPasswordChar(Obj: Pointer; Value: AnsiChar); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).PasswordChar := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetEchoMode(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomEditAccess(Obj).EchoMode);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetEchoMode(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).EchoMode := TEchoMode(Value);
  except
    ReportException;
  end;
end;

function TCustomEdit_GetCharCase(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomEditAccess(Obj).CharCase);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetCharCase(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).CharCase := TEditCharCase(Value);
  except
    ReportException;
  end;
end;

function TCustomEdit_GetAlignment(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomEditAccess(Obj).Alignment);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetAlignment(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).Alignment := TAlignment(Value);
  except
    ReportException;
  end;
end;

function TCustomEdit_GetTextHint(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TCustomEditAccess(Obj).TextHint);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TCustomEdit_SetTextHint(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).TextHint := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetNumbersOnly(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomEditAccess(Obj).NumbersOnly;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomEdit_SetNumbersOnly(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).NumbersOnly := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetAutoSelect(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomEditAccess(Obj).AutoSelect;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomEdit_SetAutoSelect(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).AutoSelect := Value;
  except
    ReportException;
  end;
end;

function TCustomEdit_GetHideSelection(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomEditAccess(Obj).HideSelection;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomEdit_SetHideSelection(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HideSelection := Value;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_GetCaretPos(Obj: Pointer; X, Y: PInteger); BETH_CALL;
var
  P: TPoint;
begin
  try
    P := TCustomEditAccess(Obj).CaretPos;
    X^ := P.X;
    Y^ := P.Y;
  except
    X^ := 0;
    Y^ := 0;
    ReportException;
  end;
end;

procedure TCustomEdit_SetCaretPos(Obj: Pointer; X, Y: Integer); BETH_CALL;
var
  P: TPoint;
begin
  try
    { Point(X, Y) は Windows ユニットの型 POINT の型変換と解釈されるため、組み立てて渡す }
    P.X := X;
    P.Y := Y;
    TCustomEditAccess(Obj).CaretPos := P;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_SelectAll(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HandleNeeded;
    TCustomEditAccess(Obj).SelectAll;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_ClearSelection(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HandleNeeded;
    TCustomEditAccess(Obj).ClearSelection;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).Clear;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_CopyToClipboard(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HandleNeeded;
    TCustomEditAccess(Obj).CopyToClipboard;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_CutToClipboard(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HandleNeeded;
    TCustomEditAccess(Obj).CutToClipboard;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_PasteFromClipboard(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HandleNeeded;
    TCustomEditAccess(Obj).PasteFromClipboard;
  except
    ReportException;
  end;
end;

procedure TCustomEdit_Undo(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomEditAccess(Obj).HandleNeeded;
    TCustomEditAccess(Obj).Undo;
  except
    ReportException;
  end;
end;

function TCustomMemo_GetWordWrap(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomMemoAccess(Obj).WordWrap;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomMemo_SetWordWrap(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomMemoAccess(Obj).WordWrap := Value;
  except
    ReportException;
  end;
end;

function TCustomMemo_GetWantReturns(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomMemoAccess(Obj).WantReturns;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomMemo_SetWantReturns(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomMemoAccess(Obj).WantReturns := Value;
  except
    ReportException;
  end;
end;

function TCustomMemo_GetWantTabs(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomMemoAccess(Obj).WantTabs;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomMemo_SetWantTabs(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomMemoAccess(Obj).WantTabs := Value;
  except
    ReportException;
  end;
end;

procedure TCustomMemo_Append(Obj: Pointer; S: PChar); BETH_CALL;
begin
  try
    TCustomMemoAccess(Obj).Append(S);
  except
    ReportException;
  end;
end;

function TCustomLabel_GetAlignment(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomLabelAccess(Obj).Alignment);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomLabel_SetAlignment(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomLabelAccess(Obj).Alignment := TAlignment(Value);
  except
    ReportException;
  end;
end;

function TCustomLabel_GetLayout(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomLabelAccess(Obj).Layout);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomLabel_SetLayout(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomLabelAccess(Obj).Layout := TTextLayout(Value);
  except
    ReportException;
  end;
end;

function TCustomLabel_GetWordWrap(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomLabelAccess(Obj).WordWrap;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomLabel_SetWordWrap(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomLabelAccess(Obj).WordWrap := Value;
  except
    ReportException;
  end;
end;

function TCustomLabel_GetTransparent(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomLabelAccess(Obj).Transparent;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomLabel_SetTransparent(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomLabelAccess(Obj).Transparent := Value;
  except
    ReportException;
  end;
end;

function TCustomLabel_GetFocusControl(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomLabelAccess(Obj).FocusControl);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomLabel_SetFocusControl(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomLabelAccess(Obj).FocusControl := TWinControl(Value);
  except
    ReportException;
  end;
end;

function TCustomLabel_GetShowAccelChar(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomLabelAccess(Obj).ShowAccelChar;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomLabel_SetShowAccelChar(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomLabelAccess(Obj).ShowAccelChar := Value;
  except
    ReportException;
  end;
end;

procedure TWinControl_SetOnEnter(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TWinControlAccess(Obj).OnEnter := @BridgeFor(TWinControl(Obj), MethodData(TWinControlAccess(Obj).OnEnter), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TWinControl_SetOnExit(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TWinControlAccess(Obj).OnExit := @BridgeFor(TWinControl(Obj), MethodData(TWinControlAccess(Obj).OnExit), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ ---------------- リスト・コンボの選択・チェックの 3 状態・Application(docs/adr/0043) ---------------- }

function TCustomListBox_GetMultiSelect(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListBoxAccess(Obj).MultiSelect;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListBox_SetMultiSelect(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).MultiSelect := Value;
  except
    ReportException;
  end;
end;

function TCustomListBox_GetExtendedSelect(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListBoxAccess(Obj).ExtendedSelect;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListBox_SetExtendedSelect(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).ExtendedSelect := Value;
  except
    ReportException;
  end;
end;

function TCustomListBox_GetSorted(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListBoxAccess(Obj).Sorted;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListBox_SetSorted(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).Sorted := Value;
  except
    ReportException;
  end;
end;

function TCustomListBox_GetTopIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomListBoxAccess(Obj).TopIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomListBox_SetTopIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).TopIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomListBox_GetSelCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomListBoxAccess(Obj).SelCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCustomListBox_GetSelected(Obj: Pointer; Index: Integer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomListBoxAccess(Obj).Selected[Index];
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomListBox_SetSelected(Obj: Pointer; Index: Integer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).Selected[Index] := Value;
  except
    ReportException;
  end;
end;

procedure TCustomListBox_ClearSelection(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).ClearSelection;
  except
    ReportException;
  end;
end;

procedure TCustomListBox_SelectAll(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).SelectAll;
  except
    ReportException;
  end;
end;

function TCustomListBox_ItemAtPos(Obj: Pointer; X, Y: Integer; Existing: LongBool): Integer; BETH_CALL;
var
  P: TPoint;
begin
  try
    P.X := X;
    P.Y := Y;
    { 項目の位置はウィンドウのハンドルが無いと求まらない(表示の前でも使えるよう、先に作る) }
    TCustomListBoxAccess(Obj).HandleNeeded;
    Result := TCustomListBoxAccess(Obj).ItemAtPos(P, Existing);
  except
    Result := -1;
    ReportException;
  end;
end;

procedure TCustomListBox_SetOnSelectionChange(Obj: Pointer; Cb: TBethBoolCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomListBoxAccess(Obj).OnSelectionChange := @BoolBridgeFor(TComponent(Obj), MethodData(TCustomListBoxAccess(Obj).OnSelectionChange), Cb, Data).DoSelectionChange;
  except
    ReportException;
  end;
end;

function TCustomComboBox_GetStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomComboBoxAccess(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomComboBox_SetStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).Style := TComboBoxStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomComboBox_GetDropDownCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomComboBoxAccess(Obj).DropDownCount;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomComboBox_SetDropDownCount(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).DropDownCount := Value;
  except
    ReportException;
  end;
end;

function TCustomComboBox_GetSorted(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomComboBoxAccess(Obj).Sorted;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomComboBox_SetSorted(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).Sorted := Value;
  except
    ReportException;
  end;
end;

function TCustomComboBox_GetReadOnly(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomComboBoxAccess(Obj).ReadOnly;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomComboBox_SetReadOnly(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).ReadOnly := Value;
  except
    ReportException;
  end;
end;

function TCustomComboBox_GetDroppedDown(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomComboBoxAccess(Obj).DroppedDown;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomComboBox_SetDroppedDown(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).DroppedDown := Value;
  except
    ReportException;
  end;
end;

function TCustomComboBox_GetAutoComplete(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomComboBoxAccess(Obj).AutoComplete;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomComboBox_SetAutoComplete(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).AutoComplete := Value;
  except
    ReportException;
  end;
end;

procedure TCustomComboBox_SetOnSelect(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).OnSelect := @BridgeFor(TComponent(Obj), MethodData(TCustomComboBoxAccess(Obj).OnSelect), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TCustomComboBox_SetOnDropDown(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).OnDropDown := @BridgeFor(TComponent(Obj), MethodData(TCustomComboBoxAccess(Obj).OnDropDown), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TCustomComboBox_SetOnCloseUp(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomComboBoxAccess(Obj).OnCloseUp := @BridgeFor(TComponent(Obj), MethodData(TCustomComboBoxAccess(Obj).OnCloseUp), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

function TCustomCheckBox_GetState(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomCheckBoxAccess(Obj).State);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomCheckBox_SetState(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomCheckBoxAccess(Obj).State := TCheckBoxState(Value);
  except
    ReportException;
  end;
end;

function TCustomCheckBox_GetAllowGrayed(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCheckBoxAccess(Obj).AllowGrayed;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCheckBox_SetAllowGrayed(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCheckBoxAccess(Obj).AllowGrayed := Value;
  except
    ReportException;
  end;
end;

procedure TCustomCheckBox_SetOnChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomCheckBoxAccess(Obj).OnChange := @BridgeFor(TComponent(Obj), MethodData(TCustomCheckBoxAccess(Obj).OnChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

function TApplication_GetExeName(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TApplication(Obj).ExeName);
  except
    Result := '';
    ReportException;
  end;
end;

function TApplication_GetHint(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TApplication(Obj).Hint);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TApplication_SetHint(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TApplication(Obj).Hint := Value;
  except
    ReportException;
  end;
end;

function TApplication_GetShowHint(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TApplication(Obj).ShowHint;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TApplication_SetShowHint(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TApplication(Obj).ShowHint := Value;
  except
    ReportException;
  end;
end;

function TApplication_GetHintPause(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TApplication(Obj).HintPause;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TApplication_SetHintPause(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TApplication(Obj).HintPause := Value;
  except
    ReportException;
  end;
end;

function TApplication_GetHintHidePause(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TApplication(Obj).HintHidePause;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TApplication_SetHintHidePause(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TApplication(Obj).HintHidePause := Value;
  except
    ReportException;
  end;
end;

procedure TApplication_Minimize(Obj: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).Minimize;
  except
    ReportException;
  end;
end;

procedure TApplication_Restore(Obj: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).Restore;
  except
    ReportException;
  end;
end;

procedure TApplication_BringToFront(Obj: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).BringToFront;
  except
    ReportException;
  end;
end;

procedure TApplication_SetOnIdle(Obj: Pointer; Cb: TBethVarCallback; Data: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).OnIdle := @VarBridgeFor(TComponent(Obj), MethodData(TApplication(Obj).OnIdle), Cb, Data).DoCloseQuery;
  except
    ReportException;
  end;
end;

procedure TApplication_SetOnException(Obj: Pointer; Cb: TBethExceptionCallback; Data: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).OnException := @ExceptionBridgeFor(TComponent(Obj), MethodData(TApplication(Obj).OnException), Cb, Data).DoException;
  except
    ReportException;
  end;
end;

{ ---------------- StatusBar のパネル(docs/adr/0044) ----------------
  パネル(TStatusPanel。TCollectionItem)は THeaderSection と同じく、ハンドルを C 側へ渡すときに WatchItem で観察者を付け、
  破棄を知らせる。Panels(TStatusPanels)はステータスバーが所有し、ステータスバーと寿命が一致する。 }

type
  { OnDrawPanel(StatusBar, Panel, Rect)用。 }
  TBethItemRectCallback = procedure(Sender: Pointer; Item: Pointer; Left, Top, Right, Bottom: Integer; Data: Pointer); BETH_CALL;

  TItemRectCallbackBridge = class(TComponent)
  private
    FCallback: TBethItemRectCallback;
    FData: Pointer;
  public
    procedure DoDrawPanel(StatusBar: TStatusBar; Panel: TStatusPanel; const Rect: TRect);
  end;

procedure TItemRectCallbackBridge.DoDrawPanel(StatusBar: TStatusBar; Panel: TStatusPanel; const Rect: TRect);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(StatusBar), WatchItem(Panel), Rect.Left, Rect.Top, Rect.Right, Rect.Bottom, FData);
  CheckCallbackError;
end;

function ItemRectBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethItemRectCallback; Data: Pointer): TItemRectCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TItemRectCallbackBridge) and (TItemRectCallbackBridge(Current).Owner = Owner) then
    Result := TItemRectCallbackBridge(Current)
  else
    Result := TItemRectCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MethodData(const M: TDrawPanelEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function TStatusBar_GetPanels(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TStatusBar(Obj).Panels);
  except
    Result := nil;
    ReportException;
  end;
end;

function TStatusBar_GetSizeGrip(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TStatusBar(Obj).SizeGrip;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TStatusBar_SetSizeGrip(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TStatusBar(Obj).SizeGrip := Value;
  except
    ReportException;
  end;
end;

function TStatusBar_GetAutoHint(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TStatusBar(Obj).AutoHint;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TStatusBar_SetAutoHint(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TStatusBar(Obj).AutoHint := Value;
  except
    ReportException;
  end;
end;

function TStatusBar_GetCanvas(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TStatusBar(Obj).Canvas);
  except
    Result := nil;
    ReportException;
  end;
end;

function TStatusBar_GetPanelIndexAt(Obj: Pointer; X, Y: Integer): Integer; BETH_CALL;
begin
  try
    Result := TStatusBar(Obj).GetPanelIndexAt(X, Y);
  except
    Result := -1;
    ReportException;
  end;
end;

procedure TStatusBar_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TStatusBar(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TStatusBar_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TStatusBar(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

procedure TStatusBar_SetOnDrawPanel(Obj: Pointer; Cb: TBethItemRectCallback; Data: Pointer); BETH_CALL;
begin
  try
    TStatusBar(Obj).OnDrawPanel := @ItemRectBridgeFor(TComponent(Obj), MethodData(TStatusBar(Obj).OnDrawPanel), Cb, Data).DoDrawPanel;
  except
    ReportException;
  end;
end;

procedure TStatusBar_SetOnHint(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TStatusBar(Obj).OnHint := @BridgeFor(TComponent(Obj), MethodData(TStatusBar(Obj).OnHint), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TStatusPanels(TCollection)。パネルを返す関数は WatchItem してから返す。 }

function TStatusPanels_Add(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TStatusPanels(Obj).Add);
  except
    Result := nil;
    ReportException;
  end;
end;

function TStatusPanels_Insert(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TStatusPanels(Obj).Insert(Index));
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TStatusPanels_Delete(Obj: Pointer; Index: Integer); BETH_CALL;
begin
  try
    TStatusPanels(Obj).Delete(Index);
  except
    ReportException;
  end;
end;

procedure TStatusPanels_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TStatusPanels(Obj).Clear;
  except
    ReportException;
  end;
end;

function TStatusPanels_GetCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TStatusPanels(Obj).Count;
  except
    Result := 0;
    ReportException;
  end;
end;

function TStatusPanels_GetItem(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := WatchItem(TStatusPanels(Obj).Items[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TStatusPanels_BeginUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TStatusPanels(Obj).BeginUpdate;
  except
    ReportException;
  end;
end;

procedure TStatusPanels_EndUpdate(Obj: Pointer); BETH_CALL;
begin
  try
    TStatusPanels(Obj).EndUpdate;
  except
    ReportException;
  end;
end;

{ TStatusPanel }

function TStatusPanel_GetText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TStatusPanel(Obj).Text);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TStatusPanel_SetText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TStatusPanel(Obj).Text := Value;
  except
    ReportException;
  end;
end;

function TStatusPanel_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TStatusPanel(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TStatusPanel_SetWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TStatusPanel(Obj).Width := Value;
  except
    ReportException;
  end;
end;

function TStatusPanel_GetAlignment(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TStatusPanel(Obj).Alignment);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TStatusPanel_SetAlignment(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TStatusPanel(Obj).Alignment := TAlignment(Value);
  except
    ReportException;
  end;
end;

function TStatusPanel_GetBevel(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TStatusPanel(Obj).Bevel);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TStatusPanel_SetBevel(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TStatusPanel(Obj).Bevel := TStatusPanelBevel(Value);
  except
    ReportException;
  end;
end;

function TStatusPanel_GetStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TStatusPanel(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TStatusPanel_SetStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TStatusPanel(Obj).Style := TStatusPanelStyle(Value);
  except
    ReportException;
  end;
end;

function TStatusPanel_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TStatusPanel(Obj).Index;
  except
    Result := -1;
    ReportException;
  end;
end;

procedure TStatusPanel_SetIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TStatusPanel(Obj).Index := Value;
  except
    ReportException;
  end;
end;

{ ---------------- フォーム・パネルへの描画と Canvas の描画の関数(docs/adr/0045) ----------------
  TCustomControl の Canvas は public、OnPaint は protected(TForm・TPanel・TScrollBox 等が published にする)。
  Canvas はコントロールの生成時に作られ、コントロールと寿命が一致する。 }

type
  TCustomControlAccess = class(TCustomControl);
  TBethPoints = array of TPoint;

function TCustomControl_GetCanvas(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomControl(Obj).Canvas);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomControl_SetOnPaint(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomControlAccess(Obj).OnPaint := @BridgeFor(TComponent(Obj), MethodData(TCustomControlAccess(Obj).OnPaint), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ 点の配列: 呼び出し側は (X, Y) を並べた Integer の配列と点の数を渡す。 }
function PointsOf(Points: PInteger; Count: Integer): TBethPoints;
var
  I: Integer;
begin
  Result := nil;
  if Count <= 0 then
    Exit;
  SetLength(Result, Count);
  for I := 0 to Count - 1 do
  begin
    Result[I].X := Points[I * 2];
    Result[I].Y := Points[I * 2 + 1];
  end;
end;

function RectOf(X1, Y1, X2, Y2: Integer): TRect;
begin
  Result.Left := X1;
  Result.Top := Y1;
  Result.Right := X2;
  Result.Bottom := Y2;
end;

function TCanvas_TextWidth(Obj: Pointer; Text: PChar): Integer; BETH_CALL;
begin
  try
    Result := TCanvas(Obj).TextWidth(Text);
  except
    Result := 0;
    ReportException;
  end;
end;

function TCanvas_TextHeight(Obj: Pointer; Text: PChar): Integer; BETH_CALL;
begin
  try
    Result := TCanvas(Obj).TextHeight(Text);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCanvas_TextRect(Obj: Pointer; X1, Y1, X2, Y2, X, Y: Integer; Text: PChar); BETH_CALL;
begin
  try
    TCanvas(Obj).TextRect(RectOf(X1, Y1, X2, Y2), X, Y, Text);
  except
    ReportException;
  end;
end;

procedure TCanvas_Polygon(Obj: Pointer; Points: PInteger; Count: Integer); BETH_CALL;
begin
  try
    if Count > 0 then
      TCanvas(Obj).Polygon(PointsOf(Points, Count));
  except
    ReportException;
  end;
end;

procedure TCanvas_Polyline(Obj: Pointer; Points: PInteger; Count: Integer); BETH_CALL;
begin
  try
    if Count > 0 then
      TCanvas(Obj).Polyline(PointsOf(Points, Count));
  except
    ReportException;
  end;
end;

procedure TCanvas_RoundRect(Obj: Pointer; X1, Y1, X2, Y2, RX, RY: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).RoundRect(X1, Y1, X2, Y2, RX, RY);
  except
    ReportException;
  end;
end;

procedure TCanvas_Arc(Obj: Pointer; X1, Y1, X2, Y2, X3, Y3, X4, Y4: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).Arc(X1, Y1, X2, Y2, X3, Y3, X4, Y4);
  except
    ReportException;
  end;
end;

procedure TCanvas_Pie(Obj: Pointer; X1, Y1, X2, Y2, X3, Y3, X4, Y4: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).Pie(X1, Y1, X2, Y2, X3, Y3, X4, Y4);
  except
    ReportException;
  end;
end;

procedure TCanvas_Chord(Obj: Pointer; X1, Y1, X2, Y2, X3, Y3, X4, Y4: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).Chord(X1, Y1, X2, Y2, X3, Y3, X4, Y4);
  except
    ReportException;
  end;
end;

procedure TCanvas_FrameRect(Obj: Pointer; X1, Y1, X2, Y2: Integer); BETH_CALL;
begin
  try
    TCanvas(Obj).FrameRect(RectOf(X1, Y1, X2, Y2));
  except
    ReportException;
  end;
end;

procedure TCanvas_CopyRect(Obj: Pointer; X1, Y1, X2, Y2: Integer; Source: Pointer; SX1, SY1, SX2, SY2: Integer); BETH_CALL;
begin
  try
    if Source <> nil then
      TCanvas(Obj).CopyRect(RectOf(X1, Y1, X2, Y2), TCanvas(Source), RectOf(SX1, SY1, SX2, SY2));
  except
    ReportException;
  end;
end;

function TPen_GetStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TPen(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TPen_SetStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TPen(Obj).Style := TPenStyle(Value);
  except
    ReportException;
  end;
end;

function TPen_GetMode(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TPen(Obj).Mode);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TPen_SetMode(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TPen(Obj).Mode := TPenMode(Value);
  except
    ReportException;
  end;
end;

function TBrush_GetStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TBrush(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TBrush_SetStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TBrush(Obj).Style := TBrushStyle(Value);
  except
    ReportException;
  end;
end;

function TFont_GetHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFont(Obj).Height;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFont_SetHeight(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFont(Obj).Height := Value;
  except
    ReportException;
  end;
end;

function TFont_GetOrientation(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TFont(Obj).Orientation;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFont_SetOrientation(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFont(Obj).Orientation := Value;
  except
    ReportException;
  end;
end;

function TFont_GetQuality(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TFont(Obj).Quality);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TFont_SetQuality(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TFont(Obj).Quality := TFontQuality(Value);
  except
    ReportException;
  end;
end;

{ ---------------- Action(docs/adr/0046) ----------------
  TAction・TActionList は TComponent(Watch で破棄を知らせる)。TAction は ActionList に代入して一覧に入れる。 }

type
  { TActionList.OnExecute・OnUpdate(TActionEvent: Action と var Handled。Sender が無いので、ActionList を Sender として渡す)用。 }
  TActionEventBridge = class(TComponent)
  private
    FCallback: TBethItemAllowCallback;
    FData: Pointer;
  public
    procedure DoAction(AAction: TBasicAction; var Handled: Boolean);
  end;

procedure TActionEventBridge.DoAction(AAction: TBasicAction; var Handled: Boolean);
var
  H: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if Handled then H := -1 else H := 0;
  FCallback(Pointer(Owner), Pointer(AAction), @H, FData);
  Handled := H <> 0;
  CheckCallbackError;
end;

function ActionEventBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethItemAllowCallback; Data: Pointer): TActionEventBridge;
begin
  if (Current <> nil) and (TObject(Current) is TActionEventBridge) and (TActionEventBridge(Current).Owner = Owner) then
    Result := TActionEventBridge(Current)
  else
    Result := TActionEventBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MethodData(const M: TActionEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

{ TBasicAction }

function TBasicAction_Execute(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TBasicAction(Obj).Execute;
  except
    Result := False;
    ReportException;
  end;
end;

function TBasicAction_Update(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TBasicAction(Obj).Update;
  except
    Result := False;
    ReportException;
  end;
end;

function TBasicAction_GetActionComponent(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TBasicAction(Obj).ActionComponent);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TBasicAction_SetOnExecute(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TBasicAction(Obj).OnExecute := @BridgeFor(TComponent(Obj), MethodData(TBasicAction(Obj).OnExecute), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TBasicAction_SetOnUpdate(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TBasicAction(Obj).OnUpdate := @BridgeFor(TComponent(Obj), MethodData(TBasicAction(Obj).OnUpdate), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TContainedAction }

function TContainedAction_GetActionList(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TContainedAction(Obj).ActionList);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TContainedAction_SetActionList(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TContainedAction(Obj).ActionList := TCustomActionList(Value);
  except
    ReportException;
  end;
end;

function TContainedAction_GetCategory(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TContainedAction(Obj).Category);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TContainedAction_SetCategory(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TContainedAction(Obj).Category := Value;
  except
    ReportException;
  end;
end;

function TContainedAction_GetIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TContainedAction(Obj).Index;
  except
    Result := -1;
    ReportException;
  end;
end;

procedure TContainedAction_SetIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TContainedAction(Obj).Index := Value;
  except
    ReportException;
  end;
end;

{ TCustomAction }

function TCustomAction_GetCaption(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TCustomAction(Obj).Caption);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomAction_SetCaption(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TCustomAction(Obj).Caption := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetHint(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TCustomAction(Obj).Hint);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomAction_SetHint(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TCustomAction(Obj).Hint := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetChecked(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).Checked;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomAction_SetChecked(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomAction(Obj).Checked := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetAutoCheck(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).AutoCheck;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomAction_SetAutoCheck(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomAction(Obj).AutoCheck := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetEnabled(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).Enabled;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomAction_SetEnabled(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomAction(Obj).Enabled := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).Visible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomAction_SetVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomAction(Obj).Visible := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetDisableIfNoHandler(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).DisableIfNoHandler;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomAction_SetDisableIfNoHandler(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomAction(Obj).DisableIfNoHandler := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetGroupIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).GroupIndex;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomAction_SetGroupIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomAction(Obj).GroupIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetImageIndex(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).ImageIndex;
  except
    Result := -1;
    ReportException;
  end;
end;

procedure TCustomAction_SetImageIndex(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomAction(Obj).ImageIndex := Value;
  except
    ReportException;
  end;
end;

function TCustomAction_GetShortCut(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomAction(Obj).ShortCut;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomAction_SetShortCut(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomAction(Obj).ShortCut := TShortCut(Value);
  except
    ReportException;
  end;
end;

function TAction_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TAction.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ TCustomActionList }

function TCustomActionList_GetActionCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomActionList(Obj).ActionCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TCustomActionList_GetActions(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomActionList(Obj).Actions[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomActionList_GetImages(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomActionList(Obj).Images);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomActionList_SetImages(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomActionList(Obj).Images := TCustomImageList(Value);
  except
    ReportException;
  end;
end;

function TCustomActionList_GetState(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomActionList(Obj).State);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomActionList_SetState(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomActionList(Obj).State := TActionListState(Value);
  except
    ReportException;
  end;
end;

procedure TCustomActionList_SetOnExecute(Obj: Pointer; Cb: TBethItemAllowCallback; Data: Pointer); BETH_CALL;
begin
  try
    TActionList(Obj).OnExecute := @ActionEventBridgeFor(TComponent(Obj), MethodData(TActionList(Obj).OnExecute), Cb, Data).DoAction;
  except
    ReportException;
  end;
end;

procedure TCustomActionList_SetOnUpdate(Obj: Pointer; Cb: TBethItemAllowCallback; Data: Pointer); BETH_CALL;
begin
  try
    TActionList(Obj).OnUpdate := @ActionEventBridgeFor(TComponent(Obj), MethodData(TActionList(Obj).OnUpdate), Cb, Data).DoAction;
  except
    ReportException;
  end;
end;

function TActionList_Create(Owner: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Watch(TActionList.Create(TComponent(Owner)));
  except
    Result := nil;
    ReportException;
  end;
end;

{ コントロール・メニュー項目の Action }

function TControl_GetAction(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TControl(Obj).Action);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TControl_SetAction(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TControl(Obj).Action := TBasicAction(Value);
  except
    ReportException;
  end;
end;

function TMenuItem_GetAction(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TMenuItem(Obj).Action);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TMenuItem_SetAction(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TMenuItem(Obj).Action := TBasicAction(Value);
  except
    ReportException;
  end;
end;

{ ---------------- Screen・Clipboard・アイコン・ファイルのドロップ(docs/adr/0047) ----------------
  Screen(Forms.Screen)と Clipboard(Clipbrd.Clipboard)は LCL が持つもので、利用者は破棄しない。
  Windows ユニットの CF_TEXT 等(定数)と名前が重なるため、Clipbrd の関数は Clipbrd. で修飾する。 }

type
  { TCustomForm.OnDropFiles(TDropFilesEvent)用。ファイル名の数と UTF-8 の文字列の配列を渡す(呼び出しの間だけ有効)。 }
  TBethDropFilesCallback = procedure(Sender: Pointer; Count: Integer; FileNames: PPChar; Data: Pointer); BETH_CALL;

  TDropFilesBridge = class(TComponent)
  private
    FCallback: TBethDropFilesCallback;
    FData: Pointer;
  public
    procedure DoDropFiles(Sender: TObject; const FileNames: array of AnsiString);
  end;

procedure TDropFilesBridge.DoDropFiles(Sender: TObject; const FileNames: array of AnsiString);
var
  Names: array of PChar;
  I: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  Names := nil;
  SetLength(Names, Length(FileNames) + 1);
  for I := 0 to High(FileNames) do
    Names[I] := PChar(FileNames[I]);
  Names[Length(FileNames)] := nil;
  FCallback(Pointer(Sender), Length(FileNames), @Names[0], FData);
  CheckCallbackError;
end;

function DropFilesBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethDropFilesCallback; Data: Pointer): TDropFilesBridge;
begin
  if (Current <> nil) and (TObject(Current) is TDropFilesBridge) and (TDropFilesBridge(Current).Owner = Owner) then
    Result := TDropFilesBridge(Current)
  else
    Result := TDropFilesBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MethodData(const M: TDropFilesEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

{ TScreen }

function GetScreen: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(Screen);
  except
    Result := nil;
    ReportException;
  end;
end;

function TScreen_GetCursor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).Cursor;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TScreen_SetCursor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TScreen(Obj).Cursor := Value;
  except
    ReportException;
  end;
end;

function TScreen_GetWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).Width;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).Height;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetDesktopLeft(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).DesktopLeft;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetDesktopTop(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).DesktopTop;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetDesktopWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).DesktopWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetDesktopHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).DesktopHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetWorkAreaLeft(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).WorkAreaLeft;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetWorkAreaTop(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).WorkAreaTop;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetWorkAreaWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).WorkAreaWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetWorkAreaHeight(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).WorkAreaHeight;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetPixelsPerInch(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).PixelsPerInch;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetMonitorCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).MonitorCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TScreen_GetFormCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TScreen(Obj).FormCount;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TScreen_GetWorkAreaRect(Obj: Pointer; Left, Top, Right, Bottom: PInteger); BETH_CALL;
var
  R: TRect;
begin
  try
    R := TScreen(Obj).WorkAreaRect;
    Left^ := R.Left;
    Top^ := R.Top;
    Right^ := R.Right;
    Bottom^ := R.Bottom;
  except
    ReportException;
  end;
end;

function TScreen_GetForms(Obj: Pointer; Index: Integer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TScreen(Obj).Forms[Index]);
  except
    Result := nil;
    ReportException;
  end;
end;

function TScreen_GetActiveForm(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TScreen(Obj).ActiveForm);
  except
    Result := nil;
    ReportException;
  end;
end;

function TScreen_GetActiveControl(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TScreen(Obj).ActiveControl);
  except
    Result := nil;
    ReportException;
  end;
end;

function TScreen_GetFonts(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TScreen(Obj).Fonts);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TScreen_SetOnActiveFormChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TScreen(Obj).OnActiveFormChange := @BridgeFor(TComponent(Obj), MethodData(TScreen(Obj).OnActiveFormChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

procedure TScreen_SetOnActiveControlChange(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TScreen(Obj).OnActiveControlChange := @BridgeFor(TComponent(Obj), MethodData(TScreen(Obj).OnActiveControlChange), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

{ TClipboard }

function GetClipboard: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(Clipbrd.Clipboard);
  except
    Result := nil;
    ReportException;
  end;
end;

function Clipboard_CF_Text: Cardinal; BETH_CALL;
begin
  try
    Result := Cardinal(Clipbrd.CF_Text);
  except
    Result := 0;
    ReportException;
  end;
end;

function Clipboard_CF_Bitmap: Cardinal; BETH_CALL;
begin
  try
    Result := Cardinal(Clipbrd.CF_Bitmap);
  except
    Result := 0;
    ReportException;
  end;
end;

function Clipboard_CF_Picture: Cardinal; BETH_CALL;
begin
  try
    Result := Cardinal(Clipbrd.CF_Picture);
  except
    Result := 0;
    ReportException;
  end;
end;

function TClipboard_GetAsText(Obj: Pointer): PChar; BETH_CALL;
begin
  try
    Result := ReturnStr(TClipboard(Obj).AsText);
  except
    Result := '';
    ReportException;
  end;
end;

procedure TClipboard_SetAsText(Obj: Pointer; Value: PChar); BETH_CALL;
begin
  try
    TClipboard(Obj).AsText := AnsiString(Value);
  except
    ReportException;
  end;
end;

function TClipboard_HasFormat(Obj: Pointer; Format: Cardinal): LongBool; BETH_CALL;
begin
  try
    Result := TClipboard(Obj).HasFormat(PtrUInt(Format));
  except
    Result := False;
    ReportException;
  end;
end;

function TClipboard_HasPictureFormat(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TClipboard(Obj).HasPictureFormat;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TClipboard_Clear(Obj: Pointer); BETH_CALL;
begin
  try
    TClipboard(Obj).Clear;
  except
    ReportException;
  end;
end;

procedure TClipboard_Open(Obj: Pointer); BETH_CALL;
begin
  try
    TClipboard(Obj).Open;
  except
    ReportException;
  end;
end;

procedure TClipboard_Close(Obj: Pointer); BETH_CALL;
begin
  try
    TClipboard(Obj).Close;
  except
    ReportException;
  end;
end;

function TClipboard_GetFormatCount(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TClipboard(Obj).FormatCount;
  except
    Result := 0;
    ReportException;
  end;
end;

function TClipboard_GetFormats(Obj: Pointer; Index: Integer): Cardinal; BETH_CALL;
begin
  try
    Result := Cardinal(TClipboard(Obj).Formats[Index]);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TClipboard_Assign(Obj: Pointer; Source: Pointer); BETH_CALL;
begin
  try
    if Source <> nil then
      TClipboard(Obj).Assign(TPersistent(Source));
  except
    ReportException;
  end;
end;

{ TIcon }

function TIcon_Create: Pointer; BETH_CALL;
begin
  try
    Result := Pointer(Graphics.TIcon.Create);
  except
    Result := nil;
    ReportException;
  end;
end;

function TCustomForm_GetIcon(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TCustomForm(Obj).Icon);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TCustomForm_SetIcon(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).Icon := Graphics.TIcon(Value);
  except
    ReportException;
  end;
end;

function TApplication_GetIcon(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TApplication(Obj).Icon);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TApplication_SetIcon(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TApplication(Obj).Icon := Graphics.TIcon(Value);
  except
    ReportException;
  end;
end;

function TPicture_GetIcon(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TPicture(Obj).Icon);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TPicture_SetIcon(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    TPicture(Obj).Icon := Graphics.TIcon(Value);
  except
    ReportException;
  end;
end;

{ TCustomForm のファイルのドロップ }

function TCustomForm_GetAllowDropFiles(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomForm(Obj).AllowDropFiles;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomForm_SetAllowDropFiles(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomForm(Obj).AllowDropFiles := Value;
  except
    ReportException;
  end;
end;

procedure TCustomForm_SetOnDropFiles(Obj: Pointer; Cb: TBethDropFilesCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomForm(Obj).OnDropFiles := @DropFilesBridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnDropFiles), Cb, Data).DoDropFiles;
  except
    ReportException;
  end;
end;

{ ---------------- パネルの縁・スクロール・コントロールの枠(docs/adr/0048) ----------------
  列挙型は Ord の整数で受け渡す。BorderStyle は TWinControl の仮想の SetBorderStyle で設定する(グリッドは自分の欄に持つため、
  読むときだけ TCustomGrid の BorderStyle を使う)。 }

type
  TCustomGridAccess048 = class(TCustomGrid);
  TCustomTreeViewAccess048 = class(TCustomTreeView);
  TCustomListViewAccess048 = class(TCustomListView);
  TScrollingWinControlAccess048 = class(TScrollingWinControl);
  TCustomPanelAccess048 = class(TCustomPanel);

{ TWinControl の BorderStyle・BorderWidth }

function TWinControl_GetBorderStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    if TObject(Obj) is TCustomGrid then
      Result := Ord(TCustomGridAccess048(Obj).BorderStyle)
    else
      Result := Ord(TWinControlAccess(Obj).BorderStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TWinControl_SetBorderStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TWinControlAccess(Obj).BorderStyle := TBorderStyle(Value);
  except
    ReportException;
  end;
end;

function TWinControl_GetBorderWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TWinControlAccess(Obj).BorderWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TWinControl_SetBorderWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TWinControlAccess(Obj).BorderWidth := Value;
  except
    ReportException;
  end;
end;

{ TCustomPanel }

function TCustomPanel_GetAlignment(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomPanelAccess048(Obj).Alignment);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPanel_SetAlignment(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPanelAccess048(Obj).Alignment := TAlignment(Value);
  except
    ReportException;
  end;
end;

function TCustomPanel_GetVerticalAlignment(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomPanelAccess048(Obj).VerticalAlignment);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPanel_SetVerticalAlignment(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPanelAccess048(Obj).VerticalAlignment := TVerticalAlignment(Value);
  except
    ReportException;
  end;
end;

function TCustomPanel_GetWordWrap(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomPanelAccess048(Obj).WordWrap;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomPanel_SetWordWrap(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomPanelAccess048(Obj).WordWrap := Value;
  except
    ReportException;
  end;
end;

function TCustomPanel_GetBevelColor(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomPanelAccess048(Obj).BevelColor;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPanel_SetBevelColor(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPanelAccess048(Obj).BevelColor := TColor(Value);
  except
    ReportException;
  end;
end;

function TCustomPanel_GetBevelInner(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomPanelAccess048(Obj).BevelInner);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPanel_SetBevelInner(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPanelAccess048(Obj).BevelInner := TPanelBevel(Value);
  except
    ReportException;
  end;
end;

function TCustomPanel_GetBevelOuter(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomPanelAccess048(Obj).BevelOuter);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPanel_SetBevelOuter(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPanelAccess048(Obj).BevelOuter := TPanelBevel(Value);
  except
    ReportException;
  end;
end;

function TCustomPanel_GetBevelWidth(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomPanelAccess048(Obj).BevelWidth;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomPanel_SetBevelWidth(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomPanelAccess048(Obj).BevelWidth := Value;
  except
    ReportException;
  end;
end;

{ ScrollBars }

function TCustomDrawGrid_GetScrollBars(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomGridAccess048(Obj).ScrollBars);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomDrawGrid_SetScrollBars(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomGridAccess048(Obj).ScrollBars := TScrollStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomTreeView_GetScrollBars(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomTreeViewAccess048(Obj).ScrollBars);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTreeView_SetScrollBars(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTreeViewAccess048(Obj).ScrollBars := TScrollStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomListView_GetScrollBars(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomListViewAccess048(Obj).ScrollBars);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomListView_SetScrollBars(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomListViewAccess048(Obj).ScrollBars := TScrollStyle(Value);
  except
    ReportException;
  end;
end;

{ TControlScrollBar }

function TControlScrollBar_GetKind(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TControlScrollBar(Obj).Kind);
  except
    Result := 0;
    ReportException;
  end;
end;

function TControlScrollBar_GetSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Size;
  except
    Result := 0;
    ReportException;
  end;
end;

function TControlScrollBar_IsScrollBarVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).IsScrollBarVisible;
  except
    Result := False;
    ReportException;
  end;
end;

function TControlScrollBar_GetIncrement(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Increment;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlScrollBar_SetIncrement(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlScrollBar(Obj).Increment := Value;
  except
    ReportException;
  end;
end;

function TControlScrollBar_GetPage(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Page;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlScrollBar_SetPage(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlScrollBar(Obj).Page := Value;
  except
    ReportException;
  end;
end;

function TControlScrollBar_GetPosition(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Position;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlScrollBar_SetPosition(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlScrollBar(Obj).Position := Value;
  except
    ReportException;
  end;
end;

function TControlScrollBar_GetRange(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Range;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TControlScrollBar_SetRange(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TControlScrollBar(Obj).Range := Value;
  except
    ReportException;
  end;
end;

function TControlScrollBar_GetSmooth(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Smooth;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControlScrollBar_SetSmooth(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControlScrollBar(Obj).Smooth := Value;
  except
    ReportException;
  end;
end;

function TControlScrollBar_GetTracking(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Tracking;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControlScrollBar_SetTracking(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControlScrollBar(Obj).Tracking := Value;
  except
    ReportException;
  end;
end;

function TControlScrollBar_GetVisible(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TControlScrollBar(Obj).Visible;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TControlScrollBar_SetVisible(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TControlScrollBar(Obj).Visible := Value;
  except
    ReportException;
  end;
end;

{ TScrollingWinControl }

function TScrollingWinControl_GetAutoScroll(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TScrollingWinControlAccess048(Obj).AutoScroll;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TScrollingWinControl_SetAutoScroll(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TScrollingWinControlAccess048(Obj).AutoScroll := Value;
  except
    ReportException;
  end;
end;

function TScrollingWinControl_GetHorzScrollBar(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TScrollingWinControlAccess048(Obj).HorzScrollBar);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TScrollingWinControl_SetHorzScrollBar(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    if Value <> nil then
      TScrollingWinControlAccess048(Obj).HorzScrollBar := TControlScrollBar(Value);
  except
    ReportException;
  end;
end;

function TScrollingWinControl_GetVertScrollBar(Obj: Pointer): Pointer; BETH_CALL;
begin
  try
    Result := Pointer(TScrollingWinControlAccess048(Obj).VertScrollBar);
  except
    Result := nil;
    ReportException;
  end;
end;

procedure TScrollingWinControl_SetVertScrollBar(Obj: Pointer; Value: Pointer); BETH_CALL;
begin
  try
    if Value <> nil then
      TScrollingWinControlAccess048(Obj).VertScrollBar := TControlScrollBar(Value);
  except
    ReportException;
  end;
end;

{ ---------------- 範囲のコントロール・グループの列(docs/adr/0049) ---------------- }

type
  { TScrollBar.OnScroll(TScrollEvent)用。操作の種類と、つまみの位置(var)を渡す。 }
  TBethScrollCallback = procedure(Sender: Pointer; ScrollCode: Integer; ScrollPos: PInteger; Data: Pointer); BETH_CALL;
  { TCheckGroup.OnItemClick(TCheckGroupClicked)用。 }
  TBethIntCallback = procedure(Sender: Pointer; Value: Integer; Data: Pointer); BETH_CALL;

  TScrollEventBridge = class(TComponent)
  private
    FCallback: TBethScrollCallback;
    FData: Pointer;
  public
    procedure DoScroll(Sender: TObject; ScrollCode: TScrollCode; var ScrollPos: Integer);
  end;

  TIntEventBridge = class(TComponent)
  private
    FCallback: TBethIntCallback;
    FData: Pointer;
  public
    procedure DoInt(Sender: TObject; Index: Integer);
  end;

procedure TScrollEventBridge.DoScroll(Sender: TObject; ScrollCode: TScrollCode; var ScrollPos: Integer);
var
  P: Integer;
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  P := ScrollPos;
  FCallback(Pointer(Sender), Ord(ScrollCode), @P, FData);
  ScrollPos := P;
  CheckCallbackError;
end;

procedure TIntEventBridge.DoInt(Sender: TObject; Index: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), Index, FData);
  CheckCallbackError;
end;

function ScrollEventBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethScrollCallback; Data: Pointer): TScrollEventBridge;
begin
  if (Current <> nil) and (TObject(Current) is TScrollEventBridge) and (TScrollEventBridge(Current).Owner = Owner) then
    Result := TScrollEventBridge(Current)
  else
    Result := TScrollEventBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function IntEventBridgeFor(Owner: TComponent; Current: Pointer; Cb: TBethIntCallback; Data: Pointer): TIntEventBridge;
begin
  if (Current <> nil) and (TObject(Current) is TIntEventBridge) and (TIntEventBridge(Current).Owner = Owner) then
    Result := TIntEventBridge(Current)
  else
    Result := TIntEventBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MethodData(const M: TScrollEvent): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function MethodData(const M: TCheckGroupClicked): Pointer; overload;
begin
  Result := TMethod(M).Data;
end;

function TCustomTrackBar_GetOrientation(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomTrackBar(Obj).Orientation);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetOrientation(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).Orientation := TTrackBarOrientation(Value);
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetFrequency(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).Frequency;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetFrequency(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).Frequency := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetTickMarks(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomTrackBar(Obj).TickMarks);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetTickMarks(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).TickMarks := TTickMark(Value);
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetTickStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomTrackBar(Obj).TickStyle);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetTickStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).TickStyle := TTickStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetLineSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).LineSize;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetLineSize(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).LineSize := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetPageSize(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).PageSize;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetPageSize(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).PageSize := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetSelStart(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).SelStart;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetSelStart(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).SelStart := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetSelEnd(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).SelEnd;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetSelEnd(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).SelEnd := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetShowSelRange(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).ShowSelRange;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetShowSelRange(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).ShowSelRange := Value;
  except
    ReportException;
  end;
end;

function TCustomTrackBar_GetReversed(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomTrackBar(Obj).Reversed;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomTrackBar_SetReversed(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomTrackBar(Obj).Reversed := Value;
  except
    ReportException;
  end;
end;

function TCustomProgressBar_GetOrientation(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomProgressBar(Obj).Orientation);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetOrientation(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).Orientation := TProgressBarOrientation(Value);
  except
    ReportException;
  end;
end;

function TCustomProgressBar_GetSmooth(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomProgressBar(Obj).Smooth;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetSmooth(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).Smooth := Value;
  except
    ReportException;
  end;
end;

function TCustomProgressBar_GetStep(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomProgressBar(Obj).Step;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetStep(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).Step := Value;
  except
    ReportException;
  end;
end;

function TCustomProgressBar_GetStyle(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomProgressBar(Obj).Style);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetStyle(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).Style := TProgressBarStyle(Value);
  except
    ReportException;
  end;
end;

function TCustomProgressBar_GetBarShowText(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomProgressBar(Obj).BarShowText;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomProgressBar_SetBarShowText(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).BarShowText := Value;
  except
    ReportException;
  end;
end;

function TCustomScrollBar_GetLargeChange(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomScrollBar(Obj).LargeChange;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetLargeChange(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).LargeChange := Value;
  except
    ReportException;
  end;
end;

function TCustomScrollBar_GetSmallChange(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomScrollBar(Obj).SmallChange;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetSmallChange(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).SmallChange := Value;
  except
    ReportException;
  end;
end;

function TUpDown_GetOrientation(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TUpDown(Obj).Orientation);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TUpDown_SetOrientation(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TUpDown(Obj).Orientation := TUDOrientation(Value);
  except
    ReportException;
  end;
end;

function TUpDown_GetAlignButton(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TUpDown(Obj).AlignButton);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TUpDown_SetAlignButton(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TUpDown(Obj).AlignButton := TUDAlignButton(Value);
  except
    ReportException;
  end;
end;

function TUpDown_GetWrap(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TUpDown(Obj).Wrap;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TUpDown_SetWrap(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TUpDown(Obj).Wrap := Value;
  except
    ReportException;
  end;
end;

function TUpDown_GetArrowKeys(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TUpDown(Obj).ArrowKeys;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TUpDown_SetArrowKeys(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TUpDown(Obj).ArrowKeys := Value;
  except
    ReportException;
  end;
end;

function TUpDown_GetThousands(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TUpDown(Obj).Thousands;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TUpDown_SetThousands(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TUpDown(Obj).Thousands := Value;
  except
    ReportException;
  end;
end;

function TCustomRadioGroup_GetColumns(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomRadioGroup(Obj).Columns;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomRadioGroup_SetColumns(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomRadioGroup(Obj).Columns := Value;
  except
    ReportException;
  end;
end;

function TCustomRadioGroup_GetColumnLayout(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomRadioGroup(Obj).ColumnLayout);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomRadioGroup_SetColumnLayout(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomRadioGroup(Obj).ColumnLayout := TColumnLayout(Value);
  except
    ReportException;
  end;
end;

function TCustomRadioGroup_GetAutoFill(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomRadioGroup(Obj).AutoFill;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomRadioGroup_SetAutoFill(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomRadioGroup(Obj).AutoFill := Value;
  except
    ReportException;
  end;
end;

function TCustomCheckGroup_GetColumns(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := TCustomCheckGroup(Obj).Columns;
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomCheckGroup_SetColumns(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomCheckGroup(Obj).Columns := Value;
  except
    ReportException;
  end;
end;

function TCustomCheckGroup_GetColumnLayout(Obj: Pointer): Integer; BETH_CALL;
begin
  try
    Result := Ord(TCustomCheckGroup(Obj).ColumnLayout);
  except
    Result := 0;
    ReportException;
  end;
end;

procedure TCustomCheckGroup_SetColumnLayout(Obj: Pointer; Value: Integer); BETH_CALL;
begin
  try
    TCustomCheckGroup(Obj).ColumnLayout := TColumnLayout(Value);
  except
    ReportException;
  end;
end;

function TCustomCheckGroup_GetAutoFill(Obj: Pointer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCheckGroup(Obj).AutoFill;
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCheckGroup_SetAutoFill(Obj: Pointer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCheckGroup(Obj).AutoFill := Value;
  except
    ReportException;
  end;
end;

procedure TCustomProgressBar_StepIt(Obj: Pointer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).StepIt;
  except
    ReportException;
  end;
end;

procedure TCustomProgressBar_StepBy(Obj: Pointer; Delta: Integer); BETH_CALL;
begin
  try
    TCustomProgressBar(Obj).StepBy(Delta);
  except
    ReportException;
  end;
end;

procedure TCustomScrollBar_SetOnScroll(Obj: Pointer; Cb: TBethScrollCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomScrollBar(Obj).OnScroll := @ScrollEventBridgeFor(TComponent(Obj), MethodData(TCustomScrollBar(Obj).OnScroll), Cb, Data).DoScroll;
  except
    ReportException;
  end;
end;

procedure TCustomRadioGroup_SetOnSelectionChanged(Obj: Pointer; Cb: TBethCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomRadioGroup(Obj).OnSelectionChanged := @BridgeFor(TComponent(Obj), MethodData(TCustomRadioGroup(Obj).OnSelectionChanged), Cb, Data).DoClick;
  except
    ReportException;
  end;
end;

function TCustomCheckGroup_GetCheckEnabled(Obj: Pointer; Index: Integer): LongBool; BETH_CALL;
begin
  try
    Result := TCustomCheckGroup(Obj).CheckEnabled[Index];
  except
    Result := False;
    ReportException;
  end;
end;

procedure TCustomCheckGroup_SetCheckEnabled(Obj: Pointer; Index: Integer; Value: LongBool); BETH_CALL;
begin
  try
    TCustomCheckGroup(Obj).CheckEnabled[Index] := Value;
  except
    ReportException;
  end;
end;

procedure TCustomCheckGroup_SetOnItemClick(Obj: Pointer; Cb: TBethIntCallback; Data: Pointer); BETH_CALL;
begin
  try
    TCustomCheckGroup(Obj).OnItemClick := @IntEventBridgeFor(TComponent(Obj), MethodData(TCustomCheckGroup(Obj).OnItemClick), Cb, Data).DoInt;
  except
    ReportException;
  end;
end;

exports
  FreeNotify_SetCallback,
  Error_SetCallback,
  SetCallbackError,

  TComponent_Destroy,
  TComponent_DestroyComponents,

  TControl_GetParent,
  TControl_SetParent,
  TControl_GetLeft,
  TControl_SetLeft,
  TControl_GetTop,
  TControl_SetTop,
  TControl_GetWidth,
  TControl_SetWidth,
  TControl_GetHeight,
  TControl_SetHeight,
  TControl_GetVisible,
  TControl_SetVisible,
  TControl_GetEnabled,
  TControl_SetEnabled,
  TControl_GetCaption,
  TControl_SetCaption,
  TControl_GetAlign,
  TControl_SetAlign,
  TControl_GetText,
  TControl_SetText,
  TControl_Show,
  TControl_Hide,
  TControl_SetOnClick,
  TControl_SetOnDblClick,
  TControl_SetOnResize,
  TControl_SetOnMouseDown,
  TControl_SetOnMouseUp,
  TControl_SetOnMouseMove,
  TControl_SetOnMouseEnter,
  TControl_SetOnMouseLeave,
  TControl_SetOnMouseWheel,

  TWinControl_SetOnKeyDown,
  TWinControl_SetOnKeyUp,
  TWinControl_SetOnKeyPress,

  TForm_Create,
  TCustomForm_Show,
  TCustomForm_Hide,
  TCustomForm_ShowModal,
  TCustomForm_Close,
  TCustomForm_Release,
  TCustomForm_SetOnClose,
  TCustomForm_SetOnCloseQuery,
  TCustomForm_SetOnShow,
  TCustomForm_SetOnHide,
  TCustomForm_SetOnActivate,
  TCustomForm_SetOnDeactivate,
  TCustomForm_SetOnDestroy,

  GetApplication,
  TApplication_CreateForm,
  TApplication_GetMainForm,
  TApplication_Run,
  TApplication_ProcessMessages,
  TApplication_Terminate,
  TApplication_GetTerminated,
  TApplication_GetTitle,
  TApplication_SetTitle,
  TApplication_GetShowMainForm,
  TApplication_SetShowMainForm,

  TPanel_Create,
  TGroupBox_Create,
  TLabel_Create,

  TButtonControl_GetChecked,
  TButtonControl_SetChecked,
  TButton_Create,
  TCheckBox_Create,
  TRadioButton_Create,

  TCustomEdit_GetMaxLength,
  TCustomEdit_SetMaxLength,
  TCustomEdit_GetReadOnly,
  TCustomEdit_SetReadOnly,
  TCustomEdit_SetOnChange,
  TEdit_Create,

  TCustomMemo_GetScrollBars,
  TCustomMemo_SetScrollBars,
  TMemo_Create,

  TCustomComboBox_GetItemIndex,
  TCustomComboBox_SetItemIndex,
  TComboBox_Create,
  TComboBox_SetOnChange,

  TCustomListBox_GetItemIndex,
  TCustomListBox_SetItemIndex,
  TListBox_Create,

  TCustomTimer_GetInterval,
  TCustomTimer_SetInterval,
  TCustomTimer_GetEnabled,
  TCustomTimer_SetEnabled,
  TCustomTimer_SetOnTimer,
  TTimer_Create,

  TPaintBox_Create,
  TPaintBox_GetCanvas,
  TPaintBox_SetOnPaint,

  TCanvas_MoveTo,
  TCanvas_LineTo,
  TCanvas_Rectangle,
  TCanvas_Ellipse,
  TCanvas_TextOut,
  TCanvas_GetPen,
  TCanvas_GetBrush,
  TCanvas_GetFont,

  TPen_GetColor,
  TPen_SetColor,
  TPen_GetWidth,
  TPen_SetWidth,
  TBrush_GetColor,
  TBrush_SetColor,
  TFont_GetName,
  TFont_SetName,
  TFont_GetSize,
  TFont_SetSize,
  TFont_GetColor,
  TFont_SetColor,

  TScrollBox_Create,
  TToggleBox_Create,

  TBevel_Create,
  TBevel_GetShape,
  TBevel_SetShape,
  TBevel_GetStyle,
  TBevel_SetStyle,

  TShape_Create,
  TCustomShape_GetShape,
  TCustomShape_SetShape,
  TCustomShape_GetPen,
  TCustomShape_GetBrush,

  TStaticText_Create,
  TCustomStaticText_GetBorderStyle,
  TCustomStaticText_SetBorderStyle,

  TStatusBar_Create,
  TStatusBar_GetSimpleText,
  TStatusBar_SetSimpleText,
  TStatusBar_GetSimplePanel,
  TStatusBar_SetSimplePanel,

  TScrollBar_Create,
  TCustomScrollBar_GetKind,
  TCustomScrollBar_SetKind,
  TCustomScrollBar_GetMin,
  TCustomScrollBar_SetMin,
  TCustomScrollBar_GetMax,
  TCustomScrollBar_SetMax,
  TCustomScrollBar_GetPosition,
  TCustomScrollBar_SetPosition,
  TCustomScrollBar_GetPageSize,
  TCustomScrollBar_SetPageSize,
  TCustomScrollBar_SetOnChange,

  TTrackBar_Create,
  TCustomTrackBar_GetMin,
  TCustomTrackBar_SetMin,
  TCustomTrackBar_GetMax,
  TCustomTrackBar_SetMax,
  TCustomTrackBar_GetPosition,
  TCustomTrackBar_SetPosition,
  TCustomTrackBar_SetOnChange,

  TProgressBar_Create,
  TCustomProgressBar_GetMin,
  TCustomProgressBar_SetMin,
  TCustomProgressBar_GetMax,
  TCustomProgressBar_SetMax,
  TCustomProgressBar_GetPosition,
  TCustomProgressBar_SetPosition,

  TUpDown_Create,
  TUpDown_GetMin,
  TUpDown_SetMin,
  TUpDown_GetMax,
  TUpDown_SetMax,
  TUpDown_GetPosition,
  TUpDown_SetPosition,
  TUpDown_GetIncrement,
  TUpDown_SetIncrement,
  TUpDown_GetAssociate,
  TUpDown_SetAssociate,

  TRadioGroup_Create,
  TCustomRadioGroup_GetItemIndex,
  TCustomRadioGroup_SetItemIndex,
  TCustomRadioGroup_SetOnClick,

  TCheckGroup_Create,
  TCustomCheckGroup_GetChecked,
  TCustomCheckGroup_SetChecked,

  TCheckListBox_Create,
  TCustomCheckListBox_GetChecked,
  TCustomCheckListBox_SetChecked,
  TCustomCheckListBox_SetOnClickCheck,

  TSpeedButton_Create,
  TCustomSpeedButton_GetDown,
  TCustomSpeedButton_SetDown,
  TCustomSpeedButton_GetGroupIndex,
  TCustomSpeedButton_SetGroupIndex,
  TCustomSpeedButton_GetFlat,
  TCustomSpeedButton_SetFlat,
  TCustomSpeedButton_GetAllowAllUp,
  TCustomSpeedButton_SetAllowAllUp,

  TBitBtn_Create,
  TCustomBitBtn_GetKind,
  TCustomBitBtn_SetKind,

  TFloatSpinEdit_Create,
  TCustomFloatSpinEdit_GetValue,
  TCustomFloatSpinEdit_SetValue,
  TCustomFloatSpinEdit_GetMinValue,
  TCustomFloatSpinEdit_SetMinValue,
  TCustomFloatSpinEdit_GetMaxValue,
  TCustomFloatSpinEdit_SetMaxValue,
  TCustomFloatSpinEdit_GetIncrement,
  TCustomFloatSpinEdit_SetIncrement,
  TCustomFloatSpinEdit_GetDecimalPlaces,
  TCustomFloatSpinEdit_SetDecimalPlaces,

  TSpinEdit_Create,
  TCustomSpinEdit_GetValue,
  TCustomSpinEdit_SetValue,
  TCustomSpinEdit_GetMinValue,
  TCustomSpinEdit_SetMinValue,
  TCustomSpinEdit_GetMaxValue,
  TCustomSpinEdit_SetMaxValue,
  TCustomSpinEdit_GetIncrement,
  TCustomSpinEdit_SetIncrement,

  TMaskEdit_Create,
  TMaskEdit_GetEditMask,
  TMaskEdit_SetEditMask,

  TLabeledEdit_Create,
  TCustomLabeledEdit_GetEditLabel,
  TCustomLabeledEdit_GetLabelPosition,
  TCustomLabeledEdit_SetLabelPosition,
  TCustomLabeledEdit_GetLabelSpacing,
  TCustomLabeledEdit_SetLabelSpacing,

  TTabControl_Create,
  TTabControl_GetTabIndex,
  TTabControl_SetTabIndex,
  TTabControl_SetOnChange,

  TSplitter_Create,
  TCustomSplitter_GetAutoSnap,
  TCustomSplitter_SetAutoSnap,
  TCustomSplitter_GetBeveled,
  TCustomSplitter_SetBeveled,
  TCustomSplitter_GetMinSize,
  TCustomSplitter_SetMinSize,
  TCustomSplitter_GetResizeAnchor,
  TCustomSplitter_SetResizeAnchor,
  TCustomSplitter_GetResizeStyle,
  TCustomSplitter_SetResizeStyle,
  TCustomSplitter_GetSplitterPosition,
  TCustomSplitter_SetSplitterPosition,
  TCustomSplitter_SetOnMoved,

  ShortCut_Make,
  ShortCut_FromText,
  ShortCut_ToText,

  TMenuItem_Create,
  TMenuItem_GetCaption,
  TMenuItem_SetCaption,
  TMenuItem_GetChecked,
  TMenuItem_SetChecked,
  TMenuItem_GetEnabled,
  TMenuItem_SetEnabled,
  TMenuItem_GetVisible,
  TMenuItem_SetVisible,
  TMenuItem_GetAutoCheck,
  TMenuItem_SetAutoCheck,
  TMenuItem_GetRadioItem,
  TMenuItem_SetRadioItem,
  TMenuItem_GetGroupIndex,
  TMenuItem_SetGroupIndex,
  TMenuItem_GetDefault,
  TMenuItem_SetDefault,
  TMenuItem_GetShortCut,
  TMenuItem_SetShortCut,
  TMenuItem_GetHint,
  TMenuItem_SetHint,
  TMenuItem_SetOnClick,
  TMenuItem_GetCount,
  TMenuItem_GetItem,
  TMenuItem_GetParent,
  TMenuItem_Add,
  TMenuItem_Insert,
  TMenuItem_Delete,
  TMenuItem_Remove,
  TMenuItem_Clear,
  TMenuItem_IndexOf,
  TMenuItem_AddSeparator,
  TMenuItem_IsLine,
  TMenuItem_Click,

  TMenu_GetItems,
  TMainMenu_Create,
  TPopupMenu_Create,
  TPopupMenu_Popup,
  TPopupMenu_GetAutoPopup,
  TPopupMenu_SetAutoPopup,
  TPopupMenu_GetPopupComponent,
  TPopupMenu_SetPopupComponent,
  TPopupMenu_SetOnPopup,
  TPopupMenu_SetOnClose,

  TCustomForm_GetMenu,
  TCustomForm_SetMenu,
  TControl_GetPopupMenu,
  TControl_SetPopupMenu,

  TPageControl_Create,
  TPageControl_GetActivePage,
  TPageControl_SetActivePage,
  TPageControl_GetActivePageIndex,
  TPageControl_SetActivePageIndex,
  TPageControl_GetPage,
  TCustomTabControl_GetPageCount,
  TPageControl_AddTabSheet,
  TPageControl_Clear,
  TPageControl_SelectNextPage,
  TPageControl_GetTabIndex,
  TPageControl_SetTabIndex,
  TPageControl_SetOnChange,
  TCustomTabControl_SetOnChanging,
  TCustomTabControl_GetMultiLine,
  TCustomTabControl_SetMultiLine,
  TCustomTabControl_GetShowTabs,
  TCustomTabControl_SetShowTabs,
  TCustomTabControl_GetTabPosition,
  TCustomTabControl_SetTabPosition,

  TTabSheet_Create,
  TTabSheet_GetPageControl,
  TTabSheet_SetPageControl,
  TTabSheet_GetTabIndex,
  TCustomPage_GetPageIndex,
  TCustomPage_SetPageIndex,
  TCustomPage_GetTabVisible,
  TCustomPage_SetTabVisible,
  TCustomPage_SetOnShow,
  TCustomPage_SetOnHide,

  ItemFree_SetCallback,
  TTreeView_Create,
  TCustomTreeView_GetItems,
  TCustomTreeView_GetSelected,
  TCustomTreeView_SetSelected,
  TCustomTreeView_FullExpand,
  TCustomTreeView_FullCollapse,
  TCustomTreeView_AlphaSort,
  TCustomTreeView_GetNodeAt,
  TTreeView_GetReadOnly,
  TTreeView_SetReadOnly,
  TTreeView_GetShowLines,
  TTreeView_SetShowLines,
  TTreeView_GetShowRoot,
  TTreeView_SetShowRoot,
  TTreeView_GetShowButtons,
  TTreeView_SetShowButtons,
  TTreeView_GetAutoExpand,
  TTreeView_SetAutoExpand,
  TTreeView_GetHideSelection,
  TTreeView_SetHideSelection,
  TTreeView_GetRowSelect,
  TTreeView_SetRowSelect,
  TTreeView_SetOnChange,
  TTreeView_SetOnExpanded,
  TTreeView_SetOnCollapsed,
  TTreeView_SetOnDeletion,
  TTreeView_SetOnChanging,
  TTreeView_SetOnExpanding,
  TTreeView_SetOnCollapsing,

  TTreeNodes_Add,
  TTreeNodes_AddFirst,
  TTreeNodes_AddChild,
  TTreeNodes_AddChildFirst,
  TTreeNodes_Insert,
  TTreeNodes_Clear,
  TTreeNodes_Delete,
  TTreeNodes_GetCount,
  TTreeNodes_GetItem,
  TTreeNodes_GetFirstNode,
  TTreeNodes_FindNodeWithText,
  TTreeNodes_BeginUpdate,
  TTreeNodes_EndUpdate,

  TTreeNode_GetText,
  TTreeNode_SetText,
  TTreeNode_GetExpanded,
  TTreeNode_SetExpanded,
  TTreeNode_GetSelected,
  TTreeNode_SetSelected,
  TTreeNode_GetHasChildren,
  TTreeNode_SetHasChildren,
  TTreeNode_GetData,
  TTreeNode_SetData,
  TTreeNode_GetCount,
  TTreeNode_GetItem,
  TTreeNode_GetIndex,
  TTreeNode_GetLevel,
  TTreeNode_GetAbsoluteIndex,
  TTreeNode_GetParent,
  TTreeNode_GetTreeView,
  TTreeNode_GetFirstChild,
  TTreeNode_GetLastChild,
  TTreeNode_GetNextSibling,
  TTreeNode_GetPrevSibling,
  TTreeNode_GetNext,
  TTreeNode_GetPrev,
  TTreeNode_IndexOf,
  TTreeNode_Expand,
  TTreeNode_Collapse,
  TTreeNode_Delete,
  TTreeNode_DeleteChildren,
  TTreeNode_MakeVisible,
  TTreeNode_MoveTo,

  TListView_Create,
  TCustomListView_GetItems,
  TCustomListView_GetSelected,
  TCustomListView_SetSelected,
  TCustomListView_GetItemIndex,
  TCustomListView_SetItemIndex,
  TCustomListView_GetSelCount,
  TCustomListView_GetCheckboxes,
  TCustomListView_SetCheckboxes,
  TCustomListView_GetGridLines,
  TCustomListView_SetGridLines,
  TCustomListView_GetMultiSelect,
  TCustomListView_SetMultiSelect,
  TCustomListView_GetReadOnly,
  TCustomListView_SetReadOnly,
  TCustomListView_GetRowSelect,
  TCustomListView_SetRowSelect,
  TCustomListView_Clear,
  TCustomListView_BeginUpdate,
  TCustomListView_EndUpdate,
  TCustomListView_GetItemAt,
  TCustomListView_ClearSelection,
  TCustomListView_SelectAll,
  TListView_GetColumns,
  TListView_GetViewStyle,
  TListView_SetViewStyle,
  TListView_GetHideSelection,
  TListView_SetHideSelection,
  TListView_GetSortType,
  TListView_SetSortType,
  TListView_GetSortColumn,
  TListView_SetSortColumn,
  TListView_GetSortDirection,
  TListView_SetSortDirection,
  TListView_SetOnSelectItem,
  TListView_SetOnChange,
  TListView_SetOnDeletion,
  TListView_SetOnItemChecked,
  TListView_SetOnColumnClick,

  TListItems_Add,
  TListItems_Insert,
  TListItems_Delete,
  TListItems_Clear,
  TListItems_GetCount,
  TListItems_GetItem,
  TListItems_IndexOf,
  TListItems_FindCaption,
  TListItems_Exchange,
  TListItems_Move,
  TListItems_BeginUpdate,
  TListItems_EndUpdate,

  TListItem_GetCaption,
  TListItem_SetCaption,
  TListItem_GetChecked,
  TListItem_SetChecked,
  TListItem_GetSelected,
  TListItem_SetSelected,
  TListItem_GetFocused,
  TListItem_SetFocused,
  TListItem_GetData,
  TListItem_SetData,
  TListItem_GetIndex,
  TListItem_GetListView,
  TListItem_Delete,
  TListItem_MakeVisible,

  TListColumns_Add,
  TListColumns_GetCount,
  TListColumns_GetItem,
  TListColumns_Delete,
  TListColumns_Clear,
  TListColumn_GetCaption,
  TListColumn_SetCaption,
  TListColumn_GetWidth,
  TListColumn_SetWidth,
  TListColumn_GetAlignment,
  TListColumn_SetAlignment,
  TListColumn_GetAutoSize,
  TListColumn_SetAutoSize,
  TListColumn_GetVisible,
  TListColumn_SetVisible,
  TListColumn_GetIndex,
  TListColumn_SetIndex,

  TDrawGrid_Create,
  TStringGrid_Create,
  TCustomGrid_BeginUpdate,
  TCustomGrid_EndUpdate,
  TCustomGrid_Clear,
  TCustomGrid_CellRect,
  TCustomGrid_MouseToCell,
  TCustomDrawGrid_GetColCount,
  TCustomDrawGrid_SetColCount,
  TCustomDrawGrid_GetRowCount,
  TCustomDrawGrid_SetRowCount,
  TCustomDrawGrid_GetFixedCols,
  TCustomDrawGrid_SetFixedCols,
  TCustomDrawGrid_GetFixedRows,
  TCustomDrawGrid_SetFixedRows,
  TCustomDrawGrid_GetCol,
  TCustomDrawGrid_SetCol,
  TCustomDrawGrid_GetRow,
  TCustomDrawGrid_SetRow,
  TCustomDrawGrid_GetDefaultColWidth,
  TCustomDrawGrid_SetDefaultColWidth,
  TCustomDrawGrid_GetDefaultRowHeight,
  TCustomDrawGrid_SetDefaultRowHeight,
  TCustomDrawGrid_GetColWidths,
  TCustomDrawGrid_SetColWidths,
  TCustomDrawGrid_GetRowHeights,
  TCustomDrawGrid_SetRowHeights,
  TCustomDrawGrid_GetOptions,
  TCustomDrawGrid_SetOptions,
  TCustomDrawGrid_GetSelection,
  TCustomDrawGrid_SetSelection,
  TCustomDrawGrid_GetLeftCol,
  TCustomDrawGrid_SetLeftCol,
  TCustomDrawGrid_GetTopRow,
  TCustomDrawGrid_SetTopRow,
  TCustomDrawGrid_GetDefaultDrawing,
  TCustomDrawGrid_SetDefaultDrawing,
  TCustomDrawGrid_GetFixedColor,
  TCustomDrawGrid_SetFixedColor,
  TCustomDrawGrid_GetEditorMode,
  TCustomDrawGrid_SetEditorMode,
  TCustomDrawGrid_InsertColRow,
  TCustomDrawGrid_DeleteColRow,
  TCustomDrawGrid_MoveColRow,
  TCustomDrawGrid_SortColRow,
  TCustomDrawGrid_SetOnDrawCell,
  TCustomDrawGrid_SetOnSelectCell,
  TCustomDrawGrid_SetOnSelection,
  TCustomDrawGrid_SetOnHeaderClick,
  TCustomStringGrid_GetCells,
  TCustomStringGrid_SetCells,
  TCustomStringGrid_Clean,
  TCustomStringGrid_AutoSizeColumns,
  TCustomStringGrid_AutoSizeColumn,
  THeaderControl_Create,
  TCustomHeaderControl_GetSections,
  TCustomHeaderControl_GetDragReorder,
  TCustomHeaderControl_SetDragReorder,
  TCustomHeaderControl_GetSectionAt,
  TCustomHeaderControl_GetSectionFromOriginalIndex,
  TCustomHeaderControl_SetOnSectionClick,
  TCustomHeaderControl_SetOnSectionResize,
  TCustomHeaderControl_SetOnSectionSeparatorDblClick,
  TCustomHeaderControl_SetOnSectionTrack,
  TCustomHeaderControl_SetOnSectionDrag,
  TCustomHeaderControl_SetOnSectionEndDrag,
  THeaderSections_Add,
  THeaderSections_Insert,
  THeaderSections_Delete,
  THeaderSections_Clear,
  THeaderSections_GetCount,
  THeaderSections_GetItem,
  THeaderSections_BeginUpdate,
  THeaderSections_EndUpdate,
  THeaderSection_GetText,
  THeaderSection_SetText,
  THeaderSection_GetWidth,
  THeaderSection_SetWidth,
  THeaderSection_GetMinWidth,
  THeaderSection_SetMinWidth,
  THeaderSection_GetMaxWidth,
  THeaderSection_SetMaxWidth,
  THeaderSection_GetAlignment,
  THeaderSection_SetAlignment,
  THeaderSection_GetVisible,
  THeaderSection_SetVisible,
  THeaderSection_GetIndex,
  THeaderSection_SetIndex,
  THeaderSection_GetLeft,
  THeaderSection_GetRight,
  THeaderSection_GetOriginalIndex,
  TToolWindow_GetEdgeBorders,
  TToolWindow_SetEdgeBorders,
  TToolWindow_GetEdgeInner,
  TToolWindow_SetEdgeInner,
  TToolWindow_GetEdgeOuter,
  TToolWindow_SetEdgeOuter,
  TToolWindow_BeginUpdate,
  TToolWindow_EndUpdate,
  TToolBar_Create,
  TToolBar_GetButtonCount,
  TToolBar_GetButton,
  TToolBar_GetRowCount,
  TToolBar_GetButtonHeight,
  TToolBar_SetButtonHeight,
  TToolBar_GetButtonWidth,
  TToolBar_SetButtonWidth,
  TToolBar_GetDropDownWidth,
  TToolBar_SetDropDownWidth,
  TToolBar_GetIndent,
  TToolBar_SetIndent,
  TToolBar_GetFlat,
  TToolBar_SetFlat,
  TToolBar_GetList,
  TToolBar_SetList,
  TToolBar_GetShowCaptions,
  TToolBar_SetShowCaptions,
  TToolBar_GetTransparent,
  TToolBar_SetTransparent,
  TToolBar_GetWrapable,
  TToolBar_SetWrapable,
  TToolBar_SetButtonSize,
  TToolButton_Create,
  TToolButton_GetAllowAllUp,
  TToolButton_SetAllowAllUp,
  TToolButton_GetDown,
  TToolButton_SetDown,
  TToolButton_GetGrouped,
  TToolButton_SetGrouped,
  TToolButton_GetIndeterminate,
  TToolButton_SetIndeterminate,
  TToolButton_GetMarked,
  TToolButton_SetMarked,
  TToolButton_GetShowCaption,
  TToolButton_SetShowCaption,
  TToolButton_GetWrap,
  TToolButton_SetWrap,
  TToolButton_GetStyle,
  TToolButton_SetStyle,
  TToolButton_GetDropdownMenu,
  TToolButton_SetDropdownMenu,
  TToolButton_GetMenuItem,
  TToolButton_SetMenuItem,
  TToolButton_GetIndex,
  TToolButton_Click,
  TToolButton_ArrowClick,
  TToolButton_PointInArrow,
  TToolButton_SetOnArrowClick,
  TCoolBar_Create,
  TCustomCoolBar_GetBands,
  TCustomCoolBar_AutosizeBands,
  TCustomCoolBar_MouseToBandPos,
  TCustomCoolBar_GetFixedSize,
  TCustomCoolBar_SetFixedSize,
  TCustomCoolBar_GetFixedOrder,
  TCustomCoolBar_SetFixedOrder,
  TCustomCoolBar_GetGrabStyle,
  TCustomCoolBar_SetGrabStyle,
  TCustomCoolBar_GetGrabWidth,
  TCustomCoolBar_SetGrabWidth,
  TCustomCoolBar_GetHorizontalSpacing,
  TCustomCoolBar_SetHorizontalSpacing,
  TCustomCoolBar_GetVerticalSpacing,
  TCustomCoolBar_SetVerticalSpacing,
  TCustomCoolBar_GetShowText,
  TCustomCoolBar_SetShowText,
  TCustomCoolBar_GetThemed,
  TCustomCoolBar_SetThemed,
  TCustomCoolBar_GetVertical,
  TCustomCoolBar_SetVertical,
  TCustomCoolBar_SetOnChange,
  TCoolBands_Add,
  TCoolBands_GetCount,
  TCoolBands_GetItem,
  TCoolBands_Delete,
  TCoolBands_Clear,
  TCoolBands_BeginUpdate,
  TCoolBands_EndUpdate,
  TCoolBands_FindBand,
  TCoolBands_FindBandIndex,
  TCoolBand_GetText,
  TCoolBand_SetText,
  TCoolBand_GetWidth,
  TCoolBand_SetWidth,
  TCoolBand_GetMinWidth,
  TCoolBand_SetMinWidth,
  TCoolBand_GetMinHeight,
  TCoolBand_SetMinHeight,
  TCoolBand_GetBreak,
  TCoolBand_SetBreak,
  TCoolBand_GetVisible,
  TCoolBand_SetVisible,
  TCoolBand_GetFixedSize,
  TCoolBand_SetFixedSize,
  TCoolBand_GetFixedBackground,
  TCoolBand_SetFixedBackground,
  TCoolBand_GetHorizontalOnly,
  TCoolBand_SetHorizontalOnly,
  TCoolBand_GetColor,
  TCoolBand_SetColor,
  TCoolBand_GetParentColor,
  TCoolBand_SetParentColor,
  TCoolBand_GetIndex,
  TCoolBand_SetIndex,
  TCoolBand_GetControl,
  TCoolBand_SetControl,
  TCoolBand_GetLeft,
  TCoolBand_GetTop,
  TCoolBand_GetRight,
  TCoolBand_GetHeight,
  TCoolBand_AutosizeWidth,
  TCustomMemo_GetLines,
  TCustomComboBox_GetItems,
  TCustomListBox_GetItems,
  TCustomRadioGroup_GetItems,
  TCustomCheckGroup_GetItems,
  TTabControl_GetTabs,
  TListItem_GetSubItems,
  TStrings_GetCount,
  TStrings_GetStrings,
  TStrings_SetStrings,
  TStrings_GetObjects,
  TStrings_SetObjects,
  TStrings_Add,
  TStrings_AddObject,
  TStrings_Insert,
  TStrings_Delete,
  TStrings_Clear,
  TStrings_IndexOf,
  TStrings_Exchange,
  TStrings_Move,
  TStrings_BeginUpdate,
  TStrings_EndUpdate,
  TStrings_GetText,
  TStrings_SetText,
  TStrings_GetCommaText,
  TStrings_SetCommaText,
  TStrings_Assign,
  TStrings_AddStrings,
  TStrings_GetNames,
  TStrings_GetValues,
  TStrings_SetValues,
  TStrings_GetValueFromIndex,
  TStrings_SetValueFromIndex,
  TStrings_IndexOfName,
  TStrings_GetDelimiter,
  TStrings_SetDelimiter,
  TStrings_GetStrictDelimiter,
  TStrings_SetStrictDelimiter,
  TStrings_GetDelimitedText,
  TStrings_SetDelimitedText,
  TStrings_LoadFromFile,
  TStrings_SaveToFile,
  TStringList_Create,
  TStringList_Destroy,
  TStringList_Sort,
  TStringList_Find,
  TStringList_GetSorted,
  TStringList_SetSorted,
  TStringList_GetDuplicates,
  TStringList_SetDuplicates,
  TStringList_GetCaseSensitive,
  TStringList_SetCaseSensitive,
  TControl_GetAutoSize,
  TControl_SetAutoSize,
  TCanvas_Draw,
  TCanvas_StretchDraw,
  TCanvas_FillRect,
  TCanvas_GetPixels,
  TCanvas_SetPixels,
  TGraphic_Destroy,
  TGraphic_GetWidth,
  TGraphic_SetWidth,
  TGraphic_GetHeight,
  TGraphic_SetHeight,
  TGraphic_GetEmpty,
  TGraphic_GetTransparent,
  TGraphic_SetTransparent,
  TGraphic_LoadFromFile,
  TGraphic_SaveToFile,
  TGraphic_Assign,
  TGraphic_Clear,
  TRasterImage_GetCanvas,
  TRasterImage_GetPixelFormat,
  TRasterImage_SetPixelFormat,
  TRasterImage_GetTransparentColor,
  TRasterImage_SetTransparentColor,
  TRasterImage_GetTransparentMode,
  TRasterImage_SetTransparentMode,
  TCustomBitmap_SetSize,
  TBitmap_Create,
  TPortableNetworkGraphic_Create,
  TJPEGImage_Create,
  TJPEGImage_GetCompressionQuality,
  TJPEGImage_SetCompressionQuality,
  TPicture_Create,
  TPicture_Destroy,
  TPicture_GetGraphic,
  TPicture_SetGraphic,
  TPicture_GetBitmap,
  TPicture_GetPNG,
  TPicture_GetJpeg,
  TPicture_GetWidth,
  TPicture_GetHeight,
  TPicture_LoadFromFile,
  TPicture_SaveToFile,
  TPicture_Assign,
  TPicture_Clear,
  TImage_Create,
  TCustomImage_GetPicture,
  TCustomImage_SetPicture,
  TCustomImage_GetCanvas,
  TCustomImage_GetHasGraphic,
  TCustomImage_GetCenter,
  TCustomImage_SetCenter,
  TCustomImage_GetStretch,
  TCustomImage_SetStretch,
  TCustomImage_GetStretchOutEnabled,
  TCustomImage_SetStretchOutEnabled,
  TCustomImage_GetStretchInEnabled,
  TCustomImage_SetStretchInEnabled,
  TCustomImage_GetProportional,
  TCustomImage_SetProportional,
  TCustomImage_GetTransparent,
  TCustomImage_SetTransparent,
  TCustomImage_SetOnPictureChanged,
  TCustomBitBtn_GetGlyph,
  TCustomBitBtn_SetGlyph,
  TCustomBitBtn_GetNumGlyphs,
  TCustomBitBtn_SetNumGlyphs,
  TCustomBitBtn_GetLayout,
  TCustomBitBtn_SetLayout,
  TCustomBitBtn_GetMargin,
  TCustomBitBtn_SetMargin,
  TCustomBitBtn_GetSpacing,
  TCustomBitBtn_SetSpacing,
  TCustomSpeedButton_GetGlyph,
  TCustomSpeedButton_SetGlyph,
  TCustomSpeedButton_GetNumGlyphs,
  TCustomSpeedButton_SetNumGlyphs,
  TCustomSpeedButton_GetLayout,
  TCustomSpeedButton_SetLayout,
  TCustomSpeedButton_GetMargin,
  TCustomSpeedButton_SetMargin,
  TCustomSpeedButton_GetSpacing,
  TCustomSpeedButton_SetSpacing,
  TImageList_Create,
  TCustomImageList_GetWidth,
  TCustomImageList_SetWidth,
  TCustomImageList_GetHeight,
  TCustomImageList_SetHeight,
  TCustomImageList_GetCount,
  TCustomImageList_GetMasked,
  TCustomImageList_SetMasked,
  TCustomImageList_GetBkColor,
  TCustomImageList_SetBkColor,
  TCustomImageList_GetDrawingStyle,
  TCustomImageList_SetDrawingStyle,
  TCustomImageList_Add,
  TCustomImageList_AddSliced,
  TCustomImageList_AddMasked,
  TCustomImageList_Insert,
  TCustomImageList_Replace,
  TCustomImageList_Delete,
  TCustomImageList_Clear,
  TCustomImageList_Move,
  TCustomImageList_GetBitmap,
  TCustomImageList_Draw,
  TCustomImageList_BeginUpdate,
  TCustomImageList_EndUpdate,
  TCustomImageList_SetOnChange,
  TCustomImage_GetImages,
  TCustomImage_SetImages,
  TCustomImage_GetImageIndex,
  TCustomImage_SetImageIndex,
  TCustomBitBtn_GetImages,
  TCustomBitBtn_SetImages,
  TCustomBitBtn_GetImageIndex,
  TCustomBitBtn_SetImageIndex,
  TCustomSpeedButton_GetImages,
  TCustomSpeedButton_SetImages,
  TCustomSpeedButton_GetImageIndex,
  TCustomSpeedButton_SetImageIndex,
  TCustomTabControl_GetImages,
  TCustomTabControl_SetImages,
  TCustomPage_GetImageIndex,
  TCustomPage_SetImageIndex,
  TCustomTreeView_GetImages,
  TCustomTreeView_SetImages,
  TCustomTreeView_GetStateImages,
  TCustomTreeView_SetStateImages,
  TTreeNode_GetImageIndex,
  TTreeNode_SetImageIndex,
  TTreeNode_GetSelectedIndex,
  TTreeNode_SetSelectedIndex,
  TTreeNode_GetStateIndex,
  TTreeNode_SetStateIndex,
  TTreeNode_GetOverlayIndex,
  TTreeNode_SetOverlayIndex,
  TListView_GetLargeImages,
  TListView_SetLargeImages,
  TListView_GetSmallImages,
  TListView_SetSmallImages,
  TListView_GetStateImages,
  TListView_SetStateImages,
  TListItem_GetImageIndex,
  TListItem_SetImageIndex,
  TListItem_GetStateIndex,
  TListItem_SetStateIndex,
  TListColumn_GetImageIndex,
  TListColumn_SetImageIndex,
  TToolBar_GetImages,
  TToolBar_SetImages,
  TToolBar_GetHotImages,
  TToolBar_SetHotImages,
  TToolBar_GetDisabledImages,
  TToolBar_SetDisabledImages,
  TToolButton_GetImageIndex,
  TToolButton_SetImageIndex,
  TCustomHeaderControl_GetImages,
  TCustomHeaderControl_SetImages,
  THeaderSection_GetImageIndex,
  THeaderSection_SetImageIndex,
  TCustomCoolBar_GetImages,
  TCustomCoolBar_SetImages,
  TCustomCoolBar_GetBitmap,
  TCustomCoolBar_SetBitmap,
  TCoolBand_GetImageIndex,
  TCoolBand_SetImageIndex,
  TCoolBand_GetBitmap,
  TCoolBand_SetBitmap,
  TMenu_GetImages,
  TMenu_SetImages,
  TMenuItem_GetImageIndex,
  TMenuItem_SetImageIndex,
  TMenuItem_GetSubMenuImages,
  TMenuItem_SetSubMenuImages,
  TMenuItem_GetBitmap,
  TMenuItem_SetBitmap,
  TControl_GetColor,
  TControl_SetColor,
  TControl_GetFont,
  TControl_SetFont,
  TFont_GetStyle,
  TFont_SetStyle,
  TFont_Assign,
  TCommonDialog_Execute,
  TCommonDialog_GetTitle,
  TCommonDialog_SetTitle,
  TCommonDialog_SetOnShow,
  TCommonDialog_SetOnClose,
  TCommonDialog_SetOnCanClose,
  TFileDialog_GetFileName,
  TFileDialog_SetFileName,
  TFileDialog_GetFilter,
  TFileDialog_SetFilter,
  TFileDialog_GetFilterIndex,
  TFileDialog_SetFilterIndex,
  TFileDialog_GetInitialDir,
  TFileDialog_SetInitialDir,
  TFileDialog_GetDefaultExt,
  TFileDialog_SetDefaultExt,
  TFileDialog_GetFiles,
  TOpenDialog_Create,
  TOpenDialog_GetOptions,
  TOpenDialog_SetOptions,
  TSaveDialog_Create,
  TSelectDirectoryDialog_Create,
  TColorDialog_Create,
  TColorDialog_GetColor,
  TColorDialog_SetColor,
  TColorDialog_GetCustomColors,
  TColorDialog_GetOptions,
  TColorDialog_SetOptions,
  TFontDialog_Create,
  TFontDialog_GetFont,
  TFontDialog_SetFont,
  TFontDialog_GetMinFontSize,
  TFontDialog_SetMinFontSize,
  TFontDialog_GetMaxFontSize,
  TFontDialog_SetMaxFontSize,
  TFontDialog_GetOptions,
  TFontDialog_SetOptions,
  TFindDialog_Create,
  TReplaceDialog_Create,
  TFindDialog_GetFindText,
  TFindDialog_SetFindText,
  TFindDialog_GetReplaceText,
  TFindDialog_SetReplaceText,
  TFindDialog_GetOptions,
  TFindDialog_SetOptions,
  TFindDialog_GetLeft,
  TFindDialog_SetLeft,
  TFindDialog_GetTop,
  TFindDialog_SetTop,
  TFindDialog_CloseDialog,
  TFindDialog_SetOnFind,
  TFindDialog_SetOnReplace,
  TComponent_GetTag,
  TComponent_SetTag,
  TControl_GetAnchors,
  TControl_SetAnchors,
  TControl_GetBorderSpacing,
  TControl_SetBorderSpacing,
  TControl_GetConstraints,
  TControl_SetConstraints,
  TControl_GetHint,
  TControl_SetHint,
  TControl_GetShowHint,
  TControl_SetShowHint,
  TControl_GetCursor,
  TControl_SetCursor,
  TControl_GetParentColor,
  TControl_SetParentColor,
  TControl_GetParentFont,
  TControl_SetParentFont,
  TControl_GetParentShowHint,
  TControl_SetParentShowHint,
  TWinControl_GetTabOrder,
  TWinControl_SetTabOrder,
  TWinControl_GetTabStop,
  TWinControl_SetTabStop,
  TSizeConstraints_GetMinWidth,
  TSizeConstraints_SetMinWidth,
  TSizeConstraints_GetMinHeight,
  TSizeConstraints_SetMinHeight,
  TSizeConstraints_GetMaxWidth,
  TSizeConstraints_SetMaxWidth,
  TSizeConstraints_GetMaxHeight,
  TSizeConstraints_SetMaxHeight,
  TControlBorderSpacing_GetLeft,
  TControlBorderSpacing_SetLeft,
  TControlBorderSpacing_GetTop,
  TControlBorderSpacing_SetTop,
  TControlBorderSpacing_GetRight,
  TControlBorderSpacing_SetRight,
  TControlBorderSpacing_GetBottom,
  TControlBorderSpacing_SetBottom,
  TControlBorderSpacing_GetAround,
  TControlBorderSpacing_SetAround,
  TControlBorderSpacing_GetInnerBorder,
  TControlBorderSpacing_SetInnerBorder,

  TCustomForm_GetModalResult,
  TCustomForm_SetModalResult,
  TCustomForm_GetBorderStyle,
  TCustomForm_SetBorderStyle,
  TCustomForm_GetPosition,
  TCustomForm_SetPosition,
  TCustomForm_GetWindowState,
  TCustomForm_SetWindowState,
  TCustomForm_GetBorderIcons,
  TCustomForm_SetBorderIcons,
  TCustomForm_GetFormStyle,
  TCustomForm_SetFormStyle,
  TCustomForm_GetKeyPreview,
  TCustomForm_SetKeyPreview,
  TCustomForm_GetActiveControl,
  TCustomForm_SetActiveControl,
  TCustomButton_GetModalResult,
  TCustomButton_SetModalResult,
  TCustomButton_GetDefault,
  TCustomButton_SetDefault,
  TCustomButton_GetCancel,
  TCustomButton_SetCancel,
  Dialogs_ShowMessage,
  Dialogs_MessageDlg,
  Dialogs_InputBox,
  Dialogs_PasswordBox,
  Dialogs_InputQuery,
  TApplication_MessageBox,

  TControl_GetClientWidth,
  TControl_SetClientWidth,
  TControl_GetClientHeight,
  TControl_SetClientHeight,
  TControl_Invalidate,
  TControl_Repaint,
  TControl_Refresh,
  TControl_Update,
  TControl_BringToFront,
  TControl_SendToBack,
  TControl_SetBounds,
  TWinControl_SetFocus,
  TWinControl_CanFocus,
  TWinControl_Focused,
  TCustomEdit_GetSelStart,
  TCustomEdit_SetSelStart,
  TCustomEdit_GetSelLength,
  TCustomEdit_SetSelLength,
  TCustomEdit_GetSelText,
  TCustomEdit_SetSelText,
  TCustomEdit_GetModified,
  TCustomEdit_SetModified,
  TCustomEdit_GetCanUndo,
  TCustomEdit_GetPasswordChar,
  TCustomEdit_SetPasswordChar,
  TCustomEdit_GetEchoMode,
  TCustomEdit_SetEchoMode,
  TCustomEdit_GetCharCase,
  TCustomEdit_SetCharCase,
  TCustomEdit_GetAlignment,
  TCustomEdit_SetAlignment,
  TCustomEdit_GetTextHint,
  TCustomEdit_SetTextHint,
  TCustomEdit_GetNumbersOnly,
  TCustomEdit_SetNumbersOnly,
  TCustomEdit_GetAutoSelect,
  TCustomEdit_SetAutoSelect,
  TCustomEdit_GetHideSelection,
  TCustomEdit_SetHideSelection,
  TCustomEdit_GetCaretPos,
  TCustomEdit_SetCaretPos,
  TCustomEdit_SelectAll,
  TCustomEdit_ClearSelection,
  TCustomEdit_Clear,
  TCustomEdit_CopyToClipboard,
  TCustomEdit_CutToClipboard,
  TCustomEdit_PasteFromClipboard,
  TCustomEdit_Undo,
  TCustomMemo_GetWordWrap,
  TCustomMemo_SetWordWrap,
  TCustomMemo_GetWantReturns,
  TCustomMemo_SetWantReturns,
  TCustomMemo_GetWantTabs,
  TCustomMemo_SetWantTabs,
  TCustomMemo_Append,
  TCustomLabel_GetAlignment,
  TCustomLabel_SetAlignment,
  TCustomLabel_GetLayout,
  TCustomLabel_SetLayout,
  TCustomLabel_GetWordWrap,
  TCustomLabel_SetWordWrap,
  TCustomLabel_GetTransparent,
  TCustomLabel_SetTransparent,
  TCustomLabel_GetFocusControl,
  TCustomLabel_SetFocusControl,
  TCustomLabel_GetShowAccelChar,
  TCustomLabel_SetShowAccelChar,
  TWinControl_SetOnEnter,
  TWinControl_SetOnExit,

  TCustomListBox_GetMultiSelect,
  TCustomListBox_SetMultiSelect,
  TCustomListBox_GetExtendedSelect,
  TCustomListBox_SetExtendedSelect,
  TCustomListBox_GetSorted,
  TCustomListBox_SetSorted,
  TCustomListBox_GetTopIndex,
  TCustomListBox_SetTopIndex,
  TCustomListBox_GetSelCount,
  TCustomListBox_GetSelected,
  TCustomListBox_SetSelected,
  TCustomListBox_ClearSelection,
  TCustomListBox_SelectAll,
  TCustomListBox_ItemAtPos,
  TCustomListBox_SetOnSelectionChange,
  TCustomComboBox_GetStyle,
  TCustomComboBox_SetStyle,
  TCustomComboBox_GetDropDownCount,
  TCustomComboBox_SetDropDownCount,
  TCustomComboBox_GetSorted,
  TCustomComboBox_SetSorted,
  TCustomComboBox_GetReadOnly,
  TCustomComboBox_SetReadOnly,
  TCustomComboBox_GetDroppedDown,
  TCustomComboBox_SetDroppedDown,
  TCustomComboBox_GetAutoComplete,
  TCustomComboBox_SetAutoComplete,
  TCustomComboBox_SetOnSelect,
  TCustomComboBox_SetOnDropDown,
  TCustomComboBox_SetOnCloseUp,
  TCustomCheckBox_GetState,
  TCustomCheckBox_SetState,
  TCustomCheckBox_GetAllowGrayed,
  TCustomCheckBox_SetAllowGrayed,
  TCustomCheckBox_SetOnChange,
  TApplication_GetExeName,
  TApplication_GetHint,
  TApplication_SetHint,
  TApplication_GetShowHint,
  TApplication_SetShowHint,
  TApplication_GetHintPause,
  TApplication_SetHintPause,
  TApplication_GetHintHidePause,
  TApplication_SetHintHidePause,
  TApplication_Minimize,
  TApplication_Restore,
  TApplication_BringToFront,
  TApplication_SetOnIdle,
  TApplication_SetOnException,
  TStatusBar_GetPanels,
  TStatusBar_GetSizeGrip,
  TStatusBar_SetSizeGrip,
  TStatusBar_GetAutoHint,
  TStatusBar_SetAutoHint,
  TStatusBar_GetCanvas,
  TStatusBar_GetPanelIndexAt,
  TStatusBar_BeginUpdate,
  TStatusBar_EndUpdate,
  TStatusBar_SetOnDrawPanel,
  TStatusBar_SetOnHint,
  TStatusPanels_Add,
  TStatusPanels_Insert,
  TStatusPanels_Delete,
  TStatusPanels_Clear,
  TStatusPanels_GetCount,
  TStatusPanels_GetItem,
  TStatusPanels_BeginUpdate,
  TStatusPanels_EndUpdate,
  TStatusPanel_GetText,
  TStatusPanel_SetText,
  TStatusPanel_GetWidth,
  TStatusPanel_SetWidth,
  TStatusPanel_GetAlignment,
  TStatusPanel_SetAlignment,
  TStatusPanel_GetBevel,
  TStatusPanel_SetBevel,
  TStatusPanel_GetStyle,
  TStatusPanel_SetStyle,
  TStatusPanel_GetIndex,
  TStatusPanel_SetIndex,
  TCustomControl_GetCanvas,
  TCustomControl_SetOnPaint,
  TCanvas_TextWidth,
  TCanvas_TextHeight,
  TCanvas_TextRect,
  TCanvas_Polygon,
  TCanvas_Polyline,
  TCanvas_RoundRect,
  TCanvas_Arc,
  TCanvas_Pie,
  TCanvas_Chord,
  TCanvas_FrameRect,
  TCanvas_CopyRect,
  TPen_GetStyle,
  TPen_SetStyle,
  TPen_GetMode,
  TPen_SetMode,
  TBrush_GetStyle,
  TBrush_SetStyle,
  TFont_GetHeight,
  TFont_SetHeight,
  TFont_GetOrientation,
  TFont_SetOrientation,
  TFont_GetQuality,
  TFont_SetQuality,
  TBasicAction_Execute,
  TBasicAction_Update,
  TBasicAction_GetActionComponent,
  TBasicAction_SetOnExecute,
  TBasicAction_SetOnUpdate,
  TContainedAction_GetActionList,
  TContainedAction_SetActionList,
  TContainedAction_GetCategory,
  TContainedAction_SetCategory,
  TContainedAction_GetIndex,
  TContainedAction_SetIndex,
  TCustomAction_GetCaption,
  TCustomAction_SetCaption,
  TCustomAction_GetHint,
  TCustomAction_SetHint,
  TCustomAction_GetChecked,
  TCustomAction_SetChecked,
  TCustomAction_GetAutoCheck,
  TCustomAction_SetAutoCheck,
  TCustomAction_GetEnabled,
  TCustomAction_SetEnabled,
  TCustomAction_GetVisible,
  TCustomAction_SetVisible,
  TCustomAction_GetDisableIfNoHandler,
  TCustomAction_SetDisableIfNoHandler,
  TCustomAction_GetGroupIndex,
  TCustomAction_SetGroupIndex,
  TCustomAction_GetImageIndex,
  TCustomAction_SetImageIndex,
  TCustomAction_GetShortCut,
  TCustomAction_SetShortCut,
  TAction_Create,
  TCustomActionList_GetActionCount,
  TCustomActionList_GetActions,
  TCustomActionList_GetImages,
  TCustomActionList_SetImages,
  TCustomActionList_GetState,
  TCustomActionList_SetState,
  TCustomActionList_SetOnExecute,
  TCustomActionList_SetOnUpdate,
  TActionList_Create,
  TControl_GetAction,
  TControl_SetAction,
  TMenuItem_GetAction,
  TMenuItem_SetAction,
  GetScreen,
  TScreen_GetCursor,
  TScreen_SetCursor,
  TScreen_GetWidth,
  TScreen_GetHeight,
  TScreen_GetDesktopLeft,
  TScreen_GetDesktopTop,
  TScreen_GetDesktopWidth,
  TScreen_GetDesktopHeight,
  TScreen_GetWorkAreaLeft,
  TScreen_GetWorkAreaTop,
  TScreen_GetWorkAreaWidth,
  TScreen_GetWorkAreaHeight,
  TScreen_GetPixelsPerInch,
  TScreen_GetMonitorCount,
  TScreen_GetFormCount,
  TScreen_GetWorkAreaRect,
  TScreen_GetForms,
  TScreen_GetActiveForm,
  TScreen_GetActiveControl,
  TScreen_GetFonts,
  TScreen_SetOnActiveFormChange,
  TScreen_SetOnActiveControlChange,
  GetClipboard,
  Clipboard_CF_Text,
  Clipboard_CF_Bitmap,
  Clipboard_CF_Picture,
  TClipboard_GetAsText,
  TClipboard_SetAsText,
  TClipboard_HasFormat,
  TClipboard_HasPictureFormat,
  TClipboard_Clear,
  TClipboard_Open,
  TClipboard_Close,
  TClipboard_GetFormatCount,
  TClipboard_GetFormats,
  TClipboard_Assign,
  TIcon_Create,
  TCustomForm_GetIcon,
  TCustomForm_SetIcon,
  TApplication_GetIcon,
  TApplication_SetIcon,
  TPicture_GetIcon,
  TPicture_SetIcon,
  TCustomForm_GetAllowDropFiles,
  TCustomForm_SetAllowDropFiles,
  TCustomForm_SetOnDropFiles,
  TWinControl_GetBorderStyle,
  TWinControl_SetBorderStyle,
  TWinControl_GetBorderWidth,
  TWinControl_SetBorderWidth,
  TCustomPanel_GetAlignment,
  TCustomPanel_SetAlignment,
  TCustomPanel_GetVerticalAlignment,
  TCustomPanel_SetVerticalAlignment,
  TCustomPanel_GetWordWrap,
  TCustomPanel_SetWordWrap,
  TCustomPanel_GetBevelColor,
  TCustomPanel_SetBevelColor,
  TCustomPanel_GetBevelInner,
  TCustomPanel_SetBevelInner,
  TCustomPanel_GetBevelOuter,
  TCustomPanel_SetBevelOuter,
  TCustomPanel_GetBevelWidth,
  TCustomPanel_SetBevelWidth,
  TCustomDrawGrid_GetScrollBars,
  TCustomDrawGrid_SetScrollBars,
  TCustomTreeView_GetScrollBars,
  TCustomTreeView_SetScrollBars,
  TCustomListView_GetScrollBars,
  TCustomListView_SetScrollBars,
  TControlScrollBar_GetKind,
  TControlScrollBar_GetSize,
  TControlScrollBar_IsScrollBarVisible,
  TControlScrollBar_GetIncrement,
  TControlScrollBar_SetIncrement,
  TControlScrollBar_GetPage,
  TControlScrollBar_SetPage,
  TControlScrollBar_GetPosition,
  TControlScrollBar_SetPosition,
  TControlScrollBar_GetRange,
  TControlScrollBar_SetRange,
  TControlScrollBar_GetSmooth,
  TControlScrollBar_SetSmooth,
  TControlScrollBar_GetTracking,
  TControlScrollBar_SetTracking,
  TControlScrollBar_GetVisible,
  TControlScrollBar_SetVisible,
  TScrollingWinControl_GetAutoScroll,
  TScrollingWinControl_SetAutoScroll,
  TScrollingWinControl_GetHorzScrollBar,
  TScrollingWinControl_SetHorzScrollBar,
  TScrollingWinControl_GetVertScrollBar,
  TScrollingWinControl_SetVertScrollBar,
  TCustomTrackBar_GetOrientation,
  TCustomTrackBar_SetOrientation,
  TCustomTrackBar_GetFrequency,
  TCustomTrackBar_SetFrequency,
  TCustomTrackBar_GetTickMarks,
  TCustomTrackBar_SetTickMarks,
  TCustomTrackBar_GetTickStyle,
  TCustomTrackBar_SetTickStyle,
  TCustomTrackBar_GetLineSize,
  TCustomTrackBar_SetLineSize,
  TCustomTrackBar_GetPageSize,
  TCustomTrackBar_SetPageSize,
  TCustomTrackBar_GetSelStart,
  TCustomTrackBar_SetSelStart,
  TCustomTrackBar_GetSelEnd,
  TCustomTrackBar_SetSelEnd,
  TCustomTrackBar_GetShowSelRange,
  TCustomTrackBar_SetShowSelRange,
  TCustomTrackBar_GetReversed,
  TCustomTrackBar_SetReversed,
  TCustomProgressBar_GetOrientation,
  TCustomProgressBar_SetOrientation,
  TCustomProgressBar_GetSmooth,
  TCustomProgressBar_SetSmooth,
  TCustomProgressBar_GetStep,
  TCustomProgressBar_SetStep,
  TCustomProgressBar_GetStyle,
  TCustomProgressBar_SetStyle,
  TCustomProgressBar_GetBarShowText,
  TCustomProgressBar_SetBarShowText,
  TCustomScrollBar_GetLargeChange,
  TCustomScrollBar_SetLargeChange,
  TCustomScrollBar_GetSmallChange,
  TCustomScrollBar_SetSmallChange,
  TUpDown_GetOrientation,
  TUpDown_SetOrientation,
  TUpDown_GetAlignButton,
  TUpDown_SetAlignButton,
  TUpDown_GetWrap,
  TUpDown_SetWrap,
  TUpDown_GetArrowKeys,
  TUpDown_SetArrowKeys,
  TUpDown_GetThousands,
  TUpDown_SetThousands,
  TCustomRadioGroup_GetColumns,
  TCustomRadioGroup_SetColumns,
  TCustomRadioGroup_GetColumnLayout,
  TCustomRadioGroup_SetColumnLayout,
  TCustomRadioGroup_GetAutoFill,
  TCustomRadioGroup_SetAutoFill,
  TCustomCheckGroup_GetColumns,
  TCustomCheckGroup_SetColumns,
  TCustomCheckGroup_GetColumnLayout,
  TCustomCheckGroup_SetColumnLayout,
  TCustomCheckGroup_GetAutoFill,
  TCustomCheckGroup_SetAutoFill,
  TCustomProgressBar_StepIt,
  TCustomProgressBar_StepBy,
  TCustomScrollBar_SetOnScroll,
  TCustomRadioGroup_SetOnSelectionChanged,
  TCustomCheckGroup_GetCheckEnabled,
  TCustomCheckGroup_SetCheckEnabled,
  TCustomCheckGroup_SetOnItemClick;

begin
  RequireDerivedFormResource := False;
  Application.Initialize;
  { DLL(IsLibrary)ではアプリケーションのウィンドウ(AppHandle)が作られないため、タスクバーのボタンはメインフォームが持つ。
    MainFormOnTaskBar が False(既定)だと、LCL はメインフォームの最小化を AppHandle の最小化に置き換えてメインフォームを隠すため、
    AppHandle が 0 の DLL ではフォームが隠れたまま、タスクバーのボタンも消えていた(Application.Minimize・Restore も同じ)。 }
  Application.MainFormOnTaskBar := True;
  GFreeNotifier := TFreeNotifier.Create(nil);
  { Dll_Process_Detach_Hook は Windows の DLL_PROCESS_DETACH 通知専用のフックで、
    Linux の共有ライブラリ(.so)には存在しない。 }
  {$ifdef WINDOWS}
  Dll_Process_Detach_Hook := @DetachHook;
  {$endif}
end.
