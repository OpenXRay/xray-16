unit frmPassword;

interface

uses Windows, SysUtils, Classes, Forms, Controls, StdCtrls, 
  Buttons, ElACtrls, ElBtnCtl, ElPopBtn, ElFlatCtl, ElXPThemedControl,
  ExtCtrls, ElPanel;

type
  TPasswordDlg = class(TForm)
    ElFlatController1: TElFlatController;
    ElFlatController2: TElFlatController;
    ElPanel1: TElPanel;
    Label1: TLabel;
    ConfLabel: TLabel;
    OKBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    Password: TEdit;
    ConfPassword: TEdit;
    procedure FormShow(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  PasswordDlg: TPasswordDlg;

implementation

{$R *.DFM}

procedure TPasswordDlg.FormShow(Sender: TObject);
begin
  ActiveControl := Password;
end;

end.

