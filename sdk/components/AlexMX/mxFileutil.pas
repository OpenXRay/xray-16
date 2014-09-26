{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995, 1996 AO ROSNO             }
{         Copyright (c) 1997 Master-Bank                }
{                                                       }
{*******************************************************}

unit mxFileUtil;

{$I MX.INC}
{$I-,R-}

interface

uses {$IFDEF WIN32} Windows, {$ELSE} WinTypes, WinProcs, {$ENDIF} Messages,
  SysUtils, Classes, Consts, Controls, RTLConsts;

procedure CopyFile(const FileName, DestName: string;
  ProgressControl: TControl);
procedure CopyFileEx(const FileName, DestName: string;
  OverwriteReadOnly, ShellDialog: Boolean; ProgressControl: TControl);
procedure MoveFile(const FileName, DestName: TFileName);
procedure MoveFileEx(const FileName, DestName: TFileName; ShellDialog: Boolean);
{$IFDEF RX_D4}
function GetFileSize(const FileName: string): Int64;
{$ELSE}
function GetFileSize(const FileName: string): Longint;
{$ENDIF}
function FileDateTime(const FileName: string): TDateTime;
function HasAttr(const FileName: string; Attr: Integer): Boolean;
function DeleteFiles(const FileMask: string): Boolean;
function DeleteFilesEx(const FileMasks: array of string): Boolean;
function ClearDir(const Path: string; Delete: Boolean): Boolean;
function NormalDir(const DirName: string): string;
function RemoveBackSlash(const DirName: string): string;
function ValidFileName(const FileName: string): Boolean;
function DirExists(Name: string): Boolean;
procedure ForceDirectories(Dir: string);

function FileLock(Handle: Integer; Offset, LockSize: Longint): Integer;
  {$IFDEF RX_D4} overload; {$ENDIF}
{$IFDEF RX_D4}
function FileLock(Handle: Integer; Offset, LockSize: Int64): Integer; overload;
{$ENDIF}
function FileUnlock(Handle: Integer; Offset, LockSize: Longint): Integer;
  {$IFDEF RX_D4} overload; {$ENDIF}
{$IFDEF RX_D4}
function FileUnlock(Handle: Integer; Offset, LockSize: Int64): Integer; overload;
{$ENDIF}

function GetTempDir: string;
function GetWindowsDir: string;
function GetSystemDir: string;

function BrowseDirectory(var AFolderName: string; const DlgText: string;
  AHelpContext: THelpContext): Boolean;

{$IFDEF WIN32}
function BrowseComputer(var ComputerName: string; const DlgText: string;
  AHelpContext: THelpContext): Boolean;

function ShortToLongFileName(const ShortName: string): string;
function ShortToLongPath(const ShortName: string): string;
function LongToShortFileName(const LongName: string): string;
function LongToShortPath(const LongName: string): string;

procedure CreateFileLink(const FileName, DisplayName: string; Folder: Integer);
procedure DeleteFileLink(const DisplayName: string; Folder: Integer);
{$ENDIF WIN32}

{$IFNDEF RX_D3}
function IsPathDelimiter(const S: string; Index: Integer): Boolean;
{$ENDIF}

implementation

uses {$IFDEF WIN32} {$IFDEF RX_D3} ActiveX, ComObj, ShlObj, {$ELSE} Ole2,
  OleAuto, {$ENDIF} {$ENDIF} mxVCLUtils, Forms, ShellAPI, MxPrgrss, mxDateUtil, mxFileCtrl; 

{$IFDEF WIN32}

{$IFNDEF RX_D3}

type

{ TSHItemID -- Item ID }

  PSHItemID = ^TSHItemID;
  TSHItemID = packed record           { mkid }
    cb: Word;                         { Size of the ID (including cb itself) }
    abID: array[0..0] of Byte;        { The item ID (variable length) }
  end;

