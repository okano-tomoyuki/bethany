library no_vcl;

{$macro on}
{$ifdef WINDOWS}
  {$define NO_VCL_CALL := stdcall}
{$else}
  {$define NO_VCL_CALL := cdecl}
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
  CheckLst,
  Buttons,
  Spin,
  MaskEdit,
  Menus,
  LCLProc,
  Graphics,
  CustomTimer
  {$ifdef LCLwin32}
  , Windows, InterfaceBase, WSControls
  {$endif};

type
  { Data は登録時に渡された利用者データをそのまま返す(C 側で状態を持ち回るため) }
  TNoVclCallback = procedure(Sender: Pointer; Data: Pointer); NO_VCL_CALL;

  { Cのプレーンな関数ポインタ(no_vcl_callback_t)を
    LCLのTNotifyEvent(オブジェクトメソッド)へ橋渡しする }
  TCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclCallback;
    FData: Pointer;
  public
    procedure DoClick(Sender: TObject);
    property Callback: TNoVclCallback read FCallback write FCallback;
    property Data: Pointer read FData write FData;
  end;

  { 書き換え可能な引数(Pascal の var 引数)を 1 つ持つイベント用のコールバック。Value はその値を指すポインタで、
    コールバックの中で書き換えると呼び出し元に反映される。
      OnClose      (TCloseEvent)     : TCloseAction の序数(caNone=0, caHide, caFree, caMinimize)
      OnCloseQuery (TCloseQueryEvent): CanClose(0 = False、0 以外 = True。渡すときの True は -1) }
  TNoVclVarCallback = procedure(Sender: Pointer; Value: PInteger; Data: Pointer); NO_VCL_CALL;

  TVarCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclVarCallback;
    FData: Pointer;
  public
    procedure DoClose(Sender: TObject; var CloseAction: TCloseAction);
    procedure DoCloseQuery(Sender: TObject; var CanClose: Boolean);
  end;

  { OnKeyDown/OnKeyUp(TKeyEvent)用。Key(キーコード)は書き換え可能(var 引数)。0 にすると LCL に渡さない。
    Shift は TShiftStateEnum の各値をビットとして表した LongWord(no_vcl_ss* のビット和、ShiftStateToInt 参照)。 }
  TNoVclKeyCallback = procedure(Sender: Pointer; Key: PInteger; Shift: LongWord; Data: Pointer); NO_VCL_CALL;

  TKeyCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclKeyCallback;
    FData: Pointer;
  public
    procedure DoKey(Sender: TObject; var Key: Word; Shift: TShiftState);
  end;

  { OnKeyPress(TKeyPressEvent)用。Key は文字コード、書き換え可能。0 にすると LCL に渡さない。 }
  TNoVclKeyPressCallback = procedure(Sender: Pointer; Key: PInteger; Data: Pointer); NO_VCL_CALL;

  TKeyPressCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclKeyPressCallback;
    FData: Pointer;
  public
    procedure DoKeyPress(Sender: TObject; var Key: char);
  end;

  { OnMouseDown/OnMouseUp(TMouseEvent)用。Button は TMouseButton の序数(no_vcl_mb*)、Shift は上記と同じ。 }
  TNoVclMouseCallback = procedure(Sender: Pointer; Button, Shift, X, Y: Integer; Data: Pointer); NO_VCL_CALL;

  TMouseCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclMouseCallback;
    FData: Pointer;
  public
    procedure DoMouse(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
  end;

  { OnMouseMove(TMouseMoveEvent)用。 }
  TNoVclMouseMoveCallback = procedure(Sender: Pointer; Shift, X, Y: Integer; Data: Pointer); NO_VCL_CALL;

  TMouseMoveCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclMouseMoveCallback;
    FData: Pointer;
  public
    procedure DoMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
  end;

  { OnMouseWheel(TMouseWheelEvent)用。Handled は書き換え可能(0 以外 = True)。
    True にすると、ホイール操作をこのハンドラで処理済みとして扱う(既定のスクロール等が起きなくなる)。 }
  TNoVclMouseWheelCallback = procedure(Sender: Pointer; Shift, WheelDelta, X, Y: Integer; Handled: PInteger; Data: Pointer); NO_VCL_CALL;

  TMouseWheelCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclMouseWheelCallback;
    FData: Pointer;
  public
    procedure DoMouseWheel(Sender: TObject; Shift: TShiftState; WheelDelta: Integer; MousePos: TPoint; var Handled: Boolean);
  end;

  { protected メンバへアクセスするための派生クラス(protected hack)。
    同一ユニット内で宣言した派生クラス経由なら、基底の protected メンバに触れられる。 }
  TControlAccess = class(TControl);
  TButtonControlAccess = class(TButtonControl);

  { *_Create で生成したすべてのコンポーネントの破棄を受け取り、C/C++ 側へ通知する。
    Owner による連鎖破棄など、呼び出し側が知らないところで起きる破棄も検知できる。 }
  TFreeNotifier = class(TComponent)
  protected
    procedure Notification(AComponent: TComponent; Operation: TOperation); override;
  end;

var
  GFreeNotifier: TFreeNotifier;
  GFreeCallback: TNoVclCallback = nil;
  GFreeData: Pointer = nil;
  { DLL の切り離し中は True。LCL の終了処理で起きるイベント(フォームの OnDestroy・OnHide 等)を呼び出し側へ送らない。 }
  GDetaching: Boolean = False;

procedure TCallbackBridge.DoClick(Sender: TObject);
begin
  if Assigned(FCallback) and not GDetaching then
    FCallback(Pointer(Sender), FData);
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
end;

{ TShiftState(集合型)を LongWord のビット集合(no_vcl_ss* と対応)に変換する。
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
end;

procedure TMouseCallbackBridge.DoMouse(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), Ord(Button), ShiftStateToInt(Shift), X, Y, FData);
end;

procedure TMouseMoveCallbackBridge.DoMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), ShiftStateToInt(Shift), X, Y, FData);
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

function KeyBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclKeyCallback; Data: Pointer): TKeyCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TKeyCallbackBridge) and (TKeyCallbackBridge(Current).Owner = Owner) then
    Result := TKeyCallbackBridge(Current)
  else
    Result := TKeyCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function KeyPressBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclKeyPressCallback; Data: Pointer): TKeyPressCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TKeyPressCallbackBridge) and (TKeyPressCallbackBridge(Current).Owner = Owner) then
    Result := TKeyPressCallbackBridge(Current)
  else
    Result := TKeyPressCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MouseBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclMouseCallback; Data: Pointer): TMouseCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TMouseCallbackBridge) and (TMouseCallbackBridge(Current).Owner = Owner) then
    Result := TMouseCallbackBridge(Current)
  else
    Result := TMouseCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MouseMoveBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclMouseMoveCallback; Data: Pointer): TMouseMoveCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TMouseMoveCallbackBridge) and (TMouseMoveCallbackBridge(Current).Owner = Owner) then
    Result := TMouseMoveCallbackBridge(Current)
  else
    Result := TMouseMoveCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function MouseWheelBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclMouseWheelCallback; Data: Pointer): TMouseWheelCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TMouseWheelCallbackBridge) and (TMouseWheelCallbackBridge(Current).Owner = Owner) then
    Result := TMouseWheelCallbackBridge(Current)
  else
    Result := TMouseWheelCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function VarBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclVarCallback; Data: Pointer): TVarCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TVarCallbackBridge) and (TVarCallbackBridge(Current).Owner = Owner) then
    Result := TVarCallbackBridge(Current)
  else
    Result := TVarCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

