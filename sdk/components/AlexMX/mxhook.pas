{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1997 Master-Bank                }
{                                                       }
{*******************************************************}

unit mxHook;

{$I mX.INC}
{$T-,W-,X+,P+}

interface

uses {$IFDEF WIN32} Windows, {$ELSE} WinTypes, WinProcs, {$ENDIF}
  Messages, SysUtils, Classes, Controls, Forms, mxConst;

type
  PClass = ^TClass;
  THookMessageEvent = procedure (Sender: TObject; var Msg: TMessage;
    var Handled: Boolean) of object;

  TRxWindowHook = class(TComponent)
  private
    FActive: Boolean;
    FControl: TWinControl;
    FControlHook: TObject;
    FBeforeMessage: THookMessageEvent;
    FAfterMessage: THookMessageEvent;
    function GetWinControl: TWinControl;
    function GetHookHandle: HWnd;
    procedure SetActive(Value: Boolean);
    procedure SetWinControl(Value: TWinControl);
    function IsForm: Boolean;
    function NotIsForm: Boolean;
    function DoUnhookControl: Pointer;
    procedure ReadForm(Reader: TReader);
    procedure WriteForm(Writer: TWriter);
  protected
    procedure DefineProperties(Filer: TFiler); override;
    procedure DoAfterMessage(var Msg: TMessage; var Handled: Boolean); dynamic;
    procedure DoBeforeMessage(var Msg: TMessage; var Handled: Boolean); dynamic;
    procedure Notification(AComponent: TComponent; Operation: TOperation); override;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    procedure HookControl;
    procedure UnhookControl;
    property HookWindow: HWnd read GetHookHandle;
  published
    property Active: Boolean read FActive write SetActive default True;
    property WinControl: TWinControl read GetWinControl write SetWinControl
      stored NotIsForm;
    property BeforeMessage: THookMessageEvent read FBeforeMessage write FBeforeMessage;
    property AfterMessage: THookMessageEvent read FAfterMessage write FAfterMessage;
  end;

function GetVirtualMethodAddress(AClass: TClass; AIndex: Integer): Pointer;
function SetVirtualMethodAddress(AClass: TClass; AIndex: Integer;
  NewAddress: Pointer): Pointer;
function FindVirtualMethodIndex(AClass: TClass; MethodAddr: Pointer): Integer;

implementation

type
  THack = class(TWinControl);
  THookOrder = (hoBeforeMsg, hoAfterMsg);
{$IFNDEF RX_D3}
  TCustomForm = TForm;
{$ENDIF}

{ TControlHook }

  TControlHook = class(TObject)
  private
    FControl: TWinControl;
    FNewWndProc: Pointer;
    FPrevWndProc: Pointer;
    FList: TList;
    FDestroying: Boolean;
    procedure SetWinControl(Value: TWinControl);
    procedure HookWndProc(var AMsg: TMessage);
    procedure NotifyHooks(Order: THookOrder; var Msg: TMessage;
      var Handled: Boolean);
  public
    constructor Create;
    destructor Destroy; override;
    procedure HookControl;
    procedure UnhookControl;
    procedure AddHook(AHook: TRxWindowHook);
    procedure RemoveHook(AHook: TRxWindowHook);
    property WinControl: TWinControl read FControl write SetWinControl;
  end;

{ THookList }

  THookList = class(TList)
  private
    FHandle: HWnd;
    procedure WndProc(var Msg: TMessage);
  public
    constructor Create;
    destructor Destroy; override;
    function FindControlHook(AControl: TWinControl): TControlHook;
    function GetControlHook(AControl: TWinControl): TControlHook;
    property Handle: HWnd read FHandle;
  end;

var
  HookList: THookList;

function GetHookList: THookList;
begin
  if HookList = nil then HookList := THookList.Create;
  Result := HookList;
end;

procedure DropHookList; far;
begin
  HookList.Free;
  HookList := nil;
end;

{ TControlHook }

