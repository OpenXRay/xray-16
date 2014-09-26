{$INCLUDE DEFINE.INC}
unit LogoMain;

interface

uses Windows, Classes, Forms, Controls, Menus, CryptCon,
  Dialogs, StdCtrls, Buttons, ExtCtrls, ElMTree, EntryData, ElHeader,
  ElImgLst, ElStack, frmRecProp, About, ElMD5, IdeaUnit, ElTools, frmPassword,
  ShellApi, Graphics, ClipBrd, Registry, KeeperOpts, ElPromptDlg,
  ElStrUtils, ElTray, ElMRU, ElPopBtn, ElIni, ToolWin, frmFolderProp,
  ElCaption, ElPanel, ElToolBar, ElBtnCtl, ElFrmPers, ElBaseComp, FrmOpts,
  ElShutdownWatcher, ElACtrls, ElTree, ElAES, Messages, ImgList,
  ElXPThemedControl;

type
  TLogoAppForm = class(TForm)
    MainMenu: TMainMenu;
    FileMenu: TMenuItem;
    FileNewItem: TMenuItem;
    FileOpenItem: TMenuItem;
    FileSaveItem: TMenuItem;
    FileExitItem: TMenuItem;
    OpenDialog: TOpenDialog;
    SaveDialog: TSaveDialog;
    Help1: TMenuItem;
    AboutItem: TMenuItem;
    CloseBtn: TMenuItem;
    PopupMenu: TPopupMenu;
    NewFoldItem: TMenuItem;
    NewRecItem: TMenuItem;
    DeleteItem: TMenuItem;
    N1: TMenuItem;
    N3: TMenuItem;
    PropItem: TMenuItem;
    ElImgList: TElImageList;
    Record1: TMenuItem;
    Newfolder1: TMenuItem;
    Newrecord1: TMenuItem;
    N4: TMenuItem;
    Delete1Item: TMenuItem;
    N5: TMenuItem;
    Prop1Item: TMenuItem;
    ContentsItem: TMenuItem;
    N6: TMenuItem;
    N7: TMenuItem;
    RecentItem: TMenuItem;
    GoItem: TMenuItem;
    Go1Item: TMenuItem;
    CopyItem: TMenuItem;
    PrintSetDlg: TPrinterSetupDialog;
    PrintSetupItem: TMenuItem;
    PrintItem: TMenuItem;
    PrintDlg: TPrintDialog;
    Tray: TElTrayIcon;
    ElMRU: TElMRU;
    RecentPopup: TPopupMenu;
    ElIniFile: TElIniFile;
    ElFormPersist: TElFormPersist;
    PasswItem: TMenuItem;
    FormCaption: TElFormCaption;
    CopyPswItem: TMenuItem;
    SetPswItem: TMenuItem;
    Go2Item: TMenuItem;
    Go3Item: TMenuItem;
    N9: TMenuItem;
    miCopyUsername: TMenuItem;
    miCopyAccount: TMenuItem;
    SuggestItem: TMenuItem;
    Timer1: TTimer;
    N10: TMenuItem;
    ContactUs1: TMenuItem;
    TellafriendItem: TMenuItem;
    RegisterItem: TMenuItem;
    HomepageItem: TMenuItem;
    TrayMenu: TPopupMenu;
    miShowQuickAccess: TMenuItem;
    Exit1: TMenuItem;
    N11: TMenuItem;
    miCheckCompat: TMenuItem;
    N12: TMenuItem;
    miTools: TMenuItem;
    ToolBar: TElToolBar;
    ExitBtn: TElToolButton;
    NewBtn: TElToolButton;
    ElToolButton3: TElToolButton;
    OpenBtn: TElToolButton;
    SaveBtn: TElToolButton;
    PrintBtn: TElToolButton;
    ElToolButton2: TElToolButton;
    NewFolderBtn: TElToolButton;
    NewRecBtn: TElToolButton;
    ElToolButton6: TElToolButton;
    DelBtn: TElToolButton;
    PropBtn: TElToolButton;
    ElToolButton1: TElToolButton;
    GoBtn: TElToolButton;
    CopyBtn: TElToolButton;
    Go2Btn: TElToolButton;
    CopyUNameBtn: TElToolButton;
    CopyAcctBtn: TElToolButton;
    CopyPswBtn: TElToolButton;
    QuickAccessBtn: TElToolButton;
    ElToolButton5: TElToolButton;
    Tree: TElTree;
    ElImgList1: TElImageList;
    miSaveAttach: TMenuItem;
    AttachSaveDlg: TSaveDialog;
    miExport: TMenuItem;
    ExportDialog: TSaveDialog;
    miShowMainWin: TMenuItem;
    N8: TMenuItem;
    OptionsItem: TMenuItem;
    ColumnsItem: TMenuItem;
    procedure FormCreate(Sender: TObject);
    procedure FileExit(Sender: TObject);
    procedure FileNew(Sender: TObject);
    procedure FileOpen(Sender: TObject);
    procedure FileSave(Sender: TObject);
    procedure FileSaveAs(Sender: TObject);
    procedure About(Sender: TObject);
    procedure CloseBtnClick(Sender: TObject);
    procedure TreeMouseUp(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure NewFoldItemClick(Sender: TObject);
    procedure PropItemClick(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
    procedure DeleteItemClick(Sender: TObject);
    procedure NewRecItemClick(Sender: TObject);
    procedure TreeStartDrag(Sender: TObject;
      var DragObject: TDragObject);
    procedure TreeDragOver(Sender, Source: TObject; X, Y: Integer;
      State: TDragState; var Accept: Boolean);
    procedure TreeDragDrop(Sender, Source: TObject; X, Y: Integer);
    procedure Record1Click(Sender: TObject);
    procedure TreeItemFocused(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure ContentsItemClick(Sender: TObject);
    function AppEventsHelp(Command: Word; Data: Integer;
      var CallHelp: Boolean): Boolean;
    procedure GoItemClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure CopyBtnClick(Sender: TObject);
    procedure PrintSetupItemClick(Sender: TObject);
    procedure PrintItemClick(Sender: TObject);
    procedure TrayDblClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure PasswItemClick(Sender: TObject);
    procedure ElMRUClick(Sender: TObject; Entry: TElMRUEntry);
    procedure FormCaptionButtonClick(Sender: TObject;
      Button: TElCaptionButton);
    procedure CopyPswItemClick(Sender: TObject);
    procedure ElFormPersistRestore(Sender: TObject);
    procedure SetPswItemClick(Sender: TObject);
    procedure Go2BtnClick(Sender: TObject);
    procedure miCopyUsernameClick(Sender: TObject);
    procedure miCopyAccountClick(Sender: TObject);
    procedure SuggestItemClick(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure TellafriendItemClick(Sender: TObject);
    procedure RegisterItemClick(Sender: TObject);
    procedure HomepageItemClick(Sender: TObject);
    procedure TreeItemPicDraw(Sender: TObject; Item: TElTreeItem;
      var ImageIndex: Integer);
    procedure QuickAccessBtnClick(Sender: TObject);
    procedure TreeItemExpand(Sender: TObject; Item: TElTreeItem);
    procedure TreeItemCollapse(Sender: TObject; Item: TElTreeItem);
    procedure Exit1Click(Sender: TObject);
    procedure miShowQuickAccessClick(Sender: TObject);
    procedure miCheckCompatClick(Sender: TObject);
    procedure TreeHeaderColumnDraw(Sender: TCustomElHeader; Canvas : TCanvas; 
      Section: TElHeaderSection; R: TRect; Pressed: Boolean);
    procedure TreeItemDraw(Sender: TObject; Item: TElTreeItem;
      Surface: TCanvas; R: TRect; SectionIndex: Integer);
    procedure miSaveAttachClick(Sender: TObject);
    procedure miExportClick(Sender: TObject);
    procedure TreeSortEnd(Sender: TObject);
    procedure FormKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure OptionsItemClick(Sender: TObject);
    procedure TreeHeaderLookup(Sender: TObject; Section: TElHeaderSection;
      var Text: String);
    procedure ColumnsItemClick(Sender: TObject);
    procedure TreeScroll(Sender: TObject; ScrollBarKind: TScrollBarKind;
      ScrollCode: Integer);
  private
    FFileName: string;
    FOpened: Boolean;
    DragItem   : TElTreeItem;
    FModified  : boolean;
    AMinimized : boolean;
    hMapping   : THandle;
    JustStarted : boolean;
    DoExit     : boolean;
    FModified2: Boolean;
    function FileClose : boolean;
    function SaveFile(AFileName : string): Boolean;
    function LoadFile(AFileName, Passw : string; UseGivenPassword : boolean):
        Boolean;
    procedure OnItemDelete(Sender : TObject; Item : TElMTreeItem; Data : pointer);
    procedure CreateRecord(IsFolder : boolean);
    procedure OnItemSave(Sender : TObject; Item : TElMTreeItem; Stream : TStream);
    procedure OnItemLoad(Sender : TObject; Item : TElMTreeItem; Stream : TStream);
    procedure UpdateItem(Item : TElTreeItem);
    procedure SetModified(Value : boolean);
    procedure SetModified2(Value: Boolean);
    procedure SetFileName(const Value: string);
    procedure SetOpened(Value: Boolean);
    procedure AppEventsIdle(Sender : TObject; var Done: Boolean);
    procedure AppEventsMinimize(Sender: TObject);
    procedure AppEventsRestore(Sender: TObject);
    procedure RestoreFontSettings;
  protected
    procedure WMSysCommand(var Message: TMessage); message WM_SYSCOMMAND;
  public
    MVis,
    QVis         : boolean;
    FilePassword : string;

    function GetSelItem : TElTreeItem;
    function PropsEdit(Item : TElTreeItem) : Boolean;
    function  GetDataIndex(Data : Pointer) : integer;

    property Modified : boolean read FModified write SetModified;
    property SelItem  : TElTreeItem read GetSelItem;
    property FileName : string read FFileName write SetFileName;
    property Opened   : Boolean read FOpened write SetOpened;
    property Modified2: Boolean read FModified2 write SetModified2;
  end;

var
  LogoAppForm: TLogoAppForm;

const
      hsiAttachment = 9;

implementation

uses SysUtils, LogoStrs, Printers, ElOpts, frmQuickAccess;

{$R *.DFM}

function TLogoAppForm.GetSelItem : TElTreeItem;
begin
  result := Tree.ItemFocused;
end;

procedure TLogoAppForm.SetModified(Value : boolean);
begin
  if FModified <> Value then
  begin
    FModified := Value;
    SaveBtn.Enabled := Modified2 or Value;
    FileSaveItem.Enabled := Modified2 or Value;
  end;
end;

procedure TLogoAppForm.RestoreFontSettings;
var S : String;
    c : integer;
    size : integer;
    St  : TFontStyles;
    bl  : integer;
begin
  if ElIniFile.ReadString('\', 'FontName', '', S) then
  begin
    Tree.Font.Name := S;
    QuickAccessForm.Tree.Font.Name := S;
  end;
  if ElIniFile.ReadInteger('\', 'FontCharset', 0, c) then
  begin
    Tree.Font.Charset := c;
    QuickAccessForm.Tree.Font.Charset := c;
  end;
  if ElIniFile.ReadInteger('\', 'FontSize', 0, size) then
  begin
    Tree.Font.Size := size;
    QuickAccessForm.Tree.Font.Size := size;
  end;
  bl := sizeof(st);
  if ElIniFile.ReadBinary( '\', 'FontStyles', st, bl) then
  begin
    Tree.Font.Style := st;
    QuickAccessForm.Tree.Font.Style := st;
  end;
end;

procedure TLogoAppForm.FormCreate(Sender: TObject);
var S : String;
begin
  Options := TKeeperOpts.Create(Self);
  Options.Storage := ElIniFile;
  Options.StorageType := eosElIni;
  Options.Load;
  S := Tree.HeaderSections.SectionsOrder;
  if ElIniFile.ReadString('\LogoAppForm\Tree\ElHeader', 'Order', S, S) then
    Tree.HeaderSections.SectionsOrder := S;
  Tree.Restore;
  ElMRU.Restore;
  Tree.HeaderSections[9].ShowSortMark := false;

  if Options.ShowPassword then
    PasswItem.Checked := true;

  Application.OnHelp := AppEventsHelp;
  Application.OnRestore := Self.AppEventsRestore;
  Application.OnMinimize := Self.AppEventsMinimize;
  Application.OnIdle := AppEventsIdle;
  FMTree := TElMTree.Create;
  FMTree.OnItemDelete:=OnItemDelete;
  FMTree.OnItemSave:=OnItemSave;
  FMTree.OnItemLoad:=OnItemLoad;
  LoadStack:=TElStack.Create;
  Opened := false;
  AboutBox:=TAboutBox.Create(Self);
  if OpenDialog.InitialDir='' then
    OpenDialog.InitialDir := ExtractFileDir(ParamStr(0));
  if SaveDialog.InitialDir='' then
    SaveDialog.InitialDir := ExtractFileDir(ParamStr(0));
  begin
    NewFolderBtn.Visible := true;
    NewRecBtn.Visible := true;
    NewRecord1.Visible := true;
    NewFolder1.Visible := true;
    DelBtn.Visible := true;
    Delete1Item.Visible := true;
    NewFoldItem.Visible := true;
    NewRecItem.Visible := true;
    DeleteItem.Visible := true;
  end;
  hMapping := CreateFileMapping($FFFFFFFF, nil, PAGE_READWRITE, 0, MAX_PATH + 1, 'EldoS Keeper current file name');
  FileName := LoadStr(sUntitled);
  AssignedID := 0;
  JustStarted := true;
end;

procedure TLogoAppForm.FileNew(Sender: TObject);
begin
  if Opened then
     if not FileClose then exit;
  Tree.Items.Clear;
  FMTree.Clear;
  DeleteItem.Enabled := false;
  PropItem.Enabled   := false;
  Delete1Item.Enabled := false;
  Prop1Item.Enabled   := false;
  GoItem.Enabled := false;
  GoBtn.Enabled := false;
  Go1Item.Enabled := false;
  PropBtn.Enabled := false;
  DelBtn.Enabled :=false;
  CopyBtn.Enabled := false;
  CopyItem.Enabled := false;
  //SelItem := nil;
  Modified := false;
  Modified2 := false;
  Opened := true;
  FilePassword := '';
  AssignedID := 0;
  FileName := LoadStr(sUntitled);
  QuickAccessForm.UpdateTree(FMTree);
  Tree.BkColor := clWindow;
  Tree.ShowColumns := true;
end;

procedure TLogoAppForm.FileOpen(Sender: TObject);
begin
  if OpenDialog.Execute then
  begin
    if Opened then
       if not FileClose then
         exit;
    if LoadFile(OpenDialog.FileName, '', false) then
    begin
      FileName := OpenDialog.FileName;
      //SelItem := nil;
      Modified:= false;
      Opened := true;
    end;
  end;
end;

procedure TLogoAppForm.FileSave(Sender: TObject);
begin
  if FileName = LoadStr(sUntitled) then
    FileSaveAs(Sender)
  else
    if SaveFile(FileName) then
    begin
      Modified := false;
      Modified2 := false;
    end;
end;

procedure TLogoAppForm.FileSaveAs(Sender: TObject);
begin
  if SaveDialog.Execute then
  begin
    if FileExists(SaveDialog.FileName) then
      if ElMessageDlg(FmtLoadStr(sOverwrite, [SaveDialog.FileName]),
        mtConfirmation, mbYesNoCancel, 0) <> idYes then Exit;
    if SaveFile(SaveDialog.FileName) then
    begin
      FileName := SaveDialog.FileName;
      Options.LastFile := FileName;
      Modified := false;
      Modified2 := false;      
      ElMRU.Sections[0].Add(FileName, 0);
    end;
  end;
end;

function TLogoAppForm.FileClose;
begin
  result := true;
  if not Opened then exit;
  if Modified then
  begin
    case ElMessageDlg(LoadStr(sSaveOnCLose), mtWarning, [mbYes, mbNo, mbCancel], 0) of
      id_Yes: begin
                FileSave(Self);
                if Modified then result:=false;
              end;
      id_No:  begin
                Modified := false;
                Modified2 := false;
              end;
      id_Cancel: result:=false;
    end;
  end;
  if Result then
  begin
    Opened := false;
    Tree.Items.Clear;
    FMTree.Clear;
    FilePassword := '';
    FileName := LoadStr(sUntitled);
  end;
end;

procedure TLogoAppForm.FileExit(Sender: TObject);
begin
  DoExit := true;
  Close;
end;

procedure TLogoAppForm.About(Sender: TObject);
begin
  AboutBox.ShowModal;
end;

function TLogoAppForm.SaveFile(AFileName : string): Boolean;

  function SaveEK3(Password : string) : boolean;
  var AESKey,
      KeyHash   : array [1..17] of char;
      MemStream : TDirectMemoryStream;
      Stream    : TStream;
      MD5       : TCrMD5;
      S         : string;
  begin
    MD5:=TCrMD5.Create;
    try
      MD5.InputType    := SourceString;
      MD5.InputString  := Password;
      MD5.pOutputArray := @AESKey;
      MD5.MD5_Hash;
      AESKey[17]:=#0;
    finally
      MD5.Free;
    end;
    MD5:=TCrMD5.Create;
    try
      MD5.InputType    := SourceByteArray;
      MD5.pInputArray  := @AESKey;
      MD5.InputLength  := 16;
      MD5.pOutputArray := @KeyHash;
      MD5.MD5_Hash;
    finally
      MD5.Free;
    end;
    MemStream := TDirectMemoryStream.Create;
    try
      MemStream.WriteBuffer(KeyHash, Sizeof(KeyHash));

      FileVersion := FILE_VERSION;
      MemStream.WriteBuffer(FileVersion, sizeof(integer));
      MemStream.WriteBuffer(AssignedID, sizeof(AssignedID));
      
      FMTree.SaveToStream(MemStream);
      FileVersion := 0;
      MemStream.Seek(0, soFromBeginning);
      try
        Stream := TFileStream.Create(AFileName, fmCreate or fmShareDenyWrite);
      except
        Stream := TFileStream.Create(AFileName, fmOpenWrite or fmShareDenyWrite);
      end;
      try
        S := AESEncrypt(MemStream.Memory, MemStream.Size, @AESKey);
        Stream.WriteBuffer(PChar(S)[0], Length(S));
      finally
        Stream.Free;
      end;
    finally
      MemStream.Free;
    end;
    result := true;
  end;

  function SaveEK2(Password : string) : boolean;
  var key    : array [1..17] of char;
      MemStream : TDirectMemoryStream;
      Stream : TStream;
      MD5    : TCrMD5;
      IDEA   : TIDEA;
  begin

    MD5:=TCrMD5.Create;
    try
      MD5.InputType    := SourceString;
      MD5.InputString  := Password;
      MD5.pOutputArray := @key;
      MD5.MD5_Hash;
      key[17]:=#0;
    finally
      MD5.Free;
    end;
    MemStream := TDirectMemoryStream.Create;
    try
      try
        FMTree.SaveToStream(MemStream);
        MemStream.Seek(0, soFromBeginning);
        if FileExists(AFileName) then
          Stream := TFileStream.Create(AFileName, fmOpenWrite or fmShareDenyWrite)
        else
          Stream := TFileStream.Create(AFileName, fmCreate or fmShareDenyWrite);
        try
          IDEA := TIDEA.Create(self);
          try
            IDEA.InputType   := SourceStream;
            IDEA.CipherMode  := ECBMode;
            IDEA.IVector     := Password;
            IDEA.Key         := StrPas(@key);
            IDEA.InputStream := MemStream;
            IDEA.OutputStream:= Stream;
            IDEA.EncipherData(False);
          finally
            IDEA.free;
          end;
        finally
          Stream.Free;
        end;
        Modified := false;
        Modified2 := false;
      except
        on E : Exception do
        begin
          ElMessageDlg(FmtLoadStr(sFailSave, [AFileName]), mtError, [mbOk], 0);
          result := false;
          exit;
        end;
      end;
    finally
      MemStream.Free;
    end;
    result := true;
  end;

var Passw  : string;

begin
  PasswordDlg.Password.Text:='';
  PasswordDlg.ConfPassword.visible:=true;
  PasswordDlg.ConfLabel.Visible:=true;
  PasswordDlg.ConfPassword.Text:='';
  result:=false;
  if (not Options.KeepPassword) or (FilePassword = '') then
  begin
    repeat
      if PasswordDlg.ShowModal = mrCancel then
        exit;
      if PasswordDlg.Password.Text<>PasswordDlg.ConfPassword.Text then
        ElMessageDlg (LoadStr(sPswNotMatch), mtError, [mbOk], 0)
      else
      if PasswordDlg.Password.Text='' then
        ElMessageDlg (LoadStr(sPswEmpty), mtError, [mbOk], 0)
      else
        break;
    until false;
    Passw := PasswordDlg.Password.Text;
    PasswordDlg.Password.Text:='';
    PasswordDlg.ConfPassword.Text:='';
  end
  else
    Passw := FilePassword;

  if lowercase(ExtractFileExt(AFileName)) = '.ek3' then
    result := SaveEK3(Passw)
  else
    result := SaveEK2(Passw);
end;  { SaveFile }

function TLogoAppForm.LoadFile(AFileName, Passw : string; UseGivenPassword :
    boolean): Boolean;

  function LoadEK3 : boolean;
  var Stream : TFileStream;
      MD5    : TCrMD5;
      MemStream : TDirectMemoryStream;
      AESKey,
      KeyHash,
      SigHash: array [1..17] of char;
      S      : string;

  begin
    result := false;
    MD5:=TCrMD5.Create;
    try
      MD5.InputType   := SourceString;
      MD5.InputString := Passw;
      MD5.pOutputArray:= @AESKey;
      MD5.MD5_Hash;
    finally
      MD5.Free;
    end;
    MD5:=TCrMD5.Create;
    try
      AESKey[17]      :=#0;
      MD5.InputType   := SourceByteArray;
      MD5.pInputArray := @AESKey;
      MD5.InputLength := 16;
      MD5.pOutputArray:= @KeyHash;
      MD5.MD5_Hash;
    finally
      MD5.Free;
    end;

    MemStream := TDirectMemoryStream.Create;
    try
      Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
      MemStream.CopyFrom(Stream, Stream.Size);
      try
        if Passw <> '' then
        begin
          S := AESDecrypt(MemStream.Memory, MemStream.Size, @AESKey);
          MemStream.Size := Length(S);
          MoveMemory(MemStream.Memory, PChar(S), Length(S));
          S := '';
          MemStream.Position := 0;
          MemStream.ReadBuffer(SigHash, sizeof(SigHash));
        end
        else
        begin
          MemStream.ReadBuffer(SigHash, Sizeof(SigHash));
        end;
        if not CompareMem(@SigHash[1], @KeyHash[1], 16) then
        begin
          result := false;
          raise Exception.Create('Failed to open the file:'#13#10'The password is incorrect or the file is corrupt.');
        end;

        FMTree.Clear;

        MemStream.ReadBuffer(FileVersion, sizeof(Integer));
        if FileVersion >= 4 then
          MemStream.ReadBuffer(AssignedID, sizeof(AssignedID))
        else
          AssignedID := 0;
        try
          Tree.Items.BeginUpdate;
          try
            FMTree.LoadFromStream(MemStream);
            result := true;
          finally
            Tree.Items.EndUpdate;
            QuickAccessForm.UpdateTree(FMTree);
          end;
          FileVersion := 0;
        except
          on E: EReadError do
            Raise Exception.Create('Failed to open the file: file seems to be corrupt.');
        end;
      finally
        Stream.Free;
      end;
    finally
      MemStream.Free;
    end;
  end;


  function LoadEK2 : boolean;
  var Stream : TFileStream;
      MD5    : TCrMD5;
      MemStream : TDirectMemoryStream;
      arr : array [1..17] of char;
      IDEA: TIDEA;
  begin
    MD5:=TCrMD5.Create;
    MD5.InputType:=SourceString;
    MD5.InputString:=Passw;
    MD5.pOutputArray:=@arr;
    MD5.MD5_Hash;
    arr[17]:=#0;
    MD5.Free;
    MemStream := nil;
    Stream := nil;
    try
      MemStream := TDirectMemoryStream.Create;
      Stream :=TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
      if Passw<>'' then
      begin
        IDEA := nil;
        try
          IDEA := TIDEA.Create(self);
          IDEA.InputType := SourceStream;
          IDEA.CipherMode := ECBMode;
          IDEA.IVector:=Passw;

          IDEA.Key:=StrPas(@arr);
          IDEA.InputStream:=Stream;
          IDEA.OutputStream:=MemStream;
          IDEA.DecipherData(False);
          MemStream.Seek(0, soFromBeginning);

          FMTree.Clear;
          Tree.Items.Clear;

          Tree.Items.BeginUpdate;
          try
            FMTree.LoadFromStream(MemStream);
            result := true;
          finally
            Tree.Items.EndUpdate;
            QuickAccessForm.UpdateTree(FMTree);
          end;
        finally
          IDEA.free;
        end;
      end
      else
        try
          FMTree.LoadFromStream(Stream);
          result := true;
        finally
          QuickAccessForm.UpdateTree(FMTree);
        end;
      MemStream.Free;
      Stream.Free;
    except
      on E:Exception do
      begin
        ElMessageDlg(FmtLoadStr(sFailLoad, [AFileName]), mtError, [mbOk], 0);

        FMTree.Clear;
        Tree.Items.Clear;

        MemStream.Free;
        Stream.Free;

        result := false;
        exit;

      end;
    end;
  end;

var hMapping : THandle;
    View     : Pointer;
    b        : boolean;

begin
  result:=False;
  b := false;
  if not (FileExists(AFileName)) then exit;
  hMapping := CreateFileMapping($FFFFFFFF, nil, PAGE_READWRITE, 0, MAX_PATH + 1, 'EldoS Keeper current file name');
  if (hMapping <> 0) then
  begin
    View := MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, MAX_PATH + 1);
    if (View <> nil) then
    begin
      if AnsiCompareText(AFileName, StrPas(PChar(View))) = 0 then
        b := true;
      UnmapViewOfFile(View);
    end;
    CloseHandle(hMapping);
  end;
  if b then
    ElMessageDlg('This file is currently opened with PDA synchronization module.'#13#10'Please try to open this file a little later.', mtInformation, [mbOk], 0);

  if not UseGivenPassword then
  begin
    PasswordDlg.Password.Text:='';
    PasswordDlg.ConfPassword.visible:=false;
    PasswordDlg.ConfLabel.Visible:=false;
    PasswordDlg.ConfPassword.Text:='';
    if PasswordDlg.ShowModal = mrCancel then exit;
    Passw := PasswordDlg.Password.Text;
    PasswordDlg.Password.Text:='';
  end;
  result := false;
  Tree.Items.Clear;

  if lowercase(ExtractFileExt(AFileName)) = '.ek3' then
  try
    result := LoadEK3;
  except
    on E : Exception do
      ElMessageDlg(E.Message, mtError, [mbOk], 0);
  end
  else
    result := LoadEK2;

  if not result then
    exit;

  ElMRU.Sections[0].Add(AFileName, 0);
  if result then
  begin
    Options.LastFile := AFileName;
    FilePassword := Passw;
  end;
end;

procedure TLogoAppForm.CloseBtnClick(Sender: TObject);
begin
  if FileClose then
  begin
    GoItem.Enabled := false;
    GoBtn.Enabled := false;
    Go1Item.Enabled := false;
    DeleteItem.Enabled := false;
    PropItem.Enabled   := false;
    Delete1Item.Enabled := false;
    Prop1Item.Enabled   := false;
    PropBtn.Enabled := false;
    DelBtn.Enabled := false;
    CopyBtn.Enabled := false;
    CopyItem.Enabled := false;
    //SelItem := nil;
    Modified := false;
    Modified2 := false;
    QuickAccessForm.UpdateTree(FMTree);
  end;
end;

procedure TLogoAppForm.OnItemDelete(Sender : TObject; Item : TElMTreeItem; Data : pointer);
var E : PEntryRec;
begin
  E:=PEntryRec(Data);
  Dispose(E);
end;

procedure TLogoAppForm.TreeMouseUp(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbRight then
  begin
    //SelItem := ElTree.ItemFocused;
    DeleteItem.Enabled := SelItem<>nil;
    PropItem.Enabled   := SelItem<>nil;
    Delete1Item.Enabled := SelItem <>nil;
    Prop1Item.Enabled   := SelItem <>nil;
    PropBtn.Enabled := SelItem <>nil;
    DelBtn.Enabled := SelItem <>nil;
    GoItem.Enabled := SelItem <>nil;
    Go1Item.Enabled := SelItem <>nil;
    GoBtn.Enabled := SelItem <>nil;
    CopyBtn.Enabled := SelItem <>nil;
    CopyItem.Enabled := SelItem <>nil;
  end;
end;

procedure TLogoAppForm.NewFoldItemClick(Sender: TObject);
begin
   CreateRecord(true);
end;

procedure TLogoAppForm.CreateRecord(IsFolder : boolean);
var Item,
    Item1 : TElTreeItem;
    MItem : TElMTreeItem;
    Entry : PEntryRec;
begin
  Tree.Items.BeginUpdate;
  try
    New(Entry);
    FillMemory(Entry, sizeof(TEntryRec), 0);
    if (SelItem <> nil) then
    begin
      if (PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Group) then
        Item1 := SelItem
      else
        Item1 := SelItem.Parent;
      Item := Tree.Items.AddItem(Item1);
      Item.Expanded := false;
      if Item1 <> nil then
        Item1.Expanded := true;
      if Item1 = nil then
      begin
        MItem := FMTree.AddItem(nil, Entry);
        Entry.ParentID := DWORD(-1);
      end
      else
      begin
        Entry.ParentID := PEntryRec(Item1.Data).RecID;
        MItem := FMTree.AddItem(Item1.Data, Entry);
      end;
    end
    else
    begin
      Item := Tree.Items.AddItem(nil);
      Item.Expanded := false;
      MItem := FMTree.AddItem(nil, Entry);
      Entry.ParentID := DWORD(-1);
    end;
    Item.Data := MItem;
    Entry^.Group   := IsFolder;
    Entry.Added    := Now;
    Entry.Modified := Now;
    Entry.Expires  := Now + 30;
    Entry.WarnDays := 1;
    Entry.RecID    := UniqueID;
    Tree.ItemFocused := Item;
    if not PropsEdit(SelItem) then
    begin
      FMTree.DeleteItem(MItem);
      Tree.Items.DeleteItem(SelItem);
    end
    else
    begin
      Modified := true;
      Opened := true;
      UpdateItem(Item);
      QuickAccessForm.UpdateTree(FMTree);
      TreeItemFocused(self);
    end;
  finally
    Tree.Items.EndUpdate;
  end;
end;

procedure TLogoAppForm.PropItemClick(Sender: TObject);
begin
  if PropsEdit(SelItem) then
  begin
    QuickAccessForm.UpdateTree(FMTree);
    TreeItemFocused(self);
  end;
end;

procedure TLogoAppForm.OnItemSave(Sender : TObject; Item : TElMTreeItem; Stream : TStream);  { protected }
begin
  DoItemSave(Item, Stream);
end;  { OnItemSave }

procedure TLogoAppForm.OnItemLoad(Sender : TObject; Item : TElMTreeItem; Stream : TStream);  { protected }
var
    VItem, Parent : TElTreeItem;
    Entry : PEntryRec;

begin
  try
    DoItemLoad(Item, Stream);
    Entry := PEntryRec(Item.Data);
    Parent := nil;
    if not LoadStack.Empty then
    begin
      Parent := TElTreeItem(LoadStack.Pop);
      if Parent.Data<>Item^.Parent then
      begin
        while (not LoadStack.Empty) do
        begin
          Parent := TElTreeItem(LoadStack.Pop);
          if Parent.Data=Item^.Parent then
          begin
            LoadStack.Push(Parent);
            break;
          end;
        end;
        if LoadStack.Empty then Parent := nil;
      end
      else
        LoadStack.Push(Parent);
    end;
    if Parent<>nil then
    begin
      VItem := Tree.Items.AddItem(Parent);
      VItem.Data := Item;
    end else
    begin
      VItem  := Tree.Items.AddItem(nil);
      VItem.Data := Item;
    end;
    Item^.Data := Entry;
    LoadStack.Push(VItem);

    UpdateItem(VItem);

    VItem.Expanded := Entry.Expanded;
  except
    on E : Exception do
    begin
      Item.Data := nil;
      raise;
    end;
  end;
end;  { OnItemLoad }

procedure TLogoAppForm.FormCloseQuery(Sender: TObject;
  var CanClose: Boolean);
begin
  if Options.MinimizeOnClose and (not DoExit) then
    CanClose := true
  else
    CanClose := FileClose;
end;

procedure TLogoAppForm.DeleteItemClick(Sender: TObject);
begin
  if (SelItem = nil) or (SelItem.Data = nil) then exit;
  if ElMessageDlg(LoadStr(sConfDelete), mtWarning, [mbYes, mbNo], 0)=mrNo then exit;
  FMTree.DeleteItem(TElMTreeItem(SelItem.Data));
  Tree.Items.DeleteItem(SelItem);
  Modified := true;
  QuickAccessForm.UpdateTree(FMTree);
  //SelItem := nil;
end;

procedure TLogoAppForm.NewRecItemClick(Sender: TObject);
begin
  CreateRecord(false);
end;

procedure TLogoAppForm.UpdateItem(Item : TElTreeItem);
var Entry : PEntryRec;
    s     : string;
begin
  if Item.Data=nil then exit;
  Entry := PEntryRec(TElMTreeItem(Item.Data)^.Data);

  Tree.IsUpdating := true;
  try
    Item.Text:=Entry^.Site;
    Item.ColumnText.Clear;
    Item.ColumnText.Add(Entry^.Location);
    if not Entry^.Group then
    begin
      Item.ColumnText.Add(Entry^.UName);
      Item.ColumnText.Add(Entry^.Acct);
      Item.ColumnText.Add(Entry^.Pswd);
      Item.ColumnText.Add(Entry^.Info);
      try
        S := DateToStr(Entry^.Added);
      except
        S := DateToStr(Now);
      end;
      Item.ColumnText.Add(S);

      try
        S := DateToStr(Entry^.Modified);
      except
        S := DateToStr(Now);
      end;
      Item.ColumnText.Add(S);

      try
        S := DateToStr(Entry^.Expires);
      except
        S := DateToStr(Now + 1);
      end;
      Item.ColumnText.Add(S);
    end;

    Item.ImageIndex := -1;
    Item.StateImageIndex := -1;

    {
    if Entry^.Group then
    begin
      Item.ImageIndex:=0;
      Item.StateImageIndex := 1;
    end
    else
      Item.ImageIndex := 2;
    }
  finally
    Tree.IsUpdating:=false;
  end;
end;

function TLogoAppForm.PropsEdit(Item : TElTreeItem) : Boolean;  { public }
begin
  result :=false;
  if (SelItem = nil) or (SelItem.Data = nil) then exit;
  if PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Group then
  begin
    with TFolderPropsForm.Create(Self) do
    begin
      SiteNameEdit.Text := PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Site;
      if ShowModal = mrOk then
      begin
        Modified := true;
        SelItem.Text := SiteNameEdit.Text;
        PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Site := SiteNameEdit.Text;
        result := true;
      end;
      Free;
    end;
  end else
  begin
    with TRecPropsForm.Create(Self) do
    begin
      try
        Entry := PEntryRec(TElMTreeItem(Self.SelItem.Data)^.Data);
        SetData;
        if ShowModal = mrOk then
        begin
          GetData;
          Self.UpdateItem(SelItem);
          Self.Modified := true;
          result := true;
        end;
      finally
        Free;
      end;
    end;
  end;
end;  { PropsEdit }

type TElDragObject = class (TDragControlObject)
        function GetDragCursor(Accepted: Boolean; X, Y: Integer): TCursor; override;
     end;

function TElDragObject.GetDragCursor(Accepted: Boolean; X, Y: Integer): TCursor;
begin
  if Control is TElTree then
  begin
    if ((Control as TElTree).GetItemAtY(Y)<>nil) or (Accepted) then
       Result := (Control as TElTree).DragCursor else
       Result := crNoDrop;
  end else result:=inherited GetDragCursor(Accepted,X,Y);
end;

procedure TLogoAppForm.TreeStartDrag(Sender: TObject;
  var DragObject: TDragObject);
begin
  DragItem   := Tree.ItemFocused;
  DragObject := TElDragObject.Create(Tree);
end;

procedure TLogoAppForm.TreeDragOver(Sender, Source: TObject; X,
  Y: Integer; State: TDragState; var Accept: Boolean);
var TSI:TElTreeItem;
begin
  Accept:=false;
  if Source.ClassType <> TElDragObject then exit;
  TSI := Tree.GetItemAtY(Y);
  if TSI = nil then
  begin
    Accept:=true;
    exit;
  end;
  if ((not TSI.IsUnder(DragItem)) and (PEntryRec(TElMTreeItem(TSI.Data)^.Data)^.Group)) then
    Accept:=true;
end;

procedure TLogoAppForm.TreeDragDrop(Sender, Source: TObject; X,
  Y: Integer);
var TSI :TElTreeItem;
begin
  TSI := Tree.GetItemAtY(Y);
  if ((TSI<>nil) and (not TSI.IsUnder(DragItem)) and
     (PEntryRec(TElMTreeItem(TSI.Data)^.Data)^.Group)) or
     (TSI = nil) then
  begin
    DragItem.MoveTo(TSI);     
    if TSI <> nil then
      FMTree.MoveTo(TElMTreeItem(DragItem.Data), TElMTreeItem(TSI.Data))
    else
      FMTree.MoveTo(TElMTreeItem(DragItem.Data), nil);
    Modified := true;
    QuickAccessForm.UpdateTree(FMTree);
  end;
  DragItem := nil;
end;

procedure TLogoAppForm.Record1Click(Sender: TObject);
begin
  //SelItem := ElTree.ItemFocused;
  DeleteItem.Enabled := SelItem <> nil;
  PropItem.Enabled   := SelItem <> nil;
  Delete1Item.Enabled := SelItem<> nil;
  Prop1Item.Enabled   := SelItem<> nil;
  PropBtn.Enabled := SelItem <> nil;
  DelBtn.Enabled := SelItem  <> nil;
  GoItem.Enabled := SelItem  <> nil;
  Go1Item.Enabled := SelItem <> nil;
  GoBtn.Enabled := SelItem   <> nil;
  CopyBtn.Enabled := SelItem <> nil;
  CopyItem.Enabled := SelItem<> nil;
end;

procedure TLogoAppForm.TreeItemFocused(Sender: TObject);
var IsRecord : boolean;
begin
  if (csDestroying in ComponentState) then exit;

  DeleteItem.Enabled     := SelItem<>nil;
  PropItem.Enabled       := SelItem<>nil;
  Delete1Item.Enabled    := SelItem <>nil;
  Prop1Item.Enabled      := SelItem <>nil;
  DelBtn.Enabled         := SelItem <>nil;
  PropBtn.Enabled        := SelItem <>nil;
  CopyBtn.Enabled        := SelItem <>nil;
  CopyItem.Enabled       := SelItem <>nil;
  IsRecord := (SelItem <>nil) and
                          (TElMTreeItem(SelItem.Data)^.Data <> nil) and
                          (not PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Group);

  GoBtn.Enabled          := IsRecord and (Length(PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Location) > 0);
  GoItem.Enabled         := GoBtn.Enabled;
  Go1Item.Enabled        := GoBtn.Enabled;
  Go2Btn.Enabled         := IsRecord and (Length(PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Location2) > 0);
  Go2Item.Enabled        := Go2Btn.Enabled;

  miCopyUsername.Enabled := IsRecord and (Length(PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.UName) > 0);
  CopyUNameBtn.Enabled := miCopyUsername.Enabled;

  miCopyAccount.Enabled  := IsRecord and (Length(PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Acct) > 0);
  CopyAcctBtn.Enabled    := miCopyAccount.Enabled;

  CopyPswItem.Enabled    := IsRecord and (Length(PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Pswd) > 0);
  CopyPswBtn.Enabled     := CopyPswItem.Enabled;

  miSaveAttach.Enabled   := IsRecord and
                            (PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.BinDataSize > 0);
end;

procedure TLogoAppForm.FormClose(Sender: TObject;
  var Action: TCloseAction);
var Reg : TRegistry;
begin
  if DoExit or (not Options.MinimizeOnClose) then
  begin
    Action := caFree;
    Tree.Items.Clear;
    FMTree.Clear;
    Reg := nil;
    try
      try
        Reg := TRegistry.Create;
        Reg.OpenKey('Software\EldoS\Keeper', true);
        Reg.WriteString('OpenDir', OpenDialog.InitialDir);
        Reg.WriteString('SaveDir', SaveDialog.InitialDir);
        Reg.CloseKey;
      finally
        Reg.Free;
      end;
    except
      on E : Exception do ;
    end;
    Application.Terminate;
  end
  else
  begin
    Application.Minimize;
  end;
end;

procedure TLogoAppForm.ContentsItemClick(Sender: TObject);
begin
  Application.HelpCommand(HELP_FINDER, 0);
end;           

function TLogoAppForm.AppEventsHelp(Command: Word; Data: Integer;
  var CallHelp: Boolean): Boolean;
begin
  if (Command = HELP_CONTEXT) and (Data = 0) then
  begin
    CallHelp := false;
    Application.HelpCommand(HELP_FINDER, 0);
  end;
  result := true;
end;

procedure TLogoAppForm.GoItemClick(Sender: TObject);
begin
 if (SelItem = nil) or (SelItem.Data = nil) then exit;
 if PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Group then exit;
 ShellExecute(0,'open',PChar(PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Location),nil, nil, SW_SHOWNORMAL);
end;

procedure TLogoAppForm.FormDestroy(Sender: TObject);
var i : integer;
    S : String;
begin
  if (hMapping <> 0) then
    CloseHandle(hMapping);
  if Options.SaveKeys then
  begin
    for i := 0 to ElMRU.Sections.Count -1 do
      ElMRU.Sections[i].Clear;

    Options.LastFile := '';
  end;
  ElMRU.Save;
  Tree.Save;
  S := Tree.HeaderSections.SectionsOrder;
  ElIniFile.WriteString('\LogoAppForm\Tree\ElHeader', 'Order', S);
  Options.Save;
  ElIniFile.Save;
  FMTree.Free;
  LogoAppForm := nil;
end;

function TLogoAppForm.GetDataIndex(Data : Pointer) : integer;
var i : integer;
begin
  result := -1;
  for i := 0 to Tree.Items.Count - 1 do
  begin
    if TElMTreeItem(Tree.Items[i].Data).Data = Data then
    begin
      result := i;
      exit;
    end;
  end;
end;

procedure TLogoAppForm.CopyBtnClick(Sender: TObject);
var C : TClipboard;
    Entry : PEntryRec;
    S : TStringList;
begin
  if Tree.ItemFocused = nil then exit;
  Entry := PEntryRec(TElMTreeItem(Tree.ItemFocused.Data)^.Data);
  C := TClipboard.Create;
  S := TStringList.Create;
  if Entry.Group
     then S.Add('Group name: '+ Entry.Site)
     else S.Add('Site/Program name: '+ Entry.Site);
  if not Entry.Group then
  begin
    if Entry.Location <> '' then S.Add('Location/address: ' + Entry.Location);
    if Entry.UName <> '' then S.Add('User name: ' + Entry.UName);
    if Entry.Acct <> '' then S.Add('Account #: '+ Entry.Acct);
    if Entry.Pswd <> '' then S.Add('Password: '+ Entry.Pswd);
    if Entry.Info <> '' then S.Add('Additional info: '+ Entry.Info);
  end;
  C.AsText := S.Text;
  S.Free;
  C.Free;
end;

procedure TLogoAppForm.PrintSetupItemClick(Sender: TObject);
begin
  try
    PrintSetDlg.Execute;
  except
    ElMessageDlg('There was an error while setting Printer properties', mtError, [mbOk], 0);
  end;
end;

procedure TLogoAppForm.PrintItemClick(Sender: TObject);
var
  PrintText: TextFile;  { declare a text-file variable }
  i: integer;
  L: TStringList;

  procedure IntPrepare(Item:TElTreeItem; Index: integer; var ContinueIterate:boolean;
                            IterateData:pointer; Tree:TCustomElTree);
  var Entry : PEntryRec;
      L, L1 : TStringList;
      gap : string;
      i : integer;
  begin
    Entry := PEntryRec(TElMTreeItem(Item.Data)^.Data);
    L := TStringList(IterateData);
    SetLength(gap, Item.Level*10);
    FillChar(gap[1], Length(gap), 32);
    if Entry.Group
     then L.Add(gap + 'Group name: '+ Entry.Site)
     else L.Add(gap + 'Site/Program name: '+ Entry.Site);
    if not Entry.Group then
    begin
      if Entry.Location <> '' then L.Add(gap + 'Location/address: ' + Entry.Location);
      if Entry.UName <> '' then L.Add(gap + 'User name: ' + Entry.UName);
      if Entry.Acct <> '' then L.Add(gap + 'Account #: '+ Entry.Acct);
      if Entry.Pswd <> '' then L.Add(gap + 'Password: '+ Entry.Pswd);
      if pos(#13, Entry.Info)>0 then
      begin
        L1 := TStringList.Create;
        try
          L1.Text:=Entry.Info;
          L.Add(gap + 'Additional info: '+ L1[0]);
          for i := 1 to L1.Count -1 do
          begin
            L.Add(Gap + '                 '+ L1[i]);
          end;
        finally
          L1.Free;
        end;
      end else
      begin
        if Entry.Info <> '' then
          L.Add(gap + 'Additional info: '+ Entry.Info);
      end;
    end;
    L.Add('');
  end;

begin
  L := nil;
  try
    L := TStringList.Create;
    L.Add('');
    L.Add('');
    L.Add('');
    L.Add('Password list from ' + FileName);
    L.Add('');
    L.Add('');
    Screen.Cursor := crHourGlass;
    try
      AssignPrn(PrintText);        { associate text file to printer device }
      Rewrite(PrintText);  { create and open output file }
      Tree.Items.Iterate(false, true, @IntPrepare, L);
      for i := 0 to L.Count -1 do
        Writeln(PrintText, StrToOEM(L[i])); { write each line to printer }
    finally
      Screen.Cursor := crDefault;
    end;  // try/finally
  finally
    L.Free;
    CloseFile(PrintText);
  end;
end;

procedure TLogoAppForm.TrayDblClick(Sender: TObject);
begin
  if AMinimized then
  begin
    Tray.Enabled := false;
    Application.Restore
  end
  else
  begin
    Tray.Enabled := false;
    ShowWindow(Handle, SW_SHOW);
    Windows.SetFocus(Handle);
    BringToFront;
    Application.BringToFront;
  end;
end;

procedure TLogoAppForm.AppEventsMinimize(Sender: TObject);
begin
  MVis := IsWindowVisible(Handle) and (WindowState <> wsMinimized);
  if MVis then
  begin
    ShowWindow(Handle,SW_HIDE);
  end;
  QVis := QuickAccessForm.Visible;
  if QVis then
  begin
    QuickAccessForm.Visible := false;
    //ShowWindow(QuickAccessForm.Handle, SW_HIDE);
  end;
  if Options.ToTray then
  begin
    Tray.Enabled := true;
    ShowWindow(Application.Handle,SW_HIDE);
  end;
  AMinimized := true;
end;

procedure TLogoAppForm.AppEventsRestore(Sender: TObject);
begin
  if Options.ToTray then
  begin
    if MVis then
      Tray.Enabled := false;
    ShowWindow(Application.Handle, SW_RESTORE);
  end;
  if MVis then
    ShowWindow(Handle, SW_SHOW);
  if QVis then
  begin
    ShowWindow(QuickAccessForm.Handle,SW_SHOW);
    QuickAccessForm.Visible := true;
  end;
  if QVis then
    QuickAccessForm.BringToFront;
  if MVis then
    BringToFront;
  Application.BringToFront;
  AMinimized := false;
end;

procedure TLogoAppForm.FormShow(Sender: TObject);
var bSucc : Boolean;
begin
  RestoreFontSettings;
  if Options.ShowPassword then
  begin
    Options.ShowPassword := false;
    Options.ShowPassword := true;
  end;
  if Options.ReopenFile and (Options.LastFile <> '') and (not Options.SaveKeys) then
  begin
    if LoadFile(Options.LastFile, '', false) then
    begin
      FileName := Options.LastFile;
      Opened := true;
      Modified := false;
      Modified2 := false;
    end;
  end;
  if (ParamCount > 0) then
  begin
    if ParamCount > 1 then
      bSucc := LoadFile(ParamStr(1), ParamStr(2), true)
    else
      bSucc := LoadFile(ParamStr(1), '', False);
    if bSucc then
    begin
      FileName := ParamStr(1);
      Modified := false;
      Modified2 := false;
      Opened := true;
    end;
  end;
end;

procedure TLogoAppForm.PasswItemClick(Sender: TObject);
begin
  Options.ShowPassword := not Options.ShowPassword;
  PasswItem.Checked := not PasswItem.Checked;
end;

procedure TLogoAppForm.ElMRUClick(Sender: TObject; Entry: TElMRUEntry);
begin
  if Opened then
   if not FileClose then
     exit;
  if LoadFile(Entry.Name, '', false) then
  begin
    FileName := Entry.Name;
    Modified := false;
    Modified2 := false;
    Opened := true;
  end;
end;

procedure TLogoAppForm.FormCaptionButtonClick(Sender: TObject;
  Button: TElCaptionButton);
begin
  ElFormPersist.TopMost := Button.Down;
end;

procedure TLogoAppForm.CopyPswItemClick(Sender: TObject);
var C : TClipboard;
    Entry : PEntryRec;
begin
  if Tree.ItemFocused = nil then exit;
  Entry := PEntryRec(TElMTreeItem(Tree.ItemFocused.Data)^.Data);
  if Entry.Group then exit;
  C := TClipboard.Create;
  C.Open;
  C.AsText := Entry.Pswd;
  C.Close;
  C.Free;
end;

procedure TLogoAppForm.ElFormPersistRestore(Sender: TObject);
begin
  FormCaption.Buttons[0].Down := ElFormPersist.TopMost;
end;

procedure TLogoAppForm.SetPswItemClick(Sender: TObject);
var Passw : string;
    b     : boolean;
begin
  PasswordDlg.Password.Text:='';
  PasswordDlg.ConfPassword.visible:=true;
  PasswordDlg.ConfLabel.Visible:=true;
  PasswordDlg.ConfPassword.Text:='';
  repeat
    if PasswordDlg.ShowModal = mrCancel then exit;
    if PasswordDlg.Password.Text<>PasswordDlg.ConfPassword.Text then
      ElMessageDlg (LoadStr(sPswNotMatch), mtError, [mbOk], 0)
    else
    if PasswordDlg.Password.Text='' then
       ElMessageDlg (LoadStr(sPswEmpty), mtError, [mbOk], 0)
    else break;
  until false;
  Passw := PasswordDlg.Password.Text;
  PasswordDlg.Password.Text := '';
  PasswordDlg.ConfPassword.Text := '';
  FilePassword := Passw;
  b := Options.KeepPassword;
  Options.KeepPassword := true;
  if FileName <> LoadStr(sUntitled) then
    SaveFile(FileName);
  Options.KeepPassword := b;
end;

procedure TLogoAppForm.Go2BtnClick(Sender: TObject);
begin
 if (SelItem = nil) or (SelItem.Data = nil) then exit;
 if PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Group then exit;
 ShellExecute(0,'open',PChar(PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.Location2),nil, nil, SW_SHOWNORMAL);
end;

procedure TLogoAppForm.miCopyUsernameClick(Sender: TObject);
var C : TClipboard;
    Entry : PEntryRec;
begin
  if Tree.ItemFocused = nil then exit;
  Entry := PEntryRec(TElMTreeItem(Tree.ItemFocused.Data)^.Data);
  if Entry.Group then exit;
  C := TClipboard.Create;
  C.Open;
  C.AsText := Entry.UName;
  C.Close;
  C.Free;
end;

procedure TLogoAppForm.miCopyAccountClick(Sender: TObject);
var C : TClipboard;
    Entry : PEntryRec;
begin
  if Tree.ItemFocused = nil then exit;
  Entry := PEntryRec(TElMTreeItem(Tree.ItemFocused.Data)^.Data);
  if Entry.Group then exit;
  C := TClipboard.Create;
  C.Open;
  C.AsText := Entry.Acct;
  C.Close;
  C.Free;
end;

procedure TLogoAppForm.SuggestItemClick(Sender: TObject);
var SHI : TShellExecuteInfo;
    saveCursor : TCursor;
begin
  saveCursor := Screen.Cursor;
  Screen.Cursor := crHourGlass;
  try
    FillMemory(@SHI, sizeof(shi), 0);
    SHI.cbSize := sizeof(SHI);
    SHI.fMask := SEE_MASK_FLAG_DDEWAIT or SEE_MASK_NOCLOSEPROCESS;
    SHI.Wnd := Application.Handle;
    SHI.lpVerb := 'open';
    SHI.lpFile := PCHAR('mailto:info@eldos.org?subject=EldoS Keeper suggestion');
    SHI.lpParameters := nil;
    SHI.lpDirectory := nil;
    ShellExecuteEx(@SHI);
    CloseHandle(SHI.hProcess);
  finally
    Screen.Cursor := saveCursor;
  end;  { try/finally }
end;

procedure TLogoAppForm.Timer1Timer(Sender: TObject);

  procedure IterateProc(Item : TElMTreeItem; Index : integer; var ContinueIterate : boolean;
    IterateData : pointer);
  var Rec : PEntryRec;
  begin
    Rec := Item.Data;
    if Rec <> nil then
    begin
      if not (Rec.ExpWarned) and Rec.DoExpires and ((Rec.Expires - Rec.WarnDays) < Now) then
      begin
        Rec.ExpWarned := true;
        ElMessageDlg(Format('%s account information expires in %d days', [Rec.Site, Trunc(Rec.Expires) - Trunc(Now)]), mtWarning, [mbOk], 0);
      end;
    end;
  end;

begin
  FMTree.Iterate(@IterateProc, nil);
end;

procedure TLogoAppForm.TellafriendItemClick(Sender: TObject);
var SHI : TShellExecuteInfo;
    saveCursor : TCursor;
begin
  saveCursor := Screen.Cursor;
  Screen.Cursor := crHourGlass;
  try
    FillMemory(@SHI, sizeof(shi), 0);
    SHI.cbSize := sizeof(SHI);
    SHI.fMask := SEE_MASK_FLAG_DDEWAIT or SEE_MASK_NOCLOSEPROCESS;
    SHI.Wnd := Application.Handle;
    SHI.lpVerb := 'open';
    SHI.lpFile := PCHAR('mailto:?subject=Take%20a%20look%20at%20EldoS%20Keeper%20at%20http://www.eldos.org/elkeeper/elkeeper.html');
    SHI.lpParameters := nil;
    SHI.lpDirectory := nil;
    ShellExecuteEx(@SHI);
    CloseHandle(SHI.hProcess);
  finally
    Screen.Cursor := saveCursor;
  end;  { try/finally }
end;

procedure TLogoAppForm.RegisterItemClick(Sender: TObject);
var SHI : TShellExecuteInfo;
    saveCursor : TCursor;
begin
  saveCursor := Screen.Cursor;
  Screen.Cursor := crHourGlass;
  try
    FillMemory(@SHI, sizeof(shi), 0);
    SHI.cbSize := sizeof(SHI);
    SHI.fMask := SEE_MASK_FLAG_DDEWAIT or SEE_MASK_NOCLOSEPROCESS;
    SHI.Wnd := Application.Handle;
    SHI.lpVerb := 'open';
    SHI.lpFile := PCHAR('http://www.shareit.com/programs/101908.htm');
    SHI.lpParameters := nil;
    SHI.lpDirectory := nil;
    ShellExecuteEx(@SHI);
    CloseHandle(SHI.hProcess);
  finally
    Screen.Cursor := saveCursor;
  end;  { try/finally }
end;

procedure TLogoAppForm.HomepageItemClick(Sender: TObject);
var SHI : TShellExecuteInfo;
    saveCursor : TCursor;
begin
  saveCursor := Screen.Cursor;
  Screen.Cursor := crHourGlass;
  try
    FillMemory(@SHI, sizeof(shi), 0);
    SHI.cbSize := sizeof(SHI);
    SHI.fMask := SEE_MASK_FLAG_DDEWAIT or SEE_MASK_NOCLOSEPROCESS;
    SHI.Wnd := Application.Handle;
    SHI.lpVerb := 'open';
    SHI.lpFile := PCHAR('http://www.eldos.org/elkeeper/elkeeper.html');
    SHI.lpParameters := nil;
    SHI.lpDirectory := nil;
    ShellExecuteEx(@SHI);
    CloseHandle(SHI.hProcess);
  finally
    Screen.Cursor := saveCursor;
  end;  { try/finally }
end;

procedure TLogoAppForm.TreeItemPicDraw(Sender: TObject;
  Item: TElTreeItem; var ImageIndex: Integer);
var Entry : PEntryRec;
begin
  Entry := PEntryRec(TelMTreeItem(Item.Data).Data);
  if Entry = nil then exit;
  if Entry^.Group then
  begin
    if Item.Expanded then
      ImageIndex := 1
    else                  
      ImageIndex := 0;
  end
  else
    ImageIndex := 2;
end;

procedure TLogoAppForm.QuickAccessBtnClick(Sender: TObject);
begin
  if not QuickAccessForm.Visible then
    QuickAccessForm.Show;
  QuickAccessForm.BringToFront;
end;

procedure TLogoAppForm.WMSysCommand(var Message: TMessage);
begin
  if Message.wParam = SC_MINIMIZE then
  begin
    if QuickAccessForm.Visible then
    begin
      Tray.Enabled := true;
      ShowWindow(Handle, SW_HIDE);
      Application.BringToFront;
      QuickAccessForm.BringToFront;
    end
    else
      Application.Minimize;
  end else
  {if wParam = SC_RESTORE then
  begin
    Tray.Enabled := false;
    ShowWindow(Handle,SW_SHOW);
  end;}
  inherited;
end;

procedure TLogoAppForm.TreeItemExpand(Sender: TObject; Item: TElTreeItem);
var Entry : PEntryRec;
begin
  Entry := PEntryRec(TelMTreeItem(Item.Data).Data);
  Entry.Expanded := true;
  if Options.CountFolderChanges then
    Modified := true
  else
    Modified2 := true;
end;

procedure TLogoAppForm.TreeItemCollapse(Sender: TObject;
  Item: TElTreeItem);
var Entry : PEntryRec;
begin
  Entry := PEntryRec(TelMTreeItem(Item.Data).Data);
  Entry.Expanded := false;
  if Options.CountFolderChanges then
    Modified := true
  else
    Modified2 := true;
end;

procedure TLogoAppForm.Exit1Click(Sender: TObject);
begin
  DoExit := true;
  Close;
end;

procedure TLogoAppForm.miShowQuickAccessClick(Sender: TObject);
begin
  if AMinimized then
  begin
    MVis := false;
    if not QuickAccessForm.Visible then
      QuickAccessForm.Show
    else
      QVis := true;
    Application.Restore;
    ShowWindow(Handle, SW_HIDE);
  end
  else
  begin
    QuickAccessForm.Show;
    ShowWindow(QuickAccessForm.Handle, SW_SHOW);
    Windows.SetFocus(QuickAccessForm.Handle);
    Application.BringToFront;
    QuickAccessForm.BringToFront;
  end;
end;

procedure TLogoAppForm.SetFileName(const Value: string);
var View : Pointer;
begin
  FFileName := Value;
  if hMapping <> 0 then
  begin
    View := MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, MAX_PATH + 1);
    if (View <> nil) then
    begin
      StrPCopy(PChar(View), Value);
      UnmapViewOfFile(View);
    end;
  end;
  FormCaption.Texts[3].Caption := FFileName;
end;

procedure TLogoAppForm.miCheckCompatClick(Sender: TObject);
begin
  ElMessageDlg('This function is not implemented yet', mtInformation, [mbOk], 0);
end;

procedure TLogoAppForm.SetOpened(Value: Boolean);
begin
  if FOpened <> Value then
  begin
    FOpened := Value;
    if Opened then
    begin
      // Tree.BkColor := clWindow;
      Tree.ShowColumns := true;
      Tree.Enabled := true;
    end
    else
    begin
      //Tree.BkColor := clBtnFace;
      Tree.ShowColumns := false;
      Tree.Enabled := false;
    end;
    FormCaption.Texts[2].Visible := FOpened;
    FormCaption.Texts[3].Visible := FOpened;
    PrintItem.Enabled := FOpened;
    PrintBtn.Enabled := FOpened;
  end;
end;

procedure TLogoAppForm.AppEventsIdle(Sender : TObject; var Done: Boolean);
begin
  if JustStarted then
  begin
    JustStarted := false;
    if not Opened then
      FileNew(Self);
  end;
end;

procedure TLogoAppForm.TreeHeaderColumnDraw(Sender: TCustomElHeader;
  Canvas : TCanvas; Section: TElHeaderSection; R: TRect; Pressed: Boolean);
var R1 : TRect;
begin
  if Section.Index = hsiAttachment then
  begin
    CenterRects(8, R.Right - R.Left, 15, R.Bottom - R.Top, R1);
    Inc(R1.Left, R.Left);
    Inc(R1.Top, R.Top);
    ElImgList1.Draw(Canvas, R1.Left, R1.Top, 0);
  end;
end;

procedure TLogoAppForm.TreeItemDraw(Sender: TObject; Item: TElTreeItem;
  Surface: TCanvas; R: TRect; SectionIndex: Integer);
var Entry  : PEntryRec;
    R1 : TRect;
begin
  if SectionIndex = hsiAttachment then
  begin
    Entry := PEntryRec(TelMTreeItem(Item.Data).Data);
    if (not Entry.Group) and (Entry.BinDataSize > 0) then
    begin
      CenterRects(8, R.Right - R.Left, 15, R.Bottom - R.Top, R1);
      Inc(R1.Left, R.Left);
      Inc(R1.Top, R.Top);
      if Item.Selected then
        ElImgList1.Draw(Surface, R1.Left, R1.Top, 1)
      else
        ElImgList1.Draw(Surface, R1.Left, R1.Top, 0);
    end;
  end;
end;

procedure TLogoAppForm.miSaveAttachClick(Sender: TObject);
var DataStream : TStream;
    fns        : integer;
    Entry      : PEntryRec;
begin
  if (SelItem <> nil) and
     (TElMTreeItem(SelItem.Data)^.Data <> nil) and
     (PEntryRec(TElMTreeItem(SelItem.Data)^.Data)^.BinDataSize > 0) then
  begin
    Entry := PEntryRec(TElMTreeItem(SelItem.Data)^.Data);
    AttachSaveDlg.FileName := StrPas(PChar(Entry.BinData));
    if AttachSaveDlg.Execute then
    begin
      DataStream := TFileStream.Create(AttachSaveDlg.FileName, fmCreate or fmShareExclusive);
      try
        fns := StrLen(PChar(Entry.BinData)) + 1;
        DataStream.WriteBuffer((PChar(Entry.BinData) + fns)^, Entry.BinDataSize - fns);
      finally
        DataStream.Free;
      end;
    end;
  end;
end;

procedure TLogoAppForm.miExportClick(Sender: TObject);
var OutStream : TStream;
    AList     : TStrings;

  procedure SaveItem(Item:TElTreeItem; Index: integer; var ContinueIterate:boolean;
                            IterateData:pointer; Tree:TCustomElTree);
  var Entry     : PEntryRec;
      L, L1     : TStringList;
      gap       : string;
      i         : integer;
  begin
    L := TStringList(IterateData);
    Entry := PEntryRec(TElMTreeItem(Item.Data).Data);
    SetLength(gap, Item.Level*10);
    FillChar(gap[1], Length(gap), 32);
    if Entry.Group
     then L.Add(gap + 'Group name: '+ Entry.Site)
     else L.Add(gap + 'Site/Program name: '+ Entry.Site);
    if not Entry.Group then
    begin
      if Entry.Location <> '' then L.Add(gap + 'Location/address: ' + Entry.Location);
      if Entry.UName <> '' then L.Add(gap + 'User name: ' + Entry.UName);
      if Entry.Acct <> '' then L.Add(gap + 'Account #: '+ Entry.Acct);
      if Entry.Pswd <> '' then L.Add(gap + 'Password: '+ Entry.Pswd);
      if pos(#13, Entry.Info)>0 then
      begin
        L1 := TStringList.Create;
        try
          L1.Text:=Entry.Info;
          L.Add(gap + 'Additional info: '+ L1[0]);
          for i := 1 to L1.Count -1 do
          begin
            L.Add(Gap + '                 '+ L1[i]);
          end;
        finally
          L1.Free;
        end;
      end else
      begin
        if Entry.Info <> '' then
          L.Add(gap + 'Additional info: '+ Entry.Info);
      end;
    end;
    L.Add('');
  end;

begin
  if ExportDialog.Execute then
  begin
    if ExportDialog.FilterIndex = 1 then
    begin
      try
        OutStream := TFileStream.Create(ExportDialog.FileName, fmCreate or fmShareDenyWrite);
        try
          AList := TStringList.Create;
          try  
            Tree.Items.Iterate(false, true, @SaveItem, AList);
            WriteTextToStream(OutStream, AList.Text);
          finally
            AList.Free;
          end;
        finally
          OutStream.Free;
        end;
      except
        on E : EInOutError do
        begin
          ElMessageDlg(Format('There was an error %d while writing %s', [E.ErrorCode, ExtractFileName(FFileName)]), mtError, [mbOk], 0);
        end;
        on E : EStreamError do
        begin
          ElMessageDlg(Format('There was an error while writing %s: %s', [ExtractFileName(FFileName), E.Message]), mtError, [mbOk], 0);
        end;
        on E : Exception do
          Application.ShowException(E);
      end;

    end;
  end;
end;

procedure TLogoAppForm.TreeSortEnd(Sender: TObject);
begin
  if QuickAccessForm <> nil then
    QuickAccessForm.UpdateTree(FMTree);
end;

procedure TLogoAppForm.FormKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
  if (Key = VK_ESCAPE) and (Shift = []) and Options.MinimizeOnEsc then
    Application.Minimize;
end;

procedure TLogoAppForm.OptionsItemClick(Sender: TObject);
begin
  with TOptionsForm.Create(Self) do
  begin
    SetData;
    if ShowModal = mrOk then
      GetData;
    Free;
  end;
end;

procedure TLogoAppForm.TreeHeaderLookup(Sender: TObject;
  Section: TElHeaderSection; var Text: String);

type
  TSRec = record
    Text: PChar;
    ColNum: integer;
  end;
  PSRec = ^TSRec;

var
  SRec: TSrec;
  TI: TElTreeItem;

  function IntCompare(Item: TElTreeItem; SearchDetails: Pointer): boolean;
  var
    i: integer;
    AT: string;
  begin
    i := PSRec(SearchDetails).ColNum;
    if LogoAppForm.Tree.MainTreeColumn = i then AT := AnsiUpperCase(Item.Text) else
    begin
      if Item.ColumnText.Count <= i then
        AT := ''
      else
      begin
        if I > LogoAppForm.Tree.MainTreeColumn then
          AT := AnsiUpperCase(Item.ColumnText[i - 1])
        else
          AT := AnsiUpperCase(Item.ColumnText[i]);
      end;
    end;
    result := Pos(AnsiUpperCase(StrPas(PSRec(SearchDetails).Text)), AT) = 1;
  end;

begin
  SRec.Text := PChar(Text);
  SRec.ColNum := Section.Index;
  TI := Tree.Items.LookForItemEx(Tree.ItemFocused, Section.Index, true, false, false, @SRec, @IntCompare);
  if TI <> nil then
  begin
    Tree.EnsureVisible(TI);
    TI.FullyExpanded := true;
    Tree.ItemFocused := TI;
  end;
end;

procedure TLogoAppForm.ColumnsItemClick(Sender: TObject);
begin
  Tree.HeaderSections.Owner.Setup;
end;

procedure TLogoAppForm.SetModified2(Value: Boolean);
begin
  if FModified2 <> Value then
  begin
    FModified2 := Value;
    SaveBtn.Enabled := Modified or Value;
    FileSaveItem.Enabled := Modified or Value;
  end;
end;


procedure TLogoAppForm.TreeScroll(Sender: TObject;
  ScrollBarKind: TScrollBarKind; ScrollCode: Integer);
begin
  if ScrollBarKind = sbVertical then
    QuickAccessForm.Tree.TopIndex := Tree.TopIndex;
end;

end.
