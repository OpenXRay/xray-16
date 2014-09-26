unit frmRecProp;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls, ShellApi, OleDrop, ActiveX, ElTools, EntryData,
  Menus, ElFrmPers, ElIni, ElACtrls, ElBtnCtl, ElPopBtn, ElCheckCtl,
  ElBtnEdit,
  ComCtrls, ElFlatCtl, ElSpin, ElDTPick, ElPromptDlg, hexeditor, Grids,
  ElPgCtl, ElXPThemedControl, ElPanel;

type
  TRecPropsForm = class(TForm)
    FormPers: TElFormPersist;
    AttachSaveDlg: TSaveDialog;
    AttachOpenDlg: TOpenDialog;
    ElFlatController1: TElFlatController;
    ElPanel1: TElPanel;
    ElPageControl1: TElPageControl;
    Main: TElTabSheet;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label7: TLabel;
    Label8: TLabel;
    Label9: TLabel;
    GoBtn: TElPopupButton;
    PswGenBtn: TElPopupButton;
    Go2Btn: TElPopupButton;
    SiteNameEdit: TElAdvancedEdit;
    URLEdit: TElAdvancedEdit;
    UNameEdit: TElAdvancedEdit;
    AcctEdit: TElAdvancedEdit;
    PswEdit: TElAdvancedEdit;
    URL2Edit: TElAdvancedEdit;
    dtpAdded: TElDateTimePicker;
    dtpModified: TElDateTimePicker;
    OKBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    ElTabSheet3: TElTabSheet;
    Label12: TLabel;
    AttachedLabel: TLabel;
    HexView: THexEditor;
    TextView: TElAdvancedMemo;
    ClearDataButton: TElPopupButton;
    AttachDataButton: TElPopupButton;
    SaveDatabutton: TElPopupButton;
    AsTextRadio: TElRadioButton;
    AsHexRadio: TElRadioButton;
    ElTabSheet2: TElTabSheet;
    Label6: TLabel;
    Label10: TLabel;
    Label11: TLabel;
    InfoMemo: TElAdvancedMemo;
    WrapCB: TElCheckBox;
    ExpiresCB: TElCheckBox;
    NotifySpin: TElSpinEdit;
    dtpExpires: TElDateTimePicker;
    procedure URLEditChange(Sender: TObject);
    procedure GoBtnClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure SiteNameEditDragDrop(Sender, Source: TObject; X, Y: Integer);
    procedure SiteNameEditDragOver(Sender, Source: TObject; X, Y: Integer;
      State: TDragState; var Accept: Boolean);
    procedure URLEditDragOver(Sender, Source: TObject; X, Y: Integer;
      State: TDragState; var Accept: Boolean);
    procedure URLEditDragDrop(Sender, Source: TObject; X, Y: Integer);
    procedure PswGenBtnClick(Sender: TObject);
    procedure Go2BtnClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure WrapCBClick(Sender: TObject);
    procedure ExpiresCBClick(Sender: TObject);
    procedure ClearDataButtonClick(Sender: TObject);
    procedure SaveDatabuttonClick(Sender: TObject);
    procedure URL2EditChange(Sender: TObject);
    procedure URL2EditDragDrop(Sender, Source: TObject; X, Y: Integer);
    procedure URL2EditDragOver(Sender, Source: TObject; X, Y: Integer;
      State: TDragState; var Accept: Boolean);
    procedure AttachDataButtonClick(Sender: TObject);
    procedure AsHexRadioClick(Sender: TObject);
    procedure AsTextRadioClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    Entry : PEntryRec;
    FSNDropTarget,
    FLDropTarget : IDropTarget;
    BinDataSize  : DWORD;
    BinData : Pointer;
    procedure GetData;
    procedure SetData;
  end;

var
  RecPropsForm: TRecPropsForm;

implementation

uses frmPswGen, LogoMain;

resourcestring
    sNone = '(None)';
    sPresent = 'Present '; 

procedure TRecPropsForm.GetData;  { public }
begin
  Entry^.Site:=SiteNameEdit.Text;
  Entry^.Location:=URLEdit.Text;
  Entry^.UName:=UNameEdit.Text;
  Entry^.Acct:=AcctEdit.Text;
  Entry^.Pswd:=PswEdit.Text;
  Entry^.Info:=InfoMemo.Text;
  Entry^.Location2 := URL2Edit.Text;
  Entry^.Modified := Now;
  if Trunc(Entry^.Expires) <> Trunc(dtpExpires.Date) then Entry.ExpWarned := false;
  Entry^.Expires := Trunc(dtpExpires.Date);
  Entry^.DoExpires := ExpiresCB.Checked;
  Entry.WarnDays := NotifySpin.Value;
  if Entry.BinData <> nil then
    FreeMem(Entry.BinData);
  Entry.BinDataSize := 0;
  if BinData <> nil then
  begin
    GetMem(Entry.BinData, BinDataSize);
    MoveMemory(Entry.BinData, BinData, BinDataSize);
    Entry.BinDataSize := BinDataSize;
  end;
