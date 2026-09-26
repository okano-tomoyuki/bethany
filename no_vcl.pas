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
  Graphics,
  CustomTimer;

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

function NewVarBridge(Owner: TComponent; Cb: TNoVclVarCallback; Data: Pointer): TVarCallbackBridge;
begin
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

function NewBridge(Owner: TComponent; Cb: TNoVclCallback; Data: Pointer): TCallbackBridge;
begin
  Result := TCallbackBridge.Create(Owner);
  Result.Callback := Cb;
  Result.Data := Data;
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
  Result := PChar(TControl(Obj).Caption);
end;

procedure TControl_SetCaption(Obj: Pointer; Value: PChar); NO_VCL_CALL;
begin
  TControl(Obj).Caption := Value;
end;

{ Text は TControl で protected。TCustomEdit と TCustomComboBox がそれぞれ公開している。 }
function TControl_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TControlAccess(Obj).Text);
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
  TControl(Obj).OnClick := @NewBridge(TControl(Obj), Cb, Data).DoClick;
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
  TCustomForm(Obj).OnClose := @NewVarBridge(TComponent(Obj), Cb, Data).DoClose;
end;

procedure TCustomForm_SetOnCloseQuery(Obj: Pointer; Cb: TNoVclVarCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnCloseQuery := @NewVarBridge(TComponent(Obj), Cb, Data).DoCloseQuery;
end;

procedure TCustomForm_SetOnShow(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnShow := @NewBridge(TComponent(Obj), Cb, Data).DoClick;
end;

procedure TCustomForm_SetOnHide(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnHide := @NewBridge(TComponent(Obj), Cb, Data).DoClick;
end;

procedure TCustomForm_SetOnActivate(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnActivate := @NewBridge(TComponent(Obj), Cb, Data).DoClick;
end;

procedure TCustomForm_SetOnDeactivate(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnDeactivate := @NewBridge(TComponent(Obj), Cb, Data).DoClick;
end;

{ 破棄の最初(BeforeDestruction)で呼ばれる。子コントロールはまだ生きており、破棄通知はこの後に来る。 }
procedure TCustomForm_SetOnDestroy(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TCustomForm(Obj).OnDestroy := @NewBridge(TComponent(Obj), Cb, Data).DoClick;
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
  Result := PChar(TApplication(Obj).Title);
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
  TCustomEdit(Obj).OnChange := @NewBridge(TCustomEdit(Obj), Cb, Data).DoClick;
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
  Result := PChar(TCustomMemo(Obj).Lines[Index]);
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
  Result := PChar(TCustomComboBox(Obj).Items[Index]);
end;

function TComboBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Watch(TComboBox.Create(TComponent(Owner)));
end;

{ OnChange は TCustomComboBox では protected で、公開しているのは TComboBox だけ。 }
procedure TComboBox_SetOnChange(Obj: Pointer; Cb: TNoVclCallback; Data: Pointer); NO_VCL_CALL;
begin
  TComboBox(Obj).OnChange := @NewBridge(TComboBox(Obj), Cb, Data).DoClick;
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
  Result := PChar(TCustomListBox(Obj).Items[Index]);
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
  TCustomTimer(Obj).OnTimer := @NewBridge(TCustomTimer(Obj), Cb, Data).DoClick;
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
  TPaintBox(Obj).OnPaint := @NewBridge(TPaintBox(Obj), Cb, Data).DoClick;
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
  Result := PChar(TFont(Obj).Name);
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
  TControl_GetText,
  TControl_SetText,
  TControl_Show,
  TControl_Hide,
  TControl_SetOnClick,

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
  TFont_SetColor;

begin
  RequireDerivedFormResource := False;
  Application.Initialize;
  GFreeNotifier := TFreeNotifier.Create(nil);
  Dll_Process_Detach_Hook := @DetachHook;
end.