constructor TControlHook.Create;
begin
  inherited Create;
  FList := TList.Create;
  FNewWndProc := MakeObjectInstance(HookWndProc);
  FPrevWndProc := nil;
  FControl := nil;
end;

destructor TControlHook.Destroy;
begin
  FDestroying := True;
  if Assigned(HookList) then
    if HookList.IndexOf(Self) >= 0 then HookList.Remove(Self);
  while FList.Count > 0 do RemoveHook(TRxWindowHook(FList.Last));
  FControl := nil;
  FList.Free;
  FreeObjectInstance(FNewWndProc);
  FNewWndProc := nil;
  inherited Destroy;
end;

procedure TControlHook.AddHook(AHook: TRxWindowHook);
begin
  if FList.IndexOf(AHook) < 0 then begin
    FList.Add(AHook);
    AHook.FControlHook := Self;
    WinControl := AHook.FControl;
  end;
  HookControl;
end;

procedure TControlHook.RemoveHook(AHook: TRxWindowHook);
begin
  AHook.FControlHook := nil;
  FList.Remove(AHook);
  if FList.Count = 0 then UnhookControl;
end;

procedure TControlHook.NotifyHooks(Order: THookOrder; var Msg: TMessage;
  var Handled: Boolean);
var
  I: Integer;
begin
  if (FList.Count > 0) and Assigned(FControl) and
    not (FDestroying or (csDestroying in FControl.ComponentState)) then
    for I := FList.Count - 1 downto 0 do begin
      try
        if Order = hoBeforeMsg then
          TRxWindowHook(FList[I]).DoBeforeMessage(Msg, Handled)
        else if Order = hoAfterMsg then
          TRxWindowHook(FList[I]).DoAfterMessage(Msg, Handled);
      except
        Application.HandleException(Self);
      end;
      if Handled then Break;
    end;
end;

procedure TControlHook.HookControl;
var
  P: Pointer;
begin
  if Assigned(FControl) and not ((csDesigning in FControl.ComponentState) or
    (csDestroying in FControl.ComponentState) or FDestroying) then
  begin
    FControl.HandleNeeded;
    P := Pointer(GetWindowLong(FControl.Handle, GWL_WNDPROC));
    if (P <> FNewWndProc) then begin
      FPrevWndProc := P;
      SetWindowLong(FControl.Handle, GWL_WNDPROC, LongInt(FNewWndProc));
    end;
  end;
end;

procedure TControlHook.UnhookControl;
begin
  if Assigned(FControl) then begin
    if Assigned(FPrevWndProc) and FControl.HandleAllocated and
    (Pointer(GetWindowLong(FControl.Handle, GWL_WNDPROC)) = FNewWndProc) then
      SetWindowLong(FControl.Handle, GWL_WNDPROC, LongInt(FPrevWndProc));
  end;
  FPrevWndProc := nil;
end;

procedure TControlHook.HookWndProc(var AMsg: TMessage);
var
  Handled: Boolean;
begin
  Handled := False;
  if Assigned(FControl) then begin
    if (AMsg.Msg <> WM_QUIT) then NotifyHooks(hoBeforeMsg, AMsg, Handled);
    with AMsg do begin
      if (not Handled) or (Msg = WM_DESTROY) then
        try
          if Assigned(FPrevWndProc) then
            Result := CallWindowProc(FPrevWndProc, FControl.Handle, Msg,
              WParam, LParam)
          else
            Result := CallWindowProc(THack(FControl).DefWndProc,
              FControl.Handle, Msg, WParam, LParam);
        finally
          NotifyHooks(hoAfterMsg, AMsg, Handled);
        end;
      if Msg = WM_DESTROY then begin
        UnhookControl;
        if Assigned(HookList) and not (FDestroying or
          (csDestroying in FControl.ComponentState)) then
          PostMessage(HookList.FHandle, CM_RECREATEWINDOW, 0, Longint(Self));
      end;
    end;
  end;
end;

