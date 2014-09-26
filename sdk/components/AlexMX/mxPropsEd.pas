{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995, 1996 AO ROSNO             }
{                                                       }
{*******************************************************}

unit mxPropsEd;

{$I MX.INC}

interface

uses
  SysUtils, Messages, Classes, Graphics, Controls, Forms, Dialogs, StdCtrls, 
  Buttons, ExtCtrls, mxPlacemnt, Consts, DesignIntf, DesignEditors, mxVclUtils, MXCtrls, MXProps; 

type

{$IFNDEF RX_D4}
  IDesigner = TDesigner;
{$ENDIF}

{ TFormPropsDlg }

  TFormPropsDlg = class(TForm)
    Bevel1: TBevel;
    Label30: TLabel;
    Label31: TLabel;
    Label2: TLabel;
    UpBtn: TSpeedButton;
    DownBtn: TSpeedButton;
    StoredList: TTextListBox;
    PropertiesList: TTextListBox;
    ComponentsList: TTextListBox;
    FormBox: TGroupBox;
    ActiveCtrlBox: TCheckBox;
    PositionBox: TCheckBox;
    StateBox: TCheckBox;
    AddButton: TButton;
    DeleteButton: TButton;
    ClearButton: TButton;
    OkBtn: TButton;
    CancelBtn: TButton;
    procedure AddButtonClick(Sender: TObject);
    procedure ClearButtonClick(Sender: TObject);
    procedure ListClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure DeleteButtonClick(Sender: TObject);
    procedure StoredListClick(Sender: TObject);
    procedure UpBtnClick(Sender: TObject);
    procedure DownBtnClick(Sender: TObject);
    procedure StoredListDragOver(Sender, Source: TObject; X, Y: Integer;
      State: TDragState; var Accept: Boolean);
    procedure StoredListDragDrop(Sender, Source: TObject; X, Y: Integer);
    procedure PropertiesListDblClick(Sender: TObject);
  private
    { Private declarations }
    FCompOwner: TComponent;
    FDesigner: IDesigner;
    procedure ListToIndex(List: TCustomListBox; Idx: Integer);
    procedure UpdateCurrent;
    procedure DeleteProp(I: Integer);
    function FindProp(const CompName, PropName: string; var IdxComp,
      IdxProp: Integer): Boolean;
    procedure ClearLists;
    procedure CheckAddItem(const CompName, PropName: string);
    procedure AddItem(IdxComp, IdxProp: Integer; AUpdate: Boolean);
    procedure BuildLists(StoredProps: TStrings);
    procedure CheckButtons;
    procedure SetStoredList(AList: TStrings);
  public
    { Public declarations }
  end;

{ TFormStorageEditor }

  TFormStorageEditor = class(TComponentEditor)
    procedure ExecuteVerb(Index: Integer); override;
    function GetVerb(Index: Integer): string; override;
    function GetVerbCount: Integer; override;
  end;

{ TStoredPropsProperty }

  TStoredPropsProperty = class(TClassProperty)
  public
    function GetAttributes: TPropertyAttributes; override;
    function GetValue: string; override;
    procedure Edit; override;
  end;

{ Show component editor }
function ShowStorageDesigner(ACompOwner: TComponent; ADesigner: IDesigner;
  AStoredList: TStrings; var Options: TPlacementOptions): Boolean;

implementation

uses Windows, MXLConst, TypInfo, mxBoxProcs;

{$R *.DFM}

{$IFDEF WIN32}
 {$D-}
{$ENDIF}

{ TFormStorageEditor }

procedure TFormStorageEditor.ExecuteVerb(Index: Integer);
var
  Storage: TFormStorage;
  Opt: TPlacementOptions;
begin
  Storage := Component as TFormStorage;
  if Index = 0 then begin
    Opt := Storage.Options;
    if ShowStorageDesigner(TComponent(Storage.Owner), Designer,
      Storage.StoredProps, Opt) then
    begin
      Storage.Options := Opt;
{$IFDEF WIN32}
      Storage.SetNotification;
{$ENDIF}
    end;
  end;
