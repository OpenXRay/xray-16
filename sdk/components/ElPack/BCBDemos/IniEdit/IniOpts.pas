unit IniOpts;

interface

uses
  Classes,
  SysUtils,
  ElHeader,
  ElTree,
  ElOpts;

type
  TOptions = class(TElOptions)
  private
    FSort: Boolean;
    FOneInstance: Boolean;
    FCustomColors: Boolean;
    FLazyWrite: Boolean;
    FSimple: Boolean;
    FLoadLastUsed: Boolean;
    FShowDailyTip : Boolean;
    procedure SetLazyWrite(newValue: Boolean);
    procedure SetSimple(newValue: Boolean);
    procedure SetCustomColors(newValue: Boolean);
    procedure SetOneInstance(newValue: Boolean);
    procedure SetSort(newValue: Boolean);
  protected
    procedure SetAutoSave (value : boolean); override;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
  published
    property LoadLastUsed: Boolean read FLoadLastUsed write FLoadLastUsed;
    property LazyWrite: Boolean read FLazyWrite write SetLazyWrite default true;
    property Simple: Boolean read FSimple write SetSimple;
    property CustomColors: Boolean read FCustomColors write SetCustomColors;
    property OneInstance: Boolean read FOneInstance write SetOneInstance;
    property Sort: Boolean read FSort write SetSort;
    property ShowDailyTip : Boolean read FShowDailyTip write FShowDailyTip;
  end;

var Options : TOptions;

implementation

uses Main;

procedure TOptions.SetAutoSave (value : boolean);
begin
  if FAutoSave <> value then
  begin
    FAutoSave := value;
    MainForm.AutoSaveItem.Checked := value;
  end;
end;

procedure TOptions.SetLazyWrite(newValue: Boolean);
begin
  if (FLazyWrite <> newValue) then
  begin
    FLazyWrite := newValue;
    MainForm.LazyItem.Checked := FLazyWrite;
    MainForm.IniFile.LazyWrite := FLazyWrite;
    if not FLazyWrite then MainForm.Modified := false;
  end;  {if}
end;

procedure TOptions.SetSimple(newValue: Boolean);
begin
  if (FSimple <> newValue) then
  begin
    FSimple := newValue;
    MainForm.IniFile.Simple := FSimple;
    if FSimple
       then MainForm.StandardItem.Checked := true
       else MainForm.EnhancedItem.Checked := true;
    MainForm.TreeItemFocused(Self);
  end;  {if}
end;

procedure TOptions.SetCustomColors(newValue: Boolean);
begin
  if (FCustomColors <> newValue) then
  begin
    FCustomColors := newValue;
    Mainform.UseCustomItem.Checked := newValue;
    MainForm.CustColorsItem.Enabled := newValue;
    MainForm.RefreshItems;
  end;  {if}
end;

procedure TOptions.SetOneInstance(newValue: Boolean);
begin
  if (FOneInstance <> newValue) then
  begin
    FOneInstance := newValue;
    MainForm.OneInst.Enabled := newValue;
    MainForm.OneInstItem.Checked := newValue;
  end;  {if}
end;

procedure TOptions.SetSort(newValue: Boolean);
var SM : TElSSortMode;
begin
  if (FSort <> newValue) then
  begin
    FSort := newValue;
    MainForm.SortItem.Checked := FSort;
    if FSort then
    begin
      MainForm.Tree.SortMode := smAddClick;
      if MainForm.Tree.SortDir = sdAscend then SM := hsmAscend else
      if MainForm.Tree.SortDir = sdDescend then SM := hsmDescend else sm:=hsmNone;
      MainForm.Tree.HeaderSections[MainForm.Tree.SortSection].SortMode := SM;
      MainForm.Tree.Sort(true);
    end else
    begin
      MainForm.Tree.SortMode := smNone;
      MainForm.Tree.HeaderSections[MainForm.Tree.SortSection].SortMode := hsmNone;
    end;
  end;  {if}
end;

destructor TOptions.Destroy;
begin
  inherited Destroy;
end;

constructor TOptions.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  StorageType := eosElIni;
  IniSection := 'Options';
  FLazyWrite := true;
  FSort := true;
  FShowDailyTip := True;
end;

end.