procedure TControlHook.SetWinControl(Value: TWinControl);
begin
  if Value <> FControl then begin
    UnhookControl;
    FControl := Value;
    if FList.Count > 0 then HookControl;
  end;
end;

{ THookList }

constructor THookList.Create;
begin
  inherited Create;
  FHandle := AllocateHWnd(WndProc);
end;

destructor THookList.Destroy;
begin
  while Count > 0 do TControlHook(Last).Free;
  DeallocateHWnd(FHandle);
  inherited Destroy;
end;

procedure THookList.WndProc(var Msg: TMessage);
var
  Hook: TControlHook;
begin
  try
    with Msg do begin
      if Msg = CM_RECREATEWINDOW then begin
        Hook := TControlHook(LParam);
        if (Hook <> nil) and (IndexOf(Hook) >= 0) then
          Hook.HookControl;
      end
      else if Msg = CM_DESTROYHOOK then begin
        Hook := TControlHook(LParam);
        if Assigned(Hook) and (IndexOf(Hook) >= 0) and
          (Hook.FList.Count = 0) then Hook.Free;
      end
      else Result := DefWindowProc(FHandle, Msg, wParam, lParam);
    end;
  except
    Application.HandleException(Self);
  end;
end;

function THookList.FindControlHook(AControl: TWinControl): TControlHook;
var
  I: Integer;
begin
  if Assigned(AControl) then
    for I := 0 to Count - 1 do
      if (TControlHook(Items[I]).WinControl = AControl) then begin
        Result := TControlHook(Items[I]);
        Exit;
      end;
  Result := nil;
end;

function THookList.GetControlHook(AControl: TWinControl): TControlHook;
begin
  Result := FindControlHook(AControl);
  if Result = nil then begin
    Result := TControlHook.Create;
    try
      Add(Result);
      Result.WinControl := AControl;
    except
      Result.Free;
      raise;
    end;
  end;
end;

{ TRxWindowHook }

constructor TRxWindowHook.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FActive := True;
end;

destructor TRxWindowHook.Destroy;
begin
  Active := False;
  WinControl := nil;
  inherited Destroy;
end;

procedure TRxWindowHook.SetActive(Value: Boolean);
begin
  if FActive <> Value then
    if Value then HookControl else UnhookControl;
end;

function TRxWindowHook.GetHookHandle: HWnd;
begin
  if Assigned(HookList) then Result := HookList.Handle
  else
{$IFDEF WIN32}
    Result := INVALID_HANDLE_VALUE;
{$ELSE}
    Result := 0;
{$ENDIF}
end;

procedure TRxWindowHook.HookControl;
begin
  if Assigned(FControl) and not (csDestroying in ComponentState) then
    GetHookList.GetControlHook(FControl).AddHook(Self);
  FActive := True;
end;

function TRxWindowHook.DoUnhookControl: Pointer;
begin
  Result := FControlHook;
  if Result <> nil then TControlHook(Result).RemoveHook(Self);
  FActive := False;
end;

procedure TRxWindowHook.UnhookControl;
begin
  DoUnhookControl;
  FActive := False;
end;

function TRxWindowHook.NotIsForm: Boolean;
begin
  Result := (WinControl <> nil) and not (WinControl is TCustomForm);
end;

function TRxWindowHook.IsForm: Boolean;
begin
  Result := (WinControl <> nil) and ((WinControl = Owner) and
    (Owner is TCustomForm));
end;

procedure TRxWindowHook.ReadForm(Reader: TReader);
begin
  if Reader.ReadBoolean then
    if Owner is TCustomForm then WinControl := TWinControl(Owner);
end;

procedure TRxWindowHook.WriteForm(Writer: TWriter);
begin
  Writer.WriteBoolean(IsForm);
end;

