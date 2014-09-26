{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995, 1996 AO ROSNO             }
{         Copyright (c) 1997 Master-Bank                }
{                                                       }
{*******************************************************}

unit mxPlacemnt;

{$I mx.INC}

interface

uses {$IFDEF WIN32} Windows, Registry, {$ELSE} WinTypes, WinProcs, {$ENDIF}
  Controls, Messages, Classes, Forms, IniFiles, Dialogs, mxVclUtils, mxHook;

type
  TPlacementOption = (fpState, fpPosition, fpActiveControl);
  TPlacementOptions = set of TPlacementOption;
  TPlacementOperation = (poSave, poRestore);
{$IFDEF WIN32}
  TPlacementRegRoot = (prCurrentUser, prLocalMachine, prCurrentConfig,
    prClassesRoot, prUsers, prDynData);
{$ENDIF}

  TIniLink = class;

{ TWinMinMaxInfo }

  TFormPlacement = class;

  TWinMinMaxInfo = class(TPersistent)
  private
    FOwner: TFormPlacement;
    FMinMaxInfo: TMinMaxInfo;
    function GetMinMaxInfo(Index: Integer): Integer;
    procedure SetMinMaxInfo(Index: Integer; Value: Integer);
  public
    function DefaultMinMaxInfo: Boolean;
    procedure Assign(Source: TPersistent); override;
  published
    property MaxPosLeft: Integer index 0 read GetMinMaxInfo write SetMinMaxInfo default 0;
    property MaxPosTop: Integer index 1 read GetMinMaxInfo write SetMinMaxInfo default 0;
    property MaxSizeHeight: Integer index 2 read GetMinMaxInfo write SetMinMaxInfo default 0;
    property MaxSizeWidth: Integer index 3 read GetMinMaxInfo write SetMinMaxInfo default 0;
    property MaxTrackHeight: Integer index 4 read GetMinMaxInfo write SetMinMaxInfo default 0;
    property MaxTrackWidth: Integer index 5 read GetMinMaxInfo write SetMinMaxInfo default 0;
    property MinTrackHeight: Integer index 6 read GetMinMaxInfo write SetMinMaxInfo default 0;
    property MinTrackWidth: Integer index 7 read GetMinMaxInfo write SetMinMaxInfo default 0;
  end;

