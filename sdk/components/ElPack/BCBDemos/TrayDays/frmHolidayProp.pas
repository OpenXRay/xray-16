unit frmHolidayProp;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ElACtrls, ExtCtrls, ElPopBtn, ElCalendar, ElBtnCtl, ElCheckCtl,
  ElCalendarDefs, ElXPThemedControl;

type
  THolidayPropForm = class(TForm)
    IsRestCB: TElCheckBox;
    Panel1: TPanel;
    Panel2: TPanel;
    Label9: TLabel;
    FloatDayCombo: TElAdvancedComboBox;
    FloatDOWCombo: TElAdvancedComboBox;
    FloatMonthCombo: TElAdvancedComboBox;
    FloatDateRB: TElRadioButton;
    Panel3: TPanel;
    Label2: TLabel;
    FixedDayCombo: TElAdvancedComboBox;
    FixedMonthCombo: TElAdvancedComboBox;
    FixedDateRB: TElRadioButton;
    DescriptionEdit: TElAdvancedEdit;
    Label1: TLabel;
    OkBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    procedure FixedDateRBClick(Sender: TObject);
  private
    { Private declarations }
  public
    AHoliday : TElHoliday;
    procedure SetData;
    procedure GetData;
  end;

var
  HolidayPropForm: THolidayPropForm;

implementation

{$R *.DFM}

procedure THolidayPropForm.GetData;
begin
  AHoliday.Description := DescriptionEdit.Text;
  AHoliday.FixedDate := FixedDateRB.Checked;
  if FixedDateRB.Checked then
  begin
    AHoliday.Month := FixedMonthCombo.ItemIndex + 1;
    AHoliday.Day := FixedDayCombo.ItemIndex + 1;
  end else
  begin
    AHoliday.Month := FloatMonthCombo.ItemIndex + 1;
    AHoliday.DayOfWeek := FloatDOWCombo.ItemIndex;
    AHoliday.Day := FloatDayCombo.ItemIndex + 1;
  end;
  AHoliday.IsRest := IsRestCB.Checked;
end;

procedure THolidayPropForm.SetData;
begin
  DescriptionEdit.Text := AHoliday.Description;
  FixedDateRB.Checked := AHoliday.FixedDate;
  FloatDateRB.Checked := not AHoliday.FixedDate;
  FixedDateRBClick(Self);
  if AHoliday.FixedDate then
  begin
    FixedMonthCombo.ItemIndex := AHoliday.Month - 1;
    FixedDayCombo.ItemIndex := AHoliday.Day - 1;
  end else
  begin
    FloatDayCombo.ItemIndex := AHoliday.Day - 1;
    FloatDOWCombo.ItemIndex := AHoliday.DayOfWeek;
    FloatMonthCombo.ItemIndex := AHoliday.Month - 1;
  end;
  IsRestCB.Checked := AHoliday.IsRest;
end;

procedure THolidayPropForm.FixedDateRBClick(Sender: TObject);
var b : Boolean;
begin
  b := FixedDateRB.Checked;
  FixedDayCombo.Enabled := b;
  FixedMonthCombo.Enabled := b;
  FloatDayCombo.Enabled := not b;
  FloatDOWCombo.Enabled := not b;
  FloatMonthCombo.Enabled := not b;
end;

end.