procedure TRxWindowHook.DefineProperties(Filer: TFiler);
{$IFDEF WIN32}
  function DoWrite: Boolean;
  begin
    if Assigned(Filer.Ancestor) then
      Result := IsForm <> TRxWindowHook(Filer.Ancestor).IsForm
    else Result := IsForm;
  end;
{$ENDIF}
begin
  inherited DefineProperties(Filer);
  Filer.DefineProperty('IsForm', ReadForm, WriteForm,
    {$IFDEF WIN32} DoWrite {$ELSE} IsForm {$ENDIF});
end;

function TRxWindowHook.GetWinControl: TWinControl;
begin
  if Assigned(FControlHook) then Result := TControlHook(FControlHook).WinControl
  else Result := FControl;
end;

procedure TRxWindowHook.DoAfterMessage(var Msg: TMessage; var Handled: Boolean);
begin
  if Assigned(FAfterMessage) then FAfterMessage(Self, Msg, Handled);
end;

procedure TRxWindowHook.DoBeforeMessage(var Msg: TMessage; var Handled: Boolean);
begin
  if Assigned(FBeforeMessage) then FBeforeMessage(Self, Msg, Handled);
end;

procedure TRxWindowHook.Notification(AComponent: TComponent; Operation: TOperation);
begin
  inherited Notification(AComponent, Operation);
  if (AComponent = WinControl) and (Operation = opRemove) then
    WinControl := nil
  else if (Operation = opRemove) and ((Owner = AComponent) or
    (Owner = nil)) then WinControl := nil;
end;

procedure TRxWindowHook.SetWinControl(Value: TWinControl);
var
  SaveActive: Boolean;
  Hook: TControlHook;
begin
  if Value <> WinControl then begin
    SaveActive := FActive;
    Hook := TControlHook(DoUnhookControl);
    FControl := Value;
{$IFDEF WIN32}
    if Value <> nil then Value.FreeNotification(Self);
{$ENDIF}
    if Assigned(Hook) and (Hook.FList.Count = 0) and Assigned(HookList) then
      PostMessage(HookList.Handle, CM_DESTROYHOOK, 0, Longint(Hook));
    if SaveActive then HookControl;
  end;
end;

{ SetVirtualMethodAddress procedure. Destroy destructor has index 0,
  first user defined virtual method has index 1. }

type
  PPointer = ^Pointer;

function GetVirtualMethodAddress(AClass: TClass; AIndex: Integer): Pointer;
var
  Table: PPointer;
begin
  Table := PPointer(AClass);
  Inc(Table, AIndex - 1);
  Result := Table^;
end;

function SetVirtualMethodAddress(AClass: TClass; AIndex: Integer;
  NewAddress: Pointer): Pointer;
{$IFDEF WIN32}
const
  PageSize = SizeOf(Pointer);
{$ENDIF}
var
  Table: PPointer;
{$IFDEF WIN32}
  SaveFlag: DWORD;
{$ELSE}
  Block: Pointer;
{$ENDIF}
begin
  Table := PPointer(AClass);
  Inc(Table, AIndex - 1);
  Result := Table^;
{$IFDEF WIN32}
  if VirtualProtect(Table, PageSize, PAGE_EXECUTE_READWRITE, @SaveFlag) then
  try
    Table^ := NewAddress;
  finally
    VirtualProtect(Table, PageSize, SaveFlag, @SaveFlag);
  end;
{$ELSE}
  PtrRec(Block).Ofs := PtrRec(Table).Ofs;
  PtrRec(Block).Seg := AllocCSToDSAlias(PtrRec(Table).Seg);
  try
    PPointer(Block)^ := NewAddress;
  finally
    FreeSelector(PtrRec(Block).Seg);
  end;
{$ENDIF}
end;

function FindVirtualMethodIndex(AClass: TClass; MethodAddr: Pointer): Integer;
begin
  Result := 0;
  repeat
    Inc(Result);
  until (GetVirtualMethodAddress(AClass, Result) = MethodAddr);
end;

initialization
  HookList := nil;
{$IFDEF WIN32}
finalization
  DropHookList;
{$ELSE}
  AddExitProc(DropHookList);
{$ENDIF}
end.