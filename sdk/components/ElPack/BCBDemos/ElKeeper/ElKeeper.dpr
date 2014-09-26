{$I DEFINE.INC}

program ElKeeper;

uses
  Forms,
  LogoMain in 'LogoMain.pas' {LogoAppForm},
  ABOUT in 'ABOUT.PAS' {AboutBox},
  frmPassword in 'frmPassword.pas' {PasswordDlg},
  frmRecProp in 'frmRecProp.pas' {RecPropsForm},
  Md5unit in '..\Libs\md5unit.pas',
  Ideaunit in '..\Libs\Ideaunit.pas',
  frmPswGen in 'frmPswGen.pas' {PswGenForm},
  KeeperOpts in 'KeeperOpts.pas',
  frmQuickAccess in 'frmQuickAccess.pas' {QuickAccessForm},
  EntryData in 'EntryData.pas',
  frmFolderProp in 'frmFolderProp.pas' {FolderPropsForm},
  FrmOpts in 'FrmOpts.pas' {OptionsForm};

{$R *.RES}
{$R LOGOSTRS.RES}

type
  TProcedure = procedure;

var Form : TForm;

procedure ExecuteApplication;
begin
  Application.Title := 'EldoS Keeper';
  Application.HelpFile := 'ElKeeper.hlp';
  Application.ShowMainForm := false;
  Application.CreateForm(TForm, Form);
  LogoAppForm := TLogoAppForm.Create(Application);
  PasswordDlg := TPasswordDlg.Create(Application);
  QuickAccessForm := TQuickAccessForm.Create(Application);
  LogoAppForm.Show;
  with Application do
    repeat
      HandleMessage
    until Terminated;
end;

begin
  ExecuteApplication;
end.

