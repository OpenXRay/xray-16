unit frmDateProp;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ElACtrls, ElBtnCtl, ElPopBtn, ElFlatCtl, ComCtrls, ElSpin, frmCalend,
  ElDTPick, ElXPThemedControl;

type
  TDatePropForm = class(TForm)
    OkBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    Label1: TLabel;
    NameEdit: TElAdvancedEdit;
    Label2: TLabel;
    ElFlatController1: TElFlatController;
    Label3: TLabel;
    RemindSpin: TElSpinEdit;
    Label4: TLabel;
    DateEdit: TElDateTimePicker;
  private
    { Private declarations }
  public
    procedure SetData(ADay : TElRemindDay);
    procedure GetData(ADay : TElRemindDay);
  end;

var
  DatePropForm: TDatePropForm;

implementation

{$R *.DFM}

procedure TDatePropForm.SetData(ADay : TElRemindDay);
begin
  NameEdit.Text := ADay.Name;
  DateEdit.Date := ADay.Date;
  RemindSpin.Value := ADay.RemindTime;
end;

procedure TDatePropForm.GetData(ADay : TElRemindDay);
begin
  ADay.Name := NameEdit.Text;
  ADay.Date := Trunc(DateEdit.Date);
  ADay.RemindTime := RemindSpin.Value;
end;

end.

