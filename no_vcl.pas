library no_vcl;

{$macro on}
{$ifdef WINDOWS}
  {$define NO_VCL_CALL := stdcall}
{$else}
  {$define NO_VCL_CALL := cdecl}
{$endif}

uses
  Classes,
  Interfaces,
  Forms,
  Controls,
  StdCtrls,
  ExtCtrls,
  Graphics;

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

procedure TCallbackBridge.DoClick(Sender: TObject);
begin
  if Assigned(FCallback) then
    FCallback(Pointer(Sender));
end;

function TForm_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TForm.Create(TComponent(Owner)));
end;

procedure TForm_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TForm(Obj).Free;
end;

function TForm_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TForm(Obj).Caption);
end;

procedure TForm_SetCaption(Obj: Pointer; Caption: PChar); NO_VCL_CALL;
begin
  TForm(Obj).Caption := Caption;
end;

function TForm_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TForm(Obj).Width;
end;

procedure TForm_SetWidth(Obj: Pointer; W: Integer); NO_VCL_CALL;
begin
  TForm(Obj).Width := W;
end;

function TForm_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TForm(Obj).Height;
end;

procedure TForm_SetHeight(Obj: Pointer; H: Integer); NO_VCL_CALL;
begin
  TForm(Obj).Height := H;
end;

function TForm_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TForm(Obj).Visible;
end;

procedure TForm_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TForm(Obj).Visible := Value;
end;

function TForm_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TForm(Obj).Enabled;
end;

procedure TForm_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TForm(Obj).Enabled := Value;
end;

procedure TForm_Show(Obj: Pointer); NO_VCL_CALL;
begin
  TForm(Obj).Show;
end;

function TForm_ShowModal(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TForm(Obj).ShowModal;
end;

procedure TForm_Hide(Obj: Pointer); NO_VCL_CALL;
begin
  TForm(Obj).Hide;
end;

procedure TForm_Close(Obj: Pointer); NO_VCL_CALL;
begin
  TForm(Obj).Close;
end;

procedure Application_Run; NO_VCL_CALL;
begin
  Application.Run;
end;

procedure Application_ProcessMessages; NO_VCL_CALL;
begin
  Application.ProcessMessages;
end;

function TButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TButton.Create(TComponent(Owner)));
end;

procedure TButton_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TButton(Obj).Free;
end;

procedure TButton_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TButton(Obj).Parent := TWinControl(ParentObj);
end;

function TButton_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TButton(Obj).Caption);
end;

procedure TButton_SetCaption(Obj: Pointer; Caption: PChar); NO_VCL_CALL;
begin
  TButton(Obj).Caption := Caption;
end;

function TButton_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TButton(Obj).Left;
end;

procedure TButton_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TButton(Obj).Left := Value;
end;

function TButton_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TButton(Obj).Top;
end;

procedure TButton_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TButton(Obj).Top := Value;
end;

function TButton_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TButton(Obj).Width;
end;

procedure TButton_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TButton(Obj).Width := Value;
end;

function TButton_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TButton(Obj).Height;
end;

procedure TButton_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TButton(Obj).Height := Value;
end;

function TButton_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TButton(Obj).Visible;
end;

procedure TButton_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TButton(Obj).Visible := Value;
end;

function TButton_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TButton(Obj).Enabled;
end;

procedure TButton_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TButton(Obj).Enabled := Value;
end;

procedure TButton_SetOnClick(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TButton(Obj));
  Bridge.Callback := Cb;
  TButton(Obj).OnClick := @Bridge.DoClick;
end;

{ TLabel }

function TLabel_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TLabel.Create(TComponent(Owner)));
end;

procedure TLabel_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TLabel(Obj).Free;
end;

procedure TLabel_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TLabel(Obj).Parent := TWinControl(ParentObj);
end;

function TLabel_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TLabel(Obj).Caption);
end;

procedure TLabel_SetCaption(Obj: Pointer; Caption: PChar); NO_VCL_CALL;
begin
  TLabel(Obj).Caption := Caption;
end;

function TLabel_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TLabel(Obj).Left;
end;

procedure TLabel_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TLabel(Obj).Left := Value;
end;

function TLabel_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TLabel(Obj).Top;
end;

procedure TLabel_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TLabel(Obj).Top := Value;
end;