end;  { GetData }

procedure TRecPropsForm.SetData;  { public }
var Stream : TDirectMemoryStream;
begin
  SiteNameEdit.Text:=Entry^.Site;
  URLEdit.Text:=Entry^.Location;
  UNameEdit.Text:=Entry^.UName;
  AcctEdit.Text:=Entry^.Acct;
  PswEdit.Text:=Entry^.Pswd;
  InfoMemo.Text:=Entry^.Info;
  URL2Edit.Text:=Entry^.Location2;
  dtpModified.Date := Entry^.Modified;
  dtpAdded.Date := Entry.Added;
  dtpExpires.Date := Entry^.Expires;
  NotifySpin.Value := Entry.WarnDays;
  ExpiresCB.Checked := Entry^.DoExpires;
  if Entry^.BinDataSize > 0 then
  begin
    BinDataSize := Entry.BinDataSize;
    GetMem(BinData, BinDataSize);
    MoveMemory(BinData, Entry.BinData, BinDataSize);
    AttachedLabel.Caption := StrPas(PChar(Entry.BinData)) + ' (' + IntToStr(BinDataSize - StrLen(PChar(Entry.BinData)) - 1) + ' bytes)';
    Stream := TDirectMemoryStream.Create;
    Stream.SetPointer(BinData, BinDataSize);
    Stream.Seek(StrLen(PChar(Entry.BinData)) + 1, soFromBeginning);
    HexView.LoadFromStream(Stream);
    Stream.SetPointer(nil, 0);
  end
  else
  begin
    BinData := nil;
    BinDataSize := 0;
    AttachedLabel.Caption := sNone;
    HexView.AsText := '';
  end;
  if AsHexRadio.Checked then
    AsHexRadioClick(Self)
  else
    AsTextRadioClick(Self);
    
  ExpiresCBClick(Self);
end;  { SetData }

{$R *.DFM}

procedure TRecPropsForm.URLEditChange(Sender: TObject);
begin
  GoBtn.Enabled:=UrlEdit.Text<>'';
end;

procedure TRecPropsForm.GoBtnClick(Sender: TObject);
begin
  ShellExecute(0,'open',PChar(UrlEdit.Text),nil, nil, SW_SHOWNORMAL);
end;

procedure TRecPropsForm.FormCreate(Sender: TObject);
begin
  FSNDropTarget := IWCDroptarget.Create(SiteNameEdit);
  FLDropTarget := IWCDroptarget.Create(URLEdit);
end;

procedure TRecPropsForm.FormDestroy(Sender: TObject);
begin
  FSNDropTarget := nil;
  FLDropTarget := nil;
end;

procedure TRecPropsForm.SiteNameEditDragDrop(Sender, Source: TObject; X,
  Y: Integer);
var i : Integer;
begin
  if Source is ToleDragObject then
  begin
    i:=ToleDragObject(source).DragContent;
    if i=CF_Text then
      SiteNameEdit.Text:= ToleDragObject(source).StringData;
  end;
end;

procedure TRecPropsForm.SiteNameEditDragOver(Sender, Source: TObject; X,
  Y: Integer; State: TDragState; var Accept: Boolean);
begin
  Accept := Source is ToleDragObject;
end;

procedure TRecPropsForm.URLEditDragOver(Sender, Source: TObject; X,
  Y: Integer; State: TDragState; var Accept: Boolean);
begin
  Accept := Source is TOleDragObject;
end;
 
procedure TRecPropsForm.URLEditDragDrop(Sender, Source: TObject; X,
  Y: Integer);
begin
  if Source is ToleDragObject then
  begin
    if ToleDragObject(source).DragContent = CF_Text then
      URLEdit.Text:= ToleDragObject(source).StringData;
  end;
end;

procedure TRecPropsForm.PswGenBtnClick(Sender: TObject);
begin
  PswGenForm := TPswGenForm.Create(nil);
  if PswGenForm.ShowModal = mrOk then
  begin
    PswEdit.Text := PswGenForm.Pssw;
  end;
  PswGenForm.Free;
  PswGenForm := nil;
end;

procedure TRecPropsForm.Go2BtnClick(Sender: TObject);
begin
  ShellExecute(0,'open',PChar(Url2Edit.Text),nil, nil, SW_SHOWNORMAL);
end;

procedure TRecPropsForm.FormShow(Sender: TObject);
begin
  ActiveControl := SiteNameEdit;
end;

