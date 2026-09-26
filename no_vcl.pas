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
  TNoVclCallback = procedure(Sender: Pointer); NO_VCL_CALL;

  { Cのプレーンな関数ポインタ(no_vcl_callback_t)を
    LCLのTNotifyEvent(オブジェクトメソッド)へ橋渡しする }
  TCallbackBridge = class(TComponent)
  private
    FCallback: TNoVclCallback;
  public
    procedure DoClick(Sender: TObject);
    property Callback: TNoVclCallback read FCallback write FCallback;
  end;

  { protected メンバへアクセスするための派生クラス(protected hack)。
    同一ユニット内で宣言した派生クラス経由なら、基底の protected メンバに触れられる。 }
  TControlAccess = class(TControl);
  TButtonControlAccess = class(TButtonControl);

procedure TCallbackBridge.DoClick(Sender: TObject);
begin
  if Assigned(FCallback) then
    FCallback(Pointer(Sender));
end;

function NewBridge(Owner: TComponent; Cb: TNoVclCallback): TCallbackBridge;
begin
  Result := TCallbackBridge.Create(Owner);
  Result.Callback := Cb;
end;

{ TComponent }

procedure TComponent_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TComponent(Obj).Free;
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

procedure TControl_SetOnClick(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
begin
  TControl(Obj).OnClick := @NewBridge(TControl(Obj), Cb).DoClick;
end;

{ TCustomForm / TForm }

function TForm_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TForm.Create(TComponent(Owner)));
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

{ Application }

procedure Application_Run; NO_VCL_CALL;
begin
  Application.Run;
end;

procedure Application_ProcessMessages; NO_VCL_CALL;
begin
  Application.ProcessMessages;
end;

{ TPanel / TGroupBox / TLabel }

function TPanel_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPanel.Create(TComponent(Owner)));
end;

function TGroupBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TGroupBox.Create(TComponent(Owner)));
end;

function TLabel_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TLabel.Create(TComponent(Owner)));
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
  Result := Pointer(TButton.Create(TComponent(Owner)));
end;

function TCheckBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCheckBox.Create(TComponent(Owner)));
end;

function TRadioButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TRadioButton.Create(TComponent(Owner)));
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

procedure TCustomEdit_SetOnChange(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
begin
  TCustomEdit(Obj).OnChange := @NewBridge(TCustomEdit(Obj), Cb).DoClick;
end;

function TEdit_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TEdit.Create(TComponent(Owner)));
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
  Result := Pointer(TMemo.Create(TComponent(Owner)));
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
  Result := Pointer(TComboBox.Create(TComponent(Owner)));
end;

{ OnChange は TCustomComboBox では protected で、公開しているのは TComboBox だけ。 }
procedure TComboBox_SetOnChange(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
begin
  TComboBox(Obj).OnChange := @NewBridge(TComboBox(Obj), Cb).DoClick;
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
  Result := Pointer(TListBox.Create(TComponent(Owner)));
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

procedure TCustomTimer_SetOnTimer(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
begin
  TCustomTimer(Obj).OnTimer := @NewBridge(TCustomTimer(Obj), Cb).DoClick;
end;

function TTimer_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TTimer.Create(TComponent(Owner)));
end;

{ TPaintBox }

function TPaintBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPaintBox.Create(TComponent(Owner)));
end;

function TPaintBox_GetCanvas(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPaintBox(Obj).Canvas);
end;

procedure TPaintBox_SetOnPaint(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
begin
  TPaintBox(Obj).OnPaint := @NewBridge(TPaintBox(Obj), Cb).DoClick;
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
  TComponent_Destroy,

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

  Application_Run,
  Application_ProcessMessages,

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
end.