function TLabel_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TLabel(Obj).Width;
end;

procedure TLabel_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TLabel(Obj).Width := Value;
end;

function TLabel_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TLabel(Obj).Height;
end;

procedure TLabel_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TLabel(Obj).Height := Value;
end;

function TLabel_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TLabel(Obj).Visible;
end;

procedure TLabel_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TLabel(Obj).Visible := Value;
end;

function TLabel_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TLabel(Obj).Enabled;
end;

procedure TLabel_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TLabel(Obj).Enabled := Value;
end;

{ TEdit }

function TEdit_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TEdit.Create(TComponent(Owner)));
end;

procedure TEdit_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TEdit(Obj).Free;
end;

procedure TEdit_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TEdit(Obj).Parent := TWinControl(ParentObj);
end;

function TEdit_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TEdit(Obj).Text);
end;

procedure TEdit_SetText(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TEdit(Obj).Text := Text;
end;

function TEdit_GetMaxLength(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TEdit(Obj).MaxLength;
end;

procedure TEdit_SetMaxLength(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TEdit(Obj).MaxLength := Value;
end;

function TEdit_GetReadOnly(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TEdit(Obj).ReadOnly;
end;

procedure TEdit_SetReadOnly(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TEdit(Obj).ReadOnly := Value;
end;

function TEdit_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TEdit(Obj).Left;
end;

procedure TEdit_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TEdit(Obj).Left := Value;
end;

function TEdit_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TEdit(Obj).Top;
end;

procedure TEdit_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TEdit(Obj).Top := Value;
end;

function TEdit_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TEdit(Obj).Width;
end;

procedure TEdit_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TEdit(Obj).Width := Value;
end;

function TEdit_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TEdit(Obj).Height;
end;

procedure TEdit_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TEdit(Obj).Height := Value;
end;

function TEdit_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TEdit(Obj).Visible;
end;

procedure TEdit_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TEdit(Obj).Visible := Value;
end;

function TEdit_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TEdit(Obj).Enabled;
end;

procedure TEdit_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TEdit(Obj).Enabled := Value;
end;

procedure TEdit_SetOnChange(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TEdit(Obj));
  Bridge.Callback := Cb;
  TEdit(Obj).OnChange := @Bridge.DoClick;
end;

{ TCheckBox }

function TCheckBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TCheckBox.Create(TComponent(Owner)));
end;

procedure TCheckBox_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TCheckBox(Obj).Free;
end;

procedure TCheckBox_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TCheckBox(Obj).Parent := TWinControl(ParentObj);
end;

function TCheckBox_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TCheckBox(Obj).Caption);
end;

procedure TCheckBox_SetCaption(Obj: Pointer; Caption: PChar); NO_VCL_CALL;
begin
  TCheckBox(Obj).Caption := Caption;
end;

function TCheckBox_GetChecked(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCheckBox(Obj).Checked;
end;

procedure TCheckBox_SetChecked(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCheckBox(Obj).Checked := Value;
end;

function TCheckBox_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCheckBox(Obj).Left;
end;

procedure TCheckBox_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCheckBox(Obj).Left := Value;
end;

function TCheckBox_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCheckBox(Obj).Top;
end;

procedure TCheckBox_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCheckBox(Obj).Top := Value;
end;

function TCheckBox_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCheckBox(Obj).Width;
end;

procedure TCheckBox_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCheckBox(Obj).Width := Value;
end;

function TCheckBox_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TCheckBox(Obj).Height;
end;

procedure TCheckBox_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TCheckBox(Obj).Height := Value;
end;

function TCheckBox_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCheckBox(Obj).Visible;
end;

procedure TCheckBox_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCheckBox(Obj).Visible := Value;
end;

function TCheckBox_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TCheckBox(Obj).Enabled;
end;

procedure TCheckBox_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TCheckBox(Obj).Enabled := Value;
end;

procedure TCheckBox_SetOnClick(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TCheckBox(Obj));
  Bridge.Callback := Cb;
  TCheckBox(Obj).OnClick := @Bridge.DoClick;
end;

{ TRadioButton }

function TRadioButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TRadioButton.Create(TComponent(Owner)));
end;

procedure TRadioButton_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TRadioButton(Obj).Free;
end;

procedure TRadioButton_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TRadioButton(Obj).Parent := TWinControl(ParentObj);
end;

function TRadioButton_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TRadioButton(Obj).Caption);
end;

procedure TRadioButton_SetCaption(Obj: Pointer; Caption: PChar); NO_VCL_CALL;
begin
  TRadioButton(Obj).Caption := Caption;
end;

function TRadioButton_GetChecked(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TRadioButton(Obj).Checked;
end;

procedure TRadioButton_SetChecked(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TRadioButton(Obj).Checked := Value;
end;

function TRadioButton_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TRadioButton(Obj).Left;
end;

procedure TRadioButton_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TRadioButton(Obj).Left := Value;
end;

function TRadioButton_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TRadioButton(Obj).Top;
end;

procedure TRadioButton_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TRadioButton(Obj).Top := Value;
end;

function TRadioButton_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TRadioButton(Obj).Width;
end;

procedure TRadioButton_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TRadioButton(Obj).Width := Value;
end;

function TRadioButton_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TRadioButton(Obj).Height;
end;

procedure TRadioButton_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TRadioButton(Obj).Height := Value;
end;

function TRadioButton_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TRadioButton(Obj).Visible;
end;

procedure TRadioButton_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TRadioButton(Obj).Visible := Value;
end;

function TRadioButton_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TRadioButton(Obj).Enabled;
end;

procedure TRadioButton_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TRadioButton(Obj).Enabled := Value;
end;

procedure TRadioButton_SetOnClick(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TRadioButton(Obj));
  Bridge.Callback := Cb;
  TRadioButton(Obj).OnClick := @Bridge.DoClick;
end;

{ TPanel }

function TPanel_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPanel.Create(TComponent(Owner)));
end;

procedure TPanel_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TPanel(Obj).Free;
end;

procedure TPanel_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TPanel(Obj).Parent := TWinControl(ParentObj);
end;

function TPanel_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TPanel(Obj).Caption);
end;

procedure TPanel_SetCaption(Obj: Pointer; Caption: PChar); NO_VCL_CALL;
begin
  TPanel(Obj).Caption := Caption;
end;

function TPanel_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPanel(Obj).Left;
end;

procedure TPanel_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPanel(Obj).Left := Value;
end;

function TPanel_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPanel(Obj).Top;
end;

procedure TPanel_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPanel(Obj).Top := Value;
end;

function TPanel_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPanel(Obj).Width;
end;

procedure TPanel_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPanel(Obj).Width := Value;
end;

function TPanel_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPanel(Obj).Height;
end;

procedure TPanel_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPanel(Obj).Height := Value;
end;

function TPanel_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TPanel(Obj).Visible;
end;

procedure TPanel_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TPanel(Obj).Visible := Value;
end;

function TPanel_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TPanel(Obj).Enabled;
end;

procedure TPanel_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TPanel(Obj).Enabled := Value;
end;

{ TGroupBox }

function TGroupBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TGroupBox.Create(TComponent(Owner)));
end;

procedure TGroupBox_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TGroupBox(Obj).Free;
end;

procedure TGroupBox_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TGroupBox(Obj).Parent := TWinControl(ParentObj);
end;

function TGroupBox_GetCaption(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TGroupBox(Obj).Caption);
end;

procedure TGroupBox_SetCaption(Obj: Pointer; Caption: PChar); NO_VCL_CALL;
begin
  TGroupBox(Obj).Caption := Caption;
end;

function TGroupBox_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TGroupBox(Obj).Left;
end;

procedure TGroupBox_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TGroupBox(Obj).Left := Value;
end;

function TGroupBox_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TGroupBox(Obj).Top;
end;

procedure TGroupBox_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TGroupBox(Obj).Top := Value;
end;

function TGroupBox_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TGroupBox(Obj).Width;
end;

procedure TGroupBox_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TGroupBox(Obj).Width := Value;
end;

function TGroupBox_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TGroupBox(Obj).Height;
end;

procedure TGroupBox_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TGroupBox(Obj).Height := Value;
end;

function TGroupBox_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TGroupBox(Obj).Visible;
end;

procedure TGroupBox_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TGroupBox(Obj).Visible := Value;
end;

function TGroupBox_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TGroupBox(Obj).Enabled;
end;

procedure TGroupBox_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TGroupBox(Obj).Enabled := Value;
end;

{ TComboBox }

function TComboBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TComboBox.Create(TComponent(Owner)));
end;

procedure TComboBox_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TComboBox(Obj).Free;
end;

procedure TComboBox_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TComboBox(Obj).Parent := TWinControl(ParentObj);
end;

function TComboBox_GetText(Obj: Pointer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TComboBox(Obj).Text);
end;

procedure TComboBox_SetText(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TComboBox(Obj).Text := Text;
end;

function TComboBox_GetItemIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).ItemIndex;
end;

procedure TComboBox_SetItemIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TComboBox(Obj).ItemIndex := Value;
end;

procedure TComboBox_Items_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TComboBox(Obj).Items.Add(Text);
end;

procedure TComboBox_Items_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TComboBox(Obj).Items.Clear;
end;

function TComboBox_Items_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).Items.Count;
end;