end;

function TFormStorageEditor.GetVerb(Index: Integer): string;
begin
  case Index of
    0: Result := LoadStr(srStorageDesigner);
    else Result := '';
  end;
end;

function TFormStorageEditor.GetVerbCount: Integer;
begin
  Result := 1;
end;

{ TStoredPropsProperty }

function TStoredPropsProperty.GetAttributes: TPropertyAttributes;
begin
  Result := inherited GetAttributes + [paDialog] - [paSubProperties];
end;

function TStoredPropsProperty.GetValue: string;
begin
  if TStrings(GetOrdValue).Count > 0 then Result := inherited GetValue
  else Result := ResStr(srNone);
end;

procedure TStoredPropsProperty.Edit;
var
  Storage: TFormStorage;
  Opt: TPlacementOptions;
begin
  Storage := GetComponent(0) as TFormStorage;
  Opt := Storage.Options;
  if ShowStorageDesigner(Storage.Owner as TComponent, Designer,
    Storage.StoredProps, Opt) then
  begin
    Storage.Options := Opt;
{$IFDEF WIN32}
    Storage.SetNotification;
{$ENDIF}
  end;
end;

{ Show component editor }

function ShowStorageDesigner(ACompOwner: TComponent; ADesigner: IDesigner;
  AStoredList: TStrings; var Options: TPlacementOptions): Boolean;
begin
  with TFormPropsDlg.Create(Application) do
  try
    FCompOwner := ACompOwner;
    FDesigner := ADesigner;
    Screen.Cursor := crHourGlass;
    try
      UpdateStoredList(ACompOwner, AStoredList, False);
      SetStoredList(AStoredList);
      ActiveCtrlBox.Checked := fpActiveControl in Options;
      PositionBox.Checked := fpPosition in Options;
      StateBox.Checked := fpState in Options;
    finally
      Screen.Cursor := crDefault;
    end;
    Result := ShowModal = mrOk;
    if Result then begin
      AStoredList.Assign(StoredList.Items);
      Options := [];
      if ActiveCtrlBox.Checked then Include(Options, fpActiveControl);
      if PositionBox.Checked then Include(Options, fpPosition);
      if StateBox.Checked then Include(Options, fpState);
    end;
  finally
    Free;
  end;
end;

{ TFormPropsDlg }

procedure TFormPropsDlg.ListToIndex(List: TCustomListBox; Idx: Integer);

  procedure SetItemIndex(Index: Integer);
  begin
    if TTextListBox(List).MultiSelect then
      TTextListBox(List).Selected[Index] := True;
    List.ItemIndex := Index;
  end;

begin
  if Idx < List.Items.Count then
    SetItemIndex(Idx)
  else if Idx - 1 < List.Items.Count then
    SetItemIndex(Idx - 1)
  else if (List.Items.Count > 0) then
    SetItemIndex(0);
end;

procedure TFormPropsDlg.UpdateCurrent;
var
  IdxProp: Integer;
  List: TStrings;
begin
  IdxProp := PropertiesList.ItemIndex;
  if IdxProp < 0 then IdxProp := 0;
  if ComponentsList.Items.Count <= 0 then
  begin
    PropertiesList.Clear;
    Exit;
  end;
  if (ComponentsList.ItemIndex < 0) then
    ComponentsList.ItemIndex := 0;
  List := TStrings(ComponentsList.Items.Objects[ComponentsList.ItemIndex]);
  if List.Count > 0 then PropertiesList.Items := List
  else PropertiesList.Clear;
  ListToIndex(PropertiesList, IdxProp);
  CheckButtons;
end;

procedure TFormPropsDlg.DeleteProp(I: Integer);
var
  CompName, PropName: string;
  IdxComp, IdxProp, Idx: Integer;
  StrList: TStringList;
