unit Main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  Menus, ElTree, StdCtrls, ElStatBar, IniOpts, IniStrings, ElIni, ExtCtrls,
  ElMRU, ElColorMap, ElOneInst, ElClipMon, ElList, ElCbFmts,
  ElImgLst, ElClock, ComCtrls, ElPopBtn, ElTools, ElHeader, ElSpinBtn,
  ElStrUtils, ActiveX, ToolWin, ElHint, ElDragDrop, ElPromptDlg
{$IFDEF VCL_4_USED}
  , ImgList
{$ENDIF}
  , ElTray, ElPanel, ElFrmPers, ElCaption, ElToolBar, ElDailyTip, ElBtnCtl,
  ElXPThemedControl, ElBaseComp, ElTreeBtnEdit, ElTreeMemoEdit,
  ElTreeSpinEdit, ElTreeCheckBoxEdit, ElTreeModalEdit, ElTreeComboBox;

type
  TMainForm = class(TForm)
    Tree: TElTree;
    MainMenu: TMainMenu;
    FileMenu: TMenuItem;
    NewItem: TMenuItem;
    StatusBar: TElStatusBar;
    EditMenu: TMenuItem;
    OptionsMenu: TMenuItem;
    StandardItem: TMenuItem;
    EnhancedItem: TMenuItem;
    N1: TMenuItem;
    IniFile: TElIniFile;
    OpenItem: TMenuItem;
    SaveItem: TMenuItem;
    SaveAsItem: TMenuItem;
    N2: TMenuItem;
    ExitItem: TMenuItem;
    ModifImage: TImage;
    LazyItem: TMenuItem;
    N3: TMenuItem;
    AutoSaveItem: TMenuItem;
    SavenowItem: TMenuItem;
    N4: TMenuItem;
    RecentMenu: TMenuItem;
    OptionsIni: TElIniFile;
    MRU: TElMRU;
    OpenDlg: TOpenDialog;
    ColorMap: TElColorMap;
    N5: TMenuItem;
    ColorsSubMenu: TMenuItem;
    UseCustomItem: TMenuItem;
    CustColorsItem: TMenuItem;
    OneInst: TElOneInstance;
    N6: TMenuItem;
    OneInstItem: TMenuItem;
    ClipMon: TElClipboardMonitor;
    Images: TElImageList;
    ElClock: TElClock;
    CutItem: TMenuItem;
    CopyItem: TMenuItem;
    PasteItem: TMenuItem;
    DeleteItem: TMenuItem;
    SelectAllItem: TMenuItem;
    MRUPopup: TPopupMenu;
    NewEntryItem: TMenuItem;
    N7: TMenuItem;
    RenameItem: TMenuItem;
    CreateKeyItem: TMenuItem;
    N8: TMenuItem;
    CreateBoolItem: TMenuItem;
    CreateIntItem: TMenuItem;
    CreateStringItem: TMenuItem;
    CreateMStringItem: TMenuItem;
    CreateBinaryItem: TMenuItem;
    ModifyItem: TMenuItem;
    SaveDlg: TSaveDialog;
    SortItem: TMenuItem;
    ElTray: TElTrayIcon;
    FormCaption: TElFormCaption;
    ElFormPersist: TElFormPersist;
    ElToolBar1: TElToolBar;
    ExitBtn: TElToolButton;
    ElToolButton3: TElToolButton;
    NewBtn: TElToolButton;
    OpenBtn: TElToolButton;
    SaveBtn: TElToolButton;
    DailyTipDlg: TElDailyTipDialog;
    ButtonEdit: TElTreeInplaceButtonEdit;
    MemoEdit: TElTreeInplaceMemoEdit;
    SpinEdit: TElTreeInplaceSpinEdit;
    CheckBoxEdit: TElTreeInplaceCheckBoxEdit;
    ModalEdit: TElTreeInplaceModalEdit;
    procedure FormCreate(Sender: TObject);
    procedure StandardItemClick(Sender: TObject);
    procedure EnhancedItemClick(Sender: TObject);
    procedure OpenItemClick(Sender: TObject);
    procedure ExitItemClick(Sender: TObject);
    procedure SaveItemClick(Sender: TObject);
    procedure LazyItemClick(Sender: TObject);
    procedure SaveAsItemClick(Sender: TObject);
    procedure SavenowItemClick(Sender: TObject);
    procedure AutoSaveItemClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure MRUClick(Sender: TObject; Entry: TElMRUEntry);
    procedure NewItemClick(Sender: TObject);
    procedure ColorMapChange(Sender: TObject);
    procedure UseCustomItemClick(Sender: TObject);
    procedure CustColorsItemClick(Sender: TObject);
    procedure OneInstInstanceRun(Sender: TObject; Parameters: TStrings);
    procedure OneInstItemClick(Sender: TObject);
    procedure DropTargetTargetDrag(Sender: TObject; State: TDragState;
      Source: TOleDragObject; Shift: TShiftState; X, Y: Integer;
      var DragType: TDragType);
    procedure ClipMonChange(Sender: TObject);
    procedure TreeItemPicDraw(Sender: TObject; Item: TElTreeItem;
      var ImageIndex: Integer);
    procedure SelectAllItemClick(Sender: TObject);
    procedure TreeItemFocused(Sender: TObject);
    procedure NewEntryItemClick(Sender: TObject);
    procedure ModifyItemClick(Sender: TObject);
    procedure RenameItemClick(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
    procedure TreeItemDeletion(Sender: TObject; Item: TElTreeItem);
    procedure TreeItemSelectedChange(Sender: TObject; Item: TElTreeItem);
    procedure DeleteItemClick(Sender: TObject);
    procedure CopyItemClick(Sender: TObject);
    procedure CutItemClick(Sender: TObject);
    procedure PasteItemClick(Sender: TObject);
    procedure DropTargetTargetDrop(Sender: TObject; Source: TOleDragObject;
      Shift: TShiftState; X, Y: Integer; var DragType: TDragType);
    procedure SortItemClick(Sender: TObject);
    procedure TreeKeyUp(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure FormCaptionButtonClick(Sender: TObject;
      Button: TElCaptionButton);
    procedure ElTrayDblClick(Sender: TObject);
    procedure TreeOleDragFinish(Sender: TObject; dwEffect: TDragType;
      Result: HResult);
    procedure TreeOleDragStart(Sender: TObject; var dataObj: IDataObject;
      var dropSource: IDropSource; var dwOKEffects: TDragTypes);
    procedure TreeTryEdit(Sender: TObject; Item: TElTreeItem;
      SectionIndex: Integer; var CellType: TElFieldType;
      var CanEdit: Boolean);
    procedure ModalEditExecute(Sender: TObject;
      var Accepted: Boolean);
    procedure CheckBoxEditAfterOperation(Sender: TObject;
      var Accepted, DefaultConversion: Boolean);
    procedure ButtonEditValidateResult(Sender: TObject;
      var InputValid: Boolean);
    procedure SpinEditAfterOperation(Sender: TObject;
      var Accepted, DefaultConversion: Boolean);
    procedure MemoEditAfterOperation(Sender: TObject;
      var Accepted, DefaultConversion: Boolean);
    procedure ButtonEditAfterOperation(Sender: TObject;
      var Accepted, DefaultConversion: Boolean);
    procedure ButtonEditBeforeOperation(Sender: TObject;
      var DefaultConversion: Boolean);
  private
    { Private declarations }
    FFileName : string;
    FModified : boolean;

    // Actions
    FAction : integer; // 0 - create key
                       // 1 - create value
                       // 2 - edit key name
                       // 3 - edit value name
                       // 4 - edit key value (strange name, isn't it?)
                       // 5 - edit value value (strange name, isn't it?)

    // FSaveCellType is needed to set the right value type after editing
    FSaveCellType : TElFieldType;

    FList1 : TElList;
    // the stream will hold the data prepared for OLE drag and Clipboard operations
    FStream1 : TDirectMemoryStream;
    FIgnoreSelect : boolean; // this one is used to prevent stack overflow in
                             // the method, that selects subitems
    FIgnoreDelete : boolean; // this one is used to prevent deleting the INI
                             // records when the tree is destroyed
    procedure SetFileName(NewValue : string);
    procedure SetModified(NewValue : boolean);        
    procedure SetItemStyles(Item : TElTreeItem);
    procedure PrepareCBList;
    procedure DoPasteData(Item : TElTreeItem);
  protected
    ClipFormat: integer;

    procedure Loaded; override;
    function CloseCurrent : boolean;
    function DoLoad(FileName : string) : boolean;
    function DoSave(FileName : string) : boolean;
    procedure UpdateBtns;
    procedure DoDeleteItems(Warn : boolean);
    procedure AppMinimize(Sender : TObject);
  public
    procedure RefreshItems;

    property FileName : string read FFileName write SetFileName;
    property Modified : boolean read FModified write SetModified;
  end;

type TIniDragObject = class(TInterfacedObject, IDropSource, IDataObject)
      private
        fEffect : LongInt;
      public
       // IDropSource implementation
       function QueryContinueDrag(FEscapePressed: Bool; GrfKeyState: LongInt): HRESULT; StdCall;
      function GiveFeedback(dwEffect: LongInt) : HRESULT; StdCall;
      // IDataObject implementation
      function GetData(const FormatEtcIn: TFormatEtc; out Medium: TStgMedium) : HRESULT;StdCall;
      function GetDataHere(const FormatEtcIn: TFormatEtc; out Medium: TStgMedium) : HRESULT;StdCall;
      function QueryGetData(const FormatEtc: TFormatEtc) : HRESULT;StdCall;
      function GetCanonicalFormatEtc(const FormatEtc: TFormatEtc;
                                     out FormatEtcOut: TFormatEtc) : HRESULT;StdCall;
      function SetData(const FormatEtc: TFormatEtc; var Medium: TStgMedium; fRelease : Bool) : HRESULT;StdCall;
      function EnumFormatEtc(dwDirection: LongInt; out EnumFormatEtc: IEnumFormatEtc) : HRESULT;StdCall;
      function dAdvise(const FormatEtc: TFormatEtc; advf: LongInt;
                       const advsink: IAdviseSink; out dwConnection: LongInt) : HRESULT;StdCall;
      function dUnadvise(dwConnection: LongInt) : HRESULT; StdCall;
      function EnumdAdvise(out EnumAdvise: IEnumStatData) : HRESULT; StdCall;
     end;

var
  MainForm: TMainForm;

// Entry ids for color map
const cidcmNwsk  = 878498542;
      cidcmNwosk = 703720164;
      cidcmVint  = 508399275;
      cidcmVbool = -131481887;
      cidcmVStr  = -1627567326;
      cidcmVMstr = 1997096895;
      cidcmVBin  = -249209621;
      cidcmVUndef= 850363309;

const BoolValues : array [boolean] of string = ('False', 'True');

implementation

{$R *.DFM}
{$R IniEdits.res}
{$R IniTips.res}
(*
function TIniDragObject._AddRef: Integer;
begin
  Inc(FRefCount);
  Result := FRefCount;
end;

function TIniDragObject._Release: Integer;
begin
  Dec(FRefCount);
  if FRefCount = 0 then
  begin
    Destroy;
    Result := 0;
    Exit;
  end;
  Result := FRefCount;
end;

function TIniDragObject.QueryInterface(const IID: TGUID; out Obj): HResult;
const
  E_NOINTERFACE = $80004002;
begin
  if GetInterface(IID, Obj) then Result := 0 else Result := E_NOINTERFACE;
end;
*)
function TIniDragObject.QueryContinueDrag(FEscapePressed: Bool; GrfKeyState: LongInt): HRESULT;
begin
  if (fEscapePressed) then RESULT := dragdrop_s_cancel else
  begin
    if (MK_LBUTTON and grfKeyState) <>0
       then result := S_OK
       else RESULT := DRAGDROP_S_DROP;
  end;
end;

function TIniDragObject.GiveFeedback(dwEffect: LongInt) : HRESULT;
begin
  fEffect := dwEffect;
  result := DRAGDROP_S_USEDEFAULTCURSORS;
end;

function TIniDragObject.QueryGetData;  { public }
begin
  if (FormatEtc.cfFormat <> MainForm.ClipFormat) then result:= DV_E_FORMATETC
  else
  if (FormatEtc.dwAspect <> DVASPECT_CONTENT) then result:= DV_E_DVASPECT
  else result:= S_OK;
end;  { QueryGetData }

function TIniDragObject.GetCanonicalFormatEtc(const FormatEtc: TFormatEtc; out FormatEtcOut: TFormatEtc) : HRESULT;  { public }
begin
   FormatEtcOut.ptd:=nil;
   Result := E_NOTIMPL;
end;  { GetCanonicalFormatEtc }

function TIniDragObject.SetData(const FormatEtc: TFormatEtc; var Medium: TStgMedium; fRelease : Bool) : HRESULT;  { public }
begin
  result := E_NOTIMPL;
end;  { SetData }

function TIniDragObject.dAdvise(const FormatEtc: TFormatEtc; advf: LongInt; const advsink: IAdviseSink; out dwConnection: LongInt) : HRESULT;  { public }
begin
  result := OLE_E_ADVISENOTSUPPORTED;
end;  { dAdvise }

function TIniDragObject.dUnadvise(dwConnection: LongInt) : HRESULT;  { public }
begin
  result := OLE_E_ADVISENOTSUPPORTED;
end;  { dUnadvise }

function TIniDragObject.EnumdAdvise(out EnumAdvise: IEnumStatData) : HRESULT;  { public }
begin
  result := OLE_E_ADVISENOTSUPPORTED;
end;  { EnumdAdvise }

function TIniDragObject.EnumFormatEtc(dwDirection: LongInt; out EnumFormatEtc: IEnumFormatEtc) : HRESULT;  { public }
var F : PFormatEtc;
begin
  if (dwDirection = DATADIR_GET) then
  begin
    New(F);
    F^.cfFormat := MainForm.ClipFormat;
    F^.ptd := nil;
    F^.dwAspect := DVASPECT_CONTENT;
    F^.lIndex := -1;
    F^.tymed := TYMED_HGLOBAL;
    EnumFormatEtc := (TEnumFormatEtc.Create(PFormatList(F), 1, 0) as IEnumFormatEtc);
    result := S_OK;
  end
  else
  if (dwDirection = DATADIR_SET) then result := E_NOTIMPL
  else result := E_INVALIDARG;
end;  { EnumFormatEtc }

function TIniDragObject.GetData(const FormatEtcIn: TFormatEtc; out Medium: TStgMedium) : HRESULT;  { public }
begin
  Medium.tymed := 0;
  Medium.UnkForRelease := nil;
  Medium.hGlobal := 0;
  if (FormatEtcIn.cfFormat = MainForm.ClipFormat) and
     (FormatEtcIn.dwAspect = DVASPECT_CONTENT) and
     (FormatEtcIn.tymed = TYMED_HGLOBAL) then
  begin
    if (MainForm.FStream1)<>nil then
    begin
      Medium.hGlobal := GlobalAlloc(GMEM_SHARE OR GMEM_ZEROINIT, MainForm.FStream1.Size);
      if (Medium.hGlobal = 0) then
      begin
        result := E_OUTOFMEMORY;
        Exit;
      end;
      medium.tymed := TYMED_HGLOBAL;
      result := GetDataHere(FormatEtcIn, Medium);
    end else result := E_UNEXPECTED;
  end
  else result := DV_E_FORMATETC;
end;  { GetData }

function TIniDragObject.GetDataHere(const FormatEtcIn: TFormatEtc; out Medium: TStgMedium) : HRESULT;  { public }
var p:pointer;
begin
  if (FormatEtcIn.cfFormat = MainForm.ClipFormat) and
     (FormatEtcIn.dwAspect = DVASPECT_CONTENT) and
     (FormatEtcIn.tymed = TYMED_HGLOBAL) and (Medium.tymed = TYMED_HGLOBAL) then
  begin
    if (Medium.hGlobal = 0) then
    begin
      result := E_OUTOFMEMORY;
      Exit;
    end;
    p := GlobalLock(Medium.hGlobal);
    if (MainForm.FStream1)<>nil then
    begin
      MoveMemory(p, MainForm.FStream1.Memory, MainForm.FStream1.Size);
      GlobalUnlock(Medium.hGlobal);
      result := S_OK;
      // now remove the old data
      if fEffect = DROPEFFECT_MOVE then MainForm.DoDeleteItems(false);
    end
    else result := E_UNEXPECTED;
    Medium.UnkForRelease := nil;
  end
  else
    result := DV_E_FORMATETC;
end;  { GetDataHere }

(*/////////////////////////////////////////////////////////////////////////////

Main form

/////////////////////////////////////////////////////////////////////////////*)

procedure TMainForm.UpdateBtns;
var b : boolean;
begin
  b := Tree.SelectedCount >0;
  if Assigned(CopyItem) then CopyItem.Enabled := b;
  if Assigned(CutItem) then CutItem.Enabled := b;
  if Assigned(DeleteItem) then DeleteItem.Enabled := b;
end;

procedure TMainForm.Loaded;
begin
  inherited;
end;

procedure TMainForm.SetModified(NewValue : boolean);
begin
  if NewValue <> FModified then
  begin
    if (not Options.LazyWrite) and (NewValue) then exit;
    FModified := NewValue;
    if FModified then ModifImage.Picture.Bitmap.LoadFromResourceName(hInstance, 'EXCLAMATION')
                 else ModifImage.Picture.Bitmap.Assign(nil);
    SaveItem.Enabled := Modified;
    SaveBtn.Enabled := Modified;
  end;
end;

function TMainForm.CloseCurrent : boolean;
begin
  if FModified then
  begin
    case MessageDlg(Format('%s was modified. Save it now?', [FileName]), mtWarning , [mbYes, mbNo, mbCancel], 0) of
      mrYes: result := DoSave(FileName);
      mrNo:  result := true;
      else   result := false;
    end;
  end else result := true;
  if Result then
  begin
    IniFile.Path := '';
    FIgnoreDelete := true;
    Tree.Items.Clear;
    FIgnoreDelete := false;
    FileName := sUntitled;
    Modified := false;
  end;
end;

procedure TMainForm.SetFileName(NewValue : string);
begin
  FFileName := newValue;
  FormCaption.Texts[3].Caption := FFileName;
end;

procedure TMainForm.FormCreate(Sender: TObject);
begin
  SetHintWindow;
  FileName := sUntitled;
  OptionsIni.Path :=  ExtractFilePath(ParamStr(0)) + 'IniEdit.ini';
  try
    OptionsIni.Load; // this is necessary!!!
  except
  end;
  Options := TOptions.Create(nil);
  Options.Storage := OptionsIni;
  Options.AutoSave := true;
  Options.Load;
  ColorMap.Restore;
  MRU.Restore;
  Tree.Restore;
  StatusBar.Restore;
  Modified := false;
  ClipFormat := RegisterClipboardFormat('EldoS IniEditor items format');
  FAction := -1;
  FList1 := TElList.Create;
  if ParamCount >0 then
  begin
    Options.Simple := (Uppercase(ExtractFileExt(ParamStr(1))) <> '.EIF');
    if DoLoad(ParamStr(1)) then
    begin                      
      FileName := ParamStr(1);
      if Uppercase(ExtractFileExt(FileName)) = '.EIF' then MRU.Sections[0].Add(FileName, 0) else
      if Uppercase(ExtractFileExt(FileName)) = '.INF' then MRU.Sections[2].Add(FileName, 0) else
      if Uppercase(ExtractFileExt(FileName)) = '.INI' then MRU.Sections[1].Add(FileName, 0) else
      MRU.Sections[3].Add(FileName, 0);
    end;
  end;
  Application.OnMinimize := AppMinimize;
  DailyTipDlg.ShowNextTime := Options.ShowDailyTip;
  if Options.ShowDailyTip then
  begin
    DailyTipDlg.Execute;
    Options.ShowDailyTip := DailyTipDlg.ShowNextTime;
  end;
end;

procedure TMainForm.FormDestroy(Sender: TObject);
begin
  if Assigned(FList1) then
  begin
    FList1.Free;
    FList1 := nil;
  end;
  if Assigned(FStream1) then
  begin
    FStream1.Free;
    FStream1 := nil;
  end;
  MRU.Save;
  ColorMap.Save;
  StatusBar.Save;
  Tree.Save;
  Options.Free;
end;

procedure TMainForm.StandardItemClick(Sender: TObject);
begin
  Options.Simple := true;
  Modified := true;
end;

procedure TMainForm.EnhancedItemClick(Sender: TObject);
begin
  Options.Simple := false;
  Modified := true;
end;

procedure TMainForm.OpenItemClick(Sender: TObject);
begin
  if OpenDlg.Execute then
  begin
    if not CloseCurrent then exit;
    Options.Simple := (Uppercase(ExtractFileExt(OpenDlg.FileName)) <> '.EIF');
    if DoLoad(OpenDlg.FileName) then
    begin
      FileName := OpenDlg.FileName;
      if Uppercase(ExtractFileExt(FileName)) = '.EIF' then MRU.Sections[0].Add(FileName, 0) else
      if Uppercase(ExtractFileExt(FileName)) = '.INF' then MRU.Sections[2].Add(FileName, 0) else
      if Uppercase(ExtractFileExt(FileName)) = '.INI' then MRU.Sections[1].Add(FileName, 0) else
      MRU.Sections[3].Add(FileName, 0);
    end;
  end;
end;

procedure TMainForm.ExitItemClick(Sender: TObject);
begin
  Close;
end;

function TMainForm.DoLoad;

   procedure IntFillData(Key, Value : string; Item : TElTreeItem);
   var bval : boolean;
       sval : string;
       ival : integer;

   begin
     case IniFile.GetValueType(Key, Value) of
       evtBoolean:
         begin
           IniFile.ReadBool(Key, Value, false, bval);
           if bval then Item.ColumnText.Add('True') else Item.ColumnText.Add('False');
           Item.Data := pointer(Integer(Item.Data) or (ord(evtBoolean) shl 1));
         end;
       evtInt:
         begin
           IniFile.ReadInteger(Key, Value, 0, ival);
           Item.ColumnText.Add(IntToStr(ival));
           Item.Data := pointer(Integer(Item.Data) or (ord(evtInt) shl 1));
         end;
       evtString:
         begin
           IniFile.ReadString(Key, Value, '', sval);
           Item.ColumnText.Add(sval);
           Item.Data := pointer(Integer(Item.Data) or (ord(evtString) shl 1));
         end;
       evtMultiString:
         begin
           IniFile.ReadString(Key, Value, '', sval);
           while true do
             if not Replace(sval, #13#10, #32) then break;
           Item.ColumnText.Add(sval);
           Item.Data := pointer(Integer(Item.Data) or (ord(evtMultiString) shl 1));
         end;
       evtBinary:
         begin
           IniFile.ReadString(Key, Value, '', sval);
           Item.ColumnText.Add(sval);
           Item.Data := pointer(Integer(Item.Data) or (ord(evtBinary) shl 1));
        end;
       else Item.ColumnText.Add('');
     end; //case
     Item.ColumnText.Add('');
     SetItemStyles(Item);
   end;

   procedure IntLoad(KeyName : string; Parent : TElTreeItem);
   var Item : TElTreeItem;
       List : TStringList;
       SaveKey : string;
       SubKey : string;
       i : integer;

   begin
     List := TStringList.Create;
     SaveKey := IniFile.CurrentKey;
     if IniFile.OpenKey(KeyName, false) then
     begin
       IniFile.EnumSubKeys('', List);
       for i := 0 to List.Count -1 do
       begin
         Item := Tree.Items.AddChildObject(Parent, List[i], TObject(1));
         if KeyName = IniFile.Delimiter
            then SubKey := KeyName + List[i]
            else SubKey := IniFile.CurrentKey + IniFile.Delimiter + List[i];
         IntFillData(SubKey, '', Item);
         IntLoad(SubKey, Item);
       end;
       List.Clear;
       IniFile.EnumValues('', List);
       for i := 0 to List.Count - 1 do
       begin
         Item := Tree.Items.AddChild(Parent, List[i]);
         IntFillData('', List[i], Item);
       end; // for
     end;
     IniFile.OpenKey(SaveKey, false);
     List.Free;
   end;

var b: boolean;

begin
  IniFile.Path := FileName;
  StatusBar.Panels[0].Text := sLoading;
  result := IniFile.Load;
  b := Options.Sort;
  Options.Sort := false;
  Tree.Items.BeginUpdate;
  IntLoad(IniFile.Delimiter, nil);
  Options.Sort := b;
  Tree.Items.EndUpdate;
  StatusBar.Panels[0].Text := '';
end;

function TMainForm.DoSave;
begin
  IniFile.Path := FileName;
  StatusBar.Panels[0].Text := sSaving;
  result := IniFile.Save;
  StatusBar.Panels[0].Text := '';
  Modified := false;  
end;

procedure TMainForm.SaveItemClick(Sender: TObject);
begin
  if FileName = sUntitled then SaveAsItemClick(Sender) else DoSave(FileName);
end;

procedure TMainForm.LazyItemClick(Sender: TObject);
begin
  Options.LazyWrite := not Options.LazyWrite;
end;

procedure TMainForm.SaveAsItemClick(Sender: TObject);
begin
  if SaveDlg.Execute then
  begin
    if DoSave(SaveDlg.FileName) then FileName := SaveDlg.FileName;
  end;
end;

procedure TMainForm.SavenowItemClick(Sender: TObject);
begin
  Options.Save;
  OptionsIni.Save;
end;

procedure TMainForm.AutoSaveItemClick(Sender: TObject);
begin
  Options.AutoSave := not Options.AutoSave;
end;

procedure TMainForm.MRUClick(Sender: TObject; Entry: TElMRUEntry);
begin
  if not CloseCurrent then exit;
  Options.Simple := (Uppercase(ExtractFileExt(Entry.Name)) <> '.EIF');
  if DoLoad(Entry.Name) then
  begin
    FileName := Entry.Name;
    if Uppercase(ExtractFileExt(FileName)) = '.EIF' then MRU.Sections[0].Add(FileName, 0) else
    if Uppercase(ExtractFileExt(FileName)) = '.INF' then MRU.Sections[2].Add(FileName, 0) else
    if Uppercase(ExtractFileExt(FileName)) = '.INI' then MRU.Sections[1].Add(FileName, 0) else
    MRU.Sections[3].Add(FileName, 0);
  end;
end;

procedure TMainForm.NewItemClick(Sender: TObject);
begin
  CloseCurrent;
end;

procedure TMainForm.ColorMapChange(Sender: TObject);
begin
  RefreshItems;
end;

procedure TMainForm.UseCustomItemClick(Sender: TObject);
begin
  Options.CustomColors := not Options.CustomColors; 
end;

procedure TMainForm.CustColorsItemClick(Sender: TObject);
begin
  ColorMap.Edit('Custom colors');
end;

procedure TMainForm.OneInstInstanceRun(Sender: TObject;
  Parameters: TStrings);
begin
  if Parameters.Count < 2 then Exit;
  if not CloseCurrent then exit;
  Options.Simple := (Uppercase(ExtractFileExt(Parameters[1])) <> '.EIF');
  if DoLoad(Parameters[1]) then
  begin
    FileName := Parameters[1];
    if Uppercase(ExtractFileExt(FileName)) = '.EIF' then MRU.Sections[0].Add(FileName, 0) else
    if Uppercase(ExtractFileExt(FileName)) = '.INF' then MRU.Sections[2].Add(FileName, 0) else
    if Uppercase(ExtractFileExt(FileName)) = '.INI' then MRU.Sections[1].Add(FileName, 0) else
    MRU.Sections[3].Add(FileName, 0);
  end;
end;

procedure TMainForm.OneInstItemClick(Sender: TObject);
begin
  Options.OneInstance := not Options.OneInstance; 
end;

procedure TMainForm.ClipMonChange(Sender: TObject);
var b: boolean;
begin
  b := ClipMon.DataFormats.IndexOf(GetFormatName(ClipFormat))<>-1;
  PasteItem.Enabled := b;
end;

procedure TMainForm.TreeItemPicDraw(Sender: TObject; Item: TElTreeItem;
  var ImageIndex: Integer);
begin
  if (Integer(Item.Data)) mod 2 = 1 then
  begin
    if Item.Focused then ImageIndex := 1 else
    if Item.Expanded then ImageIndex := 0 else ImageIndex := 2;
  end else
  begin
    if Item.Focused then ImageIndex := 4 else ImageIndex := 3;
  end;
end;

procedure TMainForm.SelectAllItemClick(Sender: TObject);
begin
  Tree.SelectAll;
  UpdateBtns;
end;

procedure TMainForm.TreeItemFocused(Sender: TObject);
var b : boolean;
begin
  UpdateBtns;
  if Options.Simple then
  begin
    b := (Tree.ItemFocused <> nil) and (Tree.ItemFocused.Level = 0);
    CreateKeyItem.Enabled := true;
    CreateBoolItem.Enabled := b;
    CreateIntItem.Enabled := b;
    CreateStringItem.Enabled := b;
    CreateMStringItem.Enabled := false;
    CreateBinaryItem.Enabled := false;
  end else
  begin
    CreateKeyItem.Enabled := true;
    CreateBoolItem.Enabled := true;
    CreateIntItem.Enabled := true;
    CreateStringItem.Enabled := true;
    CreateMStringItem.Enabled := true;
    CreateBinaryItem.Enabled := true;
  end;
  b := (Tree.ItemFocused <> nil);
  ModifyItem.Enabled := b;
  RenameItem.Enabled := b;
end;

procedure TMainForm.NewEntryItemClick(Sender: TObject);
var NewItem, ParentItem : TElTreeItem;
    Key : string;
    EType : integer;
begin
  ParentItem := Tree.ItemFocused;
  EType := TMenuItem(Sender).Tag;
  if Options.Simple and (EType = 0) then ParentItem := nil;
  if ParentItem = nil
     then Key := IniFile.Delimiter
     else Key := ParentItem.GetFullName(IniFile.Delimiter);
  Tree.IsUpdating := true;
  NewItem := Tree.Items.AddChild(ParentItem, sNewItem);
  case EType of
    1: NewItem.ColumnText.Add('False');
    2: NewItem.ColumnText.Add('0');
    else NewItem.ColumnText.Add('');
  end;
  NewItem.ColumnText.Add('');
  if ParentItem <> nil then
  begin
    ParentItem.Data := pointer(Integer(ParentItem.Data) or 1);
    SetItemStyles(ParentItem);
  end;
  if EType = 0 then NewItem.Data := pointer(1) else NewItem.Data := pointer((EType) shl 1);
  SetItemStyles(NewItem);
  if EType <> 0
     then FAction := 1
     else FAction := 0;
  Tree.ItemFocused := NewItem;
  Tree.IsUpdating := false;
  NewItem.EditText;
end;

procedure TMainForm.RefreshItems;

  procedure IntUpdProc(Item:TElTreeItem; Index: integer; var ContinueIterate:boolean;
                                          IterateData:pointer; Tree:TCustomElTree);
  begin
    MainForm.SetItemStyles(Item);
  end;

begin
  Tree.Items.BeginUpdate;
  try
    Tree.Items.Iterate(false, true, @IntUpdProc, nil);
  finally
    Tree.Items.EndUpdate;
  end;
end;
{$HINTS OFF}
procedure TMainForm.SetItemStyles(Item : TElTreeItem);
var C : TColorEntry;
    S : TElCellStyle;
    i : integer;
begin
  Tree.Items.BeginUpdate;
  Item.UseStyles := true;
  while Item.StylesCount > 0 do Item.RemoveStyle(Item.Styles[0]);
  if Item.HasChildren
     then C := ColorMap.Items[ColorMap.EntryByID(cidcmNwsk)]
     else C := ColorMap.Items[ColorMap.EntryByID(cidcmNwosk)];
  with Item.MainStyle do
  begin
    TextFlags := 0;
    OwnerProps := not Options.CustomColors;
    FontName := Font.Name;
    FontSize := Font.Size;
    FontStyles := Font.Style;
    CellBkColor := C.BkColor;
    TextColor := C.FgColor;
    TextBkColor := CellBkColor;
    Style := ElhsText;
    CellType := sftText;
  end;
  S := Item.AddStyle;
  i := Integer(Item.Data) shr 1;
  case i of
    0: i := cidcmVUndef;
    1: i := cidcmVbool;
    2: i := cidcmVInt;
    3: i := cidcmVStr;
    4: i := cidcmVMStr;
    5: i := cidcmVBin;
  end;
  C := ColorMap.Items[ColorMap.EntryByID(i)];
  with S do
  begin
    OwnerProps := not Options.CustomColors;
    FontName := Font.Name;
    FontSize := Font.Size;
    FontStyles := Font.Style;
    CellBkColor := C.BkColor;
    TextColor := C.FgColor;
    TextBkColor := CellBkColor;
    TextFlags := 0;
    Style := ElhsText;
    i := Integer(Item.Data) shr 1;
    case i of
      0: CellType := sftText;
      1: CellType := sftEnum;
      2: CellType := sftNumber;
      3: CellType := sftText;
      4: CellType := sftCustom;
      5: CellType := sftBlob;
    end;
  end;
  S := Item.AddStyle; // this one is for column with value type
  Tree.Items.EndUpdate;
end;
{$HINTS ON}

procedure TMainForm.ModifyItemClick(Sender: TObject);
begin
  Tree.EditItem(Tree.ItemFocused, 1);
  SetItemStyles(Tree.ItemFocused);
end;

procedure TMainForm.RenameItemClick(Sender: TObject);
begin
  Tree.EditItem(Tree.ItemFocused, 0);
end;

procedure TMainForm.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  CanClose := CloseCurrent;
end;

procedure TMainForm.DoDeleteItems(Warn : boolean);
begin
  FList1.Clear;
  Tree.AllSelected(FList1);
  if FList1.Count >0 then
  begin
    if Warn and (MessageDlg(sDoDelete, mtWarning, [mbYes, mbNo], 0)= mrNo) then
    begin
      FList1.Clear;
      exit;
    end;
    Tree.Items.BeginUpdate;
    while FList1.Count > 0 do
      Tree.Items.DeleteItem(TElTreeItem(FList1[0]));
    Tree.Items.EndUpdate;
  end;
end;

procedure TMainForm.TreeItemDeletion(Sender: TObject; Item: TElTreeItem);
var Key, Value : string;
    i : integer;
begin
  if FIgnoreDelete then exit;
  Key := Item.GetFullName(IniFile.Delimiter);
  i := LastPos(IniFile.Delimiter, Key);
  Value := Copy(Key, i + 1, Length(Key));
  Key := Copy(Key, 1, i - 1);
  IniFile.Delete(Key, Value);
  if Assigned(FList1) then FList1.Remove(Item);
end;

procedure TMainForm.TreeItemSelectedChange(Sender: TObject;
  Item: TElTreeItem);

type TSRec = record
       Parent : TElTreeItem;
     end;
     PSRec = ^TSRec;

     procedure IntSel (Item:TElTreeItem; Index: integer; var ContinueIterate:boolean;
                            IterateData:pointer; Tree:TCustomElTree);
     begin
       if not (Item.IsUnder(PSRec(IterateData).Parent)) then
         ContinueIterate := false
       else
         Item.Selected := true;
     end;

var SRec : TSrec;

begin
  // Selecting an item should also cause selection of all it's subitems:
  // if any operation (like deletion or cut or copy) is executed,
  // subitems follow the item.
  // So deselection of the item when one of it's parents is selected
  // is nonsence.
  if Item.Selected and (not FIgnoreSelect) then
  begin
    FIgnoreSelect := true;
    SRec.Parent := Item;
    Tree.Items.BeginUpdate;
    Tree.Items.IterateFrom(false, true, @IntSel, @SRec, Item);
    Tree.Items.EndUpdate;
    FIgnoreSelect := false;
  end else
  if not Item.Selected then
  begin
    SRec.Parent := Item.Parent;
    while SRec.Parent<>nil do
    begin
      if SRec.Parent.Selected then
      begin
        Item.Selected := true;
        exit;
      end else SRec.Parent := SRec.Parent.Parent;
    end;    // while
  end;
end;

procedure TMainForm.DoPasteData(Item : TElTreeItem);

var NoMessageKey,
    NoMessageVal,
    NoMessageVal2 : boolean;
    TempKey  : TElTreeItem;

  function UniqueEntryName(Key, Value : string) : string;
  var i, j : integer;
      SList1, SList2 : TStringList;
      nf : boolean;
  begin
    result := UpperCase(Value);
    SList1 := TStringList.Create;
    SList2 := TStringList.Create;
    IniFile.EnumSubKeys(Key, SList1);
    IniFile.EnumValues(Key, SList2);
    j := 0;
    while true do
    begin
      nf := true;
      for i := 0 to SList1.Count - 1 do
      begin
        if result = Uppercase(SList1[i]) then
        begin
          nf := false;
          break;
        end;
      end;
      if nf then
      begin
        for i := 0 to SList2.Count - 1 do
        begin
          if result = Uppercase(SList2[i]) then
          begin
            nf := false;
            break;
          end;
        end;
      end;
      if nf then
      begin
        if j > 0
           then result := Value + '(' + IntToStr(j) + ')'
           else result := Value;
        exit;
      end;
      inc(j);
      result := UpperCase(Value) + '(' + IntToStr(j) + ')';
    end;
    SList1.Free;
    SList2.Free;
  end;

  procedure IntPaste(Parent : TElTreeItem);
  var Item : TElTreeItem;
      S   : String;
      i,j : integer;
      b   : boolean;
      p   : pointer;
      IsKey : boolean;
      Key, Value, FOldKey : string;
      SList : TStringList;

  begin
    ReadStringFromStream(FStream1, S);
    FStream1.ReadBuffer(j, sizeof(integer));
    IsKey := (j mod 2) = 1;
    if (IniFile.Simple) and (IsKey xor (Parent = nil)) then
    begin
      if IsKey then
      begin
        if not NoMessageKey then
        begin
          MessageDlg('In "Simple" mode keys can not be pasted to a key.'#13#10 +
                     'Some values will be pasted to "root".', mtError, [mbOk], 0);
          NoMessageKey := true;
        end;
        Parent := nil;
      end else
      begin
        if not NoMessageVal then
        begin
          MessageDlg('In "Simple" mode values can be pasted only to some key.'#13#10 +
                     'Some values will be pasted to the temporary key.', mtError, [mbOk], 0);
          NoMessageVal := true;
        end;
        if TempKey = nil then
        begin
          TempKey := Tree.Items.AddChildObject(nil, UniqueEntryName(IniFile.Delimiter, 'Temporary'), TObject(1));
          FOldKey := IniFile.CurrentKey;
          IniFile.OpenKey(IniFile.Delimiter + TempKey.Text, true);
          IniFile.OpenKey(FOldKey, false);
        end;
        Parent := TempKey;
      end;
    end;
    if IniFile.Simple and (Parent <> nil) and (Integer(Parent.Data) mod 2 = 0) then
    begin
      if not NoMessageVal2 then
      begin
        MessageDlg('In "Simple" mode source values can''t be pasted to other values.'#13#10 +
                   'Source values will be pasted to the destination value parent key.', mtError, [mbOk], 0);
        NoMessageVal2 := true;
      end;
      Parent := Parent.Parent;
    end;
    begin
      if Parent = nil
         then Key := IniFile.Delimiter
         else Key := Parent.GetFullName(IniFile.Delimiter) + IniFile.Delimiter;
      Value := UniqueEntryName(Key, S);
      Item := Tree.Items.AddChildObject(Parent, Value, TObject(j));
      Item.Selected := true;
      if IsKey then // we have to create an entry and set it as a key
      begin
        FOldKey := IniFile.CurrentKey;
        IniFile.OpenKey(Key + Value, true);
        IniFile.OpenKey(FOldKey, false);
      end;
      case (j shr 1) of    //
        1:
          begin
            FStream1.ReadBuffer(b, sizeof(boolean));
            IniFile.WriteBool(Key, Item.Text, b);
            if b then Item.ColumnText.Add('True') else Item.ColumnText.Add('False');
          end;
        2:
          begin
            FStream1.ReadBuffer(i, sizeof(integer));
            IniFile.WriteInteger(Key, Item.Text, i);
            Item.ColumnText.Add(IntToStr(i));
          end;
        3:
          begin
            ReadStringFromStream(FStream1, S);
            IniFile.WriteString(Key, Item.Text, S);
            Item.ColumnText.Add(s);
          end;
        4:
          begin
            ReadStringFromStream(FStream1, S);
            SList := TStringList.Create;
            SList.Text := S;
            while true do
               if not Replace(s, #13#10, #32) then break;
            Item.ColumnText.Add(s);
            IniFile.WriteMultiString(Key, ITem.Text, SList);
            SList.Free;
          end;
        5:
          begin
            FStream1.ReadBuffer(i, sizeof(integer));
            GetMem(P, i);
            FStream1.ReadBuffer(PChar(p)^, i);
            IniFile.WriteBinary(Key, Item.Text, PChar(p)^, i);
            IniFile.ReadString(Key, Item.Text, '', s);
            Item.ColumnText.Add(s);
            FreeMem(P);
          end;
      end;    // case
      Item.ColumnText.Add('');
      SetItemStyles(Item);
    end; // else
    FStream1.ReadBuffer(j, sizeof(integer));
    for i := 0 to j - 1 do IntPaste(Item);
  end;

var i : integer;
begin
  NoMessageKey := false;
  NoMessageVal := false;
  NoMessageVal2:= false;
  TempKey := nil;
  Modified := True;
  try
    try
      Tree.Items.BeginUpdate;
      repeat
        FStream1.ReadBuffer(i, sizeof(integer));
        Tree.DeselectAll;
        if i = ord('D') then IntPaste(Item) else break;
      until false;
    finally
      Tree.Items.EndUpdate;
    end;
  except
  end;
end;

procedure TMainForm.PrepareCBList;

  procedure DoPrepare(Item : TElTreeItem);
  var b : boolean;
      S : string;
      i : integer;
      bd: pointer;
      bl: integer;
      Key : string;

  begin
    FList1.Remove(Item);
    WriteStringToStream(FStream1, Item.Text);
    FStream1.WriteBuffer(Integer(Item.Data), sizeof(integer));
    if Item.Parent = nil
       then Key := IniFile.Delimiter
       else Key := Item.Parent.GetFullName(IniFile.Delimiter);
    case (Integer(Item.Data) shr 1) of    //
      1:
        begin
          IniFile.ReadBool(Key, Item.Text, false, b);
          FStream1.WriteBuffer(b, sizeof(boolean));
        end;
      2:
        begin
          IniFile.ReadInteger(Key, Item.Text, 0, i);
          FStream1.WriteBuffer(i, sizeof(integer));
        end;
      3,
      4:
        begin
          IniFile.ReadString(Key, Item.Text, '', S);
          WriteStringToStream(FStream1, S);
        end;
      5:
        begin
          bd := nil;
          bl := 0;
          IniFile.ReadBinary(Key, Item.Text, bd, bl);
          GetMem(bd, bl);
          IniFile.ReadBinary(Key, Item.Text, bd, bl);
          FStream1.WriteBuffer(bl, sizeof(integer));
          FStream1.WriteBuffer(bd, bl);
        end;
    end;    // case
    i := Item.Count;
    FStream1.WriteBuffer(i, sizeof(integer));
    for i := 0 to Item.Count - 1 do    // Iterate
      DoPrepare(Item.Children[i]);
  end;

var i : integer;

begin
  if FStream1 <> nil
     then FStream1.SetSize(0)
     else FStream1 := TDirectMemoryStream.Create;
  FList1.Clear;
  Tree.AllSelected(FList1);
  while FList1.Count > 0 do
  begin
    i := ord('D'); // write a marker to the stream
    FStream1.WriteBuffer(i, sizeof(integer));
    DoPrepare(TElTreeItem(FList1[0]));
  end;
  i := ord('E'); // write End of Stream marker
  FStream1.WriteBuffer(i, sizeof(integer));
end;

procedure TMainForm.DeleteItemClick(Sender: TObject);
begin
  DoDeleteItems(true);
end;

procedure TMainForm.CopyItemClick(Sender: TObject);
var CBHandle : HGLOBAL;
    P : Pointer;
begin
  if OpenClipboard(Handle) then
  begin
    PrepareCBList;
    EmptyClipboard;
    CBHandle := GlobalAlloc(GMEM_MOVEABLE or GMEM_DDESHARE, FStream1.Size);
    P := GlobalLock(CBHandle);
    if p <> nil then
    begin
      MoveMemory(p, FStream1.Memory, FStream1.Size);
      GlobalUnlock(CBHandle);
      SetClipboardData(ClipFormat, CBHandle);
    end;
    CloseClipboard;
  end;
end;

procedure TMainForm.CutItemClick(Sender: TObject);
begin
  CopyItemClick(Self);
  DoDeleteItems(false);
  Modified := true;
end;

procedure TMainForm.PasteItemClick(Sender: TObject);
var CBHandle : HGLOBAL;
    P : Pointer;
    fmt :integer;
begin
  if OpenClipboard(Handle) then
  begin
    fmt := 0;
    repeat
      fmt := EnumClipboardFormats(fmt);
      if fmt=ClipFormat then break;
    until fmt = 0;
    if fmt=ClipFormat then // we can paste data
    begin
      CBHandle := GetClipboardData(ClipFormat);
      if CBHandle <> 0 then
      begin
        p := GlobalLock(CBHandle);
        if p <> nil then
        begin
          if FStream1 <> nil
             then FStream1.SetSize(0)
             else FStream1 := TDirectMemoryStream.Create;
          // as we don't know the size of the stream,
          // we assume it to be the maximum possible
          FStream1.SetPointer(p, $7FFFFFFF);
          // now read the data
          DoPasteData(Tree.ItemFocused);
          // now unlock the data
          FStream1.SetPointer(nil, 0);
          GlobalUnlock(CBHandle);
          Modified := true;
        end; // if
      end; // if
    end; //if
    CloseClipboard;
  end;

end;

procedure TMainForm.DropTargetTargetDrag(Sender: TObject;
  State: TDragState; Source: TOleDragObject; Shift: TShiftState; X,
  Y: Integer; var DragType: TDragType);
var FL: TStringList;
begin
  DragType := dtNone;
  if Source.HasDataFormat(ClipFormat) then
  begin
    if ssCtrl in Shift then DragType := dtCopy else
    if Shift = [ssLeft] then DragType := dtMove else DragType := dtNone;
    exit;
  end;
  if Source.HasDataFormat(CF_HDROP) then
  begin
     FL := Source.FileList;
     if (FL.Count > 0) and FileExists(FL[0]) then DragType := dtLink;
  end;
end;

procedure TMainForm.DropTargetTargetDrop(Sender: TObject;
  Source: TOleDragObject; Shift: TShiftState; X, Y: Integer;
  var DragType: TDragType);
var FL: TStringList;
    mdm: TStgMedium;
    pz : pchar;
    fmt : TFormatEtc;
    efe : iEnumFormatEtc;
    fmtCount: LongInt;
    Item : TElTreeItem;
    ItemPart : TSTItemPart;
    HitColumn : integer;
    Key : string;

begin
  Item := Tree.GetItemAt(X, Y, ItemPart, HitColumn);
  if Item <> nil
     then Key := Item.GetFullName(IniFile.Delimiter)
     else Key := IniFile.Delimiter;
  if ssCtrl in Shift then DragType := dtCopy else
  if Shift = [] then DragType := dtMove else DragType := dtNone;
  if ((DragType = dtCopy) or (DragType = dtMove)) and Source.HasDataFormat(ClipFormat) then
  begin
    fillchar(fmt,sizeof(fmt),0);
    Source.DataObject.EnumFormatEtc(datadir_get,efe);
    EFE.Reset;
    repeat
      fmtCount:=0;
      efe.Next(1,fmt,@fmtCount);
    until (fmt.cfFormat = ClipFormat) or (fmtCount=0);
    if fmt.cfFormat<>ClipFormat then exit;
    fmt.tymed := TYMED_HGLOBAL;
    fmt.lindex := -1;
    if Source.DataObject.GetData(fmt,mdm)<>S_OK then exit else
    try
      if (fmt.cfFormat=ClipFormat) and (mdm.tymed = TYMED_HGLOBAL) then
      begin
        // This is a dirty trick:
        // if the Key doesn't exist in INI file, then we are moving the data,
        // and are trying to move the data to Key's subitem, that was already deleted ;).
        // As this is not allowed, we give a message and move data to the root
        if not(IniFile.KeyExists(Key)
               or IniFile.ValueExists(Copy(Key, 1, LastPos(IniFile.Delimiter, Key) -1),
                                      Copy(Key, LastPos(IniFile.Delimiter, Key) + 1, Length(Key)))) then
        begin
          MessageDlg('Can''t move a key to itself or its subkey. Moving to "root" ... ', mtError, [mbOk], 0);
          Item := nil;
        end;
        pz := GlobalLock(mdm.HGlobal);
        if FStream1 <> nil
          then FStream1.SetSize(0)
          else FStream1 := TDirectMemoryStream.Create;
        FStream1.SetPointer(pz, $7FFFFFFF);
        DoPasteData(Item);
        FStream1.SetPointer(nil, 0);
        GlobalUnlock(mdm.HGlobal);
      end;
    finally
      if Assigned(mdm.unkForRelease) then Iunknown(mdm.unkForRelease)._Release;
    end;
  end;
  if Source.HasDataFormat(CF_HDROP) then
  begin
    if Source is TOleDragObject then
    begin
      FL := Source.FileList;
      if (FL.Count > 0) and FileExists(FL[0]) then
      begin
        if CloseCurrent then
        begin
          Options.Simple := (Uppercase(ExtractFileExt(FL[0])) <> '.EIF');
          if DoLoad(FL[0]) then
          begin
            FileName := FL[0];
            if Uppercase(ExtractFileExt(FileName)) = '.EIF' then MRU.Sections[0].Add(FileName, 0) else
            if Uppercase(ExtractFileExt(FileName)) = '.INF' then MRU.Sections[2].Add(FileName, 0) else
            if Uppercase(ExtractFileExt(FileName)) = '.INI' then MRU.Sections[1].Add(FileName, 0) else
            MRU.Sections[3].Add(FileName, 0);
          end;
        end;
      end; // if
    end;
  end;
end;

procedure TMainForm.SortItemClick(Sender: TObject);
begin
  Options.Sort := not Options.Sort; 
end;

procedure TMainForm.TreeKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
  if (Shift = [ssCtrl]) and ((Key = ord('A')) or (Key = ord('a'))) then Tree.SelectAll;
end;

procedure TMainForm.FormCaptionButtonClick(Sender: TObject;
  Button: TElCaptionButton);
begin
  ElFormPersist.Topmost := Button.Down;
end;

procedure TMainForm.ElTrayDblClick(Sender: TObject);
begin
  ShowWindow(Application.Handle, SW_SHOW);
  ElTray.Enabled := false;
  Application.Restore;
  Application.BringToFront;
end;

procedure TMainForm.AppMinimize(Sender : TObject);
begin
  ShowWindow(Application.Handle, SW_HIDE);
  ElTray.Enabled := true;
end;

procedure TMainForm.TreeOleDragStart(Sender: TObject; var dataObj: IDataObject;
  var dropSource: IDropSource; var dwOKEffects: TDragTypes);
var DragObj : TIniDragObject;
begin
  PrepareCBList;
  DragObj := TIniDragObject.Create;
  DragObj.QueryInterface(IDataObject, dataObj);
  DragObj.QueryInterface(IDropSource, dropSource);
  dwOKEffects := [dtCopy, dtMove];
end;

procedure TMainForm.TreeTryEdit(Sender: TObject; Item: TElTreeItem;
  SectionIndex: Integer; var CellType: TElFieldType; var CanEdit: Boolean);
begin
  if FAction = -1 then // we have to define, what we are going to edit
  begin
    if (SectionIndex = 0) then // editing name
    begin
      if Integer(Item.Data) mod 2 = 1 then
        FAction := 2  // editing a key
      else
        FAction := 3; // editing a value
    end
    else // editing value ...
    begin
      if Integer(Item.Data) mod 2 = 1 then
      begin
        FAction := 4;
        CellType := sftText;
      end
      else
        FAction := 5;
    end;
  end;
  FSaveCellType := CellType;
end;

procedure TMainForm.ModalEditExecute(Sender: TObject;
  var Accepted: Boolean);
begin
  ElMessageDlg('Unfortunately editing binary values is not implemented', mtInformation, [mbOk], 0);
end;

procedure TMainForm.TreeOleDragFinish(Sender: TObject; dwEffect: TDragType; Result: HResult);
begin
  Modified := true;
end;

procedure TMainForm.CheckBoxEditAfterOperation(
  Sender: TObject; var Accepted, DefaultConversion: Boolean);
var Key : String;
begin
  DefaultConversion := false;
  Accepted := true;

  CheckBoxEdit.Item.ColumnText[0] := BoolValues[CheckBoxEdit.Editor.Checked];
  if CheckBoxEdit.Item.Parent <> nil then
    Key := CheckBoxEdit.Item.Parent.GetFullName(IniFile.Delimiter)
  else
    Key := IniFile.Delimiter;
  IniFile.WriteBool(Key, CheckBoxEdit.Item.Text, CheckBoxEdit.Editor.Checked);
  Modified := true;
  FAction := -1;
end;

procedure TMainForm.ButtonEditValidateResult(Sender: TObject;
  var InputValid: Boolean);
var Text : string;
    Key  : string;
begin
  if (FAction <> 4) and (FAction <> 5) then
  begin
    Text := ButtonEdit.Editor.Text;

    if (Pos(IniFile.Delimiter, Text) >0) or
       (Pos(IniFile.Comment, Text) >0) or
       (Pos('=', Text) > 0) then
    begin
      MessageBox(0, PChar(Format('Invalid characters in key/value name ("=" or "%s" or "%s").',
                 [IniFile.Delimiter, IniFile.Comment])), nil, MB_OK);
      InputValid := false;
      exit;
    end;
    if Length(Text) = 0 then
    begin
      MessageBox(0, 'Empty names are not allowed.', nil, MB_OK);
      InputValid := false;
      exit;
    end;
    if ButtonEdit.Item.Parent = nil then
      Key := IniFile.Delimiter
    else
      Key := ButtonEdit.Item.Parent.GetFullName(IniFile.Delimiter) + IniFile.Delimiter;

    if IniFile.KeyExists(Key + Text) or IniFile.ValueExists(Key, Text) then
    begin
      MessageBox(0, 'Key/value with the name entered already exists.', nil, MB_OK);
      InputValid := false;
      exit;
    end;
  end;
end;

procedure TMainForm.SpinEditAfterOperation(Sender: TObject;
  var Accepted, DefaultConversion: Boolean);
var Key  : string;
begin
  if SpinEdit.Item.Parent = nil then
    Key := IniFile.Delimiter
  else
    Key := SpinEdit.Item.Parent.GetFullName(IniFile.Delimiter)+IniFile.Delimiter;

  Accepted := IniFile.WriteInteger(Key, SpinEdit.Item.Text, SpinEdit.Editor.Value);
  DefaultConversion := true;
  FAction := -1;
  Modified := true;
end;

procedure TMainForm.MemoEditAfterOperation(Sender: TObject;
  var Accepted, DefaultConversion: Boolean);
var Key  : string;
begin
  if MemoEdit.Item.Parent = nil then
    Key := IniFile.Delimiter
  else
    Key := MemoEdit.Item.Parent.GetFullName(IniFile.Delimiter)+IniFile.Delimiter;

  Accepted := IniFile.WriteMultiString(Key, MemoEdit.Item.Text, MemoEdit.Editor.Lines);
  DefaultConversion := true;
  FAction := -1;
  Modified := true;
end;

procedure TMainForm.ButtonEditAfterOperation(Sender: TObject;
  var Accepted, DefaultConversion: Boolean);
var Text : string;
    Key  : string;
    FOldKey: string;
begin
  if Accepted then
  begin
    Text := ButtonEdit.Editor.Text;
    if ButtonEdit.Item.Parent = nil then
      Key := IniFile.Delimiter
    else
      Key := ButtonEdit.Item.Parent.GetFullName(IniFile.Delimiter)+IniFile.Delimiter;

    if Accepted then
    begin
      case FAction of
        0: // create a key
          begin
            FOldKey := IniFile.CurrentKey;
            Accepted := IniFile.OpenKey(Key + Text, true);
            IniFile.OpenKey(FOldKey, false);
          end;
        1: // create a value
          Accepted := IniFile.CreateValue(Key, Text) <> nil;
        2: // rename a key
          Accepted := IniFile.RenameKey(Key + ButtonEdit.Item.Text, Text);
        3: // rename a value
          Accepted := IniFile.RenameValue(Key, ButtonEdit.Item.Text, Text);
        4: // edit value
          Accepted := IniFile.WriteString(Key, ButtonEdit.Item.Text, Text);
      end;
    end;
    if Accepted and (FAction = 1) then
      IniFile.SetValueType(Key, Text, TElValueType(integer(ButtonEdit.Item.Data) shr 1));

    SetItemStyles(ButtonEdit.Item);
  end;
  FAction := -1;
  Modified := true;
end;

procedure TMainForm.ButtonEditBeforeOperation(Sender: TObject;
  var DefaultConversion: Boolean);
begin
  ButtonEdit.Editor.ButtonVisible := false;
end;

end.

