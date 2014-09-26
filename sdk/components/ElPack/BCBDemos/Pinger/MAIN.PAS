unit Main;

interface

uses
  SysUtils, Windows, Messages, Classes, Graphics, Controls,
  Forms, Dialogs, StdCtrls, ExtCtrls, Menus, ComCtrls, ElInputDlg,
  ICMP, ElGraphs, SiteMan, ElTools, ElIni, ElFrmPers, ElTimers, ElPgCtl,
  ElXPThemedControl, ElStatBar;

type
  TMainForm = class(TForm)
    MainMenu: TMainMenu;
    GraphAddItem: TMenuItem;
    GraphRemoveItem: TMenuItem;
    FileExitItem: TMenuItem;
    HelpAboutItem: TMenuItem;
    PropItem: TMenuItem;
    ResetItem: TMenuItem;
    ElFormPersist1: TElFormPersist;
    ElIniFile1: TElIniFile;
    TimerList: TElTimerPool;
    Pages: TElPageControl;
    StatusBar: TElStatusBar;
    procedure FileExit(Sender: TObject);
    procedure HelpAbout(Sender: TObject);
    procedure GraphAddItemClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure GraphRemoveItemClick(Sender: TObject);
    procedure PropItemClick(Sender: TObject);
    procedure PagesChange(Sender: TObject);
    procedure FileMenuClick(Sender: TObject);
    procedure ResetItemClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
   protected
    SiteList : TList;
    ShutDown : boolean;
    function DoAddGraph(HostName:string) : TSite;
    procedure LoadList;
    procedure SaveList;
  public
    procedure Loaded ; override ;
  end;

var
  MainForm: TMainForm;

implementation

uses frmList, About, frmProp;

{$R *.DFM}

procedure TMainForm.FileExit(Sender: TObject);
var i:integer;
begin
  for i:=0 to SiteList.Count-1 do TSite(SiteList[i]).Shutdown:=true;
  MainForm.Close;
end;

procedure TMainForm.HelpAbout(Sender: TObject);
begin
  AboutBox.ShowModal;
end;

procedure TMainForm.GraphAddItemClick(Sender: TObject);
var S:String;
begin
  Application.ProcessMessages;
  S := '';
  if ElInputDlg.InputQuery('Add host to ping','Please enter the host name or address to ping', S, false) then
  begin
    if DoAddGraph(S) <> nil then
    begin
      PropItemClick(Self);
    end;
  end;
end;

function TMainForm.DoAddGraph;
var S:String;
    IC:TICMP;
    Site : TSite;
    Entry : TDataEntry;
begin
  result := nil;
  S:=HostName;
  try          
   IC:=TICMP.Create;
  except
    On E:TICMPException do
    begin
      MessageBox(0,PChar(E.Message),PChar(Application.Title),MB_ICONSTOP or MB_OK);
      exit;
    end;
  end;
  IC.Address:=S;
  IC.ResolveAddr;
  if IC.ErrorCode > 0 then
  begin
    MessageBox(0,'Failed to resolve the given address',PChar(Application.Title),MB_ICONSTOP or MB_OK);
    IC.Free;
    Exit;
  end;
  Site := TSite.Create;
  SiteList.Add(Site);
  Site.Address := IC.HostIP;
  IC.Free;
  Site.RepCount := 0;
  Site.HostName:=S;
  Site.Timeout := 1000;
  Site.StartTime:=DateTimeToStr(Now);
  Site.Sheet:=TElTabSheet.Create(Pages);
  Site.Sheet.PageControl:=Pages;
  Site.Graph:=TElGraph.Create(Site.Sheet);
  Site.Sheet.InsertControl(Site.Graph);
  Site.Graph.Align:=alClient;
  Site.Sheet.Caption:=Site.HostName;
  Entry:=Site.Graph.AddEntry;
  Site.Graph.ShowMinMax:=true;
  Site.Graph.ShowLegend:=false;
  Site.Graph.ColumnEntry:=Entry;
  Site.Graph.MinMaxEntry:=Entry;
  Site.Graph.ShowGrid:=false;
  Site.Graph.HGridLines:=5;
  Site.Graph.VGridLines:=20;
  Site.Graph.Color:=clBtnText;
  Site.Graph.LegendBkColor:=clWindow;
  Entry.Color:=clLime;
  Entry.MaxGrid:=100;
  Entry.AutoGrid:=true;
  Site.Start;
  Pages.ActivePage := Site.Sheet;
  PagesChange(Self);
  Result := Site;
end;

procedure TMainForm.FormDestroy(Sender: TObject);
var i:integer;
    Site: TSite;
begin
  ShutDown:=true;
  Screen.Cursor := crHourGlass;
  try
    i:=0;
    while i<SiteList.Count do
    begin
      Site:=TSite(SiteList[0]);
      Site.Shutdown := true;
      SiteList.Delete(0);
      Site.Free;
    end;
    SiteList.Free;
  finally
    Screen.Cursor := crDefault;
  end;  // try/finally
end;

procedure TMainForm.GraphRemoveItemClick(Sender: TObject);
var i:integer;
    Site:TSite;
    T:TElTabSheet;
begin
  SitesForm.Sites.Items.Clear;
  for i:=0 to SiteList.Count-1 do    // Iterate
    SitesForm.Sites.Items.Add(TSite(SiteList[i]).Address);
  if SitesForm.ShowModal=mrOK then
  begin
    i:=SitesForm.Sites.ItemIndex;
    if i=-1 then exit;
    Site:=TSite(SiteList[i]);
    SiteList.Delete(i);
    Site.Shutdown:=true;
    T := nil;
   try
    T:=Site.Sheet;
    Site.Free;
   finally
    T.Free;
   end;
   if Pages.PageCount>0 then
   begin
     Pages.ActivePage := Pages.Pages[0];
     PagesChange(Self);
   end else StatusBar.SimpleText := '';
  end;
