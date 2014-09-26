unit OLEDrop;
interface
uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, activex;

type
  TOleDragObject = class(TDragObject)
  private
    dataObj  : IDataObject;
    Fkeys    : longint;
    FDown    : boolean;
    FString  : string;
    FList    : TStringlist;
    function  GetFileList:TStringList;
    function GetString:string;
  public
    constructor create;
    destructor destroy; override;
    function DragContent : integer;
    property Keys : longint read Fkeys;
    property FileList : TStringList read GetFileList;
    property StringData : string read GetString;
  end;

type
  IWCDropTarget = class(TInterfacedObject, IDropTarget)
  private
    FOwner    : TWinControl;
    Fhandle   : HWND;
    FdragObj  : ToleDragObject;
    FTarget   : Pointer;
  public
    { IDropTarget }
    function DragEnter(const dataObj: IDataObject; grfKeyState: Longint;
                   pt: TPoint; var dwEffect: Longint): HResult; stdcall;
    function DragOver(grfKeyState: Longint; pt: TPoint;
                   var dwEffect: Longint): HResult; stdcall;
    function DragLeave: HResult; stdcall;
    function Drop(const dataObj: IDataObject; grfKeyState: Longint; pt: TPoint;
                   var dwEffect: Longint): HResult; stdcall;
    constructor create(aOwner:TWinControl);
    destructor destroy; override;
  end;

implementation

Type
  PDropFiles = ^TDropFiles;
  TDropFiles = record
    pfiles : DWORD;
    pt     : TPOINT;
    fNC    : BOOL;
    fWide  : BOOL;
  end;

{------------------------------------------------------------------------------}
// Copy of the control unit DragMessage function

function DragMessage(Handle: HWND; Msg: TDragMessage; Source: TDragObject; Target: Pointer; const Pos: TPoint): Longint;
var DragRec: TDragRec;
begin
  Result := 0;
  if Handle <> 0 then
  begin
    DragRec.Pos := Pos;
    DragRec.Target := Target;
    DragRec.Source := Source;
    Result := SendMessage(Handle, CM_DRAG, Longint(Msg), Longint(@DragRec));
  end;
end;

{------------------------------------------------------------------------------}
constructor TOleDragObject.create;
begin
  Inherited create;
  FList := TStringList.Create;
end;

destructor TOleDragObject.destroy;
begin
  FList.Free;
  Inherited destroy;
end;

function TOleDragObject.DragContent : integer;
var fmt : TFormatEtc;
    efe : iEnumFormatEtc;
    fmtCount: LongInt;

begin
  if (FDown) or (Not assigned(dataobj)) then
  begin
    result:=-1;
    exit;
  end;
  fillchar(fmt,sizeof(fmt),0);
  DataObj.EnumFormatEtc(datadir_get,efe);
  EFE.Reset;
  repeat
    fmtCount:=0;
    efe.Next(1,fmt,@fmtCount);
  until (fmt.cfFormat=CF_HDROP) or (fmtCount=0);
  if (fmt.cfFormat=CF_HDROP) then
  begin
    result:=fmt.cfFormat;
    exit;
  end;
  EFE.Reset;
  repeat
    fmtCount:=0;
    efe.Next(1,fmt,@fmtCount);
  until (fmt.cfFormat=CF_TEXT) or (fmtCount=0);
  if (fmt.cfFormat=CF_TEXT) then
  begin
    result:=fmt.cfFormat;
    exit;
  end;
  result:=-1;
end;

function TOleDragObject.GetString;
 var mdm: TStgMedium;
     pz : pchar;
     fmt : TFormatEtc;
     efe : iEnumFormatEtc;
     fmtCount: LongInt;

begin
  Result := FString;
  if (FDown) or (Not assigned(dataobj)) then exit;
  result:='';
  fillchar(fmt,sizeof(fmt),0);
  DataObj.EnumFormatEtc(datadir_get,efe);
  EFE.Reset;
  repeat
    fmtCount:=0;
    efe.Next(1,fmt,@fmtCount);
  until (fmt.cfFormat=CF_TEXT) or (fmtCount=0);
  if fmt.cfFormat<>CF_TEXT then
  begin
    result:='';
    exit;
  end;
  fmt.tymed := TYMED_HGLOBAL;
  fmt.lindex := -1;
  if dataobj.GetData(fmt,mdm)<>S_OK then result:='' else
  try
  if (fmt.cfFormat=CF_TEXT) and (mdm.tymed = TYMED_HGLOBAL) then
    begin
      pz := GlobalLock(mdm.HGlobal);
      FString:=StrPas(pz);
      Result:=FString;
      GlobalUnlock(mdm.HGlobal);
    end;
  finally
    if Assigned(mdm.unkForRelease) then
       Iunknown(mdm.unkForRelease)._Release;
    FDown  := TRUE;
  end;
end;

function TOleDragObject.GetFileList;
var mdm:TStgMedium;
    pz  : pchar;
    pdf : PDropFiles;
    fmt : TFormatEtc;
    s   : string;