procedure TFreeNotifier.Notification(AComponent: TComponent; Operation: TOperation);
begin
  inherited Notification(AComponent, Operation);
  if (Operation = opRemove) and Assigned(GFreeCallback) then
    GFreeCallback(Pointer(AComponent), GFreeData);
end;

function BridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclCallback; Data: Pointer): TCallbackBridge;
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
end;

{ FreeNotify }

procedure FreeNotify_SetCallback(Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  GFreeCallback := Cb;
  GFreeData := Data;
end;

{ TComponent }

procedure TComponent_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TComponent(Obj).Free;
end;

{ 所有しているコンポーネントをすべて破棄する(自身は残る)。 }
procedure TComponent_DestroyComponents(Obj: Pointer); NO_VCL_CALL;
begin
  TComponent(Obj).DestroyComponents;
end;

{ TControl }

function TControl_GetParent(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TControl(Obj).Parent);
end;

procedure TControl_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TControl(Obj).Parent := TWinControl(ParentObj);
end;

function TControl_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TControl(Obj).Left;
end;

procedure TControl_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TControl(Obj).Left := Value;
end;

function TControl_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TControl(Obj).Top;
end;

procedure TControl_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TControl(Obj).Top := Value;
end;

function TControl_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TControl(Obj).Width;
end;

procedure TControl_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TControl(Obj).Width := Value;
end;

function TControl_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TControl(Obj).Height;
end;

procedure TControl_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TControl(Obj).Height := Value;
end;

function TControl_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TControl(Obj).Visible;
end;

procedure TControl_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TControl(Obj).Visible := Value;
end;

function TControl_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TControl(Obj).Enabled;
end;

procedure TControl_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TControl(Obj).Enabled := Value;
end;

function TControl_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TControl(Obj).Caption);
end;

procedure TControl_SetCaption(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TControl(Obj).Caption := Value;
end;

{ Align は TControl の public。TAlign の序数(alNone=0, alTop, alBottom, alLeft, alRight, alClient, alCustom)で受け渡す。
  既定値はクラスごとに異なる(TControl は alNone、TStatusBar は alBottom、TSplitter は alLeft)。 }
function TControl_GetAlign(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TControl(Obj).Align);
end;

procedure TControl_SetAlign(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TControl(Obj).Align := TAlign(Value);
end;

{ Text は TControl で protected。TCustomEdit と TCustomComboBox がそれぞれ公開している。 }
function TControl_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TControlAccess(Obj).Text);
end;

procedure TControl_SetText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TControlAccess(Obj).Text := Value;
end;

procedure TControl_Show(Obj: Pointer); NO_VCL_CALL;
begin
  TControl(Obj).Show;
end;

procedure TControl_Hide(Obj: Pointer); NO_VCL_CALL;
begin
  TControl(Obj).Hide;
end;

procedure TControl_SetOnClick(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControl(Obj).OnClick := @BridgeFor(TControl(Obj), MethodData(TControl(Obj).OnClick), Cb, Data).DoClick;
end;

procedure TControl_SetOnDblClick(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControlAccess(Obj).OnDblClick := @BridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnDblClick), Cb, Data).DoClick;
end;

procedure TControl_SetOnResize(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControl(Obj).OnResize := @BridgeFor(TControl(Obj), MethodData(TControl(Obj).OnResize), Cb, Data).DoClick;
end;

procedure TControl_SetOnMouseDown(Obj: Pointer; Cb: TNoVclMouseCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControlAccess(Obj).OnMouseDown := @MouseBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseDown), Cb, Data).DoMouse;
end;

procedure TControl_SetOnMouseUp(Obj: Pointer; Cb: TNoVclMouseCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControlAccess(Obj).OnMouseUp := @MouseBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseUp), Cb, Data).DoMouse;
end;

procedure TControl_SetOnMouseMove(Obj: Pointer; Cb: TNoVclMouseMoveCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControlAccess(Obj).OnMouseMove := @MouseMoveBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseMove), Cb, Data).DoMouseMove;
end;

procedure TControl_SetOnMouseEnter(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControlAccess(Obj).OnMouseEnter := @BridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseEnter), Cb, Data).DoClick;
end;

procedure TControl_SetOnMouseLeave(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControlAccess(Obj).OnMouseLeave := @BridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseLeave), Cb, Data).DoClick;
end;

procedure TControl_SetOnMouseWheel(Obj: Pointer; Cb: TNoVclMouseWheelCallback; Data: Pointer); NO_VCL_CALL;
begin
  TControlAccess(Obj).OnMouseWheel := @MouseWheelBridgeFor(TControl(Obj), MethodData(TControlAccess(Obj).OnMouseWheel), Cb, Data).DoMouseWheel;
end;

{ TWinControl }

procedure TWinControl_SetOnKeyDown(Obj: Pointer; Cb: TNoVclKeyCallback; Data: Pointer); NO_VCL_CALL;
begin
  TWinControl(Obj).OnKeyDown := @KeyBridgeFor(TWinControl(Obj), MethodData(TWinControl(Obj).OnKeyDown), Cb, Data).DoKey;
end;

procedure TWinControl_SetOnKeyUp(Obj: Pointer; Cb: TNoVclKeyCallback; Data: Pointer); NO_VCL_CALL;
begin
  TWinControl(Obj).OnKeyUp := @KeyBridgeFor(TWinControl(Obj), MethodData(TWinControl(Obj).OnKeyUp), Cb, Data).DoKey;
end;

procedure TWinControl_SetOnKeyPress(Obj: Pointer; Cb: TNoVclKeyPressCallback; Data: Pointer); NO_VCL_CALL;
begin
  TWinControl(Obj).OnKeyPress := @KeyPressBridgeFor(TWinControl(Obj), MethodData(TWinControl(Obj).OnKeyPress), Cb, Data).DoKeyPress;
end;

{ TCustomForm / TForm }

function TForm_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TForm.Create(TComponent(Owner)));
end;

procedure TCustomForm_Show(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).Show;
end;

procedure TCustomForm_Hide(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).Hide;
end;

function TCustomForm_ShowModal(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomForm(Obj).ShowModal;
end;

procedure TCustomForm_Close(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).Close;
end;

{ 保留中のメッセージを処理し終えてから破棄する(Application.ReleaseComponent)。
  フォーム自身やその子のイベントハンドラの中からでも安全に呼べる。 }
procedure TCustomForm_Release(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).Release;
end;

procedure TCustomForm_SetOnClose(Obj: Pointer; Cb: TNoVclVarCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnClose := @VarBridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnClose), Cb, Data).DoClose;
end;

procedure TCustomForm_SetOnCloseQuery(Obj: Pointer; Cb: TNoVclVarCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnCloseQuery := @VarBridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnCloseQuery), Cb, Data).DoCloseQuery;
end;

procedure TCustomForm_SetOnShow(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnShow := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnShow), Cb, Data).DoClick;
end;