end;

procedure TMainForm.PropItemClick(Sender: TObject);
var Sheet : TElTabSheet;
    i : integer;
    Site : TSite;
begin
  Sheet:=Pages.ActivePage;
  if Sheet = nil then exit;
  i:=0;
  while TSite(SiteList[i]).Sheet<>Sheet do inc(i);
  if i>=SiteList.Count then exit;
  Site:=TSite(SiteList[i]);
  PropForm.HostLabel.Caption:=Site.Address;
  PropForm.GridCB.Checked:=Site.Graph.ShowGrid;
  PropForm.TimeOutSpin.Value:=Site.Timeout;
  PropForm.LogCountSpin.Value:=Site.RepCount;
  PropForm.IntervalSpin.Value := Site.Interval;
  PropForm.RepCB.Checked:= Site.RepCount>0;
  PropForm.LogNameEdit.Text:=Site.RepFile;
  PropForm.LogNameEdit.Enabled := Site.RepCount>0;
  if PropForm.ShowModal = mrCancel then exit;
  Site.Timeout:=Trunc(PropForm.TimeOutSpin.Value);
  Site.Interval := Trunc(PropForm.IntervalSpin.Value);
  Site.Graph.ShowGrid:=PropForm.GridCB.Checked;
  Site.RepFile:=PropForm.LogNameEdit.Text;
  if not PropForm.RepCB.Checked
     then Site.RepCount:= 0
     else Site.RepCount:= Round(PropForm.LogCountSpin.Value);
end;

procedure TMainForm.Loaded ;  { public }
var i:integer;
begin
  SiteList:=TList.Create;
  for i:=1 to ParamCount do    // Iterate
    DoAddGraph(ParamStr(i));
  LoadList;
end ;  { Loaded }

procedure TMainForm.PagesChange(Sender: TObject);
var Site : TSite;
    i : integer;
    Sheet : TElTabSheet;

begin
  i:=0;
  Sheet := Pages.ActivePage;
  if Sheet = nil then
  begin
    StatusBar.SimpleText := '';
    exit;
  end;
  if SiteList.Count < i then exit;
  while TSite(SiteList[i]).Sheet<>Sheet do inc(i);
  Site:=TSite(SiteList[i]);
  StatusBar.SimpleText := 'Started '+ Site.StartTime;
end;

procedure TMainForm.FileMenuClick(Sender: TObject);
begin
  GraphRemoveItem.Enabled := Pages.ActivePage <> nil;
  PropItem.Enabled := Pages.ActivePage <> nil;
  ResetItem.Enabled := Pages.ActivePage <> nil;
end;

procedure TMainForm.ResetItemClick(Sender: TObject);
var Sheet : TElTabSheet;
    i : integer;
    Site : TSite;

begin
  Sheet := Pages.ActivePage;
  if Sheet = nil then exit;
  if MessageBox(0, 'Reset statistics???', PChar(Application.Title), mb_YesNo) = mrNo then exit;
  i := 0;
  while TSite(SiteList[i]).Sheet<>Sheet do inc(i);
  Site:=TSite(SiteList[i]);
  Site.Graph.MinMaxEntry.Reset;
  Site.StartTime:=DateTimeToStr(Now);
  StatusBar.SimpleText := Site.StartTime;
end;

procedure TMainForm.LoadList;  { protected }
var S : TFileStream;
    i, j,
    k    : integer;
    Site : TSite;
    host : string;
    aFile : String;
begin
  try
    aFile := ExtractFilePath(ParamStr(0))+'ElPinger.hst';
    if FileExists(aFile) then
    begin
      S := TFileStream.Create(aFile, fmOpenRead or fmShareDenyWrite);
      try
       S.ReadBuffer(i, sizeof(Integer));
       for j := 0 to i - 1 do
       begin
         if (not ReadStringFromStream(S, Host)) or (Host = '') then Continue;
         Site := DoAddGraph(Host);
         if Site = nil then Continue;
         if ReadStringFromStream(S, Host) then Site.RepFile := Host;
         ReadStringFromStream(S, Site.StartTime);
         StatusBar.SimpleText := 'Started '+ Site.StartTime;
         S.ReadBuffer(K, sizeof(Integer));
         Site.Timeout := K;
         S.ReadBuffer(K, sizeof(Integer));
         Site.RepCount := K;
         S.ReadBuffer(K, sizeof(Integer));
         Site.Interval := K;
       end;
      finally
        S.Free;
      end;
    end;
  except
  end;
end;  { LoadList }

procedure TMainForm.SaveList;  { protected }
var S : TFileStream;
    i, j : integer;
    Site : TSite;

begin
  S := nil;
  try
    S := TFileStream.Create(ExtractFilePath(ParamStr(0))+'ElPinger.hst', fmCreate or fmShareDenyWrite);
    i := SiteList.Count;
    S.WriteBuffer(i, sizeof(Integer));
    for j := 0 to i - 1 do
    begin
      Site := TSite(SiteList[j]);
      WriteStringToStream(S, Site.HostName);
      WriteStringToStream(S, Site.RepFile);
      WriteStringToStream(S, Site.StartTime);
      S.WriteBuffer(Site.Timeout, sizeof(Integer));
      S.WriteBuffer(Site.RepCount, sizeof(Integer));
      S.WriteBuffer(Site.Interval, sizeof(Integer));
    end;
  finally
    S.Free;
  end;
end;  { SaveList }

procedure TMainForm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  SaveList;
end;

end.