function TComboBox_Items_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TComboBox(Obj).Items[Index]);
end;

function TComboBox_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).Left;
end;

procedure TComboBox_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TComboBox(Obj).Left := Value;
end;

function TComboBox_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).Top;
end;

procedure TComboBox_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TComboBox(Obj).Top := Value;
end;

function TComboBox_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).Width;
end;

procedure TComboBox_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TComboBox(Obj).Width := Value;
end;

function TComboBox_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).Height;
end;

procedure TComboBox_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TComboBox(Obj).Height := Value;
end;

function TComboBox_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).Visible;
end;

procedure TComboBox_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TComboBox(Obj).Visible := Value;
end;

function TComboBox_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TComboBox(Obj).Enabled;
end;

procedure TComboBox_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TComboBox(Obj).Enabled := Value;
end;

procedure TComboBox_SetOnChange(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TComboBox(Obj));
  Bridge.Callback := Cb;
  TComboBox(Obj).OnChange := @Bridge.DoClick;
end;

{ TListBox }

function TListBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TListBox.Create(TComponent(Owner)));
end;

procedure TListBox_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TListBox(Obj).Free;
end;

procedure TListBox_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TListBox(Obj).Parent := TWinControl(ParentObj);
end;

function TListBox_GetItemIndex(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListBox(Obj).ItemIndex;
end;

procedure TListBox_SetItemIndex(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListBox(Obj).ItemIndex := Value;
end;

procedure TListBox_Items_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TListBox(Obj).Items.Add(Text);
end;

procedure TListBox_Items_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TListBox(Obj).Items.Clear;
end;

function TListBox_Items_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListBox(Obj).Items.Count;
end;

function TListBox_Items_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TListBox(Obj).Items[Index]);
end;

function TListBox_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListBox(Obj).Left;
end;

procedure TListBox_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListBox(Obj).Left := Value;
end;

function TListBox_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListBox(Obj).Top;
end;

procedure TListBox_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListBox(Obj).Top := Value;
end;

function TListBox_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListBox(Obj).Width;
end;

procedure TListBox_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListBox(Obj).Width := Value;
end;

function TListBox_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TListBox(Obj).Height;
end;

procedure TListBox_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TListBox(Obj).Height := Value;
end;

function TListBox_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListBox(Obj).Visible;
end;

procedure TListBox_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListBox(Obj).Visible := Value;
end;

function TListBox_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TListBox(Obj).Enabled;
end;

procedure TListBox_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TListBox(Obj).Enabled := Value;
end;

procedure TListBox_SetOnClick(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TListBox(Obj));
  Bridge.Callback := Cb;
  TListBox(Obj).OnClick := @Bridge.DoClick;
end;

{ TMemo }

function TMemo_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TMemo.Create(TComponent(Owner)));
end;