procedure TCustomForm_SetOnHide(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnHide := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnHide), Cb, Data).DoClick;
end;

procedure TCustomForm_SetOnActivate(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnActivate := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnActivate), Cb, Data).DoClick;
end;

procedure TCustomForm_SetOnDeactivate(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnDeactivate := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnDeactivate), Cb, Data).DoClick;
end;

{ 破棄の最初(BeforeDestruction)で呼ばれる。子コントロールはまだ生きており、破棄通知はこの後に来る。 }
procedure TCustomForm_SetOnDestroy(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnDestroy := @BridgeFor(TComponent(Obj), MethodData(TCustomForm(Obj).OnDestroy), Cb, Data).DoClick;
end;

{ TApplication

  Application は LCL(Forms ユニット)のグローバル変数で、DLL の読み込み時に生成・初期化済み。
  LCL の TApplication は FCL の TCustomApplication から派生するが、C++Builder に合わせ
  TComponent 直下のクラスとして扱い、関数名も TApplication_* にそろえる。 }

function GetApplication: Pointer; NO_VCL_CALL;
begin
  Result := Pointer(Application);
end;

{ MainForm を設定できるのは LCL では CreateForm の中だけ(UpdateMainForm は CreateForm が
  生成中のフォームにしか効かない)ため、クラスを渡せない C/C++ 側向けに素の TForm を
  CreateForm で生成して返す。最初に生成したフォームが MainForm になる。 }
function TApplication_CreateForm(Obj: Pointer): Pointer; NO_VCL_CALL;
var
  F: TForm;
begin
  TApplication(Obj).CreateForm(TForm, F);
  Result := Watch(F);
end;

function TApplication_GetMainForm(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TApplication(Obj).MainForm);
end;

procedure TApplication_Run(Obj: Pointer); NO_VCL_CALL;
begin
  TApplication(Obj).Run;
end;

procedure TApplication_ProcessMessages(Obj: Pointer); NO_VCL_CALL;
begin
  TApplication(Obj).ProcessMessages;
end;

procedure TApplication_Terminate(Obj: Pointer); NO_VCL_CALL;
begin
  TApplication(Obj).Terminate;
end;

function TApplication_GetTerminated(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TApplication(Obj).Terminated;
end;

function TApplication_GetTitle(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TApplication(Obj).Title);
end;

procedure TApplication_SetTitle(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TApplication(Obj).Title := Value;
end;

function TApplication_GetShowMainForm(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TApplication(Obj).ShowMainForm;
end;

procedure TApplication_SetShowMainForm(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TApplication(Obj).ShowMainForm := Value;
end;

{ TPanel / TGroupBox / TLabel }

function TPanel_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TPanel.Create(TComponent(Owner)));
end;

function TGroupBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TGroupBox.Create(TComponent(Owner)));
end;

function TLabel_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TLabel.Create(TComponent(Owner)));
end;

{ TButtonControl / TButton / TCheckBox / TRadioButton }

{ Checked は TButtonControl で protected。TCheckBox と TRadioButton がそれぞれ公開している。 }
function TButtonControl_GetChecked(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TButtonControlAccess(Obj).Checked;
end;

procedure TButtonControl_SetChecked(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TButtonControlAccess(Obj).Checked := Value;
end;

function TButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TButton.Create(TComponent(Owner)));
end;

function TCheckBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TCheckBox.Create(TComponent(Owner)));
end;

function TRadioButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TRadioButton.Create(TComponent(Owner)));
end;

{ TCustomEdit / TEdit }

function TCustomEdit_GetMaxLength(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomEdit(Obj).MaxLength;
end;

procedure TCustomEdit_SetMaxLength(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomEdit(Obj).MaxLength := Value;
end;

function TCustomEdit_GetReadOnly(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomEdit(Obj).ReadOnly;
end;

procedure TCustomEdit_SetReadOnly(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomEdit(Obj).ReadOnly := Value;
end;

procedure TCustomEdit_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomEdit(Obj).OnChange := @BridgeFor(TCustomEdit(Obj), MethodData(TCustomEdit(Obj).OnChange), Cb, Data).DoClick;
end;

function TEdit_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TEdit.Create(TComponent(Owner)));
end;

{ TCustomMemo / TMemo }

procedure TCustomMemo_Lines_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TCustomMemo(Obj).Lines.Add(Text);
end;

procedure TCustomMemo_Lines_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomMemo(Obj).Lines.Clear;
end;

function TCustomMemo_Lines_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomMemo(Obj).Lines.Count;
end;

function TCustomMemo_Lines_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TCustomMemo(Obj).Lines[Index]);
end;

function TCustomMemo_GetScrollBars(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomMemo(Obj).ScrollBars);
end;

procedure TCustomMemo_SetScrollBars(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomMemo(Obj).ScrollBars := TScrollStyle(Value);
end;

function TMemo_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TMemo.Create(TComponent(Owner)));
end;

{ TCustomComboBox / TComboBox }

function TCustomComboBox_GetItemIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomComboBox(Obj).ItemIndex;
end;

procedure TCustomComboBox_SetItemIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomComboBox(Obj).ItemIndex := Value;
end;

procedure TCustomComboBox_Items_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TCustomComboBox(Obj).Items.Add(Text);
end;

procedure TCustomComboBox_Items_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomComboBox(Obj).Items.Clear;
end;

function TCustomComboBox_Items_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomComboBox(Obj).Items.Count;
end;

function TCustomComboBox_Items_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TCustomComboBox(Obj).Items[Index]);
end;

function TComboBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TComboBox.Create(TComponent(Owner)));
end;

{ OnChange は TCustomComboBox では protected で、公開しているのは TComboBox だけ。 }
procedure TComboBox_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TComboBox(Obj).OnChange := @BridgeFor(TComboBox(Obj), MethodData(TComboBox(Obj).OnChange), Cb, Data).DoClick;
end;

{ TCustomListBox / TListBox }

function TCustomListBox_GetItemIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomListBox(Obj).ItemIndex;
end;

procedure TCustomListBox_SetItemIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomListBox(Obj).ItemIndex := Value;
end;

procedure TCustomListBox_Items_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TCustomListBox(Obj).Items.Add(Text);
end;

procedure TCustomListBox_Items_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomListBox(Obj).Items.Clear;
end;

function TCustomListBox_Items_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomListBox(Obj).Items.Count;
end;

function TCustomListBox_Items_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TCustomListBox(Obj).Items[Index]);
end;

function TListBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TListBox.Create(TComponent(Owner)));
end;

{ TCustomTimer / TTimer }

