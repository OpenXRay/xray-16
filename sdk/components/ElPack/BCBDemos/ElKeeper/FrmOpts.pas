unit FrmOpts;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  KeeperOpts, ElBtnCtl, ElPopBtn, ElCheckCtl, StdCtrls, ExtCtrls, ElPanel,
  ElGroupBox, ElXPThemedControl;

type
  TOptionsForm = class(TForm)
    FontDlg: TFontDialog;
    ElPanel1: TElPanel;
    ElGroupBox1: TElGroupBox;
    MinToTrayCB: TElCheckBox;
    MinOnCloseCB: TElCheckBox;
    MinOnEscCB: TElCheckBox;
    ElGroupBox3: TElGroupBox;
    CountExpandCB: TElCheckBox;
    ElGroupBox2: TElGroupBox;
    RememberMRUCB: TElCheckBox;
    RememberPswCB: TElCheckBox;
    ReopenLastFileCB: TElCheckBox;
    FontBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    OKBtn: TElPopupButton;
    procedure FontBtnClick(Sender: TObject);
  private
    { Private declarations }
  public
    procedure SetData;
    procedure GetData;
  end;

var
  OptionsForm: TOptionsForm;

implementation

uses frmQuickAccess, LogoMain, Registry;

{$R *.DFM}

procedure TOptionsForm.SetData;
begin
  MinToTrayCB.Checked := Options.ToTray;
  ReopenLastFileCB.Checked := Options.ReopenFile;
  RememberMRUCB.Checked := not Options.SaveKeys;
  RememberPswCB.Checked := Options.KeepPassword;
  MinOnEscCB.Checked := Options.MinimizeOnEsc;
  MinOnCloseCB.Checked := Options.MinimizeOnClose;
  CountExpandCB.Checked := Options.CountFolderChanges;
end;

procedure TOptionsForm.GetData;
begin
  Options.ToTray := MinToTrayCB.Checked;
  Options.ReopenFile := ReopenLastFileCB.Checked;
  Options.SaveKeys := not RememberMRUCB.Checked;
  Options.KeepPassword := RememberPswCB.Checked;
  Options.MinimizeOnEsc := MinOnEscCB.Checked;
  Options.MinimizeOnClose := MinOnCloseCB.Checked;
  Options.CountFolderChanges := CountExpandCB.Checked;
end;

procedure TOptionsForm.FontBtnClick(Sender: TObject);
var Reg : TRegistry;
    St  : TFontStyles;

begin
  FontDlg.Font.Assign(LogoAppForm.Tree.Font);
  if FontDlg.Execute then
  with LogoAppForm do
  begin
    Tree.Font.Assign(FontDlg.Font);
    Tree.TextColor := FontDlg.Font.Color;

    QuickAccessForm.Tree.Font.Assign(FontDlg.Font);
    QuickAccessForm.Tree.TextColor := FontDlg.Font.Color;

    Reg:=nil;
    try
      try
        Reg:=TRegistry.Create;
        Reg.OpenKey('Software\EldoS\Keeper', true);
        Reg.WriteString('FontName', Tree.Font.Name);
        Reg.WriteInteger('FontCharset', Integer(Tree.Font.Charset));
        Reg.WriteInteger('FontSize', Tree.Font.Size);
        St := Tree.Font.Style;
        Reg.WriteBinaryData('FontStyles', St, sizeof(TFontStyles));
      finally
        Reg.Free;
      end;
    except
      on E: Exception do ;
    end;
  end;
end;

end.