{ TItemIDList -- List if item IDs (combined with 0-terminator) }

  PItemIDList = ^TItemIDList;
  TItemIDList = packed record         { idl }
     mkid: TSHItemID;
   end;

  TFNBFFCallBack = function(Wnd: HWND; uMsg: UINT; lParam, lpData: LPARAM): Integer stdcall;

  PBrowseInfo = ^TBrowseInfo;
  TBrowseInfo = packed record
    hwndOwner: HWND;
    pidlRoot: PItemIDList;
    pszDisplayName: LPSTR;  { Return display name of item selected. }
    lpszTitle: LPCSTR;      { text to go in the banner over the tree. }
    ulFlags: UINT;          { Flags that control the return stuff }
    lpfn: TFNBFFCallBack;
    lParam: LPARAM;         { extra info that's passed back in callbacks }
    iImage: Integer;        { output var: where to return the Image index. }
  end;

const

{ Browsing for directory }

  BIF_RETURNONLYFSDIRS   = $0001; { For finding a folder to start document searching }
  BIF_DONTGOBELOWDOMAIN  = $0002; { For starting the Find Computer }
  BIF_STATUSTEXT         = $0004;
  BIF_RETURNFSANCESTORS  = $0008;

  BIF_BROWSEFORCOMPUTER  = $1000; { Browsing for Computers }
  BIF_BROWSEFORPRINTER   = $2000; { Browsing for Printers }
  BIF_BROWSEINCLUDEFILES = $4000; { Browsing for Everything }

{ message from browser }

  BFFM_INITIALIZED       = 1;
  BFFM_SELCHANGED        = 2;

{ messages to browser }

  BFFM_SETSTATUSTEXT      = (WM_USER + 100);
  BFFM_ENABLEOK           = (WM_USER + 101);
  BFFM_SETSELECTION       = (WM_USER + 102);

const
  CSIDL_DRIVES             = $0011;
  CSIDL_NETWORK            = $0012;

function SHBrowseForFolder(var lpbi: TBrowseInfo): PItemIDList; stdcall;
  far; external Shell32 name 'SHBrowseForFolder';
function SHGetPathFromIDList(pidl: PItemIDList; pszPath: LPSTR): BOOL; stdcall;
  far; external Shell32 name 'SHGetPathFromIDList';
function SHGetSpecialFolderLocation(hwndOwner: HWND; nFolder: Integer;
  var ppidl: PItemIDList): HResult; stdcall; far; external Shell32
  name 'SHGetSpecialFolderLocation';

{$ENDIF RX_D3}

{ TBrowseFolderDlg }

type
  TBrowseKind = (bfFolders, bfComputers);
  TDialogPosition = (dpDefault, dpScreenCenter);

  TBrowseFolderDlg = class(TComponent)
  private
    FDefWndProc: Pointer;
    FHelpContext: THelpContext;
    FHandle: HWnd;
    FObjectInstance: Pointer;
    FDesktopRoot: Boolean;
    FBrowseKind: TBrowseKind;
    FPosition: TDialogPosition;
    FText: string;
    FDisplayName: string;
    FSelectedName: string;
    FFolderName: string;
    FImageIndex: Integer;
    FOnInitialized: TNotifyEvent;
    FOnSelChanged: TNotifyEvent;
    procedure SetSelPath(const Path: string);
    procedure SetOkEnable(Value: Boolean);
    procedure DoInitialized;
    procedure DoSelChanged(Param: PItemIDList);
    procedure WMNCDestroy(var Message: TWMNCDestroy); message WM_NCDESTROY;
    procedure WMCommand(var Message: TMessage); message WM_COMMAND;
  protected
    procedure DefaultHandler(var Message); override;
    procedure WndProc(var Message: TMessage); virtual;
    function TaskModalDialog(var Info: TBrowseInfo): PItemIDList;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    function Execute: Boolean;
    property Handle: HWnd read FHandle;
    property DisplayName: string read FDisplayName;
    property SelectedName: string read FSelectedName write FSelectedName;
    property ImageIndex: Integer read FImageIndex;
  published
    property BrowseKind: TBrowseKind read FBrowseKind write FBrowseKind default bfFolders;
    property DesktopRoot: Boolean read FDesktopRoot write FDesktopRoot default True;
    property DialogText: string read FText write FText;
    property FolderName: string read FFolderName write FFolderName;
    property HelpContext: THelpContext read FHelpContext write FHelpContext default 0;
    property Position: TDialogPosition read FPosition write FPosition default dpScreenCenter;
    property OnInitialized: TNotifyEvent read FOnInitialized write FOnInitialized;
    property OnSelChanged: TNotifyEvent read FOnSelChanged write FOnSelChanged;
  end;

function ExplorerHook(Wnd: HWnd; Msg: UINT; LParam: LPARAM; Data: LPARAM): Integer; stdcall;
begin
  Result := 0;
  if Msg = BFFM_INITIALIZED then begin
    if TBrowseFolderDlg(Data).Position = dpScreenCenter then
      CenterWindow(Wnd);
    TBrowseFolderDlg(Data).FHandle := Wnd;
    TBrowseFolderDlg(Data).FDefWndProc := Pointer(SetWindowLong(Wnd, GWL_WNDPROC,
      Longint(TBrowseFolderDlg(Data).FObjectInstance)));
    TBrowseFolderDlg(Data).DoInitialized;
  end
  else if Msg = BFFM_SELCHANGED then begin
    TBrowseFolderDlg(Data).FHandle := Wnd;
    TBrowseFolderDlg(Data).DoSelChanged(PItemIDList(LParam));
  end;
end;

const
  HelpButtonId = $FFFF;

constructor TBrowseFolderDlg.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FObjectInstance := MakeObjectInstance(WndProc);
  FDesktopRoot := True;
  FBrowseKind := bfFolders;
  FPosition := dpScreenCenter;
  SetLength(FDisplayName, MAX_PATH);
end;

destructor TBrowseFolderDlg.Destroy;
begin
  if FObjectInstance <> nil then FreeObjectInstance(FObjectInstance);
  inherited Destroy;
end;

procedure TBrowseFolderDlg.DoInitialized;
const
  SBtn = 'BUTTON';
var
  BtnHandle, HelpBtn, BtnFont: THandle;
  BtnSize: TRect;
begin
  if (FBrowseKind = bfComputers) or DirExists(FFolderName) then
    SetSelPath(FFolderName);
  if FHelpContext <> 0 then begin
    BtnHandle := FindWindowEx(FHandle, 0, SBtn, nil);
    if (BtnHandle <> 0) then begin
      GetWindowRect(BtnHandle, BtnSize);
      ScreenToClient(FHandle, BtnSize.TopLeft);
      ScreenToClient(FHandle, BtnSize.BottomRight);
      BtnFont := SendMessage(FHandle, WM_GETFONT, 0, 0);
      HelpBtn := CreateWindow(SBtn, PChar(ResStr(SHelpButton)),
        WS_CHILD or WS_CLIPSIBLINGS or WS_VISIBLE or BS_PUSHBUTTON or WS_TABSTOP,
        12, BtnSize.Top, BtnSize.Right - BtnSize.Left, BtnSize.Bottom - BtnSize.Top,
        FHandle, HelpButtonId, HInstance, nil);
      if BtnFont <> 0 then
        SendMessage(HelpBtn, WM_SETFONT, BtnFont, MakeLParam(1, 0));
      UpdateWindow(FHandle);
    end;
  end;
  if Assigned(FOnInitialized) then FOnInitialized(Self);
end;

procedure TBrowseFolderDlg.DoSelChanged(Param: PItemIDList);
var
  Temp: array[0..MAX_PATH] of Char;
begin
  if (FBrowseKind = bfComputers) then begin
    FSelectedName := DisplayName;
  end
  else begin
    if SHGetPathFromIDList(Param, Temp) then begin
      FSelectedName := StrPas(Temp);
      SetOkEnable(DirExists(FSelectedName));
    end
    else begin
      FSelectedName := '';
      SetOkEnable(False);
    end;
  end;
  if Assigned(FOnSelChanged) then FOnSelChanged(Self);
end;

procedure TBrowseFolderDlg.SetSelPath(const Path: string);
begin
  if FHandle <> 0 then
    SendMessage(FHandle, BFFM_SETSELECTION, 1, Longint(PChar(Path)));
end;

procedure TBrowseFolderDlg.SetOkEnable(Value: Boolean);
begin
  if FHandle <> 0 then SendMessage(FHandle, BFFM_ENABLEOK, 0, Ord(Value));
end;

procedure TBrowseFolderDlg.DefaultHandler(var Message);
begin
  if FHandle <> 0 then
    with TMessage(Message) do
      Result := CallWindowProc(FDefWndProc, FHandle, Msg, WParam, LParam)
  else inherited DefaultHandler(Message);
end;

procedure TBrowseFolderDlg.WndProc(var Message: TMessage);
begin
  Dispatch(Message);
end;

procedure TBrowseFolderDlg.WMCommand(var Message: TMessage);
begin
  if (Message.wParam = HelpButtonId) and (LongRec(Message.lParam).Hi =
    BN_CLICKED) and (FHelpContext <> 0) then
  begin
    Application.HelpContext(FHelpContext);
  end
  else inherited;
end;

procedure TBrowseFolderDlg.WMNCDestroy(var Message: TWMNCDestroy);
begin
  inherited;
  FHandle := 0;
end;

function TBrowseFolderDlg.Execute: Boolean;
var
  BrowseInfo: TBrowseInfo;
  ItemIDList: PItemIDList;
  Temp: array[0..MAX_PATH] of Char;
begin
  if FDesktopRoot and (FBrowseKind = bfFolders) then
    BrowseInfo.pidlRoot := nil
  else begin
    if FBrowseKind = bfComputers then { root - Network }
      OleCheck(SHGetSpecialFolderLocation(0, CSIDL_NETWORK,
        BrowseInfo.pidlRoot))
    else { root - MyComputer }
      OleCheck(SHGetSpecialFolderLocation(0, CSIDL_DRIVES,
        BrowseInfo.pidlRoot));
  end;
  try
    SetLength(FDisplayName, MAX_PATH);
    with BrowseInfo do begin
      pszDisplayName := PChar(DisplayName);
      if DialogText <> '' then lpszTitle := PChar(DialogText)
      else lpszTitle := nil;
      if FBrowseKind = bfComputers then
        ulFlags := BIF_BROWSEFORCOMPUTER
      else
        ulFlags := BIF_RETURNONLYFSDIRS or BIF_RETURNFSANCESTORS;
      lpfn := ExplorerHook;
      lParam := Longint(Self);
      hWndOwner := Application.Handle;
      iImage := 0;
    end;
    ItemIDList := TaskModalDialog(BrowseInfo);
    Result := ItemIDList <> nil;
    if Result then
    try
      if FBrowseKind = bfFolders then begin
        Win32Check(SHGetPathFromIDList(ItemIDList, Temp));
        FFolderName := RemoveBackSlash(StrPas(Temp));
      end
      else begin
        FFolderName := DisplayName;
      end;
      FSelectedName := FFolderName;
      FImageIndex := BrowseInfo.iImage;
    finally
      CoTaskMemFree(ItemIDList);
    end;
  finally
    if BrowseInfo.pidlRoot <> nil then CoTaskMemFree(BrowseInfo.pidlRoot);
  end;
end;

function TBrowseFolderDlg.TaskModalDialog(var Info: TBrowseInfo): PItemIDList;
var
  ActiveWindow: HWnd;
  WindowList: Pointer;
begin
  ActiveWindow := GetActiveWindow;
  WindowList := DisableTaskWindows(0);
  try
    try
      Result := SHBrowseForFolder(Info);
    finally
      FHandle := 0;
      FDefWndProc := nil;
    end;
  finally
    EnableTaskWindows(WindowList);
    SetActiveWindow(ActiveWindow);
  end;
end;

function BrowseDirectory(var AFolderName: string; const DlgText: string;
  AHelpContext: THelpContext): Boolean;
begin
  if NewStyleControls then begin
    with TBrowseFolderDlg.Create(Application) do
    try
      DialogText := DlgText;
      FolderName := AFolderName;
      HelpContext := AHelpContext;
      Result := Execute;
      if Result then AFolderName := FolderName;
    finally
      Free;
    end;
  end
  else Result := SelectDirectory(AFolderName, [], AHelpContext);
end;

function BrowseComputer(var ComputerName: string; const DlgText: string;
  AHelpContext: THelpContext): Boolean;
begin
  with TBrowseFolderDlg.Create(Application) do
  try
    BrowseKind := bfComputers;
    DialogText := DlgText;
    FolderName := ComputerName;
    HelpContext := AHelpContext;
    Result := Execute;
    if Result then ComputerName := FolderName;
  finally
    Free;
  end;
end;

{ TRxFileOperator }

type
  TFileOperation = (foCopy, foDelete, foMove, foRename);
  TFileOperFlag = (flAllowUndo, flConfirmMouse, flFilesOnly, flMultiDest,
    flNoConfirmation, flNoConfirmMkDir, flRenameOnCollision, flSilent,
    flSimpleProgress, flNoErrorUI);
  TFileOperFlags = set of TFileOperFlag;

  TRxFileOperator = class(TComponent)
  private
    FAborted: Boolean;
    FOperation: TFileOperation;
    FOptions: TFileOperFlags;
    FProgressTitle: string;
    FSource: string;
    FDestination: string;
    function TaskModalDialog(DialogFunc: Pointer; var DialogData): Boolean;
  public
    constructor Create(AOwner: TComponent); override;
    function Execute: Boolean; virtual;
    property Aborted: Boolean read FAborted;
  published
    property Destination: string read FDestination write FDestination;
    property Operation: TFileOperation read FOperation write FOperation
      default foCopy;
    property Options: TFileOperFlags read FOptions write FOptions
      default [flAllowUndo, flNoConfirmMkDir];
    property ProgressTitle: string read FProgressTitle write FProgressTitle;
    property Source: string read FSource write FSource;
  end;

{$IFNDEF RX_D3}
const
  FOF_NOERRORUI = $0400;
{$ENDIF}

constructor TRxFileOperator.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FOptions := [flAllowUndo, flNoConfirmMkDir];
end;

function TRxFileOperator.TaskModalDialog(DialogFunc: Pointer; var DialogData): Boolean;
type
  TDialogFunc = function(var DialogData): Integer stdcall;
var
  ActiveWindow: HWnd;
  WindowList: Pointer;
begin
  ActiveWindow := GetActiveWindow;
  WindowList := DisableTaskWindows(0);
  try
    Result := TDialogFunc(DialogFunc)(DialogData) = 0;
  finally
    EnableTaskWindows(WindowList);
    SetActiveWindow(ActiveWindow);
  end;
end;

function TRxFileOperator.Execute: Boolean;
const
  OperTypes: array[TFileOperation] of UINT = (
    FO_COPY, FO_DELETE, FO_MOVE, FO_RENAME);
  OperOptions: array[TFileOperFlag] of FILEOP_FLAGS = (
    FOF_ALLOWUNDO, FOF_CONFIRMMOUSE, FOF_FILESONLY, FOF_MULTIDESTFILES,
    FOF_NOCONFIRMATION, FOF_NOCONFIRMMKDIR, FOF_RENAMEONCOLLISION,
    FOF_SILENT, FOF_SIMPLEPROGRESS, FOF_NOERRORUI);
var
  OpStruct: TSHFileOpStruct;
  Flag: TFileOperFlag;

  function AllocFileStr(const S: string): PChar;
  var
    P: PChar;
  begin
    Result := nil;
    if S <> '' then begin
      Result := StrCopy(StrAlloc(Length(S) + 2), PChar(S));
      P := Result;
      while P^ <> #0 do begin
        if (P^ = ';') or (P^ = '|') then P^ := #0;
        Inc(P);
      end;
      Inc(P);
      P^ := #0;
    end;
  end;

begin
  FAborted := False;
  FillChar(OpStruct, SizeOf(OpStruct), 0);
  with OpStruct do
  try
    if (Application.MainForm <> nil) and
      Application.MainForm.HandleAllocated then
      Wnd := Application.MainForm.Handle
    else Wnd := Application.Handle;
    wFunc := OperTypes[Operation];
    pFrom := AllocFileStr(FSource);
    pTo := AllocFileStr(FDestination);
    fFlags := 0;
    for Flag := Low(Flag) to High(Flag) do
      if Flag in FOptions then fFlags := fFlags or OperOptions[Flag];
    lpszProgressTitle := PChar(FProgressTitle);
    Result := TaskModalDialog(@SHFileOperation, OpStruct);
    FAborted := fAnyOperationsAborted;
  finally
    if pFrom <> nil then StrDispose(pFrom);
    if pTo <> nil then StrDispose(pTo);
  end;
end;

{$ELSE}

function BrowseDirectory(var AFolderName: string; const DlgText: string;
  AHelpContext: THelpContext): Boolean;
begin
  Result := SelectDirectory(AFolderName, [], AHelpContext);
end;

{$ENDIF WIN32}

function NormalDir(const DirName: string): string;
begin
  Result := DirName;
  if (Result <> '') and
{$IFDEF RX_D3}
    not (AnsiLastChar(Result)^ in [':', '\']) then
{$ELSE}
    not (Result[Length(Result)] in [':', '\']) then
{$ENDIF}
  begin
    if (Length(Result) = 1) and (UpCase(Result[1]) in ['A'..'Z']) then
      Result := Result + ':\'
    else Result := Result + '\';
  end;
end;

function RemoveBackSlash(const DirName: string): string;
begin
  Result := DirName;
  if (Length(Result) > 1) and
{$IFDEF RX_D3}
    (AnsiLastChar(Result)^ = '\') then
{$ELSE}
    (Result[Length(Result)] = '\') then
{$ENDIF}
  begin
    if not ((Length(Result) = 3) and (UpCase(Result[1]) in ['A'..'Z']) and
      (Result[2] = ':')) then
      Delete(Result, Length(Result), 1);
  end;
end;

function DirExists(Name: string): Boolean;
{$IFDEF WIN32}
var
  Code: Integer;
begin
  Code := GetFileAttributes(PChar(Name));
  Result := (Code <> -1) and (FILE_ATTRIBUTE_DIRECTORY and Code <> 0);
end;
{$ELSE}
var
  SR: TSearchRec;
begin
  if Name[Length(Name)] = '\' then Dec(Name[0]);
  if (Length(Name) = 2) and (Name[2] = ':') then
    Name := Name + '\*.*';
  Result := FindFirst(Name, faDirectory, SR) = 0;
  Result := Result and (SR.Attr and faDirectory <> 0);
end;
{$ENDIF}

procedure ForceDirectories(Dir: string);
begin
  if Length(Dir) = 0 then Exit;
{$IFDEF RX_D3}
  if (AnsiLastChar(Dir) <> nil) and (AnsiLastChar(Dir)^ = '\') then
{$ELSE}
  if Dir[Length(Dir)] = '\' then
{$ENDIF}
    Delete(Dir, Length(Dir), 1);
  if (Length(Dir) < 3) or DirectoryExists(Dir) or
    (ExtractFilePath(Dir) = Dir) then Exit;
  ForceDirectories(ExtractFilePath(Dir));
{$IFDEF WIN32}
  CreateDir(Dir);
{$ELSE}
  MkDir(Dir);
{$ENDIF}
end;

{$IFDEF WIN32}
procedure CopyMoveFileShell(const FileName, DestName: string; Confirmation,
  AllowUndo, MoveFile: Boolean);
begin
  with TRxFileOperator.Create(nil) do
  try
    Source := FileName;
    Destination := DestName;
    if MoveFile then begin
      if AnsiCompareText(ExtractFilePath(FileName),
        ExtractFilePath(DestName)) = 0 then
        Operation := foRename
      else Operation := foMove;
    end
    else Operation := foCopy;
    if not AllowUndo then
      Options := Options - [flAllowUndo];
    if not Confirmation then
      Options := Options + [flNoConfirmation];
    if not Execute or Aborted then SysUtils.Abort;
  finally
    Free;
  end;
end;
{$ENDIF}

procedure CopyFile(const FileName, DestName: string;
  ProgressControl: TControl);
begin
  CopyFileEx(FileName, DestName, False, False, ProgressControl);
end;

procedure CopyFileEx(const FileName, DestName: string;
  OverwriteReadOnly, ShellDialog: Boolean; ProgressControl: TControl);
var
  CopyBuffer: Pointer;
  Source, Dest: Integer;
  Destination: TFileName;
  FSize, BytesCopied, TotalCopied: Longint;
  Attr: Integer;
const
  ChunkSize: Longint = 8192;
begin
{$IFDEF WIN32}
  if NewStyleControls and ShellDialog then begin
    CopyMoveFileShell(FileName, DestName, not OverwriteReadOnly,
      False, False);
    Exit;
  end;
{$ENDIF}
  Destination := DestName;
  if HasAttr(Destination, faDirectory) then
    Destination := NormalDir(Destination) + ExtractFileName(FileName);
  GetMem(CopyBuffer, ChunkSize);
  try
    TotalCopied := 0;
    FSize := GetFileSize(FileName);
    Source := FileOpen(FileName, fmShareDenyWrite);
    if Source < 0 then
      raise EFOpenError.CreateFmt(ResStr(SFOpenError), [FileName]);
    try
      if ProgressControl <> nil then begin
        SetProgressMax(ProgressControl, FSize);
        SetProgressMin(ProgressControl, 0);
        SetProgressValue(ProgressControl, 0);
      end;
      ForceDirectories(ExtractFilePath(Destination));
      if OverwriteReadOnly then begin
        Attr := FileGetAttr(Destination);
        if (Attr >= 0) and ((Attr and faReadOnly) <> 0) then
          FileSetAttr(Destination, Attr and not faReadOnly);
      end;
      Dest := FileCreate(Destination);
      if Dest < 0 then
        raise EFCreateError.CreateFmt(ResStr(SFCreateError), [Destination]);
      try
        repeat
          BytesCopied := FileRead(Source, CopyBuffer^, ChunkSize);
          if BytesCopied = -1 then
            raise EReadError.Create(ResStr(SReadError));
          TotalCopied := TotalCopied + BytesCopied;
          if BytesCopied > 0 then begin
            if FileWrite(Dest, CopyBuffer^, BytesCopied) = -1 then
              raise EWriteError.Create(ResStr(SWriteError));
          end;
          if ProgressControl <> nil then
            SetProgressValue(ProgressControl, TotalCopied);
        until BytesCopied < ChunkSize;
        FileSetDate(Dest, FileGetDate(Source));
      finally
        FileClose(Dest);
      end;
    finally
      FileClose(Source);
    end;
  finally
    FreeMem(CopyBuffer, ChunkSize);
    if ProgressControl <> nil then
      SetProgressValue(ProgressControl, 0);
  end;
end;

procedure MoveFile(const FileName, DestName: TFileName);
var
  Destination: TFileName;
  Attr: Integer;
begin
  Destination := ExpandFileName(DestName);
  if not RenameFile(FileName, Destination) then begin
    Attr := FileGetAttr(FileName);
    if Attr < 0 then Exit;
    if (Attr and faReadOnly) <> 0 then
      FileSetAttr(FileName, Attr and not faReadOnly);
    CopyFile(FileName, Destination, nil);
    DeleteFile(FileName);
  end;
end;

procedure MoveFileEx(const FileName, DestName: TFileName;
  ShellDialog: Boolean);
begin
{$IFDEF WIN32}
  if NewStyleControls and ShellDialog then
    CopyMoveFileShell(FileName, DestName, False, False, True)
  else
{$ENDIF}
    MoveFile(FileName, DestName);
end;

{$IFDEF RX_D4}
function GetFileSize(const FileName: string): Int64;
var
  Handle: THandle;
  FindData: TWin32FindData;
begin
  Handle := FindFirstFile(PChar(FileName), FindData);
  if Handle <> INVALID_HANDLE_VALUE then begin
    Windows.FindClose(Handle);
    if (FindData.dwFileAttributes and FILE_ATTRIBUTE_DIRECTORY) = 0 then
    begin
      Int64Rec(Result).Lo := FindData.nFileSizeLow;
      Int64Rec(Result).Hi := FindData.nFileSizeHigh;
      Exit;
    end;
  end;
  Result := -1;
end;
{$ELSE}
function GetFileSize(const FileName: string): Longint;
var
  SearchRec: TSearchRec;
begin
  if FindFirst(ExpandFileName(FileName), faAnyFile, SearchRec) = 0 then
    Result := SearchRec.Size
  else Result := -1;
  FindClose(SearchRec);
end;
{$ENDIF RX_D4}

function FileDateTime(const FileName: string): System.TDateTime;
var
  Age: Longint;
begin
  Age := FileAge(FileName);
  if Age = -1 then
    Result := NullDate
  else
    Result := FileDateToDateTime(Age);
end;

function HasAttr(const FileName: string; Attr: Integer): Boolean;
var
  FileAttr: Integer;
begin
  FileAttr := FileGetAttr(FileName);
  Result := (FileAttr >= 0) and (FileAttr and Attr = Attr);
end;

function DeleteFiles(const FileMask: string): Boolean;
var
  SearchRec: TSearchRec;
begin
  Result := FindFirst(ExpandFileName(FileMask), faAnyFile, SearchRec) = 0;
  try
    if Result then
      repeat
        if (SearchRec.Name[1] <> '.') and
          (SearchRec.Attr and faVolumeID <> faVolumeID) and
          (SearchRec.Attr and faDirectory <> faDirectory) then
        begin
          Result := DeleteFile(ExtractFilePath(FileMask) + SearchRec.Name);
          if not Result then Break;
        end;
      until FindNext(SearchRec) <> 0;
  finally
    FindClose(SearchRec);
  end;
end;

function DeleteFilesEx(const FileMasks: array of string): Boolean;
var
  I: Integer;
begin
  Result := True;
  for I := Low(FileMasks) to High(FileMasks) do
    Result := Result and DeleteFiles(FileMasks[I]);
end;

function ClearDir(const Path: string; Delete: Boolean): Boolean;
const
{$IFDEF WIN32}
  FileNotFound = 18;
{$ELSE}
  FileNotFound = -18;
{$ENDIF}
var
  FileInfo: TSearchRec;
  DosCode: Integer;
begin
  Result := DirExists(Path);
  if not Result then Exit;
  DosCode := FindFirst(NormalDir(Path) + '*.*', faAnyFile, FileInfo);
  try
    while DosCode = 0 do begin
      if (FileInfo.Name[1] <> '.') and (FileInfo.Attr <> faVolumeID) then
      begin
        if (FileInfo.Attr and faDirectory = faDirectory) then
          Result := ClearDir(NormalDir(Path) + FileInfo.Name, Delete) and Result
        else if (FileInfo.Attr and faVolumeID <> faVolumeID) then begin
          if (FileInfo.Attr and faReadOnly = faReadOnly) then
            FileSetAttr(NormalDir(Path) + FileInfo.Name, faArchive);
          Result := DeleteFile(NormalDir(Path) + FileInfo.Name) and Result;
        end;
      end;
      DosCode := FindNext(FileInfo);
    end;
  finally
    FindClose(FileInfo);
  end;
  if Delete and Result and (DosCode = FileNotFound) and
    not ((Length(Path) = 2) and (Path[2] = ':')) then
  begin
    RmDir(Path);
    Result := (IOResult = 0) and Result;
  end;
end;

function GetTempDir: string;
{$IFDEF WIN32}
var
  Buffer: array[0..1023] of Char;
begin
  SetString(Result, Buffer, GetTempPath(SizeOf(Buffer), Buffer));
{$ELSE}
var
  Buffer: array[0..255] of Char;
begin
  GetTempFileName(GetTempDrive(#0), '$', 1, Buffer);
  Result := ExtractFilePath(StrPas(Buffer));
{$ENDIF}
end;

function GetWindowsDir: string;
{$IFDEF WIN32}
var
  Buffer: array[0..1023] of Char;
begin
  SetString(Result, Buffer, GetWindowsDirectory(Buffer, SizeOf(Buffer)));
{$ELSE}
begin
  Result[0] := Char(GetWindowsDirectory(@Result[1], 254));
{$ENDIF}
end;

function GetSystemDir: string;
{$IFDEF WIN32}
var
  Buffer: array[0..1023] of Char;
begin
  SetString(Result, Buffer, GetSystemDirectory(Buffer, SizeOf(Buffer)));
{$ELSE}
begin
  Result[0] := Char(GetSystemDirectory(@Result[1], 254));
{$ENDIF}
end;

{$IFDEF WIN32}

function ValidFileName(const FileName: string): Boolean;
  function HasAny(const Str, Substr: string): Boolean;
  var
    I: Integer;
  begin
    Result := False;
    for I := 1 to Length(Substr) do begin
      if Pos(Substr[I], Str) > 0 then begin
        Result := True;
        Break;
      end;
    end;
  end;
begin
  Result := (FileName <> '') and (not HasAny(FileName, '<>"[]|'));
  if Result then Result := Pos('\', ExtractFileName(FileName)) = 0;
end;

function FileLock(Handle: Integer; Offset, LockSize: Longint): Integer;
begin
  if LockFile(Handle, Offset, 0, LockSize, 0) then
    Result := 0
  else
    Result := GetLastError;
end;

function FileUnlock(Handle: Integer; Offset, LockSize: Longint): Integer;
begin
  if UnlockFile(Handle, Offset, 0, LockSize, 0) then
    Result := 0
  else
    Result := GetLastError;
end;

{$IFDEF RX_D4}
function FileLock(Handle: Integer; Offset, LockSize: Int64): Integer;
begin
  if LockFile(Handle, Int64Rec(Offset).Lo, Int64Rec(Offset).Hi,
    Int64Rec(LockSize).Lo, Int64Rec(LockSize).Hi) then Result := 0
  else
    Result := GetLastError;
end;

function FileUnlock(Handle: Integer; Offset, LockSize: Int64): Integer;
begin
  if UnlockFile(Handle, Int64Rec(Offset).Lo, Int64Rec(Offset).Hi,
    Int64Rec(LockSize).Lo, Int64Rec(LockSize).Hi) then Result := 0
  else
    Result := GetLastError;
end;
{$ENDIF RX_D4}

{$ELSE}

function ValidFileName(const FileName: string): Boolean;
const
  MaxNameLen = 12; { file name and extension }
  MaxExtLen  =  4; { extension with point }
  MaxPathLen = 79; { full file path in DOS }
var
  Dir, Name, Ext: TFileName;

  function HasAny(Str, SubStr: string): Boolean; near; assembler;
  asm
        PUSH     DS
        CLD
        LDS      SI,Str
        LES      DI,SubStr
        INC      DI
        MOV      DX,DI
        XOR      AH,AH
        LODSB
        MOV      BX,AX
        OR       BX,BX
        JZ       @@2
        MOV      AL,ES:[DI-1]
        XCHG     AX,CX
  @@1:  PUSH     CX
        MOV      DI,DX
        LODSB
        REPNE    SCASB
        POP      CX
        JE       @@3
        DEC      BX
        JNZ      @@1
  @@2:  XOR      AL,AL
        JMP      @@4
  @@3:  MOV      AL,1
  @@4:  POP      DS
  end;

begin
  Result := True;
  Dir := Copy(ExtractFilePath(FileName), 1, MaxPathLen);
  Name := Copy(ExtractFileName(FileName), 1, MaxNameLen);
  Ext := Copy(ExtractFileExt(FileName), 1, MaxExtLen);
  if (Dir + Name <> FileName) or HasAny(Name, ';,=+<>|"[] \') or
    HasAny(Copy(Ext, 2, 255), ';,=+<>|"[] \.') then Result := False;
end;

function LockFile(Handle: Integer; StartPos, Length: Longint;
  Unlock: Boolean): Integer; assembler;
asm
      PUSH     DS
      MOV      AH,5CH
      MOV      AL,Unlock
      MOV      BX,Handle
      MOV      DX,StartPos.Word[0]
      MOV      CX,StartPos.Word[2]
      MOV      DI,Length.Word[0]
      MOV      SI,Length.Word[2]
      INT      21H
      JNC      @@1
      NEG      AX
      JMP      @@2
@@1:  MOV      AX,0
@@2:  POP      DS
end;

function FileLock(Handle: Integer; Offset, LockSize: Longint): Integer;
begin
  Result := LockFile(Handle, Offset, LockSize, False);
end;

function FileUnlock(Handle: Integer; Offset, LockSize: Longint): Integer;
begin
  Result := LockFile(Handle, Offset, LockSize, True);
end;

{$ENDIF WIN32}

{$IFDEF WIN32}

function ShortToLongFileName(const ShortName: string): string;
var
  Temp: TWin32FindData;
  SearchHandle: THandle;
begin
  SearchHandle := FindFirstFile(PChar(ShortName), Temp);
  if SearchHandle <> INVALID_HANDLE_VALUE then begin
    Result := string(Temp.cFileName);
    if Result = '' then Result := string(Temp.cAlternateFileName);
  end
  else Result := '';
  Windows.FindClose(SearchHandle);
end;

function LongToShortFileName(const LongName: string): string;
var
  Temp: TWin32FindData;
  SearchHandle: THandle;
begin
  SearchHandle := FindFirstFile(PChar(LongName), Temp);
  if SearchHandle <> INVALID_HANDLE_VALUE then begin
    Result := string(Temp.cAlternateFileName);
    if Result = '' then Result := string(Temp.cFileName);
  end
  else Result := '';
  Windows.FindClose(SearchHandle);
end;

function ShortToLongPath(const ShortName: string): string;
var
  LastSlash: PChar;
  TempPathPtr: PChar;
begin
  Result := '';
  TempPathPtr := PChar(ShortName);
  LastSlash := StrRScan(TempPathPtr, '\');
  while LastSlash <> nil do begin
    Result := '\' + ShortToLongFileName(TempPathPtr) + Result;
    if LastSlash <> nil then begin
      LastSlash^ := char(0);
      LastSlash := StrRScan(TempPathPtr, '\');
    end;
  end;
  Result := TempPathPtr + Result;
end;

function LongToShortPath(const LongName: string): string;
var
  LastSlash: PChar;
  TempPathPtr: PChar;
begin
  Result := '';
  TempPathPtr := PChar(LongName);
  LastSlash := StrRScan(TempPathPtr, '\');
  while LastSlash <> nil do begin
    Result := '\' + LongToShortFileName(TempPathPtr) + Result;
    if LastSlash <> nil then begin
      LastSlash^ := char(0);
      LastSlash := StrRScan(TempPathPtr, '\');
    end;
  end;
  Result := TempPathPtr + Result;
end;

const
  IID_IPersistFile: TGUID = (
    D1:$0000010B;D2:$0000;D3:$0000;D4:($C0,$00,$00,$00,$00,$00,$00,$46));

{$IFNDEF RX_D3}
const
  IID_IShellLinkA: TGUID = (
    D1:$000214EE; D2:$0000; D3:$0000; D4:($C0,$00,$00,$00,$00,$00,$00,$46));
  CLSID_ShellLink: TGUID = (
    D1:$00021401; D2:$0000; D3:$0000; D4:($C0,$00,$00,$00,$00,$00,$00,$46));

type
  IShellLink = class(IUnknown) { sl }
    function GetPath(pszFile: LPSTR; cchMaxPath: Integer;
      var pfd: TWin32FindData; fFlags: DWORD): HResult; virtual; stdcall; abstract;
    function GetIDList(var ppidl: PItemIDList): HResult; virtual; stdcall; abstract;
    function SetIDList(pidl: PItemIDList): HResult; virtual; stdcall; abstract;
    function GetDescription(pszName: LPSTR; cchMaxName: Integer): HResult; virtual; stdcall; abstract;
    function SetDescription(pszName: LPSTR): HResult; virtual; stdcall; abstract;
    function GetWorkingDirectory(pszDir: LPSTR; cchMaxPath: Integer): HResult; virtual; stdcall; abstract;
    function SetWorkingDirectory(pszDir: LPSTR): HResult; virtual; stdcall; abstract;
    function GetArguments(pszArgs: LPSTR; cchMaxPath: Integer): HResult; virtual; stdcall; abstract;
    function SetArguments(pszArgs: LPSTR): HResult; virtual; stdcall; abstract;
    function GetHotkey(var pwHotkey: Word): HResult; virtual; stdcall; abstract;
    function SetHotkey(wHotkey: Word): HResult; virtual; stdcall; abstract;
    function GetShowCmd(var piShowCmd: Integer): HResult; virtual; stdcall; abstract;
    function SetShowCmd(iShowCmd: Integer): HResult; virtual; stdcall; abstract;
    function GetIconLocation(pszIconPath: LPSTR; cchIconPath: Integer;
      var piIcon: Integer): HResult; virtual; stdcall; abstract;
    function SetIconLocation(pszIconPath: LPSTR; iIcon: Integer): HResult; virtual; stdcall; abstract;
    function SetRelativePath(pszPathRel: LPSTR; dwReserved: DWORD): HResult; virtual; stdcall; abstract;
    function Resolve(Wnd: HWND; fFlags: DWORD): HResult; virtual; stdcall; abstract;
    function SetPath(pszFile: LPSTR): HResult; virtual; stdcall; abstract;
  end;
{$ENDIF}

const
  LinkExt = '.lnk';

procedure CreateFileLink(const FileName, DisplayName: string; Folder: Integer);
var
  ShellLink: IShellLink;
  PersistFile: IPersistFile;
  ItemIDList: PItemIDList;
  FileDestPath: array[0..MAX_PATH] of Char;
  FileNameW: array[0..MAX_PATH] of WideChar;
begin
  CoInitialize(nil);
  try
    OleCheck(CoCreateInstance(CLSID_ShellLink, nil, CLSCTX_SERVER,
      IID_IShellLinkA, ShellLink));
    try
      OleCheck(ShellLink.QueryInterface(IID_IPersistFile, PersistFile));
      try
        OleCheck(SHGetSpecialFolderLocation(0, Folder, ItemIDList));
        SHGetPathFromIDList(ItemIDList, FileDestPath);
        StrCat(FileDestPath, PChar('\' + DisplayName + LinkExt));
        ShellLink.SetPath(PChar(FileName));
        ShellLink.SetIconLocation(PChar(FileName), 0);
        MultiByteToWideChar(CP_ACP, 0, FileDestPath, -1, FileNameW, MAX_PATH);
        OleCheck(PersistFile.Save(FileNameW, True));
      finally
{$IFDEF RX_D3}
        PersistFile := nil;
{$ELSE}
        PersistFile.Release;
{$ENDIF}
      end;
    finally
{$IFDEF RX_D3}
      ShellLink := nil;
{$ELSE}
      ShellLink.Release;
{$ENDIF}
    end;
  finally
    CoUninitialize;
  end;
end;

procedure DeleteFileLink(const DisplayName: string; Folder: Integer);
var
  ShellLink: IShellLink;
  ItemIDList: PItemIDList;
  FileDestPath: array[0..MAX_PATH] of Char;
begin
  CoInitialize(nil);
  try
    OleCheck(CoCreateInstance(CLSID_ShellLink, nil, CLSCTX_SERVER,
      IID_IShellLinkA, ShellLink));
    try
      OleCheck(SHGetSpecialFolderLocation(0, Folder, ItemIDList));
      SHGetPathFromIDList(ItemIDList, FileDestPath);
      StrCat(FileDestPath, PChar('\' + DisplayName + LinkExt));
      DeleteFile(FileDestPath);
    finally
{$IFDEF RX_D3}
      ShellLink := nil;
{$ELSE}
      ShellLink.Release;
{$ENDIF}
    end;
  finally
    CoUninitialize;
  end;
end;

{$ENDIF WIN32}

{$IFNDEF RX_D3}
function IsPathDelimiter(const S: string; Index: Integer): Boolean;
begin
  Result := (Index > 0) and (Index <= Length(S)) and (S[Index] = '\');
end;
{$ENDIF}

end.