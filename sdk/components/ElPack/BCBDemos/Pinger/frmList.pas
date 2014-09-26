unit frmList;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ElACtrls, ElXPThemedControl, ElBtnCtl, ElPopBtn;

type
  TSitesForm = class(TForm)
    Sites: TElAdvancedListBox;
    OkBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    procedure SitesClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  SitesForm: TSitesForm;

implementation

{$R *.DFM}


procedure TSitesForm.SitesClick(Sender: TObject);
begin
  OkBtn.Enabled := Sites.ItemIndex <> -1;
end;

procedure TSitesForm.FormShow(Sender: TObject);
begin
  ActiveControl := Sites;
  OkBtn.Enabled := Sites.ItemIndex <> -1;
end;
             
end.