function TCustomTimer_GetInterval(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomTimer(Obj).Interval;
end;

procedure TCustomTimer_SetInterval(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomTimer(Obj).Interval := Value;
end;

function TCustomTimer_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomTimer(Obj).Enabled;
end;

procedure TCustomTimer_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomTimer(Obj).Enabled := Value;
end;

procedure TCustomTimer_SetOnTimer(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomTimer(Obj).OnTimer := @BridgeFor(TCustomTimer(Obj), MethodData(TCustomTimer(Obj).OnTimer), Cb, Data).DoClick;
end;

function TTimer_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TTimer.Create(TComponent(Owner)));
end;

{ TPaintBox }

function TPaintBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TPaintBox.Create(TComponent(Owner)));
end;

function TPaintBox_GetCanvas(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPaintBox(Obj).Canvas);
end;

procedure TPaintBox_SetOnPaint(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TPaintBox(Obj).OnPaint := @BridgeFor(TPaintBox(Obj), MethodData(TPaintBox(Obj).OnPaint), Cb, Data).DoClick;
end;

{ TCanvas }
{ Canvasはコントロール(TPaintBox等)が内部で保持するオブジェクトであり、
  独自のCreate/Destroyは持たない。取得元のコントロールが破棄されれば
  一緒に破棄される。 }

procedure TCanvas_MoveTo(Obj: Pointer; X, Y: Integer); NO_VCL_CALL;
begin
  TCanvas(Obj).MoveTo(X, Y);
end;

procedure TCanvas_LineTo(Obj: Pointer; X, Y: Integer); NO_VCL_CALL;
begin
  TCanvas(Obj).LineTo(X, Y);
end;

procedure TCanvas_Rectangle(Obj: Pointer; X1, Y1, X2, Y2: Integer); NO_VCL_CALL;
begin
  TCanvas(Obj).Rectangle(X1, Y1, X2, Y2);
end;

procedure TCanvas_Ellipse(Obj: Pointer; X1, Y1, X2, Y2: Integer); NO_VCL_CALL;
begin
  TCanvas(Obj).Ellipse(X1, Y1, X2, Y2);
end;

procedure TCanvas_TextOut(Obj: Pointer; X, Y: Integer; Text: PChar); NO_VCL_CALL;
begin
  TCanvas(Obj).TextOut(X, Y, Text);
end;

function TCanvas_GetPen(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCanvas(Obj).Pen);
end;

function TCanvas_GetBrush(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCanvas(Obj).Brush);
end;

function TCanvas_GetFont(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCanvas(Obj).Font);
end;

{ TPen / TBrush / TFont (いずれも非所有) }

function TPen_GetColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TPen(Obj).Color);
end;

procedure TPen_SetColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPen(Obj).Color := TColor(Value);
end;

function TPen_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPen(Obj).Width;
end;

procedure TPen_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPen(Obj).Width := Value;
end;

function TBrush_GetColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TBrush(Obj).Color);
end;

procedure TBrush_SetColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TBrush(Obj).Color := TColor(Value);
end;

function TFont_GetName(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TFont(Obj).Name);
end;

procedure TFont_SetName(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TFont(Obj).Name := Value;
end;

function TFont_GetSize(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TFont(Obj).Size;
end;

procedure TFont_SetSize(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TFont(Obj).Size := Value;
end;

function TFont_GetColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TFont(Obj).Color);
end;

procedure TFont_SetColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TFont(Obj).Color := TColor(Value);
end;

{ docs/component-coverage.md の Tier 1 で挙げたコントロール。既存クラスの部分列として追加する。 }

{ TScrollBox: TScrollingWinControl(実装済み)の直接の派生で、追加のメンバは無い。 }
function TScrollBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TScrollBox.Create(TComponent(Owner)));
end;

{ TToggleBox: TCustomCheckBox(実装済み)の直接の派生で、追加のメンバは無い(Checked を共有)。 }
function TToggleBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TToggleBox.Create(TComponent(Owner)));
end;

{ TBevel }

function TBevel_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TBevel.Create(TComponent(Owner)));
end;

function TBevel_GetShape(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TBevel(Obj).Shape);
end;

procedure TBevel_SetShape(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TBevel(Obj).Shape := TBevelShape(Value);
end;

function TBevel_GetStyle(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TBevel(Obj).Style);
end;

procedure TBevel_SetStyle(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TBevel(Obj).Style := TBevelStyle(Value);
end;

{ TShape: Pen/Brush は TCustomShape が所有する実体で、TCanvas の Pen/Brush と同じく非所有のハンドルとして返す。 }

function TShape_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TShape.Create(TComponent(Owner)));
end;

function TCustomShape_GetShape(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomShape(Obj).Shape);
end;

procedure TCustomShape_SetShape(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomShape(Obj).Shape := TShapeType(Value);
end;

function TCustomShape_GetPen(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomShape(Obj).Pen);
end;

function TCustomShape_GetBrush(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomShape(Obj).Brush);
end;

{ TStaticText }

function TStaticText_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TStaticText.Create(TComponent(Owner)));
end;

function TCustomStaticText_GetBorderStyle(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomStaticText(Obj).BorderStyle);
end;

procedure TCustomStaticText_SetBorderStyle(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomStaticText(Obj).BorderStyle := TStaticBorderStyle(Value);
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

function TStatusBar_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TStatusBar.Create(TComponent(Owner)));
  {$ifdef LCLwin32}
  WarmUpStatusBarHeight(TStatusBar(Result));
  {$endif}
end;

function TStatusBar_GetSimpleText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStatusBar(Obj).SimpleText);
end;

procedure TStatusBar_SetSimpleText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TStatusBar(Obj).SimpleText := Value;
end;