procedure TRecPropsForm.WrapCBClick(Sender: TObject);
begin
  InfoMemo.WordWrap := WrapCB.Checked;
  if WrapCB.Checked then
     InfoMemo.Scrollbars := ssVertical
  else
     InfoMemo.Scrollbars := ssBoth;  
end;

procedure TRecPropsForm.ExpiresCBClick(Sender: TObject);
begin
  dtpExpires.Enabled := ExpiresCB.Checked;
  NotifySpin.Enabled := ExpiresCB.Checked;
end;

procedure TRecPropsForm.ClearDataButtonClick(Sender: TObject);
begin
  if BinDataSize > 0 then
  begin
    if ElMessageDlg('Do you want to clear all associated data?', mtWarning, [mbYes, mbNo], 0) = idYes then
    begin
      FreeMem(BinData);
      BinData := nil;
      BinDataSize := 0;
      AttachedLabel.Caption := sNone;
      HexView.CreateEmptyFile('unnamed');
      TextView.Text := '';
    end;
  end;
end;

procedure TRecPropsForm.SaveDatabuttonClick(Sender: TObject);
var DataStream : TStream;
    fns        : DWORD;
begin
  if BinDataSize > 0 then
  begin
    AttachSaveDlg.FileName := StrPas(PChar(Entry.BinData));
    if AttachSaveDlg.Execute then
    begin
      DataStream := TFileStream.Create(AttachSaveDlg.FileName, fmCreate or fmShareExclusive);
      try
        fns := StrLen(PChar(Entry.BinData)) + 1;
        DataStream.WriteBuffer((PChar(BinData) + fns)^, BinDataSize - fns);
      finally
        DataStream.Free;
      end;
    end;
  end
  else
    ElMessageDlg('There is no data available for saving', mtInformation, [mbOk], 0);
end;

procedure TRecPropsForm.URL2EditChange(Sender: TObject);
begin
  Go2Btn.Enabled:=Url2Edit.Text<>'';
end;

procedure TRecPropsForm.URL2EditDragDrop(Sender, Source: TObject; X,
  Y: Integer);
begin
  if Source is ToleDragObject then
  begin
    if ToleDragObject(source).DragContent = CF_Text then
      URL2Edit.Text := ToleDragObject(source).StringData;
  end;
end;

procedure TRecPropsForm.URL2EditDragOver(Sender, Source: TObject; X,
  Y: Integer; State: TDragState; var Accept: Boolean);
begin
  Accept := Source is TOleDragObject;
end;

procedure TRecPropsForm.AttachDataButtonClick(Sender: TObject);
var NewData : Pointer;
    NewSize : integer;
    DataStream : TStream;
    b : boolean;
    FN: string;
begin
  if BinDataSize > 0 then
    b := ElMessageDlg('Attaching new data will clear all currently associated data. Continue?', mtWarning, [mbYes, mbNo], 0) = idYes
  else
    b := true;
  if b then
  begin
    if AttachOpenDlg.Execute then
    begin
      DataStream := TFileStream.Create(AttachOpenDlg.FileName, fmOpenRead or fmShareDenyWrite);
      try
        FN := ExtractFileName(AttachOpenDlg.FileName);
        NewSize := DataStream.Size + Length(FN) + 1;
        GetMem(NewData, NewSize);
        StrCopy(PChar(NewData), PChar(FN));
        DataStream.ReadBuffer((PChar(NewData) + Length(FN) + 1)^, DataStream.Size);
        if BinData <> nil then
        begin
          FreeMem(BinData);
          BinData := nil;
          BinDataSize := 0;
        end;
        BinData := NewData;
        BinDataSize := NewSize;
        AttachedLabel.Caption := FN + ' (' + IntToStr(BinDataSize - Length(FN) - 1) + ' bytes)';
        DataStream.Position := 0;
        HexView.LoadFromStream(DataStream);
        if AsTextRadio.Checked then
          AsTextRadioClick(Self); 

        if ElMessageDlg('Add the name of the attached file and current time to record notes?', mtConfirmation, [mbYes, mbNo], 0) = mrYes then
        begin
         InfoMemo.Lines.Add('Filename: ' + ExtractFileName(AttachOpenDlg.FileName));
         InfoMemo.Lines.Add('Added: ' + FormatDateTime('c', Now));
        end;
      finally
        DataStream.Free;
      end;
    end;
  end;
end;

procedure TRecPropsForm.AsHexRadioClick(Sender: TObject);
begin
  HexView.Visible  := true;
  TextView.Visible := false;
  TextView.Text    := '';
end;

procedure TRecPropsForm.AsTextRadioClick(Sender: TObject);
begin
  TextView.Text    := HexView.AsText;
  TextView.Visible := true;
  HexView.Visible  := false;
end;

end.