{ TFormPlacement }

  TFormPlacement = class(TComponent)
  private
    FActive: Boolean;
    FIniFileName: PString;
    FIniSection: PString;
    FIniFile: TIniFile;
    FUseRegistry: Boolean;
{$IFDEF WIN32}
    FRegIniFile: TRegIniFile;
    FRegistryRoot: TPlacementRegRoot;
{$ENDIF WIN32}
    FLinks: TList;
    FOptions: TPlacementOptions;
    FVersion: Integer;
    FSaved: Boolean;
    FRestored: Boolean;
    FDestroying: Boolean;
    FPreventResize: Boolean;
    FWinMinMaxInfo: TWinMinMaxInfo;
    FDefMaximize: Boolean;
    FWinHook: TRxWindowHook;
    FSaveFormShow: TNotifyEvent;
    FSaveFormDestroy: TNotifyEvent;
    FSaveFormCloseQuery: TCloseQueryEvent;
    FOnSavePlacement: TNotifyEvent;
    FOnRestorePlacement: TNotifyEvent;
    procedure SetEvents;
    procedure RestoreEvents;
    procedure SetHook;
    procedure ReleaseHook;
    procedure CheckToggleHook;
    function CheckMinMaxInfo: Boolean;
    procedure MinMaxInfoModified;
    procedure SetWinMinMaxInfo(Value: TWinMinMaxInfo);
    function GetIniSection: string;
    procedure SetIniSection(const Value: string);
    function GetIniFileName: string;
    procedure SetIniFileName(const Value: string);
    function GetIniFile: TObject;
    procedure SetPreventResize(Value: Boolean);
    procedure UpdatePreventResize;
    procedure UpdatePlacement;
    procedure IniNeeded(ReadOnly: Boolean);
    procedure IniFree;
    procedure AddLink(ALink: TIniLink);
    procedure NotifyLinks(Operation: TPlacementOperation);
    procedure RemoveLink(ALink: TIniLink);
    procedure WndMessage(Sender: TObject; var Msg: TMessage; var Handled: Boolean);
    procedure FormShow(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
    procedure FormDestroy(Sender: TObject);
    function GetForm: TForm;
  protected
    procedure Loaded; override;
    procedure Save; dynamic;
    procedure Restore; dynamic;
    procedure SavePlacement; virtual;
    procedure RestorePlacement; virtual;
    function DoReadString(const Section, Ident, Default: string): string; virtual;
    procedure DoWriteString(const Section, Ident, Value: string); virtual;
    property Form: TForm read GetForm;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    procedure SaveFormPlacement;
    procedure RestoreFormPlacement;
    function ReadString(const Ident, Default: string): string;
    procedure WriteString(const Ident, Value: string);
    function ReadInteger(const Ident: string; Default: Longint): Longint;
    procedure WriteInteger(const Ident: string; Value: Longint);
    procedure EraseSections;
    property IniFileObject: TObject read GetIniFile;
    property IniFile: TIniFile read FIniFile;
{$IFDEF WIN32}
    property RegIniFile: TRegIniFile read FRegIniFile;
{$ENDIF WIN32}
  published
    property Active: Boolean read FActive write FActive default True;
    property IniFileName: string read GetIniFileName write SetIniFileName;
    property IniSection: string read GetIniSection write SetIniSection;
    property MinMaxInfo: TWinMinMaxInfo read FWinMinMaxInfo write SetWinMinMaxInfo;
    property Options: TPlacementOptions read FOptions write FOptions default [fpState, fpPosition];
    property PreventResize: Boolean read FPreventResize write SetPreventResize default False;
{$IFDEF WIN32}
    property RegistryRoot: TPlacementRegRoot read FRegistryRoot write FRegistryRoot default prCurrentUser;
{$ENDIF WIN32}
    property UseRegistry: Boolean read FUseRegistry write FUseRegistry default False;
    property Version: Integer read FVersion write FVersion default 0;
    property OnSavePlacement: TNotifyEvent read FOnSavePlacement
      write FOnSavePlacement;
    property OnRestorePlacement: TNotifyEvent read FOnRestorePlacement
      write FOnRestorePlacement;
  end;

{ TFormStorage }

{$IFDEF RX_D3}
  TStoredValues = class;
  TStoredValue = class;
{$ENDIF RX_D3}

  TFormStorage = class(TFormPlacement)
  private
    FStoredProps: TStrings;
{$IFDEF RX_D3}
    FStoredValues: TStoredValues;
{$ENDIF RX_D3}
    procedure SetStoredProps(Value: TStrings);
{$IFDEF RX_D3}
    procedure SetStoredValues(Value: TStoredValues);
    function GetStoredValue(const Name: string): Variant;
    procedure SetStoredValue(const Name: string; Value: Variant);
{$ENDIF RX_D3}
  protected
    procedure Loaded; override;
    procedure Notification(AComponent: TComponent; Operation: TOperation); override;
    procedure SavePlacement; override;
    procedure RestorePlacement; override;
    procedure SaveProperties; virtual;
    procedure RestoreProperties; virtual;
    procedure WriteState(Writer: TWriter); override;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
{$IFDEF WIN32}
    procedure SetNotification;
{$ENDIF WIN32}
{$IFDEF RX_D3}
    property StoredValue[const Name: string]: Variant read GetStoredValue write SetStoredValue;
{$ENDIF RX_D3}
  published
    property StoredProps: TStrings read FStoredProps write SetStoredProps;
{$IFDEF RX_D3}
    property StoredValues: TStoredValues read FStoredValues write SetStoredValues;
{$ENDIF RX_D3}
  end;

{ TIniLink }

  TIniLink = class(TPersistent)
  private
    FStorage: TFormPlacement;
    FOnSave: TNotifyEvent;
    FOnLoad: TNotifyEvent;
    function GetIniObject: TObject;
    function GetRootSection: string;
    procedure SetStorage(Value: TFormPlacement);
  protected
    procedure SaveToIni; virtual;
    procedure LoadFromIni; virtual;
  public
    destructor Destroy; override;
    property IniObject: TObject read GetIniObject;
    property Storage: TFormPlacement read FStorage write SetStorage;
    property RootSection: string read GetRootSection;
    property OnSave: TNotifyEvent read FOnSave write FOnSave;
    property OnLoad: TNotifyEvent read FOnLoad write FOnLoad;
  end;

{$IFDEF RX_D3}

{ TStoredValue }

  TStoredValueEvent = procedure(Sender: TStoredValue; var Value: Variant) of object;

  TStoredValue = class(TCollectionItem)
  private
    FName: string;
    FValue: Variant;
    FKeyString: string;
    FOnSave: TStoredValueEvent;
    FOnRestore: TStoredValueEvent;
    function IsValueStored: Boolean;
    function GetStoredValues: TStoredValues;
  protected
    function GetDisplayName: string; override;
    procedure SetDisplayName(const Value: string); override;
  public
    constructor Create(Collection: TCollection); override;
    procedure Assign(Source: TPersistent); override;
    procedure Clear;
    procedure Save; virtual;
    procedure Restore; virtual;
    property StoredValues: TStoredValues read GetStoredValues;
  published
    property Name: string read FName write SetDisplayName;
    property Value: Variant read FValue write FValue stored IsValueStored;
    property KeyString: string read FKeyString write FKeyString;
    property OnSave: TStoredValueEvent read FOnSave write FOnSave;
    property OnRestore: TStoredValueEvent read FOnRestore write FOnRestore;
  end;

{ TStoredValues }

  TStoredValues = class({$IFDEF RX_D4}TOwnedCollection{$ELSE}TCollection{$ENDIF})
  private
    FStorage: TFormPlacement;
    function GetValue(const Name: string): TStoredValue;
    procedure SetValue(const Name: string; StoredValue: TStoredValue);
    function GetStoredValue(const Name: string): Variant;
    procedure SetStoredValue(const Name: string; Value: Variant);
    function GetItem(Index: Integer): TStoredValue;
    procedure SetItem(Index: Integer; StoredValue: TStoredValue);
  public
{$IFDEF RX_D4}
    constructor Create(AOwner: TPersistent);
{$ELSE}
    constructor Create;
{$ENDIF}
    function IndexOf(const Name: string): Integer;
    procedure SaveValues; virtual;
    procedure RestoreValues; virtual;
    property Storage: TFormPlacement read FStorage write FStorage;
    property Items[Index: Integer]: TStoredValue read GetItem write SetItem; default;
    property Values[const Name: string]: TStoredValue read GetValue write SetValue;
    property StoredValue[const Name: string]: Variant read GetStoredValue write SetStoredValue;
  end;

{$ENDIF RX_D3}

implementation

uses SysUtils,
{$IFDEF RX_D3}
  Consts,
{$ENDIF RX_D3}
  mxAppUtils, mxStrUtils, mxProps, Variants, RTLConsts;

const
{ The following string should not be localized }
  siActiveCtrl = 'ActiveControl';
  siVisible = 'Visible';
  siVersion = 'FormVersion';

{ TFormPlacement }

constructor TFormPlacement.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FIniFileName := NullStr;
  FIniSection := NullStr;
  FActive := True;
  if AOwner is TForm then FOptions := [fpState, fpPosition]
  else FOptions := [];
  FWinHook := TRxWindowHook.Create(Self);
  FWinHook.AfterMessage := WndMessage;
  FWinMinMaxInfo := TWinMinMaxInfo.Create;
  FWinMinMaxInfo.FOwner := Self;
  FLinks := TList.Create;
end;

destructor TFormPlacement.Destroy;
begin
  IniFree;
  while FLinks.Count > 0 do RemoveLink(FLinks.Last);
  FLinks.Free;
  if not (csDesigning in ComponentState) then begin
    ReleaseHook;
    RestoreEvents;
  end;
  DisposeStr(FIniFileName);
  DisposeStr(FIniSection);
  FWinMinMaxInfo.Free;
  inherited Destroy;
end;

procedure TFormPlacement.Loaded;
var
  Loading: Boolean;
begin
  Loading := csLoading in ComponentState;
  inherited Loaded;
  if not (csDesigning in ComponentState) then begin
    if Loading then SetEvents;
    CheckToggleHook;
  end;
end;

procedure TFormPlacement.AddLink(ALink: TIniLink);
begin
  FLinks.Add(ALink);
  ALink.FStorage := Self;
end;

procedure TFormPlacement.NotifyLinks(Operation: TPlacementOperation);
var
  I: Integer;
begin
  for I := 0 to FLinks.Count - 1 do
    with TIniLink(FLinks[I]) do
      case Operation of
        poSave: SaveToIni;
        poRestore: LoadFromIni;
      end;
end;

procedure TFormPlacement.RemoveLink(ALink: TIniLink);
begin
  ALink.FStorage := nil;
  FLinks.Remove(ALink);
end;

function TFormPlacement.GetForm: TForm;
begin
  if Owner is TCustomForm then Result := TForm(Owner as TCustomForm)
  else Result := nil;
end;

procedure TFormPlacement.SetEvents;
begin
  if Owner is TCustomForm then begin
    with TForm(Form) do begin
      FSaveFormShow := OnShow;
      OnShow := FormShow;
      FSaveFormCloseQuery := OnCloseQuery;
      OnCloseQuery := FormCloseQuery;
      FSaveFormDestroy := OnDestroy;
      OnDestroy := FormDestroy;
      FDefMaximize := (biMaximize in BorderIcons);
    end;
    if FPreventResize then UpdatePreventResize;
  end;
end;

procedure TFormPlacement.RestoreEvents;
begin
  if (Owner <> nil) and (Owner is TCustomForm) then
    with TForm(Form) do begin
      OnShow := FSaveFormShow;
      OnCloseQuery := FSaveFormCloseQuery;
      OnDestroy := FSaveFormDestroy;
    end;
end;

procedure TFormPlacement.SetHook;
begin
  if not (csDesigning in ComponentState) and (Owner <> nil) and
    (Owner is TCustomForm) then
    FWinHook.WinControl := Form;
end;

procedure TFormPlacement.ReleaseHook;
begin
  FWinHook.WinControl := nil;
end;

procedure TFormPlacement.CheckToggleHook;
begin
  if CheckMinMaxInfo or PreventResize then SetHook else ReleaseHook;
end;

function TFormPlacement.CheckMinMaxInfo: Boolean;
begin
  Result := not FWinMinMaxInfo.DefaultMinMaxInfo;
end;

procedure TFormPlacement.MinMaxInfoModified;
begin
  UpdatePlacement;
  if not (csLoading in ComponentState) then CheckToggleHook;
end;

procedure TFormPlacement.SetWinMinMaxInfo(Value: TWinMinMaxInfo);
begin
  FWinMinMaxInfo.Assign(Value);
end;

procedure TFormPlacement.WndMessage(Sender: TObject; var Msg: TMessage;
  var Handled: Boolean);
begin
  if FPreventResize and (Owner is TCustomForm) then begin
    case Msg.Msg of
      WM_GETMINMAXINFO:
        if Form.HandleAllocated and IsWindowVisible(Form.Handle) then begin
          with TWMGetMinMaxInfo(Msg).MinMaxInfo^ do begin
            ptMinTrackSize := Point(Form.Width, Form.Height);
            ptMaxTrackSize := Point(Form.Width, Form.Height);
          end;
          Msg.Result := 1;
        end;
      WM_INITMENUPOPUP:
        if TWMInitMenuPopup(Msg).SystemMenu then begin
          if Form.Menu <> nil then
            Form.Menu.DispatchPopup(TWMInitMenuPopup(Msg).MenuPopup);
          EnableMenuItem(TWMInitMenuPopup(Msg).MenuPopup, SC_SIZE,
            MF_BYCOMMAND or MF_GRAYED);
          EnableMenuItem(TWMInitMenuPopup(Msg).MenuPopup, SC_MAXIMIZE,
            MF_BYCOMMAND or MF_GRAYED);
          Msg.Result := 1;
        end;
      WM_NCHITTEST:
        begin
          if Msg.Result in [HTLEFT, HTRIGHT, HTBOTTOM, HTBOTTOMRIGHT,
            HTBOTTOMLEFT, HTTOP, HTTOPRIGHT, HTTOPLEFT]
          then Msg.Result := HTNOWHERE;
        end;
    end;
  end
  else if (Msg.Msg = WM_GETMINMAXINFO) then begin
    if CheckMinMaxInfo then begin
      with TWMGetMinMaxInfo(Msg).MinMaxInfo^ do begin
         if FWinMinMaxInfo.MinTrackWidth <> 0 then
           ptMinTrackSize.X := FWinMinMaxInfo.MinTrackWidth;
         if FWinMinMaxInfo.MinTrackHeight <> 0 then
           ptMinTrackSize.Y := FWinMinMaxInfo.MinTrackHeight;
         if FWinMinMaxInfo.MaxTrackWidth <> 0 then
           ptMaxTrackSize.X := FWinMinMaxInfo.MaxTrackWidth;
         if FWinMinMaxInfo.MaxTrackHeight <> 0 then
           ptMaxTrackSize.Y := FWinMinMaxInfo.MaxTrackHeight;
         if FWinMinMaxInfo.MaxSizeWidth <> 0 then
           ptMaxSize.X := FWinMinMaxInfo.MaxSizeWidth;
         if FWinMinMaxInfo.MaxSizeHeight <> 0 then
           ptMaxSize.Y := FWinMinMaxInfo.MaxSizeHeight;
         if FWinMinMaxInfo.MaxPosLeft <> 0 then
           ptMaxPosition.X := FWinMinMaxInfo.MaxPosLeft;
         if FWinMinMaxInfo.MaxPosTop <> 0 then
           ptMaxPosition.Y := FWinMinMaxInfo.MaxPosTop;
      end;
    end
    else begin
      TWMGetMinMaxInfo(Msg).MinMaxInfo^.ptMaxPosition.X := 0;
      TWMGetMinMaxInfo(Msg).MinMaxInfo^.ptMaxPosition.Y := 0;
    end;
    Msg.Result := 1;
  end;
end;

procedure TFormPlacement.FormShow(Sender: TObject);
begin
  if Active then
    try
      RestoreFormPlacement;
    except
      Application.HandleException(Self);
    end;
  if Assigned(FSaveFormShow) then FSaveFormShow(Sender);
end;

procedure TFormPlacement.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  if Assigned(FSaveFormCloseQuery) then
    FSaveFormCloseQuery(Sender, CanClose);
  if CanClose and Active and (Owner is TCustomForm) and (Form.Handle <> 0) then
    try
      SaveFormPlacement;
    except
      Application.HandleException(Self);
    end;
end;

procedure TFormPlacement.FormDestroy(Sender: TObject);
begin
  if Active and not FSaved then begin
    FDestroying := True;
    try
      SaveFormPlacement;
    except
      Application.HandleException(Self);
    end;
    FDestroying := False;
  end;
  if Assigned(FSaveFormDestroy) then FSaveFormDestroy(Sender);
end;

procedure TFormPlacement.UpdatePlacement;
const
{$IFDEF WIN32}
  Metrics: array[bsSingle..bsSizeToolWin] of Word =
    (SM_CXBORDER, SM_CXFRAME, SM_CXDLGFRAME, SM_CXBORDER, SM_CXFRAME);
{$ELSE}
  Metrics: array[bsSingle..bsDialog] of Word =
    (SM_CXBORDER, SM_CXFRAME, SM_CXDLGFRAME);
{$ENDIF}
var
  Placement: TWindowPlacement;
begin
  if (Owner <> nil) and (Owner is TCustomForm) and Form.HandleAllocated and
  not (csLoading in ComponentState) then
    if not (FPreventResize or CheckMinMaxInfo) then begin
      Placement.Length := SizeOf(TWindowPlacement);
      GetWindowPlacement(Form.Handle, @Placement);
      if not IsWindowVisible(Form.Handle) then
        Placement.ShowCmd := SW_HIDE;
      if Form.BorderStyle <> bsNone then begin
        Placement.ptMaxPosition.X := -GetSystemMetrics(Metrics[Form.BorderStyle]);
        Placement.ptMaxPosition.Y := -GetSystemMetrics(Metrics[Form.BorderStyle] + 1);
      end
      else Placement.ptMaxPosition := Point(0, 0);
      SetWindowPlacement(Form.Handle, @Placement);
    end;
end;

procedure TFormPlacement.UpdatePreventResize;
var
  IsActive: Boolean;
begin
  if not (csDesigning in ComponentState) and (Owner is TCustomForm) then
  begin
    if FPreventResize then
      FDefMaximize := (biMaximize in Form.BorderIcons);
    IsActive := Active;
    Active := False;
    try
      if (not FPreventResize) and FDefMaximize and
        (Form.BorderStyle <> bsDialog) then
        Form.BorderIcons := Form.BorderIcons + [biMaximize]
      else Form.BorderIcons := Form.BorderIcons - [biMaximize];
    finally
      Active := IsActive;
    end;
    if not (csLoading in ComponentState) then CheckToggleHook;
  end;
end;

procedure TFormPlacement.SetPreventResize(Value: Boolean);
begin
  if (Form <> nil) and (FPreventResize <> Value) then begin
    FPreventResize := Value;
    UpdatePlacement;
    UpdatePreventResize;
  end;
end;

function TFormPlacement.GetIniFile: TObject;
begin
{$IFDEF WIN32}
  if UseRegistry then Result := FRegIniFile
  else Result := FIniFile;
{$ELSE}
  Result := FIniFile;
{$ENDIF WIN32}
end;

function TFormPlacement.GetIniFileName: string;
begin
  Result := FIniFileName^;
  if (Result = '') and not (csDesigning in ComponentState) then begin
{$IFDEF WIN32}
    if UseRegistry then Result := GetDefaultIniRegKey
    else Result := GetDefaultIniName;
{$ELSE}
    Result := GetDefaultIniName;
{$ENDIF}
  end;
end;

procedure TFormPlacement.SetIniFileName(const Value: string);
begin
  AssignStr(FIniFileName, Value);
end;

function TFormPlacement.GetIniSection: string;
begin
  Result := FIniSection^;
  if (Result = '') and not (csDesigning in ComponentState) then
    Result := GetDefaultSection(Owner);
end;

procedure TFormPlacement.SetIniSection(const Value: string);
begin
  AssignStr(FIniSection, Value);
end;

procedure TFormPlacement.Save;
begin
  if Assigned(FOnSavePlacement) then FOnSavePlacement(Self);
end;

procedure TFormPlacement.Restore;
begin
  if Assigned(FOnRestorePlacement) then FOnRestorePlacement(Self);
end;

procedure TFormPlacement.SavePlacement;
begin
  if Owner is TCustomForm then begin
{$IFDEF WIN32}
    if UseRegistry then begin
      if (Options * [fpState, fpPosition] <> []) then begin
        WriteFormPlacementReg(Form, FRegIniFile, IniSection);
        FRegIniFile.WriteBool(IniSection, siVisible, FDestroying);
      end;
      if (fpActiveControl in Options) and (Form.ActiveControl <> nil) then
        FRegIniFile.WriteString(IniSection, siActiveCtrl, Form.ActiveControl.Name);
    end
    else begin
      if (Options * [fpState, fpPosition] <> []) then begin
        WriteFormPlacement(Form, FIniFile, IniSection);
        FIniFile.WriteBool(IniSection, siVisible, FDestroying);
      end;
      if (fpActiveControl in Options) and (Form.ActiveControl <> nil) then
        FIniFile.WriteString(IniSection, siActiveCtrl, Form.ActiveControl.Name);
    end;
{$ELSE}
    if (Options * [fpState, fpPosition] <> []) then begin
      WriteFormPlacement(Form, FIniFile, IniSection);
      FIniFile.WriteBool(IniSection, siVisible, FDestroying);
    end;
    if (fpActiveControl in Options) and (Form.ActiveControl <> nil) then
      FIniFile.WriteString(IniSection, siActiveCtrl, Form.ActiveControl.Name);
{$ENDIF}
  end;
  NotifyLinks(poSave);
end;

procedure TFormPlacement.RestorePlacement;
begin
  if Owner is TCustomForm then begin
{$IFDEF WIN32}
    if UseRegistry then
      ReadFormPlacementReg(Form, FRegIniFile, IniSection, fpState in Options,
        fpPosition in Options)
    else
{$ENDIF}
      ReadFormPlacement(Form, FIniFile, IniSection, fpState in Options,
        fpPosition in Options);
  end;
  NotifyLinks(poRestore);
end;

procedure TFormPlacement.IniNeeded(ReadOnly: Boolean);
begin
  if IniFileObject = nil then begin
{$IFDEF WIN32}
    if UseRegistry then begin
      FRegIniFile := TRegIniFile.Create(IniFileName);
{$IFDEF RX_D5}
      if ReadOnly then FRegIniFile.Access := KEY_READ;
{$ENDIF}
      case FRegistryRoot of
        prLocalMachine:
          FRegIniFile.RootKey := HKEY_LOCAL_MACHINE;
        prClassesRoot: 
          FRegIniFile.RootKey := HKEY_CLASSES_ROOT;
        prCurrentConfig: 
          FRegIniFile.RootKey := HKEY_CURRENT_CONFIG;
        prUsers: 
          FRegIniFile.RootKey := HKEY_USERS;
        prDynData:
          FRegIniFile.RootKey := HKEY_DYN_DATA;
      end;
      if FRegIniFile.RootKey <> HKEY_CURRENT_USER then
        FRegIniFile.OpenKey(FRegIniFile.FileName, not ReadOnly);
    end
    else
{$ENDIF}
    FIniFile := TIniFile.Create(IniFileName);
  end;
end;

procedure TFormPlacement.IniFree;
begin
  if IniFileObject <> nil then begin
    IniFileObject.Free;
    FIniFile := nil;
{$IFDEF WIN32}
    FRegIniFile := nil;
{$ENDIF}
  end;
end;

function TFormPlacement.DoReadString(const Section, Ident,
  Default: string): string;
begin
  if IniFileObject <> nil then
    Result := IniReadString(IniFileObject, Section, Ident, Default)
  else begin
    IniNeeded(True);
    try
      Result := IniReadString(IniFileObject, Section, Ident, Default);
    finally
      IniFree;
    end;
  end;
end;

function TFormPlacement.ReadString(const Ident, Default: string): string;
begin
  Result := DoReadString(IniSection, Ident, Default);
end;

procedure TFormPlacement.DoWriteString(const Section, Ident, Value: string);
begin
  if IniFileObject <> nil then
    IniWriteString(IniFileObject, Section, Ident, Value)
  else begin
    IniNeeded(False);
    try
      IniWriteString(IniFileObject, Section, Ident, Value);
    finally
      IniFree;
    end;
  end;
end;

procedure TFormPlacement.WriteString(const Ident, Value: string);
begin
  DoWriteString(IniSection, Ident, Value);
end;

function TFormPlacement.ReadInteger(const Ident: string; Default: Longint): Longint;
begin
  if IniFileObject <> nil then
    Result := IniReadInteger(IniFileObject, IniSection, Ident, Default)
  else begin
    IniNeeded(True);
    try
      Result := IniReadInteger(IniFileObject, IniSection, Ident, Default);
    finally
      IniFree;
    end;
  end;
end;

procedure TFormPlacement.WriteInteger(const Ident: string; Value: Longint);
begin
  if IniFileObject <> nil then
    IniWriteInteger(IniFileObject, IniSection, Ident, Value)
  else begin
    IniNeeded(False);
    try
      IniWriteInteger(IniFileObject, IniSection, Ident, Value);
    finally
      IniFree;
    end;
  end;
end;

procedure TFormPlacement.EraseSections;
var
  Lines: TStrings;
  I: Integer;
begin
  if IniFileObject = nil then begin
    IniNeeded(False);
    try
      Lines := TStringList.Create;
      try
        IniReadSections(IniFileObject, Lines);
        for I := 0 to Lines.Count - 1 do begin
          if (Lines[I] = IniSection) or
            (IsWild(Lines[I], IniSection + '.*', False) or
            IsWild(Lines[I], IniSection + '\*', False)) then
            IniEraseSection(IniFileObject, Lines[I]);
        end;
      finally
        Lines.Free;
      end;
    finally
      IniFree;
    end;
  end;
end;

procedure TFormPlacement.SaveFormPlacement;
begin
  if FRestored or not Active then begin
    IniNeeded(False);
    try
      WriteInteger(siVersion, FVersion);
      SavePlacement;
      Save;
      FSaved := True;
    finally
      IniFree;
    end;
  end;
end;

procedure TFormPlacement.RestoreFormPlacement;
var
  cActive: TComponent;
begin
  FSaved := False;
  IniNeeded(True);
  try
    if ReadInteger(siVersion, 0) >= FVersion then begin
      RestorePlacement;
      FRestored := True;
      Restore;
      if (fpActiveControl in Options) and (Owner is TCustomForm) then begin
        cActive := Form.FindComponent(IniReadString(IniFileObject,
          IniSection, siActiveCtrl, ''));
        if (cActive <> nil) and (cActive is TWinControl) and
          TWinControl(cActive).CanFocus then
            Form.ActiveControl := TWinControl(cActive);
      end;
    end;
    FRestored := True;
  finally
    IniFree;
  end;
  UpdatePlacement;
end;

{ TWinMinMaxInfo }

procedure TWinMinMaxInfo.Assign(Source: TPersistent);
begin
  if Source is TWinMinMaxInfo then begin
    FMinMaxInfo := TWinMinMaxInfo(Source).FMinMaxInfo;
    if FOwner <> nil then FOwner.MinMaxInfoModified;
  end
  else inherited Assign(Source);
end;

function TWinMinMaxInfo.GetMinMaxInfo(Index: Integer): Integer;
begin
  with FMinMaxInfo do begin
    case Index of
      0: Result := ptMaxPosition.X;
      1: Result := ptMaxPosition.Y;
      2: Result := ptMaxSize.Y;
      3: Result := ptMaxSize.X;
      4: Result := ptMaxTrackSize.Y;
      5: Result := ptMaxTrackSize.X;
      6: Result := ptMinTrackSize.Y;
      7: Result := ptMinTrackSize.X;
      else Result := 0;
    end;
  end;
end;

procedure TWinMinMaxInfo.SetMinMaxInfo(Index: Integer; Value: Integer);
begin
  if GetMinMaxInfo(Index) <> Value then begin
    with FMinMaxInfo do begin
      case Index of
        0: ptMaxPosition.X := Value;
        1: ptMaxPosition.Y := Value;
        2: ptMaxSize.Y := Value;
        3: ptMaxSize.X := Value;
        4: ptMaxTrackSize.Y := Value;
        5: ptMaxTrackSize.X := Value;
        6: ptMinTrackSize.Y := Value;
        7: ptMinTrackSize.X := Value;
      end;
    end;
    if FOwner <> nil then FOwner.MinMaxInfoModified;
  end;
end;

function TWinMinMaxInfo.DefaultMinMaxInfo: Boolean;
begin
  with FMinMaxInfo do begin
    Result := not ((ptMinTrackSize.X <> 0) or (ptMinTrackSize.Y <> 0) or
      (ptMaxTrackSize.X <> 0) or (ptMaxTrackSize.Y <> 0) or
      (ptMaxSize.X <> 0) or (ptMaxSize.Y <> 0) or
      (ptMaxPosition.X <> 0) or (ptMaxPosition.Y <> 0));
  end;
end;

{ TFormStorage }

constructor TFormStorage.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FStoredProps := TStringList.Create;
{$IFDEF RX_D3}
  FStoredValues := TStoredValues.Create{$IFDEF RX_D4}(Self){$ENDIF RX_D4};
  FStoredValues.Storage := Self;
{$ENDIF RX_D3}
end;

destructor TFormStorage.Destroy;
begin
  FStoredProps.Free;
  FStoredProps := nil;
{$IFDEF RX_D3}
  FStoredValues.Free;
  FStoredValues := nil;
{$ENDIF RX_D3}
  inherited Destroy;
end;

{$IFDEF WIN32}
procedure TFormStorage.SetNotification;
var
  I: Integer;
  Component: TComponent;
begin
  for I := FStoredProps.Count - 1 downto 0 do begin
    Component := TComponent(FStoredProps.Objects[I]);
    if Component <> nil then Component.FreeNotification(Self);
  end;
end;
{$ENDIF WIN32}

procedure TFormStorage.SetStoredProps(Value: TStrings);
begin
  FStoredProps.Assign(Value);
{$IFDEF WIN32}
  SetNotification;
{$ENDIF}
end;

{$IFDEF RX_D3}
procedure TFormStorage.SetStoredValues(Value: TStoredValues);
begin
  FStoredValues.Assign(Value);
end;

function TFormStorage.GetStoredValue(const Name: string): Variant;
begin
  Result := StoredValues.StoredValue[Name];
end;

procedure TFormStorage.SetStoredValue(const Name: string; Value: Variant);
begin
  StoredValues.StoredValue[Name] := Value;
end;

{$ENDIF RX_D3}

procedure TFormStorage.Loaded;
begin
  inherited Loaded;
  UpdateStoredList(Owner, FStoredProps, True);
end;

procedure TFormStorage.WriteState(Writer: TWriter);
begin
  UpdateStoredList(Owner, FStoredProps, False);
  inherited WriteState(Writer);
end;

procedure TFormStorage.Notification(AComponent: TComponent; Operation: TOperation);
var
  I: Integer;
  Component: TComponent;
begin
  inherited Notification(AComponent, Operation);
  if not (csDestroying in ComponentState) and (Operation = opRemove) and
    (FStoredProps <> nil) then
    for I := FStoredProps.Count - 1 downto 0 do begin
      Component := TComponent(FStoredProps.Objects[I]);
      if Component = AComponent then FStoredProps.Delete(I);
    end;
end;

procedure TFormStorage.SaveProperties;
begin
  with TPropsStorage.Create do
  try
    Section := IniSection;
    OnWriteString := DoWriteString;
{$IFDEF WIN32}
    if UseRegistry then OnEraseSection := FRegIniFile.EraseSection
    else OnEraseSection := FIniFile.EraseSection;
{$ELSE}
    OnEraseSection := FIniFile.EraseSection;
{$ENDIF WIN32}
    StoreObjectsProps(Owner, FStoredProps);
  finally
    Free;
  end;
end;

procedure TFormStorage.RestoreProperties;
begin
  with TPropsStorage.Create do
  try
    Section := IniSection;
    OnReadString := DoReadString;
    try
      LoadObjectsProps(Owner, FStoredProps);
    except
      { ignore any exceptions }
    end;
  finally
    Free;
  end;
end;

procedure TFormStorage.SavePlacement;
begin
  inherited SavePlacement;
  SaveProperties;
{$IFDEF RX_D3}
  StoredValues.SaveValues;
{$ENDIF}
end;

procedure TFormStorage.RestorePlacement;
begin
  inherited RestorePlacement;
  FRestored := True;
  RestoreProperties;
{$IFDEF RX_D3}
  StoredValues.RestoreValues;
{$ENDIF}
end;

{ TIniLink }

destructor TIniLink.Destroy;
begin
  FOnSave := nil;
  FOnLoad := nil;
  SetStorage(nil);
  inherited Destroy;
end;

function TIniLink.GetIniObject: TObject;
begin
  if Assigned(FStorage) then Result := FStorage.IniFileObject
  else Result := nil;
end;

function TIniLink.GetRootSection: string;
begin
  if Assigned(FStorage) then Result := FStorage.FIniSection^
  else Result := '';
  if Result <> '' then Result := Result + '\';
end;

procedure TIniLink.SetStorage(Value: TFormPlacement);
begin
  if FStorage <> Value then begin
    if FStorage <> nil then FStorage.RemoveLink(Self);
    if Value <> nil then Value.AddLink(Self);
  end;
end;

procedure TIniLink.SaveToIni;
begin
  if Assigned(FOnSave) then FOnSave(Self);
end;

procedure TIniLink.LoadFromIni;
begin
  if Assigned(FOnLoad) then FOnLoad(Self);
end;

{$IFDEF RX_D3}

{ TStoredValue }

constructor TStoredValue.Create(Collection: TCollection);
begin
  inherited Create(Collection);
  FValue := Unassigned;
end;

procedure TStoredValue.Assign(Source: TPersistent);
begin
  if (Source is TStoredValue) and (Source <> nil) then begin
    if VarIsEmpty(TStoredValue(Source).FValue) then
      Clear
    else
      Value := TStoredValue(Source).FValue;
    Name := TStoredValue(Source).Name;
    KeyString := TStoredValue(Source).KeyString;
  end;
end;

function TStoredValue.GetDisplayName: string;
begin
  if FName = '' then
    Result := inherited GetDisplayName
  else
    Result := FName;
end;

procedure TStoredValue.SetDisplayName(const Value: string);
begin
  if (Value <> '') and (AnsiCompareText(Value, FName) <> 0) and
    (Collection is TStoredValues) and (TStoredValues(Collection).IndexOf(Value) >= 0) then
    raise Exception.Create(SDuplicateString);
  FName := Value;
  inherited;
end;

function TStoredValue.GetStoredValues: TStoredValues;
begin
  if Collection is TStoredValues then
    Result := TStoredValues(Collection)
  else
    Result := nil;
end;

procedure TStoredValue.Clear;
begin
  FValue := Unassigned;
end;

function TStoredValue.IsValueStored: Boolean;
begin
  Result := not VarIsEmpty(FValue);
end;

procedure TStoredValue.Save;
var
  SaveValue: Variant;
  SaveStrValue: string;
begin
  SaveValue := Value;
  if Assigned(FOnSave) then
    FOnSave(Self, SaveValue);
  SaveStrValue := VarToStr(SaveValue);
  if KeyString <> '' then
    SaveStrValue := XorEncode(KeyString, SaveStrValue);
  StoredValues.Storage.WriteString(Name, SaveStrValue);
end;

procedure TStoredValue.Restore;
var
  RestoreValue: Variant;
  RestoreStrValue, DefaultStrValue: string;
begin
  DefaultStrValue := VarToStr(Value);
  if KeyString <> '' then
    DefaultStrValue := XorEncode(KeyString, DefaultStrValue);
  RestoreStrValue := StoredValues.Storage.ReadString(Name, DefaultStrValue);
  if KeyString <> '' then
    RestoreStrValue := XorDecode(KeyString, RestoreStrValue);
  RestoreValue := RestoreStrValue;
  if Assigned(FOnRestore) then
    FOnRestore(Self, RestoreValue);
  Value := RestoreValue;  
end;

{ TStoredValues }

{$IFDEF RX_D4}
constructor TStoredValues.Create(AOwner: TPersistent);
begin
  inherited Create(AOwner, TStoredValue);
end;
{$ELSE}
constructor TStoredValues.Create;
begin
  inherited Create(TStoredValue);
end;
{$ENDIF}

function TStoredValues.IndexOf(const Name: string): Integer;
begin
  for Result := 0 to Count - 1 do
    if AnsiCompareText(Items[Result].Name, Name) = 0 then Exit;
  Result := -1;
end;

function TStoredValues.GetItem(Index: Integer): TStoredValue;
begin
  Result := TStoredValue(inherited Items[Index]);
end;

procedure TStoredValues.SetItem(Index: Integer; StoredValue: TStoredValue);
begin
  inherited SetItem(Index, TCollectionItem(StoredValue));
end;

function TStoredValues.GetStoredValue(const Name: string): Variant;
var
  StoredValue: TStoredValue;
begin
  StoredValue := GetValue(Name);
  if StoredValue = nil then Result := Null
  else Result := StoredValue.Value;
end;

procedure TStoredValues.SetStoredValue(const Name: string; Value: Variant);
var
  StoredValue: TStoredValue;
begin
  StoredValue := GetValue(Name);
  if StoredValue = nil then begin
    StoredValue := TStoredValue(Add);
    StoredValue.Name := Name; 
    StoredValue.Value := Value;
  end
  else StoredValue.Value := Value;
end;

function TStoredValues.GetValue(const Name: string): TStoredValue;
var
  I: Integer;
begin
  I := IndexOf(Name);
  if I < 0 then
    Result := nil
  else
    Result := Items[I];
end;

procedure TStoredValues.SetValue(const Name: string; StoredValue: TStoredValue);
var
  I: Integer;
begin
  I := IndexOf(Name);
  if I >= 0 then
    Items[I].Assign(StoredValue);
end;

procedure TStoredValues.SaveValues;
var
  I: Integer;
begin
  for I := 0 to Count - 1 do
    Items[I].Save;
end;

procedure TStoredValues.RestoreValues;
var
  I: Integer;
begin
  for I := 0 to Count - 1 do
    Items[I].Restore;
end;

{$ENDIF RX_D3}

end.