function TStatusBar_GetSimplePanel(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TStatusBar(Obj).SimplePanel;
end;

procedure TStatusBar_SetSimplePanel(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TStatusBar(Obj).SimplePanel := Value;
end;

{ docs/component-coverage.md の Tier 1、2 バッチ目(範囲・数値系のコントロール)。
  いずれも ComCtrls のネイティブコントロール。ADR 0015 の TStatusBar の問題(推奨の高さの計測が
  Application.Run 開始前に失敗する)は TStatusBar 固有の処理によるもので、これらには関係しない。 }

{ TScrollBar }

function TScrollBar_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TScrollBar.Create(TComponent(Owner)));
end;

function TCustomScrollBar_GetKind(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomScrollBar(Obj).Kind);
end;

procedure TCustomScrollBar_SetKind(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomScrollBar(Obj).Kind := TScrollBarKind(Value);
end;

function TCustomScrollBar_GetMin(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomScrollBar(Obj).Min;
end;

procedure TCustomScrollBar_SetMin(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomScrollBar(Obj).Min := Value;
end;

function TCustomScrollBar_GetMax(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomScrollBar(Obj).Max;
end;

procedure TCustomScrollBar_SetMax(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomScrollBar(Obj).Max := Value;
end;

function TCustomScrollBar_GetPosition(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomScrollBar(Obj).Position;
end;

procedure TCustomScrollBar_SetPosition(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomScrollBar(Obj).Position := Value;
end;

function TCustomScrollBar_GetPageSize(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomScrollBar(Obj).PageSize;
end;

procedure TCustomScrollBar_SetPageSize(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomScrollBar(Obj).PageSize := Value;
end;

procedure TCustomScrollBar_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomScrollBar(Obj).OnChange := @BridgeFor(TCustomScrollBar(Obj), MethodData(TCustomScrollBar(Obj).OnChange), Cb, Data).DoClick;
end;

{ TTrackBar }

function TTrackBar_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TTrackBar.Create(TComponent(Owner)));
end;

function TCustomTrackBar_GetMin(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomTrackBar(Obj).Min;
end;

procedure TCustomTrackBar_SetMin(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomTrackBar(Obj).Min := Value;
end;

function TCustomTrackBar_GetMax(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomTrackBar(Obj).Max;
end;

procedure TCustomTrackBar_SetMax(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomTrackBar(Obj).Max := Value;
end;

function TCustomTrackBar_GetPosition(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomTrackBar(Obj).Position;
end;

procedure TCustomTrackBar_SetPosition(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomTrackBar(Obj).Position := Value;
end;

procedure TCustomTrackBar_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomTrackBar(Obj).OnChange := @BridgeFor(TCustomTrackBar(Obj), MethodData(TCustomTrackBar(Obj).OnChange), Cb, Data).DoClick;
end;

{ TProgressBar: 表示専用で、対応するイベントは無い。 }

function TProgressBar_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TProgressBar.Create(TComponent(Owner)));
end;

function TCustomProgressBar_GetMin(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomProgressBar(Obj).Min;
end;

procedure TCustomProgressBar_SetMin(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomProgressBar(Obj).Min := Value;
end;

function TCustomProgressBar_GetMax(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomProgressBar(Obj).Max;
end;

procedure TCustomProgressBar_SetMax(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomProgressBar(Obj).Max := Value;
end;

function TCustomProgressBar_GetPosition(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomProgressBar(Obj).Position;
end;

procedure TCustomProgressBar_SetPosition(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomProgressBar(Obj).Position := Value;
end;

{ TUpDown: Min/Max/Position/Increment/Associate は TCustomUpDown では protected で、
  唯一の具象クラス TUpDown が published にしている(TCheckBox の Checked と同じ形)ため、
  関数名は TUpDown_* にし、TUpDown(Obj) で直接アクセスする(protected hack は不要)。
  Associate は対象の TWinControl(TEdit 等)への参照で、TControl.Parent と同じくハンドルで表す。
  OnClick/OnChanging は独自のシグネチャ(ボタン方向・ユーザー操作かどうかを渡す)のため今回は未対応。 }

function TUpDown_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TUpDown.Create(TComponent(Owner)));
end;

function TUpDown_GetMin(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TUpDown(Obj).Min;
end;

procedure TUpDown_SetMin(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TUpDown(Obj).Min := Value;
end;

function TUpDown_GetMax(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TUpDown(Obj).Max;
end;

procedure TUpDown_SetMax(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TUpDown(Obj).Max := Value;
end;

function TUpDown_GetPosition(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TUpDown(Obj).Position;
end;

procedure TUpDown_SetPosition(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TUpDown(Obj).Position := Value;
end;

function TUpDown_GetIncrement(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TUpDown(Obj).Increment;
end;

procedure TUpDown_SetIncrement(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TUpDown(Obj).Increment := Value;
end;

function TUpDown_GetAssociate(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TUpDown(Obj).Associate);
end;

procedure TUpDown_SetAssociate(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TUpDown(Obj).Associate := TWinControl(Value);
end;

{ docs/component-coverage.md の Tier 1、4 バッチ目(Items を持つグループ・リスト系のコントロール)。 }

{ TRadioGroup: OnClick は TCustomRadioGroup 自身のフィールド(TControl.OnClick とは別)なので、
  専用のブリッジで登録する(TComboBox の OnChange と同じ理由)。 }

function TRadioGroup_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TRadioGroup.Create(TComponent(Owner)));
end;

procedure TCustomRadioGroup_Items_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TCustomRadioGroup(Obj).Items.Add(Text);
end;

procedure TCustomRadioGroup_Items_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomRadioGroup(Obj).Items.Clear;
end;

function TCustomRadioGroup_Items_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomRadioGroup(Obj).Items.Count;
end;

function TCustomRadioGroup_Items_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TCustomRadioGroup(Obj).Items[Index]);
end;

function TCustomRadioGroup_GetItemIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomRadioGroup(Obj).ItemIndex;
end;

procedure TCustomRadioGroup_SetItemIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomRadioGroup(Obj).ItemIndex := Value;
end;

procedure TCustomRadioGroup_SetOnClick(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomRadioGroup(Obj).OnClick := @BridgeFor(TCustomRadioGroup(Obj), MethodData(TCustomRadioGroup(Obj).OnClick), Cb, Data).DoClick;
end;

{ TCheckGroup: Checked はインデックス付きプロパティ。値は他のインデックス付きアクセスと同様、
  引数に Index を追加して表す(TCustomMemo_Lines_GetText 等と同じ形)。 }

function TCheckGroup_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TCheckGroup.Create(TComponent(Owner)));
end;

procedure TCustomCheckGroup_Items_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TCustomCheckGroup(Obj).Items.Add(Text);
end;

procedure TCustomCheckGroup_Items_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomCheckGroup(Obj).Items.Clear;
end;

function TCustomCheckGroup_Items_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomCheckGroup(Obj).Items.Count;
end;

function TCustomCheckGroup_Items_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TCustomCheckGroup(Obj).Items[Index]);
end;

function TCustomCheckGroup_GetChecked(Obj: Pointer; Index: Integer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCheckGroup(Obj).Checked[Index];
end;

procedure TCustomCheckGroup_SetChecked(Obj: Pointer; Index: Integer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCheckGroup(Obj).Checked[Index] := Value;
end;

{ TCheckListBox: Items は基底 TCustomListBox のものをそのまま使う(no_vcl_TCustomListBox_Items_* で
  共通)。Checked はインデックス付き。OnClickCheck は Sender のみの TNotifyEvent。 }

function TCheckListBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TCheckListBox.Create(TComponent(Owner)));
end;

function TCustomCheckListBox_GetChecked(Obj: Pointer; Index: Integer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCheckListBox(Obj).Checked[Index];
end;

procedure TCustomCheckListBox_SetChecked(Obj: Pointer; Index: Integer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCheckListBox(Obj).Checked[Index] := Value;
end;

procedure TCustomCheckListBox_SetOnClickCheck(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomCheckListBox(Obj).OnClickCheck := @BridgeFor(TCustomCheckListBox(Obj), MethodData(TCustomCheckListBox(Obj).OnClickCheck), Cb, Data).DoClick;
end;

{ docs/component-coverage.md の Tier 1、5 バッチ目(ボタンの派生)。
  Down/GroupIndex/Flat/AllowAllUp(TCustomSpeedButton)、Kind(TCustomBitBtn)はいずれも public のため
  protected hack は不要。Caption/OnClick は TControl から共有する。Glyph(ビットマップ)は未対応。 }

function TSpeedButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TSpeedButton.Create(TComponent(Owner)));
end;

function TCustomSpeedButton_GetDown(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).Down;
end;

procedure TCustomSpeedButton_SetDown(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).Down := Value;
end;

function TCustomSpeedButton_GetGroupIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).GroupIndex;
end;

procedure TCustomSpeedButton_SetGroupIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).GroupIndex := Value;
end;

function TCustomSpeedButton_GetFlat(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).Flat;
end;

procedure TCustomSpeedButton_SetFlat(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).Flat := Value;
end;

function TCustomSpeedButton_GetAllowAllUp(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).AllowAllUp;
end;

procedure TCustomSpeedButton_SetAllowAllUp(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).AllowAllUp := Value;
end;

function TBitBtn_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TBitBtn.Create(TComponent(Owner)));
end;

function TCustomBitBtn_GetKind(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomBitBtn(Obj).Kind);
end;

procedure TCustomBitBtn_SetKind(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).Kind := TBitBtnKind(Value);
end;

{ docs/component-coverage.md の Tier 1、6 バッチ目(数値・書式付き Edit)。
  TCustomSpinEdit(整数)は TCustomFloatSpinEdit(実数)の派生で、Value/MinValue/MaxValue/Increment を
  Integer で再宣言して Double 版を隠す。C++ 側でも同じ隠蔽を再現するため、関数名を宣言元のクラスごとに
  分ける(TCustomFloatSpinEdit_* は Double、TCustomSpinEdit_* は Integer)。 }

function TFloatSpinEdit_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TFloatSpinEdit.Create(TComponent(Owner)));
end;

function TCustomFloatSpinEdit_GetValue(Obj: Pointer): Double; NO_VCL_CALL;
begin
  Result := TCustomFloatSpinEdit(Obj).Value;
end;

procedure TCustomFloatSpinEdit_SetValue(Obj: Pointer; Value: Double); NO_VCL_CALL;
begin
  TCustomFloatSpinEdit(Obj).Value := Value;
end;

function TCustomFloatSpinEdit_GetMinValue(Obj: Pointer): Double; NO_VCL_CALL;
begin
  Result := TCustomFloatSpinEdit(Obj).MinValue;
end;

procedure TCustomFloatSpinEdit_SetMinValue(Obj: Pointer; Value: Double); NO_VCL_CALL;
begin
  TCustomFloatSpinEdit(Obj).MinValue := Value;
end;

function TCustomFloatSpinEdit_GetMaxValue(Obj: Pointer): Double; NO_VCL_CALL;
begin
  Result := TCustomFloatSpinEdit(Obj).MaxValue;
end;

procedure TCustomFloatSpinEdit_SetMaxValue(Obj: Pointer; Value: Double); NO_VCL_CALL;
begin
  TCustomFloatSpinEdit(Obj).MaxValue := Value;
end;

function TCustomFloatSpinEdit_GetIncrement(Obj: Pointer): Double; NO_VCL_CALL;
begin
  Result := TCustomFloatSpinEdit(Obj).Increment;
end;

procedure TCustomFloatSpinEdit_SetIncrement(Obj: Pointer; Value: Double); NO_VCL_CALL;
begin
  TCustomFloatSpinEdit(Obj).Increment := Value;
end;

function TCustomFloatSpinEdit_GetDecimalPlaces(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomFloatSpinEdit(Obj).DecimalPlaces;
end;

procedure TCustomFloatSpinEdit_SetDecimalPlaces(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomFloatSpinEdit(Obj).DecimalPlaces := Value;
end;

function TSpinEdit_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TSpinEdit.Create(TComponent(Owner)));
end;

function TCustomSpinEdit_GetValue(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpinEdit(Obj).Value;
end;

procedure TCustomSpinEdit_SetValue(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpinEdit(Obj).Value := Value;
end;

function TCustomSpinEdit_GetMinValue(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpinEdit(Obj).MinValue;
end;

procedure TCustomSpinEdit_SetMinValue(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpinEdit(Obj).MinValue := Value;
end;

function TCustomSpinEdit_GetMaxValue(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpinEdit(Obj).MaxValue;
end;

procedure TCustomSpinEdit_SetMaxValue(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpinEdit(Obj).MaxValue := Value;
end;

function TCustomSpinEdit_GetIncrement(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpinEdit(Obj).Increment;
end;

procedure TCustomSpinEdit_SetIncrement(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpinEdit(Obj).Increment := Value;
end;

{ TMaskEdit: EditMask は TCustomMaskEdit では protected だが、唯一の具象クラス TMaskEdit が
  published にしているため、TMaskEdit(Obj) で直接アクセスする(TUpDown と同じ形)。 }

function TMaskEdit_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TMaskEdit.Create(TComponent(Owner)));
end;

function TMaskEdit_GetEditMask(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TMaskEdit(Obj).EditMask);
end;

procedure TMaskEdit_SetEditMask(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TMaskEdit(Obj).EditMask := Value;
end;

{ docs/component-coverage.md の Tier 1、7 バッチ目(最後のバッチ)。
  Tabs/TabIndex/OnChange は TCustomTabControl では protected だが、唯一の具象クラス TTabControl が
  独自のフィールドで再宣言して published にしているため、TTabControl(Obj) で直接アクセスする
  (TUpDown と同じ形)。TPageControl/TTabSheet(所有ページの生成・破棄)は今回見送る。 }

function TTabControl_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TTabControl.Create(TComponent(Owner)));
end;

procedure TTabControl_Tabs_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TTabControl(Obj).Tabs.Add(Text);
end;

procedure TTabControl_Tabs_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TTabControl(Obj).Tabs.Clear;
end;

function TTabControl_Tabs_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTabControl(Obj).Tabs.Count;
end;

function TTabControl_Tabs_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TTabControl(Obj).Tabs[Index]);
end;

function TTabControl_GetTabIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTabControl(Obj).TabIndex;
end;

procedure TTabControl_SetTabIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TTabControl(Obj).TabIndex := Value;
end;

procedure TTabControl_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTabControl(Obj).OnChange := @BridgeFor(TTabControl(Obj), MethodData(TTabControl(Obj).OnChange), Cb, Data).DoClick;
end;

{ TSplitter: 隣の Align 済みコントロール(同じ Align を持つ直前のコントロール)の幅・高さをドラッグで変える。
  Align=alLeft/alRight なら縦のバー、alTop/alBottom なら横のバーになる(既定は alLeft)。
  メンバはすべて TCustomSplitter の public。OnCanResize/OnCanOffset(var 引数 2 つの独自のイベント形)は今回見送る。 }

function TSplitter_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TSplitter.Create(TComponent(Owner)));
end;

function TCustomSplitter_GetAutoSnap(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomSplitter(Obj).AutoSnap;
end;

procedure TCustomSplitter_SetAutoSnap(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomSplitter(Obj).AutoSnap := Value;
end;

function TCustomSplitter_GetBeveled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomSplitter(Obj).Beveled;
end;

procedure TCustomSplitter_SetBeveled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomSplitter(Obj).Beveled := Value;
end;

function TCustomSplitter_GetMinSize(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSplitter(Obj).MinSize;
end;

procedure TCustomSplitter_SetMinSize(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSplitter(Obj).MinSize := Value;
end;

{ TAnchorKind の序数(akTop=0, akLeft, akRight, akBottom)。 }
function TCustomSplitter_GetResizeAnchor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomSplitter(Obj).ResizeAnchor);
end;

procedure TCustomSplitter_SetResizeAnchor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSplitter(Obj).ResizeAnchor := TAnchorKind(Value);
end;

{ TResizeStyle の序数(rsLine=0, rsNone, rsPattern, rsUpdate)。 }
function TCustomSplitter_GetResizeStyle(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomSplitter(Obj).ResizeStyle);
end;

procedure TCustomSplitter_SetResizeStyle(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSplitter(Obj).ResizeStyle := TResizeStyle(Value);
end;

function TCustomSplitter_GetSplitterPosition(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSplitter(Obj).GetSplitterPosition;
end;

procedure TCustomSplitter_SetSplitterPosition(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSplitter(Obj).SetSplitterPosition(Value);
end;

procedure TCustomSplitter_SetOnMoved(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomSplitter(Obj).OnMoved := @BridgeFor(TCustomSplitter(Obj), MethodData(TCustomSplitter(Obj).OnMoved), Cb, Data).DoClick;
end;

{ docs/component-coverage.md の Tier 5(メニュー)。
  TMenu.Items(ルートの TMenuItem)は LCL が TMenu の中で生成し、*_Create を経由しない。
  このようにコンポーネントを返す関数のうち、LCL が内部で生成したものを返しうるもの(TMenu_GetItems・
  TMenuItem_GetItem・TMenuItem_GetParent)は、返す前に Watch で破棄通知の対象に登録する
  (FreeNotification は同じ相手に何度呼んでも 1 回分しか登録されない)。
  これにより、C++ 側は初めて受け取ったハンドルにラッパーを後から作っても、破棄通知で寿命を合わせられる。 }

{ ShortCut: Key は仮想キーコード、Shift は no_vcl_ss* のビット集合。VCL と同じ値(scShift=$2000 等)を返す。 }
function ShortCut_Make(Key: Integer; Shift: LongWord): Integer; NO_VCL_CALL;
begin
  Result := Menus.ShortCut(Word(Key), IntToShiftState(Shift));
end;

function ShortCut_FromText(Text: PChar): Integer; NO_VCL_CALL;
begin
  Result := TextToShortCut(Text);
end;

function ShortCut_ToText(Value: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(ShortCutToText(TShortCut(Value)));
end;

{ TMenuItem }

function TMenuItem_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TMenuItem.Create(TComponent(Owner)));
end;

function TMenuItem_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TMenuItem(Obj).Caption);
end;

procedure TMenuItem_SetCaption(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TMenuItem(Obj).Caption := Value;
end;

function TMenuItem_GetChecked(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).Checked;
end;

procedure TMenuItem_SetChecked(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMenuItem(Obj).Checked := Value;
end;

function TMenuItem_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).Enabled;
end;

procedure TMenuItem_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMenuItem(Obj).Enabled := Value;
end;

function TMenuItem_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).Visible;
end;

procedure TMenuItem_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMenuItem(Obj).Visible := Value;
end;

function TMenuItem_GetAutoCheck(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).AutoCheck;
end;

procedure TMenuItem_SetAutoCheck(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMenuItem(Obj).AutoCheck := Value;
end;

function TMenuItem_GetRadioItem(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).RadioItem;
end;

procedure TMenuItem_SetRadioItem(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMenuItem(Obj).RadioItem := Value;
end;

function TMenuItem_GetGroupIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).GroupIndex;
end;

procedure TMenuItem_SetGroupIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMenuItem(Obj).GroupIndex := Byte(Value);
end;

function TMenuItem_GetDefault(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).Default;
end;

procedure TMenuItem_SetDefault(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMenuItem(Obj).Default := Value;
end;

function TMenuItem_GetShortCut(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).ShortCut;
end;

procedure TMenuItem_SetShortCut(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMenuItem(Obj).ShortCut := TShortCut(Value);
end;

function TMenuItem_GetHint(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TMenuItem(Obj).Hint);
end;

procedure TMenuItem_SetHint(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TMenuItem(Obj).Hint := Value;
end;

procedure TMenuItem_SetOnClick(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).OnClick := @BridgeFor(TMenuItem(Obj), MethodData(TMenuItem(Obj).OnClick), Cb, Data).DoClick;
end;

function TMenuItem_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).Count;
end;

function TMenuItem_GetItem(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TMenuItem(Obj).Items[Index]);
end;

{ 親の TMenuItem(TMenu のルートの Items 直下の項目なら、そのルート)。どこにも追加されていなければ nil。 }
function TMenuItem_GetParent(Obj: Pointer): Pointer; NO_VCL_CALL;
var
  P: TMenuItem;
begin
  P := TMenuItem(Obj).Parent;
  if P = nil then
    Result := nil
  else
    Result := Watch(P);
end;

procedure TMenuItem_Add(Obj: Pointer; Item: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).Add(TMenuItem(Item));
end;

procedure TMenuItem_Insert(Obj: Pointer; Index: Integer; Item: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).Insert(Index, TMenuItem(Item));
end;

{ Delete/Remove は子から外すだけで破棄しない(VCL と同じ。破棄は Owner に任せるか、明示的に行う)。
  Clear はすべての子を破棄する。 }
procedure TMenuItem_Delete(Obj: Pointer; Index: Integer); NO_VCL_CALL;
begin
  TMenuItem(Obj).Delete(Index);
end;

procedure TMenuItem_Remove(Obj: Pointer; Item: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).Remove(TMenuItem(Item));
end;

procedure TMenuItem_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).Clear;
end;

function TMenuItem_IndexOf(Obj: Pointer; Item: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).IndexOf(TMenuItem(Item));
end;

{ 区切り線(Caption が '-' の項目)を末尾に追加する。追加される項目は LCL が内部で生成する(Owner はこの項目)。 }
procedure TMenuItem_AddSeparator(Obj: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).AddSeparator;
end;

function TMenuItem_IsLine(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).IsLine;
end;

{ 利用者が項目を選んだときと同じ処理(AutoCheck の反映と OnClick)を行う。 }
procedure TMenuItem_Click(Obj: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).Click;
end;

{ TMenu / TMainMenu / TPopupMenu }

function TMenu_GetItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TMenu(Obj).Items);
end;

function TMainMenu_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TMainMenu.Create(TComponent(Owner)));
end;

function TPopupMenu_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TPopupMenu.Create(TComponent(Owner)));
end;

{ X, Y はスクリーン座標。Win32 ではメニューが閉じるまで戻らない。 }
procedure TPopupMenu_Popup(Obj: Pointer; X, Y: Integer); NO_VCL_CALL;
begin
  TPopupMenu(Obj).PopUp(X, Y);
end;

function TPopupMenu_GetAutoPopup(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TPopupMenu(Obj).AutoPopup;
end;

procedure TPopupMenu_SetAutoPopup(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TPopupMenu(Obj).AutoPopup := Value;
end;

{ 右クリック等でメニューを開いたコントロール(OnPopup の中で、どのコントロールから開かれたかを知るのに使う)。 }
function TPopupMenu_GetPopupComponent(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPopupMenu(Obj).PopupComponent);
end;

procedure TPopupMenu_SetPopupComponent(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TPopupMenu(Obj).PopupComponent := TComponent(Value);
end;

procedure TPopupMenu_SetOnPopup(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TPopupMenu(Obj).OnPopup := @BridgeFor(TPopupMenu(Obj), MethodData(TPopupMenu(Obj).OnPopup), Cb, Data).DoClick;
end;

procedure TPopupMenu_SetOnClose(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TPopupMenu(Obj).OnClose := @BridgeFor(TPopupMenu(Obj), MethodData(TPopupMenu(Obj).OnClose), Cb, Data).DoClick;
end;

{ TCustomForm.Menu(public。TForm が published)と TControl.PopupMenu(public)。 }

function TCustomForm_GetMenu(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomForm(Obj).Menu);
end;

procedure TCustomForm_SetMenu(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).Menu := TMainMenu(Value);
end;

function TControl_GetPopupMenu(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TControl(Obj).PopupMenu);
end;

procedure TControl_SetPopupMenu(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TControl(Obj).PopupMenu := TPopupMenu(Value);
end;

{ docs/component-coverage.md の Tier 2、1 バッチ目(TPageControl + TTabSheet)。
  TPageControl は TTabControl と同じ TCustomTabControl の派生。TabIndex/OnChange は TCustomTabControl の protected を
  TPageControl が published にしているため、TPageControl(Obj) で直接アクセスする(関数名も TPageControl_*)。
  AddTabSheet が生成するページは LCL の内部で生成される(Owner はページコントロール)ため、ページを返す関数は
  TMenu_GetItems と同じく Watch してから返す(docs/adr/0017-... を参照)。 }

function TPageControl_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TPageControl.Create(TComponent(Owner)));
end;

function WatchOrNil(C: TComponent): Pointer;
begin
  if C = nil then
    Result := nil
  else
    Result := Watch(C);
end;

function TPageControl_GetActivePage(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchOrNil(TPageControl(Obj).ActivePage);
end;

procedure TPageControl_SetActivePage(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TPageControl(Obj).ActivePage := TTabSheet(Value);
end;

function TPageControl_GetActivePageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPageControl(Obj).ActivePageIndex;
end;

procedure TPageControl_SetActivePageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPageControl(Obj).ActivePageIndex := Value;
end;

function TPageControl_GetPage(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchOrNil(TPageControl(Obj).Pages[Index]);
end;

function TCustomTabControl_GetPageCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomTabControl(Obj).PageCount;
end;

function TPageControl_AddTabSheet(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TPageControl(Obj).AddTabSheet);
end;

{ すべてのページを外して破棄する。LCL の TNBPages.Delete は Application.ReleaseComponent を使うため、
  破棄は遅延され、次にメッセージを処理したとき(または Owner の破棄時)に行われる。 }
procedure TPageControl_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TPageControl(Obj).Clear;
end;

procedure TPageControl_SelectNextPage(Obj: Pointer; GoForward: LongBool); NO_VCL_CALL;
begin
  TPageControl(Obj).SelectNextPage(GoForward);
end;

function TPageControl_GetTabIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPageControl(Obj).TabIndex;
end;

procedure TPageControl_SetTabIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPageControl(Obj).TabIndex := Value;
end;

procedure TPageControl_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TPageControl(Obj).OnChange := @BridgeFor(TPageControl(Obj), MethodData(TPageControl(Obj).OnChange), Cb, Data).DoClick;
end;

{ OnChanging(TTabChangingEvent: Sender, var AllowChange)は OnCloseQuery と同じ形のため、同じブリッジ(DoCloseQuery)を使う。 }
procedure TCustomTabControl_SetOnChanging(Obj: Pointer; Cb: TNoVclVarCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomTabControl(Obj).OnChanging := @VarBridgeFor(TComponent(Obj), MethodData(TCustomTabControl(Obj).OnChanging), Cb, Data).DoCloseQuery;
end;

function TCustomTabControl_GetMultiLine(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomTabControl(Obj).MultiLine;
end;

procedure TCustomTabControl_SetMultiLine(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomTabControl(Obj).MultiLine := Value;
end;

function TCustomTabControl_GetShowTabs(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomTabControl(Obj).ShowTabs;
end;

procedure TCustomTabControl_SetShowTabs(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomTabControl(Obj).ShowTabs := Value;
end;

{ TTabPosition の序数(tpTop=0, tpBottom, tpLeft, tpRight)。 }
function TCustomTabControl_GetTabPosition(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomTabControl(Obj).TabPosition);
end;

procedure TCustomTabControl_SetTabPosition(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomTabControl(Obj).TabPosition := TTabPosition(Value);
end;

{ TTabSheet(TCustomPage の派生)。タブの文字列は Caption(TControl_SetCaption)。 }

function TTabSheet_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TTabSheet.Create(TComponent(Owner)));
end;

function TTabSheet_GetPageControl(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TTabSheet(Obj).PageControl);
end;

procedure TTabSheet_SetPageControl(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TTabSheet(Obj).PageControl := TPageControl(Value);
end;

function TTabSheet_GetTabIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTabSheet(Obj).TabIndex;
end;

function TCustomPage_GetPageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomPage(Obj).PageIndex;
end;

procedure TCustomPage_SetPageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomPage(Obj).PageIndex := Value;
end;

function TCustomPage_GetTabVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomPage(Obj).TabVisible;
end;

procedure TCustomPage_SetTabVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomPage(Obj).TabVisible := Value;
end;

procedure TCustomPage_SetOnShow(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomPage(Obj).OnShow := @BridgeFor(TCustomPage(Obj), MethodData(TCustomPage(Obj).OnShow), Cb, Data).DoClick;
end;

procedure TCustomPage_SetOnHide(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomPage(Obj).OnHide := @BridgeFor(TCustomPage(Obj), MethodData(TCustomPage(Obj).OnHide), Cb, Data).DoClick;
end;

exports
  FreeNotify_SetCallback,

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

  TCustomMemo_Lines_Add,
  TCustomMemo_Lines_Clear,
  TCustomMemo_Lines_Count,
  TCustomMemo_Lines_GetText,
  TCustomMemo_GetScrollBars,
  TCustomMemo_SetScrollBars,
  TMemo_Create,

  TCustomComboBox_GetItemIndex,
  TCustomComboBox_SetItemIndex,
  TCustomComboBox_Items_Add,
  TCustomComboBox_Items_Clear,
  TCustomComboBox_Items_Count,
  TCustomComboBox_Items_GetText,
  TComboBox_Create,
  TComboBox_SetOnChange,

  TCustomListBox_GetItemIndex,
  TCustomListBox_SetItemIndex,
  TCustomListBox_Items_Add,
  TCustomListBox_Items_Clear,
  TCustomListBox_Items_Count,
  TCustomListBox_Items_GetText,
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
  TCustomRadioGroup_Items_Add,
  TCustomRadioGroup_Items_Clear,
  TCustomRadioGroup_Items_Count,
  TCustomRadioGroup_Items_GetText,
  TCustomRadioGroup_GetItemIndex,
  TCustomRadioGroup_SetItemIndex,
  TCustomRadioGroup_SetOnClick,

  TCheckGroup_Create,
  TCustomCheckGroup_Items_Add,
  TCustomCheckGroup_Items_Clear,
  TCustomCheckGroup_Items_Count,
  TCustomCheckGroup_Items_GetText,
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

  TTabControl_Create,
  TTabControl_Tabs_Add,
  TTabControl_Tabs_Clear,
  TTabControl_Tabs_Count,
  TTabControl_Tabs_GetText,
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
  TCustomPage_SetOnHide;

begin
  RequireDerivedFormResource := False;
  Application.Initialize;
  GFreeNotifier := TFreeNotifier.Create(nil);
  { Dll_Process_Detach_Hook は Windows の DLL_PROCESS_DETACH 通知専用のフックで、
    Linux の共有ライブラリ(.so)には存在しない。 }
  {$ifdef WINDOWS}
  Dll_Process_Detach_Hook := @DetachHook;
  {$endif}
end.
