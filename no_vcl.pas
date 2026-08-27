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

procedure TButton_SetOnClick(Obj: Pointer; Cb: TNoVclCallback); NO_VCL_CALL;
var
  Bridge: TCallbackBridge;
begin
  Bridge := TCallbackBridge.Create(TButton(Obj));
  Bridge.Callback := Cb;
  TButton(Obj).OnClick := @Bridge.DoClick;
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
  TButton_SetOnClick;

begin
  RequireDerivedFormResource := False;
  Application.Initialize;
end.
