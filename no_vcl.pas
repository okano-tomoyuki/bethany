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
  ToolWin,
  CheckLst,
  Buttons,
  Spin,
  MaskEdit,
  Grids,
  AVL_Tree,
  Menus,
  LCLProc,
  Graphics,
  ImgList,
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
  { TComponent ではない項目(TTreeNode・TListItem・TListColumn・THeaderSection・TCoolBand。FreeNotification が無い)の破棄通知。
    WatchItem で付けた観察者が、項目の破棄(TPersistent.Destroy の ooFree)で呼ぶ(NotifyItemFreed)。 }
  GItemFreeCallback: TNoVclCallback = nil;
  GItemFreeData: Pointer = nil;
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
  GItemFreeCallback := nil;
  GItemFreeData := nil;
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
  { TCustomCoolBar は Align の Setter を reintroduce で差し替え(非仮想)、alLeft/alRight なら Vertical も切り替えるため、
    TControl の Setter を経由せずにその Setter を呼ぶ。 }
  if TObject(Obj) is TCustomCoolBar then
    TCustomCoolBar(Obj).Align := TAlign(Value)
  else
    TControl(Obj).Align := TAlign(Value);
end;

{ AutoSize は TControl の public(docs/adr/0029)。LCL では Align と同じく、配置はフォームの表示まで行われないことがある。 }
function TControl_GetAutoSize(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TControl(Obj).AutoSize;
end;

procedure TControl_SetAutoSize(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TControl(Obj).AutoSize := Value;
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

{ Lines(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomMemo_GetLines(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomMemo(Obj).Lines);
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

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomComboBox_GetItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomComboBox(Obj).Items);
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

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomListBox_GetItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomListBox(Obj).Items);
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

{ グラフィック(TGraphic の派生のハンドル)を描く(docs/adr/0029)。Graphic が nil なら何もしない。 }
procedure TCanvas_Draw(Obj: Pointer; X, Y: Integer; Graphic: Pointer); NO_VCL_CALL;
begin
  if Graphic <> nil then
    TCanvas(Obj).Draw(X, Y, TGraphic(Graphic));
end;

procedure TCanvas_StretchDraw(Obj: Pointer; X1, Y1, X2, Y2: Integer; Graphic: Pointer); NO_VCL_CALL;
var
  R: TRect;
begin
  if Graphic = nil then
    Exit;
  { Rect(...) は Win32 では Windows ユニットの型名に隠されるため、フィールドで組み立てる。 }
  R.Left := X1;
  R.Top := Y1;
  R.Right := X2;
  R.Bottom := Y2;
  TCanvas(Obj).StretchDraw(R, TGraphic(Graphic));
end;

{ Brush で塗りつぶす(枠は描かない)。 }
procedure TCanvas_FillRect(Obj: Pointer; X1, Y1, X2, Y2: Integer); NO_VCL_CALL;
begin
  TCanvas(Obj).FillRect(X1, Y1, X2, Y2);
end;

function TCanvas_GetPixels(Obj: Pointer; X, Y: Integer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TCanvas(Obj).Pixels[X, Y]);
end;

procedure TCanvas_SetPixels(Obj: Pointer; X, Y: Integer; Value: Integer); NO_VCL_CALL;
begin
  TCanvas(Obj).Pixels[X, Y] := TColor(Value);
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

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomRadioGroup_GetItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomRadioGroup(Obj).Items);
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
  引数に Index を追加して表す(TCustomDrawGrid_GetColWidths 等と同じ形)。 }

function TCheckGroup_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TCheckGroup.Create(TComponent(Owner)));
end;

{ Items(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TCustomCheckGroup_GetItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomCheckGroup(Obj).Items);
end;

function TCustomCheckGroup_GetChecked(Obj: Pointer; Index: Integer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCheckGroup(Obj).Checked[Index];
end;

procedure TCustomCheckGroup_SetChecked(Obj: Pointer; Index: Integer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCheckGroup(Obj).Checked[Index] := Value;
end;

{ TCheckListBox: Items は基底 TCustomListBox のものをそのまま使う(no_vcl_TCustomListBox_GetItems で
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

{ TCustomLabeledEdit(docs/adr/0028)。EditLabel(TBoundLabel)は、LCL が生成時に内部で作る子コンポーネント
  (Owner は LabeledEdit 自身。LabeledEdit と一緒に破棄される)。返すときに Watch し、C++ 側は WrapExisting でラップする
  (ADR 0017 と同じ形)。ラベルの Parent と位置は、LabeledEdit の Parent・LabelPosition・LabelSpacing に合わせて LCL が決める。 }

function TLabeledEdit_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TLabeledEdit.Create(TComponent(Owner)));
end;

function TCustomLabeledEdit_GetEditLabel(Obj: Pointer): Pointer; NO_VCL_CALL;
var
  L: TBoundLabel;
begin
  L := TCustomLabeledEdit(Obj).EditLabel;
  if L = nil then
    Result := nil
  else
    Result := Watch(L);
end;

function TCustomLabeledEdit_GetLabelPosition(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomLabeledEdit(Obj).LabelPosition);
end;

procedure TCustomLabeledEdit_SetLabelPosition(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomLabeledEdit(Obj).LabelPosition := TLabelPosition(Value);
end;

function TCustomLabeledEdit_GetLabelSpacing(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomLabeledEdit(Obj).LabelSpacing;
end;

procedure TCustomLabeledEdit_SetLabelSpacing(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomLabeledEdit(Obj).LabelSpacing := Value;
end;

{ docs/component-coverage.md の Tier 1、7 バッチ目(最後のバッチ)。
  Tabs/TabIndex/OnChange は TCustomTabControl では protected だが、唯一の具象クラス TTabControl が
  独自のフィールドで再宣言して published にしているため、TTabControl(Obj) で直接アクセスする
  (TUpDown と同じ形)。TPageControl/TTabSheet(所有ページの生成・破棄)は今回見送る。 }

function TTabControl_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TTabControl.Create(TComponent(Owner)));
end;

{ Tabs(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TTabControl_GetTabs(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TTabControl(Obj).Tabs);
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

{ docs/component-coverage.md の Tier 2、2 バッチ目(TTreeView)。docs/adr/0019-... を参照。
  TTreeNode・TTreeNodes は TComponent ではなく TPersistent で、FreeNotification が使えない。
  ノードの破棄は、ハンドルを C 側へ渡すときに付ける観察者(WatchItem)で通知する(docs/adr/0026 で ADR 0019 の方式を置き換えた)。 }

type
  { TComponent ではない項目(ツリービューのノード・リストビューの項目や列)を 1 つ受け取るイベント用。
    ツリービューの OnChange/OnExpanded/OnCollapsed/OnDeletion、リストビューの OnDeletion/OnItemChecked/OnColumnClick、
    ヘッダーコントロールの OnSectionClick/OnSectionResize/OnSectionSeparatorDblClick。
    イベントの型ごとに引数の型が異なるため、メソッドを分ける(コールバックの形は同じ)。 }
  TNoVclItemCallback = procedure(Sender: Pointer; Item: Pointer; Data: Pointer); NO_VCL_CALL;

  TItemCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclItemCallback;
    FData: Pointer;
  public
    procedure DoNode(Sender: TObject; Node: TTreeNode);
    procedure DoListItem(Sender: TObject; Item: TListItem);
    procedure DoColumn(Sender: TObject; Column: TListColumn);
    procedure DoSection(HeaderControl: TCustomHeaderControl; Section: THeaderSection);
  end;

  { 項目と整数(または真偽値)を 1 つずつ受け取るイベント用。リストビューの OnSelectItem(Selected)・OnChange(TItemChange の序数)。 }
  TNoVclItemIntCallback = procedure(Sender: Pointer; Item: Pointer; Value: Integer; Data: Pointer); NO_VCL_CALL;

  TItemIntCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclItemIntCallback;
    FData: Pointer;
  public
    procedure DoSelectItem(Sender: TObject; Item: TListItem; Selected: Boolean);
    procedure DoItemChange(Sender: TObject; Item: TListItem; Change: TItemChange);
  end;

  { OnChanging/OnExpanding/OnCollapsing(Sender, Node, var Allow)用。Allow は書き換え可能(0 = False)。 }
  TNoVclItemAllowCallback = procedure(Sender: Pointer; Node: Pointer; Allow: PInteger; Data: Pointer); NO_VCL_CALL;

  TItemAllowCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclItemAllowCallback;
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
end;

procedure TItemCallbackBridge.DoListItem(Sender: TObject; Item: TListItem);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), WatchItem(Item), FData);
end;

procedure TItemCallbackBridge.DoColumn(Sender: TObject; Column: TListColumn);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), WatchItem(Column), FData);
end;

procedure TItemCallbackBridge.DoSection(HeaderControl: TCustomHeaderControl; Section: THeaderSection);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(HeaderControl), WatchItem(Section), FData);
end;

procedure TItemIntCallbackBridge.DoSelectItem(Sender: TObject; Item: TListItem; Selected: Boolean);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if Selected then
    FCallback(Pointer(Sender), WatchItem(Item), -1, FData)
  else
    FCallback(Pointer(Sender), WatchItem(Item), 0, FData);
end;

procedure TItemIntCallbackBridge.DoItemChange(Sender: TObject; Item: TListItem; Change: TItemChange);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), WatchItem(Item), Ord(Change), FData);
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
end;

{ BridgeFor と同じく、同じイベントに何度登録してもブリッジを再利用する。
  イベントの型(TTVChangedEvent・TTVExpandedEvent 等)は構造が同じ別名の型のため、MethodData の多重定義ではなく
  呼び出し側で TMethod(...).Data を渡す。 }
function ItemBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclItemCallback; Data: Pointer): TItemCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TItemCallbackBridge) and (TItemCallbackBridge(Current).Owner = Owner) then
    Result := TItemCallbackBridge(Current)
  else
    Result := TItemCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function ItemAllowBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclItemAllowCallback; Data: Pointer): TItemAllowCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TItemAllowCallbackBridge) and (TItemAllowCallbackBridge(Current).Owner = Owner) then
    Result := TItemAllowCallbackBridge(Current)
  else
    Result := TItemAllowCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function ItemIntBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclItemIntCallback; Data: Pointer): TItemIntCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TItemIntCallbackBridge) and (TItemIntCallbackBridge(Current).Owner = Owner) then
    Result := TItemIntCallbackBridge(Current)
  else
    Result := TItemIntCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

procedure ItemFree_SetCallback(Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  GItemFreeCallback := Cb;
  GItemFreeData := Data;
end;

{ TTreeView / TCustomTreeView }

function TTreeView_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TTreeView.Create(TComponent(Owner)));
end;

{ Items(TTreeNodes)はツリービューが所有する非所有のハンドル(TCanvas と同じく、ツリービューと寿命が一致する)。 }
function TCustomTreeView_GetItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomTreeView(Obj).Items);
end;

function TCustomTreeView_GetSelected(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCustomTreeView(Obj).Selected);
end;

procedure TCustomTreeView_SetSelected(Obj: Pointer; Node: Pointer); NO_VCL_CALL;
begin
  TCustomTreeView(Obj).Selected := TTreeNode(Node);
end;

