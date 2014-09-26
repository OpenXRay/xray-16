unit ABOUT;

interface

uses Windows, Classes, SysUtils, Graphics, Forms, Controls, StdCtrls,
  Buttons, ExtCtrls, ElURLLabel, ElBtnCtl, ElPopBtn, ElFrmPers, ElCLabel,
  ElVerInfo, ElXPThemedControl, ElPanel;

type
  TAboutBox = class(TForm)
    ElFormPersist1: TElFormPersist;
    VerInfo: TElVersionInfo;
    Panel1: TElPanel;
    ProgramIcon: TImage;
    ProductName: TLabel;
    Copyright: TLabel;
    MailLabel: TElURLLabel;
    HomeLabel: TElURLLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label5: TLabel;
    lblVersion: TLabel;
    NameLabel: TLabel;
    Label6: TLabel;
    OkBtn: TElPopupButton;
    procedure FormCreate(Sender: TObject);
    procedure OkBtnClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  AboutBox: TAboutBox;

implementation

{$R *.DFM}

procedure TAboutBox.FormCreate(Sender: TObject);
var InfoString : string;
begin
  InfoString := '';
  if (InfoString <> '') then
  begin
    NameLabel.Caption := InfoString;
    NameLabel.Visible := true;
    NameLabel.Font.Color := clGreen;
  end;
  lblVersion.Caption := IntToStr(VerInfo.MajorVersion) + '.' +
                        IntToStr(VerInfo.MinorVersion) +
                        IntToStr(VerInfo.Release);
end;

procedure TAboutBox.OkBtnClick(Sender: TObject);
begin
  ModalResult := mrCancel;
  Close;
end;

end.

