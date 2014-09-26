unit MxRegister;

{$I MX.INC}
{$D-,L-,S-}

interface

procedure Register;

implementation

{$R *.res}
uses Classes, SysUtils, mxPlacemnt, DesignIntf, mxPropsEd, mxMenus, mxCtrls, ExtBtn, gradient, multi_check,
	multi_color, multi_edit, renderwindow, mxshortcut;

{ Designer registration }
procedure Register;
begin
{ Components }                                                                  
  RegisterComponents('AlexMX', [TFormStorage, TMxPopupMenu, TMxLabel, TMxPanel, TExtBtn, TGradient, TMultiObjCheck, TMultiObjColor, TMultiObjSpinEdit, TD3DWindow, TMxHotKey]);
  RegisterNonActiveX([TFormStorage],axrComponentOnly);

{ TFormStorage }
  RegisterComponentEditor(TFormStorage, TFormStorageEditor);
  RegisterPropertyEditor(TypeInfo(TStrings), TFormStorage, 'StoredProps',
    TStoredPropsProperty);
{ RxMenus }
  RegisterPropertyEditor(TypeInfo(Boolean), TMxPopupMenu, 'OwnerDraw', nil);
end;

end.