procedure TCustomTreeView_FullExpand(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomTreeView(Obj).FullExpand;
end;

procedure TCustomTreeView_FullCollapse(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomTreeView(Obj).FullCollapse;
end;

function TCustomTreeView_AlphaSort(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomTreeView(Obj).AlphaSort;
end;

{ X, Y はツリービューのクライアント座標。そこにノードが無ければ nil。 }
function TCustomTreeView_GetNodeAt(Obj: Pointer; X, Y: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCustomTreeView(Obj).GetNodeAt(X, Y));
end;

{ TCustomTreeView の protected を TTreeView が published にしているプロパティ。 }

function TTreeView_GetReadOnly(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeView(Obj).ReadOnly;
end;

procedure TTreeView_SetReadOnly(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeView(Obj).ReadOnly := Value;
end;

function TTreeView_GetShowLines(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeView(Obj).ShowLines;
end;

procedure TTreeView_SetShowLines(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeView(Obj).ShowLines := Value;
end;

function TTreeView_GetShowRoot(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeView(Obj).ShowRoot;
end;

procedure TTreeView_SetShowRoot(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeView(Obj).ShowRoot := Value;
end;

function TTreeView_GetShowButtons(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeView(Obj).ShowButtons;
end;

procedure TTreeView_SetShowButtons(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeView(Obj).ShowButtons := Value;
end;

function TTreeView_GetAutoExpand(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeView(Obj).AutoExpand;
end;

procedure TTreeView_SetAutoExpand(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeView(Obj).AutoExpand := Value;
end;

function TTreeView_GetHideSelection(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeView(Obj).HideSelection;
end;

procedure TTreeView_SetHideSelection(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeView(Obj).HideSelection := Value;
end;

function TTreeView_GetRowSelect(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeView(Obj).RowSelect;
end;

procedure TTreeView_SetRowSelect(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeView(Obj).RowSelect := Value;
end;

procedure TTreeView_SetOnChange(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTreeView(Obj).OnChange := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnChange).Data, Cb, Data).DoNode;
end;

procedure TTreeView_SetOnExpanded(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTreeView(Obj).OnExpanded := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnExpanded).Data, Cb, Data).DoNode;
end;

procedure TTreeView_SetOnCollapsed(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTreeView(Obj).OnCollapsed := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnCollapsed).Data, Cb, Data).DoNode;
end;

procedure TTreeView_SetOnDeletion(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTreeView(Obj).OnDeletion := @ItemBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnDeletion).Data, Cb, Data).DoNode;
end;

procedure TTreeView_SetOnChanging(Obj: Pointer; Cb: TNoVclItemAllowCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTreeView(Obj).OnChanging := @ItemAllowBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnChanging).Data, Cb, Data).DoNodeAllow;
end;

procedure TTreeView_SetOnExpanding(Obj: Pointer; Cb: TNoVclItemAllowCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTreeView(Obj).OnExpanding := @ItemAllowBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnExpanding).Data, Cb, Data).DoNodeAllow;
end;

procedure TTreeView_SetOnCollapsing(Obj: Pointer; Cb: TNoVclItemAllowCallback; Data: Pointer); NO_VCL_CALL;
begin
  TTreeView(Obj).OnCollapsing := @ItemAllowBridgeFor(TTreeView(Obj), TMethod(TTreeView(Obj).OnCollapsing).Data, Cb, Data).DoNodeAllow;
end;

{ TTreeNodes。Sibling/Parent に nil を渡すと最上位のノードになる(LCL と同じ)。 }

function TTreeNodes_Add(Obj: Pointer; Sibling: Pointer; Text: PChar): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).Add(TTreeNode(Sibling), Text));
end;

function TTreeNodes_AddFirst(Obj: Pointer; Sibling: Pointer; Text: PChar): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).AddFirst(TTreeNode(Sibling), Text));
end;

function TTreeNodes_AddChild(Obj: Pointer; Parent: Pointer; Text: PChar): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).AddChild(TTreeNode(Parent), Text));
end;

function TTreeNodes_AddChildFirst(Obj: Pointer; Parent: Pointer; Text: PChar): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).AddChildFirst(TTreeNode(Parent), Text));
end;

function TTreeNodes_Insert(Obj: Pointer; NextNode: Pointer; Text: PChar): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).Insert(TTreeNode(NextNode), Text));
end;

procedure TTreeNodes_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TTreeNodes(Obj).Clear;
end;

procedure TTreeNodes_Delete(Obj: Pointer; Node: Pointer); NO_VCL_CALL;
begin
  TTreeNodes(Obj).Delete(TTreeNode(Node));
end;

{ すべてのノード(子孫を含む)の数。GetItem の Index は、上から順に数えた位置(AbsoluteIndex)。 }
function TTreeNodes_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNodes(Obj).Count;
end;

function TTreeNodes_GetItem(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).Item[Index]);
end;

function TTreeNodes_GetFirstNode(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).GetFirstNode);
end;

function TTreeNodes_FindNodeWithText(Obj: Pointer; Text: PChar): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNodes(Obj).FindNodeWithText(Text));
end;

procedure TTreeNodes_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TTreeNodes(Obj).BeginUpdate;
end;

procedure TTreeNodes_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TTreeNodes(Obj).EndUpdate;
end;

{ TTreeNode }

function TTreeNode_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TTreeNode(Obj).Text);
end;

procedure TTreeNode_SetText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TTreeNode(Obj).Text := Value;
end;

function TTreeNode_GetExpanded(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).Expanded;
end;

procedure TTreeNode_SetExpanded(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeNode(Obj).Expanded := Value;
end;

function TTreeNode_GetSelected(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).Selected;
end;

procedure TTreeNode_SetSelected(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeNode(Obj).Selected := Value;
end;

function TTreeNode_GetHasChildren(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).HasChildren;
end;

procedure TTreeNode_SetHasChildren(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTreeNode(Obj).HasChildren := Value;
end;

{ 利用者データ(LCL は解釈しない)。 }
function TTreeNode_GetData(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).Data;
end;

procedure TTreeNode_SetData(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TTreeNode(Obj).Data := Value;
end;

function TTreeNode_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).Count;
end;

function TTreeNode_GetItem(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).Items[Index]);
end;

function TTreeNode_GetIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).Index;
end;

function TTreeNode_GetLevel(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).Level;
end;

function TTreeNode_GetAbsoluteIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).AbsoluteIndex;
end;

function TTreeNode_GetParent(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).Parent);
end;

function TTreeNode_GetTreeView(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TTreeNode(Obj).TreeView);
end;

function TTreeNode_GetFirstChild(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).GetFirstChild);
end;

function TTreeNode_GetLastChild(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).GetLastChild);
end;

function TTreeNode_GetNextSibling(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).GetNextSibling);
end;

function TTreeNode_GetPrevSibling(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).GetPrevSibling);
end;

{ 上から順(子孫を含む)の次/前のノード。 }
function TTreeNode_GetNext(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).GetNext);
end;

function TTreeNode_GetPrev(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TTreeNode(Obj).GetPrev);
end;

function TTreeNode_IndexOf(Obj: Pointer; Node: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).IndexOf(TTreeNode(Node));
end;

procedure TTreeNode_Expand(Obj: Pointer; Recurse: LongBool); NO_VCL_CALL;
begin
  TTreeNode(Obj).Expand(Recurse);
end;

procedure TTreeNode_Collapse(Obj: Pointer; Recurse: LongBool); NO_VCL_CALL;
begin
  TTreeNode(Obj).Collapse(Recurse);
end;

{ このノード(と子孫)を削除する。削除されたノードごとに OnDeletion と破棄通知が呼ばれる。 }
procedure TTreeNode_Delete(Obj: Pointer); NO_VCL_CALL;
begin
  TTreeNode(Obj).Delete;
end;

procedure TTreeNode_DeleteChildren(Obj: Pointer); NO_VCL_CALL;
begin
  TTreeNode(Obj).DeleteChildren;
end;

procedure TTreeNode_MakeVisible(Obj: Pointer); NO_VCL_CALL;
begin
  TTreeNode(Obj).MakeVisible;
end;

{ Mode は TNodeAttachMode の序数(naAdd=0, naAddFirst, naAddChild, naAddChildFirst, naInsert, naInsertBehind)。 }
procedure TTreeNode_MoveTo(Obj: Pointer; Destination: Pointer; Mode: Integer); NO_VCL_CALL;
begin
  TTreeNode(Obj).MoveTo(TTreeNode(Destination), TNodeAttachMode(Mode));
end;

{ docs/component-coverage.md の Tier 2、3 バッチ目(TListView)。docs/adr/0020-... を参照。
  項目(TListItem。TPersistent)と列(TListColumn。TCollectionItem)は TTreeNode と同じく、ハンドルを C 側へ渡すときに付ける
  観察者(WatchItem)で破棄を通知する(docs/adr/0026 で ADR 0020 の方式を置き換えた)。
  TCustomListView.Destroy は inherited Destroy(=破棄通知)の後で項目を破棄するため、リストビュー自身のラッパーが
  delete された後にも項目の破棄通知が届く(項目のレジストリはリストビューのラッパーに依存しないので問題ない)。 }

{ TListView / TCustomListView }

function TListView_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TListView.Create(TComponent(Owner)));
end;

{ Items(TListItems)と Columns(TListColumns)は、リストビューが所有する非所有のハンドル(リストビューと寿命が一致する)。 }
function TCustomListView_GetItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomListView(Obj).Items);
end;

function TCustomListView_GetSelected(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCustomListView(Obj).Selected);
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

procedure TCustomListView_SetSelected(Obj: Pointer; Item: Pointer); NO_VCL_CALL;
begin
  SelectListItem(TCustomListView(Obj), TListItem(Item));
end;

function TCustomListView_GetItemIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomListView(Obj).ItemIndex;
end;

{ -1 で選択を外す。範囲外の値は無視する。 }
procedure TCustomListView_SetItemIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
var
  LV: TCustomListView;
begin
  LV := TCustomListView(Obj);
  if Value = -1 then
    SelectListItem(LV, nil)
  else if (Value >= 0) and (Value < LV.Items.Count) then
    SelectListItem(LV, LV.Items[Value]);
end;

function TCustomListView_GetSelCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomListView(Obj).SelCount;
end;

