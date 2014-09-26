unit frmMstrEdit;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ElPopBtn, StdCtrls, ElBtnCtl, ElACtrls;

type
  TMStrEditForm = class(TForm)
    OkBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    MStrMemo: TElFlatMemo;
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  MStrEditForm: TMStrEditForm;

implementation

{$R *.DFM}

end.