begin
  Idx := StoredList.ItemIndex;
  if ParseStoredItem(StoredList.Items[I], CompName, PropName) then begin
    StoredList.Items.Delete(I);
    if FDesigner <> nil then FDesigner.Modified;
    ListToIndex(StoredList, Idx);
    {I := ComponentsList.ItemIndex;}
    if not FindProp(CompName, PropName, IdxComp, IdxProp) then begin
      if IdxComp < 0 then begin
        StrList := TStringList.Create;
        try
          StrList.Add(PropName);
          ComponentsList.Items.AddObject(CompName, StrList);
          ComponentsList.ItemIndex := ComponentsList.Items.IndexOf(CompName);
        except
          StrList.Free;
          raise;
        end;
      end
      else begin
        TStrings(ComponentsList.Items.Objects[IdxComp]).Add(PropName);
      end;
      UpdateCurrent;
    end;
  end;
end;

function TFormPropsDlg.FindProp(const CompName, PropName: string; var IdxComp,
  IdxProp: Integer): Boolean;
begin
  Result := False;
  IdxComp := ComponentsList.Items.IndexOf(CompName);
  if IdxComp >= 0 then begin
    IdxProp := TStrings(ComponentsList.Items.Objects[IdxComp]).IndexOf(PropName);
    if IdxProp >= 0 then Result := True;
  end;
end;

procedure TFormPropsDlg.ClearLists;
var
  I: Integer;
begin
  for I := 0 to ComponentsList.Items.Count - 1 do begin
    ComponentsList.Items.Objects[I].Free;
  end;
  ComponentsList.Items.Clear;
  ComponentsList.Clear;
  PropertiesList.Clear;
  StoredList.Clear;
end;

procedure TFormPropsDlg.AddItem(IdxComp, IdxProp: Integer; AUpdate: Boolean);
var
  Idx: Integer;
  StrList: TStringList;
  CompName, PropName: string;
  Component: TComponent;
begin
  CompName := ComponentsList.Items[IdxComp];
  Component := FCompOwner.FindComponent(CompName);
  if Component = nil then Exit;
  StrList := TStringList(ComponentsList.Items.Objects[IdxComp]);
  PropName := StrList[IdxProp];
  StrList.Delete(IdxProp);
  if StrList.Count = 0 then begin
    Idx := ComponentsList.ItemIndex;
    StrList.Free;
    ComponentsList.Items.Delete(IdxComp);
    ListToIndex(ComponentsList, Idx);
  end;
  StoredList.Items.AddObject(CreateStoredItem(CompName, PropName), Component);
  if FDesigner <> nil then FDesigner.Modified;
  StoredList.ItemIndex := StoredList.Items.Count - 1;
  if AUpdate then UpdateCurrent;
end;

procedure TFormPropsDlg.CheckAddItem(const CompName, PropName: string);
var
  IdxComp, IdxProp: Integer;
begin
  if FindProp(CompName, PropName, IdxComp, IdxProp) then
    AddItem(IdxComp, IdxProp, True);
end;

procedure TFormPropsDlg.BuildLists(StoredProps: TStrings);
var
  I, J: Integer;
  C: TComponent;
  List: TPropInfoList;
  StrList: TStrings;
  CompName, PropName: string;
begin
  ClearLists;
  if FCompOwner <> nil then begin
    for I := 0 to FCompOwner.ComponentCount - 1 do begin
      C := FCompOwner.Components[I];
      if (C is TFormPlacement) or (C.Name = '') then Continue;
      List := TPropInfoList.Create(C, tkProperties);
      try
        StrList := TStringList.Create;
        try
          TStringList(StrList).Sorted := True;
          for J := 0 to List.Count - 1 do
            StrList.Add(List.Items[J]^.Name);
          ComponentsList.Items.AddObject(C.Name, StrList);
        except
          StrList.Free;
          raise;
        end;
      finally
        List.Free;
      end;
    end;
    if StoredProps <> nil then begin
      for I := 0 to StoredProps.Count - 1 do begin
        if ParseStoredItem(StoredProps[I], CompName, PropName) then
          CheckAddItem(CompName, PropName);
      end;
      ListToIndex(StoredList, 0);
    end;
  end
  else StoredList.Items.Clear;
  UpdateCurrent;
