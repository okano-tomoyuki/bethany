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
  StdCtrls;

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
  TRadioButton_SetOnClick;

begin
  RequireDerivedFormResource := False;
  Application.Initialize;
end.
