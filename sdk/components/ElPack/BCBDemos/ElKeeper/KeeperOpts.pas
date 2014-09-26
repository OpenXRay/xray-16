unit KeeperOpts;

interface

uses ElOpts, Classes;

type

  TKeeperOpts = class(TElOptions)
  private
    FSaveKeys: Boolean;
    FShowPassword: Boolean;
    FKeepPassword: Boolean;
    FLastFile: String;
    FReopenFile: Boolean;
    FToTray: Boolean;
    FMinimizeOnEsc: Boolean;
    FMinimizeOnClose: Boolean;
    FCountFolderChanges: Boolean;
    procedure SetShowPassword(newValue: Boolean);
  public
    constructor Create(AOwner: TComponent); override;
  published
    property ShowPassword: Boolean read FShowPassword write SetShowPassword default False;
    property KeepPassword: Boolean read FKeepPassword write FKeepPassword;
    property LastFile: String read FLastFile write FLastFile;
    property ReopenFile: Boolean read FReopenFile write FReopenFile;
    property ToTray: Boolean read FToTray write FToTray;
    property SaveKeys: Boolean read FSaveKeys write FSaveKeys;
    property MinimizeOnEsc: Boolean read FMinimizeOnEsc write FMinimizeOnEsc;
    property MinimizeOnClose: Boolean read FMinimizeOnClose write FMinimizeOnClose
        default false;
    property CountFolderChanges: Boolean read FCountFolderChanges write
        FCountFolderChanges;
  end;

var Options : TKeeperOpts;

implementation

uses LogoMain;

constructor TKeeperOpts.Create(AOwner: TComponent);
begin
  FShowPassword := False;
  FCountFolderChanges := true;
end;

procedure TKeeperOpts.SetShowPassword(newValue: Boolean);
begin
  if (FShowPassword <> newValue) then
  begin
    FShowPassword := newValue;
    if LogoAppForm <> nil then
      with LogoAppForm do
      begin
        Tree.HeaderSections[4].Password := not newValue;
        Tree.HeaderSections[3].Password := not newValue;
      end;
  end;  {if}
end;

end.