begin
  Result := FList;
  if (FDown) or (Not assigned(DataObj)) then exit;
  FList.Clear;
  FillChar(fmt,sizeof(fmt),0);
  fmt.cfFormat := CF_HDROP;
  fmt.tymed := TYMED_HGLOBAL;
  fmt.lindex := -1;
  if dataobj.GetData(fmt,mdm)<>S_OK then
     raise Exception.Create('IDataObject.GetData failed');
  try
  if mdm.tymed = TYMED_HGLOBAL then
    begin
      pdf := GlobalLock(mdm.HGlobal);
      pz  := pchar(pdf);
      Inc(pz, pdf^.pFiles);
      if not (pdf.fWide) then
      while (pz[0]<>#0) do
      begin
        FList.Add(string(pz));
        Inc(pz,1+strlen(pz));
      end
      else
      while (pz[0]<>#0) do
      begin
        s:=WideCharToString(PWideChar(pz));
        FList.Add(s);
        Inc(pz,length(s)*2+2);
      end;
      GlobalUnlock(mdm.HGlobal);
    end;
  finally
    if Assigned(mdm.unkForRelease) then
       IUnknown(mdm.unkForRelease)._Release;
    FDown  := TRUE;
  end;
end;

{------------------------------------------------------------------------------}

constructor IWCDropTarget.create(aOwner:TWinControl);
var rslt:HResult; obj:IDropTarget;
begin
  Inherited create;
  oleInitialize(nil);
  FOwner  := AOwner;
  FHandle := aOwner.Handle;
  _AddRef;
  if not GetInterface(Iunknown,obj) then
     raise Exception.Create('GetInterface failed');

  Rslt := RegisterDragDrop(FHandle,obj as IDroptarget);
  case Rslt of
    S_OK : ;
    DRAGDROP_E_INVALIDHWND       : raise Exception.Create('RegisterDragDrop failed, invalide hwnd ');
    DRAGDROP_E_ALREADYREGISTERED : raise Exception.Create('RegisterDragDrop failed, already registered');
    E_OUTOFMEMORY                : raise Exception.Create('RegisterDragDrop failed, out of memory');
    E_INVALIDARG                 : raise Exception.Create('RegisterDragDrop failed, invalid arg');
    CO_E_NOTINITIALIZED          : raise Exception.Create('RegisterDragDrop failed, coInitialize had not been called');
    else                           raise Exception.Create('RegisterDragDrop failed, unknown error code '+IntToStr(rslt and $7FFFFFFF));
  end;
end;

destructor IWCDropTarget.destroy;
begin
  RevokeDragDrop(FHandle);
  oleUnInitialize;
  Inherited destroy;
end;

function IWCDropTarget.DragEnter(const dataObj: IDataObject; grfKeyState: Longint; pt: TPoint; var dwEffect: Longint): HResult;
begin
  // TDragState = (dsDragEnter, dsDragLeave, dsDragMove);
  // TDragMessage = (dmDragEnter, dmDragLeave, dmDragMove, dmDragDrop, dmDragCancel,dmFindTarget);

  FDragobj := ToleDragObject.Create; Fdragobj.dataObj := dataObj; Fdragobj.Fkeys := grfKeyState;
  FTarget  := pointer(DragMessage(FHandle,dmFindTarget,FDragObj,NIL,pt));

  if DragMessage(FHandle,dmDragEnter,Fdragobj,FTarget,pt)<>0
     then dwEffect := DROPEFFECT_LINK
     else dwEffect := DROPEFFECT_NONE;

  Result := S_OK;
end;

function IWCDropTarget.DragOver(grfKeyState: Longint; pt: TPoint; var dwEffect: Longint): HResult;
begin
  Fdragobj.Fkeys := grfKeyState;
  FTarget := pointer(DragMessage(FHandle,dmFindTarget,FDragObj,NIL,pt));
  if DragMessage(FHandle,dmDragMove,Fdragobj,Ftarget,pt)<>0
     then dwEffect := DROPEFFECT_LINK
     else dwEffect := DROPEFFECT_NONE;
  Result := S_OK;
end;

function IWCDropTarget.DragLeave: HResult;
var pt:TPoint;
begin
  DragMessage(FHandle,dmDragCancel,Fdragobj,FTarget,pt);
  if Assigned(FDragObj) then
  begin
    Fdragobj.Free;
    Fdragobj := nil;
  end;
  Result := S_OK;
end;

function IWCDropTarget.Drop(const dataObj: IDataObject; grfKeyState: Longint; pt: TPoint; var dwEffect: Longint): HResult;
begin
  Fdragobj.Fkeys := grfKeyState;
  if DragMessage(FHandle,dmDragDrop,Fdragobj,FTarget,pt)<>0
     then dwEffect := DROPEFFECT_LINK
     else dwEffect := DROPEFFECT_NONE;
  if Assigned(FDragObj) then
  begin
    Fdragobj.Free;
    Fdragobj := nil;
  end;
  Result := S_OK;
end;

end.