end;

procedure TFormPropsDlg.SetStoredList(AList: TStrings);
begin
  BuildLists(AList);
  if ComponentsList.Items.Count > 0 then
    ComponentsList.ItemIndex := 0;
  CheckButtons;
end;

procedure TFormPropsDlg.CheckButtons;
var
  Enable: Boolean;
begin
  AddButton.Enabled := (ComponentsList.ItemIndex >= 0) and
    (PropertiesList.ItemIndex >= 0);
  Enable := (StoredList.Items.Count > 0) and
    (StoredList.ItemIndex >= 0);
  DeleteButton.Enabled := Enable;
  ClearButton.Enabled := Enable;
  UpBtn.Enabled := Enable and (StoredList.ItemIndex > 0);
  DownBtn.Enabled := Enable and (StoredList.ItemIndex < StoredList.Items.Count - 1);
end;

procedure TFormPropsDlg.AddButtonClick(Sender: TObject);
var
  I: Integer;
begin
  if PropertiesList.SelCount > 0 then begin
    for I := PropertiesList.Items.Count - 1 downto 0 do begin
      if PropertiesList.Selected[I] then
        AddItem(ComponentsList.ItemIndex, I, False);
    end;
    UpdateCurrent;
  end
  else AddItem(ComponentsList.ItemIndex, PropertiesList.ItemIndex, True);
  CheckButtons;
end;

procedure TFormPropsDlg.ClearButtonClick(Sender: TObject);
begin
  if StoredList.Items.Count > 0 then begin
    SetStoredList(nil);
    if FDesigner <> nil then FDesigner.Modified;
  end;
end;

procedure TFormPropsDlg.DeleteButtonClick(Sender: TObject);
begin
  DeleteProp(StoredList.ItemIndex);
end;

procedure TFormPropsDlg.ListClick(Sender: TObject);
begin
  if Sender = ComponentsList then UpdateCurrent
  else CheckButtons;
end;

procedure TFormPropsDlg.FormDestroy(Sender: TObject);
begin
  ClearLists;
end;

procedure TFormPropsDlg.StoredListClick(Sender: TObject);
begin
  CheckButtons;
end;

procedure TFormPropsDlg.UpBtnClick(Sender: TObject);
begin
  BoxMoveFocusedItem(StoredList, StoredList.ItemIndex - 1);
  if FDesigner <> nil then FDesigner.Modified;
  CheckButtons;
end;

procedure TFormPropsDlg.DownBtnClick(Sender: TObject);
begin
  BoxMoveFocusedItem(StoredList, StoredList.ItemIndex + 1);
  if FDesigner <> nil then FDesigner.Modified;
  CheckButtons;
end;

procedure TFormPropsDlg.StoredListDragOver(Sender, Source: TObject; X,
  Y: Integer; State: TDragState; var Accept: Boolean);
begin
  BoxDragOver(StoredList, Source, X, Y, State, Accept, StoredList.Sorted);
  CheckButtons;
end;

procedure TFormPropsDlg.StoredListDragDrop(Sender, Source: TObject; X,
  Y: Integer);
begin
  BoxMoveFocusedItem(StoredList, StoredList.ItemAtPos(Point(X, Y), True));
  if FDesigner <> nil then FDesigner.Modified;
  CheckButtons;
end;

procedure TFormPropsDlg.PropertiesListDblClick(Sender: TObject);
begin
  if AddButton.Enabled then AddButtonClick(nil);
end;

end.