procedure TMemo_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TMemo(Obj).Free;
end;

procedure TMemo_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TMemo(Obj).Parent := TWinControl(ParentObj);
end;

function TMemo_GetReadOnly(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMemo(Obj).ReadOnly;
end;

procedure TMemo_SetReadOnly(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMemo(Obj).ReadOnly := Value;
end;

function TMemo_GetScrollBars(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Ord(TMemo(Obj).ScrollBars);
end;

procedure TMemo_SetScrollBars(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMemo(Obj).ScrollBars := TScrollStyle(Value);
end;

procedure TMemo_Lines_Add(Obj: Pointer; Text: PChar); NO_VCL_CALL;
begin
  TMemo(Obj).Lines.Add(Text);
end;

procedure TMemo_Lines_Clear(Obj: Pointer); NO_VCL_CALL;
begin
  TMemo(Obj).Lines.Clear;
end;

function TMemo_Lines_Count(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMemo(Obj).Lines.Count;
end;

function TMemo_Lines_GetText(Obj: Pointer; Index: Integer): PChar; NO_VCL_CALL;
begin
  Result := PChar(TMemo(Obj).Lines[Index]);
end;

function TMemo_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMemo(Obj).Left;
end;

procedure TMemo_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMemo(Obj).Left := Value;
end;

function TMemo_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMemo(Obj).Top;
end;

procedure TMemo_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMemo(Obj).Top := Value;
end;

function TMemo_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMemo(Obj).Width;
end;

procedure TMemo_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMemo(Obj).Width := Value;
end;

function TMemo_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TMemo(Obj).Height;
end;

procedure TMemo_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TMemo(Obj).Height := Value;
end;

function TMemo_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMemo(Obj).Visible;
end;

procedure TMemo_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMemo(Obj).Visible := Value;
end;

function TMemo_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TMemo(Obj).Enabled;
end;

procedure TMemo_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TMemo(Obj).Enabled := Value;
end;

procedure TMemo_SetOnChange(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TMemo(Obj));
  Bridge.Callback := Cb;
  TMemo(Obj).OnChange := @Bridge.DoClick;
end;

{ TTimer }

function TTimer_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TTimer.Create(TComponent(Owner)));
end;

procedure TTimer_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TTimer(Obj).Free;
end;

function TTimer_GetInterval(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TTimer(Obj).Interval;
end;

procedure TTimer_SetInterval(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TTimer(Obj).Interval := Value;
end;

function TTimer_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TTimer(Obj).Enabled;
end;

procedure TTimer_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TTimer(Obj).Enabled := Value;
end;

procedure TTimer_SetOnTimer(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TTimer(Obj));
  Bridge.Callback := Cb;
  TTimer(Obj).OnTimer := @Bridge.DoClick;
end;

{ TPaintBox }

function TPaintBox_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPaintBox.Create(TComponent(Owner)));
end;

procedure TPaintBox_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TPaintBox(Obj).Free;
end;

procedure TPaintBox_SetParent(Obj: Pointer; ParentObj: Pointer); NO_VCL_CALL;
begin
  TPaintBox(Obj).Parent := TWinControl(ParentObj);
end;

function TPaintBox_GetLeft(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPaintBox(Obj).Left;
end;

procedure TPaintBox_SetLeft(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPaintBox(Obj).Left := Value;
end;

function TPaintBox_GetTop(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPaintBox(Obj).Top;
end;

procedure TPaintBox_SetTop(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPaintBox(Obj).Top := Value;
end;

function TPaintBox_GetWidth(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPaintBox(Obj).Width;
end;

procedure TPaintBox_SetWidth(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPaintBox(Obj).Width := Value;
end;

function TPaintBox_GetHeight(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := TPaintBox(Obj).Height;
end;

procedure TPaintBox_SetHeight(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TPaintBox(Obj).Height := Value;
end;

function TPaintBox_GetVisible(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TPaintBox(Obj).Visible;
end;

procedure TPaintBox_SetVisible(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TPaintBox(Obj).Visible := Value;
end;

function TPaintBox_GetEnabled(Obj: Pointer): LongBool; NO_VCL_CALL;
begin
  Result := TPaintBox(Obj).Enabled;
end;

procedure TPaintBox_SetEnabled(Obj: Pointer; Value: LongBool); NO_VCL_CALL;
begin
  TPaintBox(Obj).Enabled := Value;
end;

function TPaintBox_GetCanvas(Obj: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TPaintBox(Obj).Canvas);
end;

procedure TPaintBox_SetOnPaint(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TPaintBox(Obj));
  Bridge.Callback := Cb;
  TPaintBox(Obj).OnPaint := @Bridge.DoClick;
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

{ TPen }
{ CanvasのPen/Brush/Fontと同様、独自のCreate/Destroyは持たない。 }

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

{ TBrush }

function TBrush_GetColor(Obj: Pointer): Integer; NO_VCL_CALL;
begin
  Result := Integer(TBrush(Obj).Color);
end;

procedure TBrush_SetColor(Obj: Pointer; Value: Integer); NO_VCL_CALL;
begin
  TBrush(Obj).Color := TColor(Value);
end;

{ TFont }

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
  TForm_Create,
  TForm_Destroy,
  TForm_GetCaption,
  TForm_SetCaption,
  TForm_GetWidth,
  TForm_SetWidth,
  TForm_GetHeight,
  TForm_SetHeight,
  TForm_GetVisible,
  TForm_SetVisible,
  TForm_GetEnabled,
  TForm_SetEnabled,
  TForm_Show,
  TForm_ShowModal,
  TForm_Hide,
  TForm_Close,
  Application_Run,
  Application_ProcessMessages,
  TButton_Create,
  TButton_Destroy,
  TButton_SetParent,
  TButton_GetCaption,
  TButton_SetCaption,
  TButton_GetLeft,
  TButton_SetLeft,
  TButton_GetTop,
  TButton_SetTop,
  TButton_GetWidth,
  TButton_SetWidth,
  TButton_GetHeight,
  TButton_SetHeight,
  TButton_GetVisible,
  TButton_SetVisible,
  TButton_GetEnabled,
  TButton_SetEnabled,
  TButton_SetOnClick,
  TLabel_Create,
  TLabel_Destroy,
  TLabel_SetParent,
  TLabel_GetCaption,
  TLabel_SetCaption,
  TLabel_GetLeft,
  TLabel_SetLeft,
  TLabel_GetTop,
  TLabel_SetTop,
  TLabel_GetWidth,
  TLabel_SetWidth,
  TLabel_GetHeight,
  TLabel_SetHeight,
  TLabel_GetVisible,
  TLabel_SetVisible,
  TLabel_GetEnabled,
  TLabel_SetEnabled,
  TEdit_Create,
  TEdit_Destroy,
  TEdit_SetParent,
  TEdit_GetText,
  TEdit_SetText,
  TEdit_GetMaxLength,
  TEdit_SetMaxLength,
  TEdit_GetReadOnly,
  TEdit_SetReadOnly,
  TEdit_GetLeft,
  TEdit_SetLeft,
  TEdit_GetTop,
  TEdit_SetTop,
  TEdit_GetWidth,
  TEdit_SetWidth,
  TEdit_GetHeight,
  TEdit_SetHeight,
  TEdit_GetVisible,
  TEdit_SetVisible,
  TEdit_GetEnabled,
  TEdit_SetEnabled,
  TEdit_SetOnChange,
  TCheckBox_Create,
  TCheckBox_Destroy,
  TCheckBox_SetParent,
  TCheckBox_GetCaption,
  TCheckBox_SetCaption,
  TCheckBox_GetChecked,
  TCheckBox_SetChecked,
  TCheckBox_GetLeft,
  TCheckBox_SetLeft,
  TCheckBox_GetTop,
  TCheckBox_SetTop,
  TCheckBox_GetWidth,
  TCheckBox_SetWidth,
  TCheckBox_GetHeight,
  TCheckBox_SetHeight,
  TCheckBox_GetVisible,
  TCheckBox_SetVisible,
  TCheckBox_GetEnabled,
  TCheckBox_SetEnabled,
  TCheckBox_SetOnClick,
  TRadioButton_Create,
  TRadioButton_Destroy,
  TRadioButton_SetParent,
  TRadioButton_GetCaption,
  TRadioButton_SetCaption,
  TRadioButton_GetChecked,
  TRadioButton_SetChecked,
  TRadioButton_GetLeft,
  TRadioButton_SetLeft,
  TRadioButton_GetTop,
  TRadioButton_SetTop,
  TRadioButton_GetWidth,
  TRadioButton_SetWidth,
  TRadioButton_GetHeight,
  TRadioButton_SetHeight,
  TRadioButton_GetVisible,
  TRadioButton_SetVisible,
  TRadioButton_GetEnabled,
  TRadioButton_SetEnabled,
  TRadioButton_SetOnClick,
  TPanel_Create,
  TPanel_Destroy,
  TPanel_SetParent,
  TPanel_GetCaption,
  TPanel_SetCaption,
  TPanel_GetLeft,
  TPanel_SetLeft,
  TPanel_GetTop,
  TPanel_SetTop,
  TPanel_GetWidth,
  TPanel_SetWidth,
  TPanel_GetHeight,
  TPanel_SetHeight,
  TPanel_GetVisible,
  TPanel_SetVisible,
  TPanel_GetEnabled,
  TPanel_SetEnabled,
  TGroupBox_Create,
  TGroupBox_Destroy,
  TGroupBox_SetParent,
  TGroupBox_GetCaption,
  TGroupBox_SetCaption,
  TGroupBox_GetLeft,
  TGroupBox_SetLeft,
  TGroupBox_GetTop,
  TGroupBox_SetTop,
  TGroupBox_GetWidth,
  TGroupBox_SetWidth,
  TGroupBox_GetHeight,
  TGroupBox_SetHeight,
  TGroupBox_GetVisible,
  TGroupBox_SetVisible,
  TGroupBox_GetEnabled,
  TGroupBox_SetEnabled,
  TComboBox_Create,
  TComboBox_Destroy,
  TComboBox_SetParent,
  TComboBox_GetText,
  TComboBox_SetText,
  TComboBox_GetItemIndex,
  TComboBox_SetItemIndex,
  TComboBox_Items_Add,
  TComboBox_Items_Clear,
  TComboBox_Items_Count,
  TComboBox_Items_GetText,
  TComboBox_GetLeft,
  TComboBox_SetLeft,
  TComboBox_GetTop,
  TComboBox_SetTop,
  TComboBox_GetWidth,
  TComboBox_SetWidth,
  TComboBox_GetHeight,
  TComboBox_SetHeight,
  TComboBox_GetVisible,
  TComboBox_SetVisible,
  TComboBox_GetEnabled,
  TComboBox_SetEnabled,
  TComboBox_SetOnChange,
  TListBox_Create,
  TListBox_Destroy,
  TListBox_SetParent,
  TListBox_GetItemIndex,
  TListBox_SetItemIndex,
  TListBox_Items_Add,
  TListBox_Items_Clear,
  TListBox_Items_Count,
  TListBox_Items_GetText,
  TListBox_GetLeft,
  TListBox_SetLeft,
  TListBox_GetTop,
  TListBox_SetTop,
  TListBox_GetWidth,
  TListBox_SetWidth,
  TListBox_GetHeight,
  TListBox_SetHeight,
  TListBox_GetVisible,
  TListBox_SetVisible,
  TListBox_GetEnabled,
  TListBox_SetEnabled,
  TListBox_SetOnClick,
  TMemo_Create,
  TMemo_Destroy,
  TMemo_SetParent,
  TMemo_GetReadOnly,
  TMemo_SetReadOnly,
  TMemo_GetScrollBars,
  TMemo_SetScrollBars,
  TMemo_Lines_Add,
  TMemo_Lines_Clear,
  TMemo_Lines_Count,
  TMemo_Lines_GetText,
  TMemo_GetLeft,
  TMemo_SetLeft,
  TMemo_GetTop,
  TMemo_SetTop,
  TMemo_GetWidth,
  TMemo_SetWidth,
  TMemo_GetHeight,
  TMemo_SetHeight,
  TMemo_GetVisible,
  TMemo_SetVisible,
  TMemo_GetEnabled,
  TMemo_SetEnabled,
  TMemo_SetOnChange,
  TTimer_Create,
  TTimer_Destroy,
  TTimer_GetInterval,
  TTimer_SetInterval,
  TTimer_GetEnabled,
  TTimer_SetEnabled,
  TTimer_SetOnTimer,
  TPaintBox_Create,
  TPaintBox_Destroy,
  TPaintBox_SetParent,
  TPaintBox_GetLeft,
  TPaintBox_SetLeft,
  TPaintBox_GetTop,
  TPaintBox_SetTop,
  TPaintBox_GetWidth,
  TPaintBox_SetWidth,
  TPaintBox_GetHeight,
  TPaintBox_SetHeight,
  TPaintBox_GetVisible,
  TPaintBox_SetVisible,
  TPaintBox_GetEnabled,
  TPaintBox_SetEnabled,
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
