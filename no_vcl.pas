library no_vcl;

{$ifdef WINDOWS}
  {$define NO_VCL_CALL stdcall}
{$else}
  {$define NO_VCL_CALL cdecl}
{$endif}

uses
  Interfaces,
  Forms,
  StdCtrls;

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

function TButton_Create(Owner: Pointer): Pointer; NO_VCL_CALL;
begin
  Result := Pointer(TButton.Create(TComponent(Owner)));
end;

procedure TButton_Destroy(Obj: Pointer); NO_VCL_CALL;
begin
  TButton(Obj).Free;
end;

exports
  TForm_Create,
  TForm_Destroy,
  TForm_GetCaption,
  TForm_SetCaption,
  TForm_GetWidth,
  TForm_SetWidth,
  TButton_Create,
  TButton_Destroy;

begin
  RequireDerivedFormResource := False;
  Application.Initialize;
end.
