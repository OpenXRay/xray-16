unit frmOpts;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ExtCtrls, ElPanel, ElBtnCtl, ElCheckCtl, ElPopBtn;

type
  TOptionsForm = class(TForm)
    ElPanel1: TElPanel;
    LeftCB: TElCheckBox;
    RightCB: TElCheckBox;
    TopCB: TElCheckBox;
    BottomCB: TElCheckBox;
    FloatingCB: TElCheckBox;
    btnOK: TElPopupButton;
    btnCancel: TElPopupButton;
    ElPanel2: TElPanel;
    AutoHideCB: TElCheckBox;
    KeepSizeCB: TElCheckBox;
    TopmostCB: TElCheckBox;
    OnScreenCB: TElCheckBox;
    TaskBarCB: TElCheckBox;
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  OptionsForm: TOptionsForm;

implementation

{$R *.DFM}

end.