function TCustomListView_GetCheckboxes(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomListView(Obj).Checkboxes;
end;

procedure TCustomListView_SetCheckboxes(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomListView(Obj).Checkboxes := Value;
end;

function TCustomListView_GetGridLines(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomListView(Obj).GridLines;
end;

procedure TCustomListView_SetGridLines(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomListView(Obj).GridLines := Value;
end;

function TCustomListView_GetMultiSelect(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomListView(Obj).MultiSelect;
end;

procedure TCustomListView_SetMultiSelect(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomListView(Obj).MultiSelect := Value;
end;

function TCustomListView_GetReadOnly(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomListView(Obj).ReadOnly;
end;

procedure TCustomListView_SetReadOnly(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomListView(Obj).ReadOnly := Value;
end;

function TCustomListView_GetRowSelect(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomListView(Obj).RowSelect;
end;

procedure TCustomListView_SetRowSelect(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomListView(Obj).RowSelect := Value;
end;

procedure TCustomListView_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomListView(Obj).Clear;
end;

procedure TCustomListView_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomListView(Obj).BeginUpdate;
end;

procedure TCustomListView_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomListView(Obj).EndUpdate;
end;

{ X, Y はクライアント座標。そこに項目が無ければ nil。 }
function TCustomListView_GetItemAt(Obj: Pointer; X, Y: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCustomListView(Obj).GetItemAt(X, Y));
end;

procedure TCustomListView_ClearSelection(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomListView(Obj).ClearSelection;
end;

procedure TCustomListView_SelectAll(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomListView(Obj).SelectAll;
end;

{ TCustomListView の protected を TListView が published にしているメンバ。 }

function TListView_GetColumns(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TListView(Obj).Columns);
end;

{ TViewStyle の序数(vsIcon=0, vsSmallIcon, vsList, vsReport)。 }
function TListView_GetViewStyle(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TListView(Obj).ViewStyle);
end;

procedure TListView_SetViewStyle(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListView(Obj).ViewStyle := TViewStyle(Value);
end;

function TListView_GetHideSelection(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListView(Obj).HideSelection;
end;

procedure TListView_SetHideSelection(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListView(Obj).HideSelection := Value;
end;

{ TSortType の序数(stNone=0, stData, stText, stBoth)。 }
function TListView_GetSortType(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TListView(Obj).SortType);
end;

procedure TListView_SetSortType(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListView(Obj).SortType := TSortType(Value);
end;

{ 並べ替えに使う列の位置(0 が Caption の列)。既定の -1 のままでは、SortType を設定しても並べ替えない(LCL の Sort)。 }
function TListView_GetSortColumn(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListView(Obj).SortColumn;
end;

procedure TListView_SetSortColumn(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListView(Obj).SortColumn := Value;
end;

{ TSortDirection の序数(sdAscending=0, sdDescending)。 }
function TListView_GetSortDirection(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TListView(Obj).SortDirection);
end;

procedure TListView_SetSortDirection(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListView(Obj).SortDirection := TSortDirection(Value);
end;

procedure TListView_SetOnSelectItem(Obj: Pointer; Cb: TNoVclItemIntCallback; Data: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).OnSelectItem := @ItemIntBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnSelectItem).Data, Cb, Data).DoSelectItem;
end;

procedure TListView_SetOnChange(Obj: Pointer; Cb: TNoVclItemIntCallback; Data: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).OnChange := @ItemIntBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnChange).Data, Cb, Data).DoItemChange;
end;

procedure TListView_SetOnDeletion(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).OnDeletion := @ItemBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnDeletion).Data, Cb, Data).DoListItem;
end;

procedure TListView_SetOnItemChecked(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).OnItemChecked := @ItemBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnItemChecked).Data, Cb, Data).DoListItem;
end;

procedure TListView_SetOnColumnClick(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).OnColumnClick := @ItemBridgeFor(TListView(Obj), TMethod(TListView(Obj).OnColumnClick).Data, Cb, Data).DoColumn;
end;

{ TListItems }

function TListItems_Add(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TListItems(Obj).Add);
end;

function TListItems_Insert(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TListItems(Obj).Insert(Index));
end;

procedure TListItems_Delete(Obj: Pointer; Index: Integer); NO_VCL_CALL;
begin
  TListItems(Obj).Delete(Index);
end;

procedure TListItems_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TListItems(Obj).Clear;
end;

function TListItems_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListItems(Obj).Count;
end;

function TListItems_GetItem(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TListItems(Obj).Item[Index]);
end;

function TListItems_IndexOf(Obj: Pointer; Item: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListItems(Obj).IndexOf(TListItem(Item));
end;

{ StartIndex の次(Inclusive なら StartIndex から)から Caption を探す。Partial なら前方一致、Wrap なら末尾から先頭へ続けて探す。 }
function TListItems_FindCaption(Obj: Pointer; StartIndex: Integer; Value: PChar; Partial, Inclusive, Wrap: LongBool): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TListItems(Obj).FindCaption(StartIndex, Value, Partial, Inclusive, Wrap));
end;

procedure TListItems_Exchange(Obj: Pointer; Index1, Index2: Integer); NO_VCL_CALL;
begin
  TListItems(Obj).Exchange(Index1, Index2);
end;

procedure TListItems_Move(Obj: Pointer; FromIndex, ToIndex: Integer); NO_VCL_CALL;
begin
  TListItems(Obj).Move(FromIndex, ToIndex);
end;

procedure TListItems_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TListItems(Obj).BeginUpdate;
end;

procedure TListItems_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TListItems(Obj).EndUpdate;
end;

{ TListItem }

function TListItem_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TListItem(Obj).Caption);
end;

procedure TListItem_SetCaption(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TListItem(Obj).Caption := Value;
end;

function TListItem_GetChecked(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListItem(Obj).Checked;
end;

procedure TListItem_SetChecked(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListItem(Obj).Checked := Value;
end;

function TListItem_GetSelected(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListItem(Obj).Selected;
end;

procedure TListItem_SetSelected(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListItem(Obj).Selected := Value;
end;

function TListItem_GetFocused(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListItem(Obj).Focused;
end;

procedure TListItem_SetFocused(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListItem(Obj).Focused := Value;
end;

function TListItem_GetData(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := TListItem(Obj).Data;
end;

procedure TListItem_SetData(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TListItem(Obj).Data := Value;
end;

function TListItem_GetIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListItem(Obj).Index;
end;

function TListItem_GetListView(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TListItem(Obj).ListView);
end;

{ SubItems(2 列目以降の文字列)。 }

{ SubItems(TStrings)。LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがあるため、ハンドルは保存せず、使うたびに取得する(docs/adr/0027)。 }
function TListItem_GetSubItems(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TListItem(Obj).SubItems);
end;

{ この項目を削除する。OnDeletion と項目の破棄通知が呼ばれる。 }
procedure TListItem_Delete(Obj: Pointer); NO_VCL_CALL;
begin
  TListItem(Obj).Delete;
end;

procedure TListItem_MakeVisible(Obj: Pointer; PartialOK: LongBool); NO_VCL_CALL;
begin
  TListItem(Obj).MakeVisible(PartialOK);
end;

{ TListColumns / TListColumn }

function TListColumns_Add(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TListColumns(Obj).Add);
end;

function TListColumns_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListColumns(Obj).Count;
end;

function TListColumns_GetItem(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TListColumns(Obj).Items[Index]);
end;

procedure TListColumns_Delete(Obj: Pointer; Index: Integer); NO_VCL_CALL;
begin
  TListColumns(Obj).Delete(Index);
end;

procedure TListColumns_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TListColumns(Obj).Clear;
end;

function TListColumn_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TListColumn(Obj).Caption);
end;

procedure TListColumn_SetCaption(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TListColumn(Obj).Caption := Value;
end;

function TListColumn_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListColumn(Obj).Width;
end;

procedure TListColumn_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListColumn(Obj).Width := Value;
end;

{ TAlignment の序数(taLeftJustify=0, taRightJustify, taCenter)。 }
function TListColumn_GetAlignment(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TListColumn(Obj).Alignment);
end;

procedure TListColumn_SetAlignment(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListColumn(Obj).Alignment := TAlignment(Value);
end;

function TListColumn_GetAutoSize(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListColumn(Obj).AutoSize;
end;

procedure TListColumn_SetAutoSize(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListColumn(Obj).AutoSize := Value;
end;

function TListColumn_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListColumn(Obj).Visible;
end;

procedure TListColumn_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListColumn(Obj).Visible := Value;
end;

{ 列の並び順。書き換えると列が移動する。 }
function TListColumn_GetIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListColumn(Obj).Index;
end;

procedure TListColumn_SetIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListColumn(Obj).Index := Value;
end;

{ docs/component-coverage.md の Tier 2、4 バッチ目(TDrawGrid・TStringGrid)。docs/adr/0021-... を参照。
  セルは LCL でもオブジェクトではなく、(列, 行)の位置で指定するため、TTreeNode・TListItem のような寿命管理は要らない。
  主なメンバは TCustomGrid の protected を TCustomDrawGrid が public にしているため、関数名は TCustomDrawGrid_* にする。 }

type
  { OnSelection(Sender, ACol, ARow)・OnHeaderClick(Sender, IsColumn, Index)用。整数 2 つを渡す。 }
  TNoVclCellCallback = procedure(Sender: Pointer; A, B: Integer; Data: Pointer); NO_VCL_CALL;

  TCellCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclCellCallback;
    FData: Pointer;
  public
    procedure DoSelection(Sender: TObject; aCol, aRow: Integer);
    procedure DoHeaderClick(Sender: TObject; IsColumn: Boolean; Index: Integer);
  end;

  { OnSelectCell(Sender, ACol, ARow, var CanSelect)用。CanSelect は書き換え可能(0 = False)。 }
  TNoVclCellAllowCallback = procedure(Sender: Pointer; ACol, ARow: Integer; Allow: PInteger; Data: Pointer); NO_VCL_CALL;

  TCellAllowCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclCellAllowCallback;
    FData: Pointer;
  public
    procedure DoSelectCell(Sender: TObject; aCol, aRow: Integer; var CanSelect: Boolean);
  end;

  { OnDrawCell(Sender, ACol, ARow, Rect, State)用。State は TGridDrawState のビット集合(no_vcl_gd*)。 }
  TNoVclDrawCellCallback = procedure(Sender: Pointer; ACol, ARow, Left, Top, Right, Bottom: Integer; State: LongWord; Data: Pointer); NO_VCL_CALL;

  TDrawCellCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclDrawCellCallback;
    FData: Pointer;
  public
    procedure DoDrawCell(Sender: TObject; aCol, aRow: Integer; aRect: TRect; aState: TGridDrawState);
  end;

procedure TCellCallbackBridge.DoSelection(Sender: TObject; aCol, aRow: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(Sender), aCol, aRow, FData);
end;

procedure TCellCallbackBridge.DoHeaderClick(Sender: TObject; IsColumn: Boolean; Index: Integer);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  if IsColumn then
    FCallback(Pointer(Sender), -1, Index, FData)
  else
    FCallback(Pointer(Sender), 0, Index, FData);
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
end;

function CellBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclCellCallback; Data: Pointer): TCellCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TCellCallbackBridge) and (TCellCallbackBridge(Current).Owner = Owner) then
    Result := TCellCallbackBridge(Current)
  else
    Result := TCellCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function CellAllowBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclCellAllowCallback; Data: Pointer): TCellAllowCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TCellAllowCallbackBridge) and (TCellAllowCallbackBridge(Current).Owner = Owner) then
    Result := TCellAllowCallbackBridge(Current)
  else
    Result := TCellAllowCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

function DrawCellBridgeFor(Owner: TComponent; Current: Pointer; Cb: TNoVclDrawCellCallback; Data: Pointer): TDrawCellCallbackBridge;
begin
  if (Current <> nil) and (TObject(Current) is TDrawCellCallbackBridge) and (TDrawCellCallbackBridge(Current).Owner = Owner) then
    Result := TDrawCellCallbackBridge(Current)
  else
    Result := TDrawCellCallbackBridge.Create(Owner);
  Result.FCallback := Cb;
  Result.FData := Data;
end;

{ TGridOptions(集合型)と LongWord のビット集合(no_vcl_go* と対応。ビットの位置は TGridOption の序数)の変換。 }
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

function TDrawGrid_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TDrawGrid.Create(TComponent(Owner)));
end;

function TStringGrid_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TStringGrid.Create(TComponent(Owner)));
end;

{ TCustomGrid の public。 }

procedure TCustomGrid_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomGrid(Obj).BeginUpdate;
end;

procedure TCustomGrid_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomGrid(Obj).EndUpdate;
end;

{ すべての行・列を削除する(ColCount・RowCount が 0 になる)。セルの文字列だけを消すのは TCustomStringGrid_Clean。 }
procedure TCustomGrid_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomGrid(Obj).Clear;
end;

{ セルのクライアント座標での矩形。 }
procedure TCustomGrid_CellRect(Obj: Pointer; ACol, ARow: Integer; Left, Top, Right, Bottom: PInteger); NO_VCL_CALL;
var
  R: TRect;
begin
  R := TCustomGrid(Obj).CellRect(ACol, ARow);
  Left^ := R.Left;
  Top^ := R.Top;
  Right^ := R.Right;
  Bottom^ := R.Bottom;
end;

{ クライアント座標 X, Y にあるセル。セルの外なら -1。 }
procedure TCustomGrid_MouseToCell(Obj: Pointer; X, Y: Integer; ACol, ARow: PInteger); NO_VCL_CALL;
var
  C, R: Longint;
begin
  TCustomGrid(Obj).MouseToCell(X, Y, C, R);
  ACol^ := C;
  ARow^ := R;
end;

{ TCustomDrawGrid の public(LCL では TCustomGrid の protected を公開したもの)。 }

function TCustomDrawGrid_GetCanvas(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomDrawGrid(Obj).Canvas);
end;

function TCustomDrawGrid_GetColCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).ColCount;
end;

procedure TCustomDrawGrid_SetColCount(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).ColCount := Value;
end;

function TCustomDrawGrid_GetRowCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).RowCount;
end;

procedure TCustomDrawGrid_SetRowCount(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).RowCount := Value;
end;

function TCustomDrawGrid_GetFixedCols(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).FixedCols;
end;

procedure TCustomDrawGrid_SetFixedCols(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).FixedCols := Value;
end;

function TCustomDrawGrid_GetFixedRows(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).FixedRows;
end;

procedure TCustomDrawGrid_SetFixedRows(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).FixedRows := Value;
end;

{ 現在のセル(フォーカスのあるセル)の列・行。 }
function TCustomDrawGrid_GetCol(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).Col;
end;

procedure TCustomDrawGrid_SetCol(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).Col := Value;
end;

function TCustomDrawGrid_GetRow(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).Row;
end;

procedure TCustomDrawGrid_SetRow(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).Row := Value;
end;

function TCustomDrawGrid_GetDefaultColWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).DefaultColWidth;
end;

procedure TCustomDrawGrid_SetDefaultColWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).DefaultColWidth := Value;
end;

function TCustomDrawGrid_GetDefaultRowHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).DefaultRowHeight;
end;

procedure TCustomDrawGrid_SetDefaultRowHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).DefaultRowHeight := Value;
end;

function TCustomDrawGrid_GetColWidths(Obj: Pointer; ACol: Integer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).ColWidths[ACol];
end;

procedure TCustomDrawGrid_SetColWidths(Obj: Pointer; ACol, Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).ColWidths[ACol] := Value;
end;

function TCustomDrawGrid_GetRowHeights(Obj: Pointer; ARow: Integer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).RowHeights[ARow];
end;

procedure TCustomDrawGrid_SetRowHeights(Obj: Pointer; ARow, Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).RowHeights[ARow] := Value;
end;

function TCustomDrawGrid_GetOptions(Obj: Pointer): LongWord; NO_VCL_CALL;
begin
  Result := GridOptionsToInt(TCustomDrawGrid(Obj).Options);
end;

procedure TCustomDrawGrid_SetOptions(Obj: Pointer; Value: LongWord); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).Options := IntToGridOptions(Value);
end;

{ 選択範囲(Left/Right が列、Top/Bottom が行。単一のセルなら Left = Right、Top = Bottom)。 }
procedure TCustomDrawGrid_GetSelection(Obj: Pointer; Left, Top, Right, Bottom: PInteger); NO_VCL_CALL;
var
  R: TGridRect;
begin
  R := TCustomDrawGrid(Obj).Selection;
  Left^ := R.Left;
  Top^ := R.Top;
  Right^ := R.Right;
  Bottom^ := R.Bottom;
end;

procedure TCustomDrawGrid_SetSelection(Obj: Pointer; Left, Top, Right, Bottom: Integer); NO_VCL_CALL;
var
  R: TGridRect;
begin
  { Rect(...) は Win32 では Windows ユニットの型名(TRect の別名)に隠されるため、フィールドで組み立てる。 }
  R.Left := Left;
  R.Top := Top;
  R.Right := Right;
  R.Bottom := Bottom;
  TCustomDrawGrid(Obj).Selection := R;
end;

function TCustomDrawGrid_GetLeftCol(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).LeftCol;
end;

procedure TCustomDrawGrid_SetLeftCol(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).LeftCol := Value;
end;

function TCustomDrawGrid_GetTopRow(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).TopRow;
end;

procedure TCustomDrawGrid_SetTopRow(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).TopRow := Value;
end;

function TCustomDrawGrid_GetDefaultDrawing(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).DefaultDrawing;
end;

procedure TCustomDrawGrid_SetDefaultDrawing(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).DefaultDrawing := Value;
end;

function TCustomDrawGrid_GetFixedColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).FixedColor;
end;

procedure TCustomDrawGrid_SetFixedColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).FixedColor := TColor(Value);
end;

function TCustomDrawGrid_GetEditorMode(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomDrawGrid(Obj).EditorMode;
end;

procedure TCustomDrawGrid_SetEditorMode(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).EditorMode := Value;
end;

procedure TCustomDrawGrid_InsertColRow(Obj: Pointer; IsColumn: LongBool; Index: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).InsertColRow(IsColumn, Index);
end;

procedure TCustomDrawGrid_DeleteColRow(Obj: Pointer; IsColumn: LongBool; Index: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).DeleteColRow(IsColumn, Index);
end;

procedure TCustomDrawGrid_MoveColRow(Obj: Pointer; IsColumn: LongBool; FromIndex, ToIndex: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).MoveColRow(IsColumn, FromIndex, ToIndex);
end;

{ IsColumn が True なら、列 Index の値で行を並べ替える(固定行は除く)。False なら行 Index の値で列を並べ替える。 }
procedure TCustomDrawGrid_SortColRow(Obj: Pointer; IsColumn: LongBool; Index: Integer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).SortColRow(IsColumn, Index);
end;

procedure TCustomDrawGrid_SetOnDrawCell(Obj: Pointer; Cb: TNoVclDrawCellCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).OnDrawCell := @DrawCellBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnDrawCell).Data, Cb, Data).DoDrawCell;
end;

procedure TCustomDrawGrid_SetOnSelectCell(Obj: Pointer; Cb: TNoVclCellAllowCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).OnSelectCell := @CellAllowBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnSelectCell).Data, Cb, Data).DoSelectCell;
end;

procedure TCustomDrawGrid_SetOnSelection(Obj: Pointer; Cb: TNoVclCellCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).OnSelection := @CellBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnSelection).Data, Cb, Data).DoSelection;
end;

procedure TCustomDrawGrid_SetOnHeaderClick(Obj: Pointer; Cb: TNoVclCellCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomDrawGrid(Obj).OnHeaderClick := @CellBridgeFor(TCustomDrawGrid(Obj), TMethod(TCustomDrawGrid(Obj).OnHeaderClick).Data, Cb, Data).DoHeaderClick;
end;

{ TCustomStringGrid の public。 }

function TCustomStringGrid_GetCells(Obj: Pointer; ACol, ARow: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TCustomStringGrid(Obj).Cells[ACol, ARow]);
end;

procedure TCustomStringGrid_SetCells(Obj: Pointer; ACol, ARow: Integer; Value: PChar); NO_VCL_CALL;
begin
  TCustomStringGrid(Obj).Cells[ACol, ARow] := Value;
end;

{ すべてのセルの文字列を消す(行・列の数は変わらない)。 }
procedure TCustomStringGrid_Clean(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomStringGrid(Obj).Clean;
end;

procedure TCustomStringGrid_AutoSizeColumns(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomStringGrid(Obj).AutoSizeColumns;
end;

procedure TCustomStringGrid_AutoSizeColumn(Obj: Pointer; ACol: Integer); NO_VCL_CALL;
begin
  TCustomStringGrid(Obj).AutoSizeColumn(ACol);
end;

{ docs/component-coverage.md の Tier 2、5 バッチ目(THeaderControl)。docs/adr/0024-... を参照。
  セクション(THeaderSection。TCollectionItem)は TListColumn と同じく、ハンドルを C 側へ渡すときに付ける観察者(WatchItem)で
  破棄を通知する(docs/adr/0026 で ADR 0024 の方式を置き換えた)。 }

type
  { OnSectionTrack(HeaderControl, Section, Width, State)用。State は TSectionTrackState の序数。 }
  TNoVclSectionTrackCallback = procedure(Sender: Pointer; Section: Pointer; Width: Integer; State: Integer; Data: Pointer); NO_VCL_CALL;

  TSectionTrackCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclSectionTrackCallback;
    FData: Pointer;
  public
    procedure DoTrack(HeaderControl: TCustomHeaderControl; Section: THeaderSection; Width: Integer; State: TSectionTrackState);
  end;

  { OnSectionDrag(Sender, FromSection, ToSection, var AllowDrag)用。Allow は書き換え可能(0 = False)。 }
  TNoVclSectionDragCallback = procedure(Sender: Pointer; FromSection: Pointer; ToSection: Pointer; Allow: PInteger; Data: Pointer); NO_VCL_CALL;

  TSectionDragCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclSectionDragCallback;
    FData: Pointer;
  public
    procedure DoDrag(Sender: TObject; FromSection, ToSection: THeaderSection; var AllowDrag: Boolean);
  end;

procedure TSectionTrackCallbackBridge.DoTrack(HeaderControl: TCustomHeaderControl; Section: THeaderSection; Width: Integer; State: TSectionTrackState);
begin
  if not Assigned(FCallback) or GDetaching then
    Exit;
  FCallback(Pointer(HeaderControl), WatchItem(Section), Width, Ord(State), FData);
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
end;

{ THeaderControl / TCustomHeaderControl }

function THeaderControl_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(THeaderControl.Create(TComponent(Owner)));
end;

{ Sections(THeaderSections)は、ヘッダーコントロールが所有する非所有のハンドル(ヘッダーコントロールと寿命が一致する)。 }
function TCustomHeaderControl_GetSections(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomHeaderControl(Obj).Sections);
end;

function TCustomHeaderControl_GetDragReorder(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomHeaderControl(Obj).DragReorder;
end;

procedure TCustomHeaderControl_SetDragReorder(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomHeaderControl(Obj).DragReorder := Value;
end;

{ Win32 では Windows ユニットの型名 Point(TPoint の別名)が Types.Point を隠すため、TPoint はフィールドで組み立てる。 }
function TCustomHeaderControl_GetSectionAt(Obj: Pointer; X, Y: Integer): Integer; NO_VCL_CALL;
var
  P: TPoint;
begin
  P.X := X;
  P.Y := Y;
  Result := TCustomHeaderControl(Obj).GetSectionAt(P);
end;

function TCustomHeaderControl_GetSectionFromOriginalIndex(Obj: Pointer; OriginalIndex: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCustomHeaderControl(Obj).SectionFromOriginalIndex[OriginalIndex]);
end;

procedure TCustomHeaderControl_SetOnSectionClick(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  THeaderControl(Obj).OnSectionClick := @ItemBridgeFor(THeaderControl(Obj), TMethod(THeaderControl(Obj).OnSectionClick).Data, Cb, Data).DoSection;
end;

procedure TCustomHeaderControl_SetOnSectionResize(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  THeaderControl(Obj).OnSectionResize := @ItemBridgeFor(THeaderControl(Obj), TMethod(THeaderControl(Obj).OnSectionResize).Data, Cb, Data).DoSection;
end;

procedure TCustomHeaderControl_SetOnSectionSeparatorDblClick(Obj: Pointer; Cb: TNoVclItemCallback; Data: Pointer); NO_VCL_CALL;
begin
  THeaderControl(Obj).OnSectionSeparatorDblClick := @ItemBridgeFor(THeaderControl(Obj), TMethod(THeaderControl(Obj).OnSectionSeparatorDblClick).Data, Cb, Data).DoSection;
end;

procedure TCustomHeaderControl_SetOnSectionTrack(Obj: Pointer; Cb: TNoVclSectionTrackCallback; Data: Pointer); NO_VCL_CALL;
var
  HC: THeaderControl;
  Bridge: TSectionTrackCallbackBridge;
begin
  HC := THeaderControl(Obj);
  if (TMethod(HC.OnSectionTrack).Data <> nil) and (TObject(TMethod(HC.OnSectionTrack).Data) is TSectionTrackCallbackBridge)
     and (TSectionTrackCallbackBridge(TMethod(HC.OnSectionTrack).Data).Owner = HC) then
    Bridge := TSectionTrackCallbackBridge(TMethod(HC.OnSectionTrack).Data)
  else
    Bridge := TSectionTrackCallbackBridge.Create(HC);
  Bridge.FCallback := Cb;
  Bridge.FData := Data;
  HC.OnSectionTrack := @Bridge.DoTrack;
end;

procedure TCustomHeaderControl_SetOnSectionDrag(Obj: Pointer; Cb: TNoVclSectionDragCallback; Data: Pointer); NO_VCL_CALL;
var
  HC: THeaderControl;
  Bridge: TSectionDragCallbackBridge;
begin
  HC := THeaderControl(Obj);
  if (TMethod(HC.OnSectionDrag).Data <> nil) and (TObject(TMethod(HC.OnSectionDrag).Data) is TSectionDragCallbackBridge)
     and (TSectionDragCallbackBridge(TMethod(HC.OnSectionDrag).Data).Owner = HC) then
    Bridge := TSectionDragCallbackBridge(TMethod(HC.OnSectionDrag).Data)
  else
    Bridge := TSectionDragCallbackBridge.Create(HC);
  Bridge.FCallback := Cb;
  Bridge.FData := Data;
  HC.OnSectionDrag := @Bridge.DoDrag;
end;

procedure TCustomHeaderControl_SetOnSectionEndDrag(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  THeaderControl(Obj).OnSectionEndDrag := @BridgeFor(THeaderControl(Obj), MethodData(THeaderControl(Obj).OnSectionEndDrag), Cb, Data).DoClick;
end;

{ THeaderSections(TCollection)。セクションを返す関数は WatchItem してから返す。 }

function THeaderSections_Add(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(THeaderSections(Obj).Add);
end;

function THeaderSections_Insert(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(THeaderSections(Obj).Insert(Index));
end;

procedure THeaderSections_Delete(Obj: Pointer; Index: Integer); NO_VCL_CALL;
begin
  THeaderSections(Obj).Delete(Index);
end;

procedure THeaderSections_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  THeaderSections(Obj).Clear;
end;

function THeaderSections_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSections(Obj).Count;
end;

function THeaderSections_GetItem(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(THeaderSections(Obj).Items[Index]);
end;

procedure THeaderSections_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  THeaderSections(Obj).BeginUpdate;
end;

procedure THeaderSections_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  THeaderSections(Obj).EndUpdate;
end;

{ THeaderSection }

function THeaderSection_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(THeaderSection(Obj).Text);
end;

procedure THeaderSection_SetText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  THeaderSection(Obj).Text := Value;
end;

function THeaderSection_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).Width;
end;

procedure THeaderSection_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  THeaderSection(Obj).Width := Value;
end;

function THeaderSection_GetMinWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).MinWidth;
end;

procedure THeaderSection_SetMinWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  THeaderSection(Obj).MinWidth := Value;
end;

function THeaderSection_GetMaxWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).MaxWidth;
end;

procedure THeaderSection_SetMaxWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  THeaderSection(Obj).MaxWidth := Value;
end;

{ TAlignment の序数(taLeftJustify = 0, taRightJustify, taCenter)。 }
function THeaderSection_GetAlignment(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(THeaderSection(Obj).Alignment);
end;

procedure THeaderSection_SetAlignment(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  THeaderSection(Obj).Alignment := TAlignment(Value);
end;

function THeaderSection_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).Visible;
end;

procedure THeaderSection_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  THeaderSection(Obj).Visible := Value;
end;

{ TCollectionItem.Index。書き換えるとセクションが移動する。 }
function THeaderSection_GetIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).Index;
end;

procedure THeaderSection_SetIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  THeaderSection(Obj).Index := Value;
end;

function THeaderSection_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).Left;
end;

function THeaderSection_GetRight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).Right;
end;

function THeaderSection_GetOriginalIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).OriginalIndex;
end;

{ docs/component-coverage.md の Tier 2、6 バッチ目(TToolBar・TToolButton)。docs/adr/0025-... を参照。
  TToolButton は TGraphicControl(TComponent)なので、他のコントロールと同じく Owner と FreeNotification で寿命を管理する。
  ツールバーへの追加は Parent にツールバーを設定することで行う(LCL の TToolButton.SetParent がツールバーに登録する)。 }

{ TToolWindow。EdgeBorders は TEdgeBorder の序数をビットの位置とするビット集合(ebLeft = 1, ebTop = 2, ...)。 }

function TToolWindow_GetEdgeBorders(Obj: Pointer): LongWord; NO_VCL_CALL;
var
  B: TEdgeBorder;
begin
  Result := 0;
  for B := Low(TEdgeBorder) to High(TEdgeBorder) do
    if B in TToolWindow(Obj).EdgeBorders then
      Result := Result or (LongWord(1) shl Ord(B));
end;

procedure TToolWindow_SetEdgeBorders(Obj: Pointer; Value: LongWord); NO_VCL_CALL;
var
  B: TEdgeBorder;
  S: TEdgeBorders;
begin
  S := [];
  for B := Low(TEdgeBorder) to High(TEdgeBorder) do
    if (Value and (LongWord(1) shl Ord(B))) <> 0 then
      Include(S, B);
  TToolWindow(Obj).EdgeBorders := S;
end;

{ TEdgeStyle の序数(esNone = 0, esRaised, esLowered)。 }
function TToolWindow_GetEdgeInner(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TToolWindow(Obj).EdgeInner);
end;

procedure TToolWindow_SetEdgeInner(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolWindow(Obj).EdgeInner := TEdgeStyle(Value);
end;

function TToolWindow_GetEdgeOuter(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TToolWindow(Obj).EdgeOuter);
end;

procedure TToolWindow_SetEdgeOuter(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolWindow(Obj).EdgeOuter := TEdgeStyle(Value);
end;

procedure TToolWindow_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TToolWindow(Obj).BeginUpdate;
end;

procedure TToolWindow_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TToolWindow(Obj).EndUpdate;
end;

{ TToolBar }

function TToolBar_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TToolBar.Create(TComponent(Owner)));
end;

function TToolBar_GetButtonCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).ButtonCount;
end;

function TToolBar_GetButton(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchOrNil(TToolBar(Obj).Buttons[Index]);
end;

function TToolBar_GetRowCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).RowCount;
end;

function TToolBar_GetButtonHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).ButtonHeight;
end;

procedure TToolBar_SetButtonHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolBar(Obj).ButtonHeight := Value;
end;

function TToolBar_GetButtonWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).ButtonWidth;
end;

procedure TToolBar_SetButtonWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolBar(Obj).ButtonWidth := Value;
end;

function TToolBar_GetDropDownWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).DropDownWidth;
end;

procedure TToolBar_SetDropDownWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolBar(Obj).DropDownWidth := Value;
end;

function TToolBar_GetIndent(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).Indent;
end;

procedure TToolBar_SetIndent(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolBar(Obj).Indent := Value;
end;

function TToolBar_GetFlat(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).Flat;
end;

procedure TToolBar_SetFlat(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolBar(Obj).Flat := Value;
end;

function TToolBar_GetList(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).List;
end;

procedure TToolBar_SetList(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolBar(Obj).List := Value;
end;

function TToolBar_GetShowCaptions(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).ShowCaptions;
end;

procedure TToolBar_SetShowCaptions(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolBar(Obj).ShowCaptions := Value;
end;

function TToolBar_GetTransparent(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).Transparent;
end;

procedure TToolBar_SetTransparent(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolBar(Obj).Transparent := Value;
end;

function TToolBar_GetWrapable(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolBar(Obj).Wrapable;
end;

procedure TToolBar_SetWrapable(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolBar(Obj).Wrapable := Value;
end;

procedure TToolBar_SetButtonSize(Obj: Pointer; NewButtonWidth, NewButtonHeight: Integer); NO_VCL_CALL;
begin
  TToolBar(Obj).SetButtonSize(NewButtonWidth, NewButtonHeight);
end;

{ TToolButton }

function TToolButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TToolButton.Create(TComponent(Owner)));
end;

function TToolButton_GetAllowAllUp(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).AllowAllUp;
end;

procedure TToolButton_SetAllowAllUp(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolButton(Obj).AllowAllUp := Value;
end;

function TToolButton_GetDown(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).Down;
end;

procedure TToolButton_SetDown(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolButton(Obj).Down := Value;
end;

function TToolButton_GetGrouped(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).Grouped;
end;

procedure TToolButton_SetGrouped(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolButton(Obj).Grouped := Value;
end;

function TToolButton_GetIndeterminate(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).Indeterminate;
end;

procedure TToolButton_SetIndeterminate(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolButton(Obj).Indeterminate := Value;
end;

function TToolButton_GetMarked(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).Marked;
end;

procedure TToolButton_SetMarked(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolButton(Obj).Marked := Value;
end;

function TToolButton_GetShowCaption(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).ShowCaption;
end;

procedure TToolButton_SetShowCaption(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolButton(Obj).ShowCaption := Value;
end;

function TToolButton_GetWrap(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).Wrap;
end;

procedure TToolButton_SetWrap(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TToolButton(Obj).Wrap := Value;
end;

{ TToolButtonStyle の序数(tbsButton = 0, tbsCheck, tbsDropDown, tbsSeparator, tbsDivider, tbsButtonDrop)。 }
function TToolButton_GetStyle(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TToolButton(Obj).Style);
end;

procedure TToolButton_SetStyle(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolButton(Obj).Style := TToolButtonStyle(Value);
end;

function TToolButton_GetDropdownMenu(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchOrNil(TToolButton(Obj).DropdownMenu);
end;

procedure TToolButton_SetDropdownMenu(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TToolButton(Obj).DropdownMenu := TPopupMenu(Value);
end;

function TToolButton_GetMenuItem(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchOrNil(TToolButton(Obj).MenuItem);
end;

procedure TToolButton_SetMenuItem(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TToolButton(Obj).MenuItem := TMenuItem(Value);
end;

{ ツールバーの中での位置(ツールバーに置かれていなければ -1)。 }
function TToolButton_GetIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).Index;
end;

procedure TToolButton_Click(Obj: Pointer); NO_VCL_CALL;
begin
  TToolButton(Obj).Click;
end;

procedure TToolButton_ArrowClick(Obj: Pointer); NO_VCL_CALL;
begin
  TToolButton(Obj).ArrowClick;
end;

function TToolButton_PointInArrow(Obj: Pointer; X, Y: Integer): LongBool; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).PointInArrow(X, Y);
end;

procedure TToolButton_SetOnArrowClick(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TToolButton(Obj).OnArrowClick := @BridgeFor(TToolButton(Obj), MethodData(TToolButton(Obj).OnArrowClick), Cb, Data).DoClick;
end;

{ docs/component-coverage.md の Tier 2、7 バッチ目(TCoolBar)。docs/adr/0026-... を参照。
  バンド(TCoolBand。TCollectionItem)は、コントロールの Parent をクールバーにしたとき(InsertControl)に LCL が内部で生成し、
  コントロールを外したとき(RemoveControl)にも内部で削除されるため、この DLL の関数を通らずに生成・破棄される。
  破棄は他の項目と同じく、ハンドルを C 側へ返すときに付ける観察者(WatchItem)で通知する(どの経路で破棄されても届く)。 }

{ TCoolBar / TCustomCoolBar。既定の Align は alTop。 }

function TCoolBar_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TCoolBar.Create(TComponent(Owner)));
end;

{ Bands(TCoolBands)は、クールバーが所有する非所有のハンドル(クールバーと寿命が一致する)。 }
function TCustomCoolBar_GetBands(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomCoolBar(Obj).Bands);
end;

procedure TCustomCoolBar_AutosizeBands(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).AutosizeBands;
end;

procedure TCustomCoolBar_MouseToBandPos(Obj: Pointer; X, Y: Integer; ABand: PInteger; AGrabber: PInteger); NO_VCL_CALL;
var
  B: Integer;
  G: Boolean;
begin
  TCustomCoolBar(Obj).MouseToBandPos(X, Y, B, G);
  ABand^ := B;
  if G then AGrabber^ := -1 else AGrabber^ := 0;
end;

function TCustomCoolBar_GetFixedSize(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).FixedSize;
end;

procedure TCustomCoolBar_SetFixedSize(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).FixedSize := Value;
end;

function TCustomCoolBar_GetFixedOrder(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).FixedOrder;
end;

procedure TCustomCoolBar_SetFixedOrder(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).FixedOrder := Value;
end;

{ TGrabStyle の序数(gsSimple = 0, gsDouble, gsHorLines, gsVerLines, gsGripper, gsButton)。 }
function TCustomCoolBar_GetGrabStyle(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomCoolBar(Obj).GrabStyle);
end;

procedure TCustomCoolBar_SetGrabStyle(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).GrabStyle := TGrabStyle(Value);
end;

function TCustomCoolBar_GetGrabWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).GrabWidth;
end;

procedure TCustomCoolBar_SetGrabWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).GrabWidth := Value;
end;

function TCustomCoolBar_GetHorizontalSpacing(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).HorizontalSpacing;
end;

procedure TCustomCoolBar_SetHorizontalSpacing(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).HorizontalSpacing := Value;
end;

function TCustomCoolBar_GetVerticalSpacing(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).VerticalSpacing;
end;

procedure TCustomCoolBar_SetVerticalSpacing(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).VerticalSpacing := Value;
end;

function TCustomCoolBar_GetShowText(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).ShowText;
end;

procedure TCustomCoolBar_SetShowText(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).ShowText := Value;
end;

function TCustomCoolBar_GetThemed(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).Themed;
end;

procedure TCustomCoolBar_SetThemed(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).Themed := Value;
end;

function TCustomCoolBar_GetVertical(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomCoolBar(Obj).Vertical;
end;

procedure TCustomCoolBar_SetVertical(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).Vertical := Value;
end;

procedure TCustomCoolBar_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).OnChange := @BridgeFor(TCustomCoolBar(Obj), MethodData(TCustomCoolBar(Obj).OnChange), Cb, Data).DoClick;
end;

{ TCoolBands(TCollection)。バンドを返す関数は WatchItem してから返す。 }

function TCoolBands_Add(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCoolBands(Obj).Add);
end;

function TCoolBands_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBands(Obj).Count;
end;

function TCoolBands_GetItem(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCoolBands(Obj).Items[Index]);
end;

procedure TCoolBands_Delete(Obj: Pointer; Index: Integer); NO_VCL_CALL;
begin
  TCoolBands(Obj).Delete(Index);
end;

procedure TCoolBands_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCoolBands(Obj).Clear;
end;

procedure TCoolBands_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCoolBands(Obj).BeginUpdate;
end;

procedure TCoolBands_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCoolBands(Obj).EndUpdate;
end;

function TCoolBands_FindBand(Obj: Pointer; AControl: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchItem(TCoolBands(Obj).FindBand(TControl(AControl)));
end;

function TCoolBands_FindBandIndex(Obj: Pointer; AControl: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBands(Obj).FindBandIndex(TControl(AControl));
end;

{ TCoolBand }

function TCoolBand_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TCoolBand(Obj).Text);
end;

procedure TCoolBand_SetText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TCoolBand(Obj).Text := Value;
end;

function TCoolBand_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Width;
end;

procedure TCoolBand_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCoolBand(Obj).Width := Value;
end;

function TCoolBand_GetMinWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).MinWidth;
end;

procedure TCoolBand_SetMinWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCoolBand(Obj).MinWidth := Value;
end;

function TCoolBand_GetMinHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).MinHeight;
end;

procedure TCoolBand_SetMinHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCoolBand(Obj).MinHeight := Value;
end;

function TCoolBand_GetBreak(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Break;
end;

procedure TCoolBand_SetBreak(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCoolBand(Obj).Break := Value;
end;

function TCoolBand_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Visible;
end;

procedure TCoolBand_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCoolBand(Obj).Visible := Value;
end;

function TCoolBand_GetFixedSize(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).FixedSize;
end;

procedure TCoolBand_SetFixedSize(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCoolBand(Obj).FixedSize := Value;
end;

function TCoolBand_GetFixedBackground(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).FixedBackground;
end;

procedure TCoolBand_SetFixedBackground(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCoolBand(Obj).FixedBackground := Value;
end;

function TCoolBand_GetHorizontalOnly(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).HorizontalOnly;
end;

procedure TCoolBand_SetHorizontalOnly(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCoolBand(Obj).HorizontalOnly := Value;
end;

function TCoolBand_GetColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TCoolBand(Obj).Color);
end;

procedure TCoolBand_SetColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCoolBand(Obj).Color := TColor(Value);
end;

function TCoolBand_GetParentColor(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).ParentColor;
end;

procedure TCoolBand_SetParentColor(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCoolBand(Obj).ParentColor := Value;
end;

{ TCollectionItem.Index。書き換えるとバンドが移動する。 }
function TCoolBand_GetIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Index;
end;

procedure TCoolBand_SetIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCoolBand(Obj).Index := Value;
end;

{ バンドに置くコントロール。設定するとそのコントロールの Parent がクールバーになり、Align は alNone になる。 }
function TCoolBand_GetControl(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := WatchOrNil(TCoolBand(Obj).Control);
end;

procedure TCoolBand_SetControl(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCoolBand(Obj).Control := TControl(Value);
end;

function TCoolBand_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Left;
end;

function TCoolBand_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Top;
end;

function TCoolBand_GetRight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Right;
end;

function TCoolBand_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).Height;
end;

procedure TCoolBand_AutosizeWidth(Obj: Pointer); NO_VCL_CALL;
begin
  TCoolBand(Obj).AutosizeWidth;
end;

{ TStrings(docs/adr/0027)。ハンドルはコントロールの Items・Lines・Tabs 等(TCustomListBox_GetItems 等)から得る。
  ハンドルは所有者の持ち物で、LCL がウィンドウの生成・破棄のときに差し替えることがあるため、呼び出し側は保存しない。 }

function TStrings_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TStrings(Obj).Count;
end;

function TStrings_GetStrings(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStrings(Obj).Strings[Index]);
end;

procedure TStrings_SetStrings(Obj: Pointer; Index: Integer; Value: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).Strings[Index] := Value;
end;

{ Objects[Index] は利用者データ(C 側のポインタ)として扱う。LCL は解釈しない(TStringList は所有しない)。 }
function TStrings_GetObjects(Obj: Pointer; Index: Integer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TStrings(Obj).Objects[Index]);
end;

procedure TStrings_SetObjects(Obj: Pointer; Index: Integer; Value: Pointer); NO_VCL_CALL;
begin
  TStrings(Obj).Objects[Index] := TObject(Value);
end;

function TStrings_Add(Obj: Pointer; S: PChar): Integer; NO_VCL_CALL;
begin
  Result := TStrings(Obj).Add(S);
end;

function TStrings_AddObject(Obj: Pointer; S: PChar; AObject: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TStrings(Obj).AddObject(S, TObject(AObject));
end;

procedure TStrings_Insert(Obj: Pointer; Index: Integer; S: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).Insert(Index, S);
end;

procedure TStrings_Delete(Obj: Pointer; Index: Integer); NO_VCL_CALL;
begin
  TStrings(Obj).Delete(Index);
end;

procedure TStrings_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TStrings(Obj).Clear;
end;

function TStrings_IndexOf(Obj: Pointer; S: PChar): Integer; NO_VCL_CALL;
begin
  Result := TStrings(Obj).IndexOf(S);
end;

procedure TStrings_Exchange(Obj: Pointer; Index1, Index2: Integer); NO_VCL_CALL;
begin
  TStrings(Obj).Exchange(Index1, Index2);
end;

procedure TStrings_Move(Obj: Pointer; CurIndex, NewIndex: Integer); NO_VCL_CALL;
begin
  TStrings(Obj).Move(CurIndex, NewIndex);
end;

procedure TStrings_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TStrings(Obj).BeginUpdate;
end;

procedure TStrings_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TStrings(Obj).EndUpdate;
end;

{ すべての行を改行でつないだ文字列。設定すると改行で分けて置き換える。 }
function TStrings_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStrings(Obj).Text);
end;

procedure TStrings_SetText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).Text := Value;
end;

function TStrings_GetCommaText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStrings(Obj).CommaText);
end;

procedure TStrings_SetCommaText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).CommaText := Value;
end;

procedure TStrings_Assign(Obj: Pointer; Source: Pointer); NO_VCL_CALL;
begin
  TStrings(Obj).Assign(TStrings(Source));
end;

procedure TStrings_AddStrings(Obj: Pointer; Source: Pointer); NO_VCL_CALL;
begin
  TStrings(Obj).AddStrings(TStrings(Source));
end;

{ 名前=値 の形の行(Names・Values・ValueFromIndex・IndexOfName)。区切りは LCL の既定の '='。 }

function TStrings_GetNames(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStrings(Obj).Names[Index]);
end;

function TStrings_GetValues(Obj: Pointer; Name: PChar): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStrings(Obj).Values[Name]);
end;

procedure TStrings_SetValues(Obj: Pointer; Name: PChar; Value: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).Values[Name] := Value;
end;

function TStrings_GetValueFromIndex(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStrings(Obj).ValueFromIndex[Index]);
end;

procedure TStrings_SetValueFromIndex(Obj: Pointer; Index: Integer; Value: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).ValueFromIndex[Index] := Value;
end;

function TStrings_IndexOfName(Obj: Pointer; Name: PChar): Integer; NO_VCL_CALL;
begin
  Result := TStrings(Obj).IndexOfName(Name);
end;

{ 任意の区切り文字の文字列(Delimiter・StrictDelimiter・DelimitedText)。 }

function TStrings_GetDelimiter(Obj: Pointer): AnsiChar; NO_VCL_CALL;
begin
  Result := TStrings(Obj).Delimiter;
end;

procedure TStrings_SetDelimiter(Obj: Pointer; Value: AnsiChar); NO_VCL_CALL;
begin
  TStrings(Obj).Delimiter := Value;
end;

function TStrings_GetStrictDelimiter(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TStrings(Obj).StrictDelimiter;
end;

procedure TStrings_SetStrictDelimiter(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TStrings(Obj).StrictDelimiter := Value;
end;

function TStrings_GetDelimitedText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := ReturnStr(TStrings(Obj).DelimitedText);
end;

procedure TStrings_SetDelimitedText(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).DelimitedText := Value;
end;

{ ファイル名・内容とも UTF-8 のまま扱う(LCL は文字列を UTF-8 として扱う)。 }

procedure TStrings_LoadFromFile(Obj: Pointer; FileName: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).LoadFromFile(FileName);
end;

procedure TStrings_SaveToFile(Obj: Pointer; FileName: PChar); NO_VCL_CALL;
begin
  TStrings(Obj).SaveToFile(FileName);
end;

{ TStringList(docs/adr/0028)。利用者が生成し、TStringList_Destroy で破棄する(TComponent ではなく、Owner も破棄通知も無い)。
  TStrings の操作は TStrings_* を使う。 }

function TStringList_Create: Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TStringList.Create);
end;

procedure TStringList_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TStringList(Obj).Free;
end;

procedure TStringList_Sort(Obj: Pointer); NO_VCL_CALL;
begin
  TStringList(Obj).Sort;
end;

{ ソートされた一覧から S を二分探索する。見つからなければ、S を挿入すべき位置を Index に入れて False を返す。
  Sorted でない一覧に使うと例外になる(LCL の仕様)。 }
function TStringList_Find(Obj: Pointer; S: PChar; Index: PInteger): LongBool; NO_VCL_CALL;
var
  I: Integer;
begin
  Result := TStringList(Obj).Find(S, I);
  if Index <> nil then
    Index^ := I;
end;

function TStringList_GetSorted(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TStringList(Obj).Sorted;
end;

procedure TStringList_SetSorted(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TStringList(Obj).Sorted := Value;
end;

function TStringList_GetDuplicates(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TStringList(Obj).Duplicates);
end;

procedure TStringList_SetDuplicates(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TStringList(Obj).Duplicates := TDuplicates(Value);
end;

function TStringList_GetCaseSensitive(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TStringList(Obj).CaseSensitive;
end;

procedure TStringList_SetCaseSensitive(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TStringList(Obj).CaseSensitive := Value;
end;

{ ---------------- グラフィックス(docs/adr/0029) ----------------
  TGraphic の派生(TBitmap・TPortableNetworkGraphic・TJPEGImage)は TPersistent で、TComponent ではない。
  *_Create で生成したものは利用者の持ち物で、TGraphic_Destroy で破棄する(TStringList と同じ)。
  TPicture.Graphic・TCustomBitBtn.Glyph 等が返すハンドルは所有者の持ち物で、LCL が差し替える
  (TPicture は LoadFromFile・Bitmap の参照・Graphic への代入のたびに中身のオブジェクトを作り直す)ため、保存してはならない。 }

procedure TGraphic_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TGraphic(Obj).Free;
end;

function TGraphic_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TGraphic(Obj).Width;
end;

procedure TGraphic_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TGraphic(Obj).Width := Value;
end;

function TGraphic_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TGraphic(Obj).Height;
end;

procedure TGraphic_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TGraphic(Obj).Height := Value;
end;

function TGraphic_GetEmpty(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TGraphic(Obj).Empty;
end;

function TGraphic_GetTransparent(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TGraphic(Obj).Transparent;
end;

procedure TGraphic_SetTransparent(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TGraphic(Obj).Transparent := Value;
end;

{ ファイル名は UTF-8。形式はクラスで決まる(TBitmap に PNG のファイルを読むと例外になる)。
  拡張子から形式を選ぶのは TPicture_LoadFromFile のほう。 }
procedure TGraphic_LoadFromFile(Obj: Pointer; FileName: PChar); NO_VCL_CALL;
begin
  TGraphic(Obj).LoadFromFile(FileName);
end;

procedure TGraphic_SaveToFile(Obj: Pointer; FileName: PChar); NO_VCL_CALL;
begin
  TGraphic(Obj).SaveToFile(FileName);
end;

{ Source はグラフィックか TPicture のハンドル。nil なら Clear と同じ。 }
procedure TGraphic_Assign(Obj: Pointer; Source: Pointer); NO_VCL_CALL;
begin
  if Source = nil then
    TGraphic(Obj).Clear
  else
    TGraphic(Obj).Assign(TPersistent(Source));
end;

procedure TGraphic_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TGraphic(Obj).Clear;
end;

{ TRasterImage の public。Canvas はグラフィックが所有し、初めて参照したときに作られる。 }
function TRasterImage_GetCanvas(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TRasterImage(Obj).Canvas);
end;

{ TPixelFormat の序数(pfDevice=0, pf1bit, pf4bit, pf8bit, pf15bit, pf16bit, pf24bit, pf32bit, pfCustom)。 }
function TRasterImage_GetPixelFormat(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TRasterImage(Obj).PixelFormat);
end;

procedure TRasterImage_SetPixelFormat(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TRasterImage(Obj).PixelFormat := TPixelFormat(Value);
end;

function TRasterImage_GetTransparentColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TRasterImage(Obj).TransparentColor);
end;

procedure TRasterImage_SetTransparentColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TRasterImage(Obj).TransparentColor := TColor(Value);
end;

{ TTransparentMode の序数(tmAuto=0, tmFixed)。 }
function TRasterImage_GetTransparentMode(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TRasterImage(Obj).TransparentMode);
end;

procedure TRasterImage_SetTransparentMode(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TRasterImage(Obj).TransparentMode := TTransparentMode(Value);
end;

procedure TCustomBitmap_SetSize(Obj: Pointer; AWidth, AHeight: Integer); NO_VCL_CALL;
begin
  TCustomBitmap(Obj).SetSize(AWidth, AHeight);
end;

{ TBitmap は Win32 では Windows ユニットの構造体(BITMAP)に隠されるため、Graphics.TBitmap と書く。 }
function TBitmap_Create: Pointer; NO_VCL_CALL;
begin
  Result := Pointer(Graphics.TBitmap.Create);
end;

function TPortableNetworkGraphic_Create: Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPortableNetworkGraphic.Create);
end;

function TJPEGImage_Create: Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TJPEGImage.Create);
end;

{ 保存するときの品質(1〜100。既定は 75)。 }
function TJPEGImage_GetCompressionQuality(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TJPEGImage(Obj).CompressionQuality;
end;

procedure TJPEGImage_SetCompressionQuality(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TJPEGImage(Obj).CompressionQuality := TJPEGQualityRange(Value);
end;

{ TPicture。TCustomImage.Picture は画像コントロールが所有する(生成時に作られ、差し替わらない)。
  TPicture_Create で生成したものは利用者の持ち物で、TPicture_Destroy で破棄する。 }

function TPicture_Create: Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPicture.Create);
end;

procedure TPicture_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TPicture(Obj).Free;
end;

{ 空なら nil。 }
function TPicture_GetGraphic(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPicture(Obj).Graphic);
end;

{ Value と同じクラスのグラフィックを作って内容を写す(Value はそのまま呼び出し側の持ち物)。nil なら空にする。 }
procedure TPicture_SetGraphic(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TPicture(Obj).Graphic := TGraphic(Value);
end;

{ 中身がそのクラスでなければ、そのクラスに変換する(中身のオブジェクトが作り直される。空なら空のものを作る)。 }
function TPicture_GetBitmap(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPicture(Obj).Bitmap);
end;

function TPicture_GetPNG(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPicture(Obj).PNG);
end;

function TPicture_GetJpeg(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPicture(Obj).Jpeg);
end;

function TPicture_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPicture(Obj).Width;
end;

function TPicture_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPicture(Obj).Height;
end;

{ 拡張子から形式(クラス)を選んで読み込む。ファイル名は UTF-8。 }
procedure TPicture_LoadFromFile(Obj: Pointer; FileName: PChar); NO_VCL_CALL;
begin
  TPicture(Obj).LoadFromFile(FileName);
end;

procedure TPicture_SaveToFile(Obj: Pointer; FileName: PChar); NO_VCL_CALL;
begin
  TPicture(Obj).SaveToFile(FileName);
end;

{ Source は TPicture かグラフィックのハンドル。nil なら空にする。 }
procedure TPicture_Assign(Obj: Pointer; Source: Pointer); NO_VCL_CALL;
begin
  TPicture(Obj).Assign(TPersistent(Source));
end;

procedure TPicture_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TPicture(Obj).Clear;
end;

{ TImage(TCustomImage)。 }

function TImage_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TImage.Create(TComponent(Owner)));
end;

function TCustomImage_GetPicture(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomImage(Obj).Picture);
end;

{ Value(TPicture)の内容を写す。 }
procedure TCustomImage_SetPicture(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomImage(Obj).Picture := TPicture(Value);
end;

{ Picture が空なら、コントロールの大きさの TBitmap を作ってからその Canvas を返す。
  中身がビットマップの類でない(アイコン等)なら、コントロール自身の Canvas を返す。 }
function TCustomImage_GetCanvas(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomImage(Obj).Canvas);
end;

function TCustomImage_GetHasGraphic(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).HasGraphic;
end;

function TCustomImage_GetCenter(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).Center;
end;

procedure TCustomImage_SetCenter(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomImage(Obj).Center := Value;
end;

function TCustomImage_GetStretch(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).Stretch;
end;

procedure TCustomImage_SetStretch(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomImage(Obj).Stretch := Value;
end;

function TCustomImage_GetStretchOutEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).StretchOutEnabled;
end;

procedure TCustomImage_SetStretchOutEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomImage(Obj).StretchOutEnabled := Value;
end;

function TCustomImage_GetStretchInEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).StretchInEnabled;
end;

procedure TCustomImage_SetStretchInEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomImage(Obj).StretchInEnabled := Value;
end;

function TCustomImage_GetProportional(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).Proportional;
end;

procedure TCustomImage_SetProportional(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomImage(Obj).Proportional := Value;
end;

function TCustomImage_GetTransparent(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).Transparent;
end;

procedure TCustomImage_SetTransparent(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomImage(Obj).Transparent := Value;
end;

{ Picture(またはその中身)が変わったときに呼ばれる。 }
procedure TCustomImage_SetOnPictureChanged(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomImage(Obj).OnPictureChanged := @BridgeFor(TCustomImage(Obj), MethodData(TCustomImage(Obj).OnPictureChanged), Cb, Data).DoClick;
end;

{ TCustomBitBtn・TCustomSpeedButton の Glyph。ボタンが所有する TBitmap(差し替わらない)を返す。
  Set は Value の内容を写す(nil なら空にする)。NumGlyphs は、横に並べた状態別の画像の数(1〜4)。
  Layout は TButtonLayout の序数(blGlyphLeft=0, blGlyphRight, blGlyphTop, blGlyphBottom)。
  Margin は端から画像までの距離(-1 なら画像と文字列を中央に置く)、Spacing は画像と文字列の間隔。 }

function TCustomBitBtn_GetGlyph(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomBitBtn(Obj).Glyph);
end;

procedure TCustomBitBtn_SetGlyph(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).Glyph := Graphics.TBitmap(Value);
end;

function TCustomBitBtn_GetNumGlyphs(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomBitBtn(Obj).NumGlyphs;
end;

procedure TCustomBitBtn_SetNumGlyphs(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).NumGlyphs := Value;
end;

function TCustomBitBtn_GetLayout(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomBitBtn(Obj).Layout);
end;

procedure TCustomBitBtn_SetLayout(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).Layout := TButtonLayout(Value);
end;

function TCustomBitBtn_GetMargin(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomBitBtn(Obj).Margin;
end;

procedure TCustomBitBtn_SetMargin(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).Margin := Value;
end;

function TCustomBitBtn_GetSpacing(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomBitBtn(Obj).Spacing;
end;

procedure TCustomBitBtn_SetSpacing(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).Spacing := Value;
end;

function TCustomSpeedButton_GetGlyph(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomSpeedButton(Obj).Glyph);
end;

procedure TCustomSpeedButton_SetGlyph(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).Glyph := Graphics.TBitmap(Value);
end;

function TCustomSpeedButton_GetNumGlyphs(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).NumGlyphs;
end;

procedure TCustomSpeedButton_SetNumGlyphs(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).NumGlyphs := Value;
end;

function TCustomSpeedButton_GetLayout(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomSpeedButton(Obj).Layout);
end;

procedure TCustomSpeedButton_SetLayout(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).Layout := TButtonLayout(Value);
end;

function TCustomSpeedButton_GetMargin(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).Margin;
end;

procedure TCustomSpeedButton_SetMargin(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).Margin := Value;
end;

function TCustomSpeedButton_GetSpacing(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).Spacing;
end;

procedure TCustomSpeedButton_SetSpacing(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).Spacing := Value;
end;

{ ---------------- TImageList(docs/adr/0030) ----------------
  TCustomImageList は TComponent(TLCLComponent)なので、生成・破棄は他のコンポーネントと同じ(Watch して返し、Owner に任せてよい)。
  画像を受け取る関数は、画像を写して加える(渡したグラフィックは呼び出し側の持ち物のまま)。
  Add・Insert 等は、画像を Width・Height の大きさに伸縮して 1 つとして加える(VCL と違い、幅が Width の倍数でも分けない)。
  横に並んだ複数の画像を分けて加えるのは AddSliced。 }

function TImageList_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TImageList.Create(TComponent(Owner)));
end;

function TCustomImageList_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomImageList(Obj).Width;
end;

procedure TCustomImageList_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Width := Value;
end;

function TCustomImageList_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomImageList(Obj).Height;
end;

procedure TCustomImageList_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Height := Value;
end;

function TCustomImageList_GetCount(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomImageList(Obj).Count;
end;

function TCustomImageList_GetMasked(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCustomImageList(Obj).Masked;
end;

procedure TCustomImageList_SetMasked(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Masked := Value;
end;

function TCustomImageList_GetBkColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TCustomImageList(Obj).BkColor);
end;

procedure TCustomImageList_SetBkColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).BkColor := TColor(Value);
end;

{ TDrawingStyle の序数(dsFocus=0, dsSelected, dsNormal, dsTransparent)。 }
function TCustomImageList_GetDrawingStyle(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TCustomImageList(Obj).DrawingStyle);
end;

procedure TCustomImageList_SetDrawingStyle(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).DrawingStyle := TDrawingStyle(Value);
end;

{ Image・Mask は TCustomBitmap の派生(TBitmap・TPortableNetworkGraphic・TJPEGImage)のハンドル。Mask は nil でよい。
  加えた最初の画像の位置を返す。 }
function TCustomImageList_Add(Obj: Pointer; Image, Mask: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomImageList(Obj).Add(TCustomBitmap(Image), TCustomBitmap(Mask));
end;

{ Image を横 AHorizontalCount・縦 AVerticalCount に分けて、それぞれを画像として加える。加えた最初の画像の位置を返す。 }
function TCustomImageList_AddSliced(Obj: Pointer; Image: Pointer; AHorizontalCount, AVerticalCount: Integer): Integer; NO_VCL_CALL;
begin
  Result := TCustomImageList(Obj).AddSliced(TCustomBitmap(Image), AHorizontalCount, AVerticalCount);
end;

{ MaskColor の画素を透明として加える。Image は TBitmap のハンドル。 }
function TCustomImageList_AddMasked(Obj: Pointer; Image: Pointer; MaskColor: Integer): Integer; NO_VCL_CALL;
begin
  Result := TCustomImageList(Obj).AddMasked(Graphics.TBitmap(Image), TColor(MaskColor));
end;

procedure TCustomImageList_Insert(Obj: Pointer; Index: Integer; Image, Mask: Pointer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Insert(Index, TCustomBitmap(Image), TCustomBitmap(Mask));
end;

procedure TCustomImageList_Replace(Obj: Pointer; Index: Integer; Image, Mask: Pointer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Replace(Index, TCustomBitmap(Image), TCustomBitmap(Mask));
end;

procedure TCustomImageList_Delete(Obj: Pointer; Index: Integer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Delete(Index);
end;

procedure TCustomImageList_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Clear;
end;

procedure TCustomImageList_Move(Obj: Pointer; CurIndex, NewIndex: Integer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Move(CurIndex, NewIndex);
end;

{ Index 番目の画像を Image(TCustomBitmap の派生)に写す。 }
procedure TCustomImageList_GetBitmap(Obj: Pointer; Index: Integer; Image: Pointer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).GetBitmap(Index, TCustomBitmap(Image));
end;

procedure TCustomImageList_Draw(Obj: Pointer; Canvas: Pointer; X, Y, Index: Integer; Enabled: LongBool); NO_VCL_CALL;
begin
  TCustomImageList(Obj).Draw(TCanvas(Canvas), X, Y, Index, Boolean(Enabled));
end;

procedure TCustomImageList_BeginUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).BeginUpdate;
end;

procedure TCustomImageList_EndUpdate(Obj: Pointer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).EndUpdate;
end;

{ Clear・Delete・Move・BkColor の変更で呼ばれる(LCL の仕様で、Add・Insert 等では呼ばれない。BeginUpdate の間は EndUpdate まで遅れる)。 }
procedure TCustomImageList_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomImageList(Obj).OnChange := @BridgeFor(TCustomImageList(Obj), MethodData(TCustomImageList(Obj).OnChange), Cb, Data).DoClick;
end;

{ 各コントロール・項目の Images・ImageIndex・Bitmap(docs/adr/0030)。
  Images は TCustomImageList のハンドル(nil なら画像リストを外す)。LCL は画像リストの破棄を FreeNotification で受け、
  コントロールの Images を nil に戻す。ImageIndex は画像リストでの位置(-1 なら無し)。
  Bitmap は所有者が持つ TBitmap(差し替わらない)を返し、Set は内容を写す。 }

function TCustomImage_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomImage(Obj).Images);
end;

procedure TCustomImage_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomImage(Obj).Images := TCustomImageList(Value);
end;

function TCustomImage_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomImage(Obj).ImageIndex;
end;

procedure TCustomImage_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomImage(Obj).ImageIndex := Value;
end;

function TCustomBitBtn_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomBitBtn(Obj).Images);
end;

procedure TCustomBitBtn_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).Images := TCustomImageList(Value);
end;

function TCustomBitBtn_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomBitBtn(Obj).ImageIndex;
end;

procedure TCustomBitBtn_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomBitBtn(Obj).ImageIndex := Value;
end;

function TCustomSpeedButton_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomSpeedButton(Obj).Images);
end;

procedure TCustomSpeedButton_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).Images := TCustomImageList(Value);
end;

function TCustomSpeedButton_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomSpeedButton(Obj).ImageIndex;
end;

procedure TCustomSpeedButton_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomSpeedButton(Obj).ImageIndex := Value;
end;

function TCustomTabControl_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomTabControl(Obj).Images);
end;

procedure TCustomTabControl_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomTabControl(Obj).Images := TCustomImageList(Value);
end;

function TCustomPage_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCustomPage(Obj).ImageIndex;
end;

procedure TCustomPage_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCustomPage(Obj).ImageIndex := Value;
end;

function TCustomTreeView_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomTreeView(Obj).Images);
end;

procedure TCustomTreeView_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomTreeView(Obj).Images := TCustomImageList(Value);
end;

function TCustomTreeView_GetStateImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomTreeView(Obj).StateImages);
end;

procedure TCustomTreeView_SetStateImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomTreeView(Obj).StateImages := TCustomImageList(Value);
end;

function TTreeNode_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).ImageIndex;
end;

procedure TTreeNode_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TTreeNode(Obj).ImageIndex := Value;
end;

function TTreeNode_GetSelectedIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).SelectedIndex;
end;

procedure TTreeNode_SetSelectedIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TTreeNode(Obj).SelectedIndex := Value;
end;

function TTreeNode_GetStateIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).StateIndex;
end;

procedure TTreeNode_SetStateIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TTreeNode(Obj).StateIndex := Value;
end;

function TTreeNode_GetOverlayIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTreeNode(Obj).OverlayIndex;
end;

procedure TTreeNode_SetOverlayIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TTreeNode(Obj).OverlayIndex := Value;
end;

function TListView_GetLargeImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TListView(Obj).LargeImages);
end;

procedure TListView_SetLargeImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).LargeImages := TCustomImageList(Value);
end;

function TListView_GetSmallImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TListView(Obj).SmallImages);
end;

procedure TListView_SetSmallImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).SmallImages := TCustomImageList(Value);
end;

function TListView_GetStateImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TListView(Obj).StateImages);
end;

procedure TListView_SetStateImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TListView(Obj).StateImages := TCustomImageList(Value);
end;

function TListItem_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListItem(Obj).ImageIndex;
end;

procedure TListItem_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListItem(Obj).ImageIndex := Value;
end;

function TListItem_GetStateIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListItem(Obj).StateIndex;
end;

procedure TListItem_SetStateIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListItem(Obj).StateIndex := Value;
end;

function TListColumn_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListColumn(Obj).ImageIndex;
end;

procedure TListColumn_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListColumn(Obj).ImageIndex := Value;
end;

function TToolBar_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TToolBar(Obj).Images);
end;

procedure TToolBar_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TToolBar(Obj).Images := TCustomImageList(Value);
end;

function TToolBar_GetHotImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TToolBar(Obj).HotImages);
end;

procedure TToolBar_SetHotImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TToolBar(Obj).HotImages := TCustomImageList(Value);
end;

function TToolBar_GetDisabledImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TToolBar(Obj).DisabledImages);
end;

procedure TToolBar_SetDisabledImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TToolBar(Obj).DisabledImages := TCustomImageList(Value);
end;

function TToolButton_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TToolButton(Obj).ImageIndex;
end;

procedure TToolButton_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TToolButton(Obj).ImageIndex := Value;
end;

function TCustomHeaderControl_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomHeaderControl(Obj).Images);
end;

procedure TCustomHeaderControl_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomHeaderControl(Obj).Images := TCustomImageList(Value);
end;

function THeaderSection_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := THeaderSection(Obj).ImageIndex;
end;

procedure THeaderSection_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  THeaderSection(Obj).ImageIndex := Value;
end;

function TCustomCoolBar_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomCoolBar(Obj).Images);
end;

procedure TCustomCoolBar_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).Images := TCustomImageList(Value);
end;

function TCustomCoolBar_GetBitmap(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCustomCoolBar(Obj).Bitmap);
end;

procedure TCustomCoolBar_SetBitmap(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCustomCoolBar(Obj).Bitmap := Graphics.TBitmap(Value);
end;

function TCoolBand_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCoolBand(Obj).ImageIndex;
end;

procedure TCoolBand_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCoolBand(Obj).ImageIndex := Value;
end;

function TCoolBand_GetBitmap(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCoolBand(Obj).Bitmap);
end;

procedure TCoolBand_SetBitmap(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TCoolBand(Obj).Bitmap := Graphics.TBitmap(Value);
end;

function TMenu_GetImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TMenu(Obj).Images);
end;

procedure TMenu_SetImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TMenu(Obj).Images := TCustomImageList(Value);
end;

function TMenuItem_GetImageIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMenuItem(Obj).ImageIndex;
end;

procedure TMenuItem_SetImageIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMenuItem(Obj).ImageIndex := Value;
end;

function TMenuItem_GetSubMenuImages(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TMenuItem(Obj).SubMenuImages);
end;

procedure TMenuItem_SetSubMenuImages(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).SubMenuImages := TCustomImageList(Value);
end;

function TMenuItem_GetBitmap(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TMenuItem(Obj).Bitmap);
end;

procedure TMenuItem_SetBitmap(Obj: Pointer; Value: Pointer); NO_VCL_CALL;
begin
  TMenuItem(Obj).Bitmap := Graphics.TBitmap(Value);
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
  TCustomDrawGrid_GetCanvas,
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
  TMenuItem_SetBitmap;

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
