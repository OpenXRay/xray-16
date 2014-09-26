unit frmCalend;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  Grids, ElCalendar, StdCtrls, ElACtrls, ElSpin, ElBtnCtl, ElPopBtn,
  ExtCtrls, ElPanel, ElSplit, ElObjList, ElPromptDlg, ElCombos,
  ElHTMLView, ElXPThemedControl;

type

  TElRemindDay = class;               

  TCalendarForm = class(TForm)
    Panel1: TPanel;
    PrevMonBtn: TElPopupButton;
    PrevYearBtn: TElPopupButton;
    NextMonBtn: TElPopupButton;
    NextYearBtn: TElPopupButton;
    YearSpin: TElSpinEdit;
    Calendar: TElCalendar;
    BottomPanel: TElPanel;
    ReminderView: TElHTMLView;
    Splitter: TElSplitter;
    btnAddDay: TElPopupButton;
    MonthCombo: TElComboBox;
    procedure PrevYearBtnClick(Sender: TObject);
    procedure NextYearBtnClick(Sender: TObject);
    procedure NextMonBtnClick(Sender: TObject);
    procedure PrevMonBtnClick(Sender: TObject);
    procedure MonthComboChange(Sender: TObject);
    procedure YearSpinChange(Sender: TObject);
    procedure CalendarChange(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure ReminderViewLinkClick(Sender: TObject; HRef: TElFString);
    procedure ReminderViewImageNeeded(Sender: TObject; Src: TElFString;
      var Image: TBitmap);
    procedure btnAddDayClick(Sender: TObject);
  private
  public
    procedure CreateParams(var Params: TCreateParams); override;
    procedure SetNames;
    procedure UpdateLabel;
    procedure UpdateRemindDays;
    function ConfigureRemindDay(ADay : TElRemindDay) : boolean;
  end;

  TElRemindDay = class(TElObjectListItem)
  private
    FRemindTime : integer;
    FName       : string;
    FDate       : TDateTime;
    FNotified   : boolean;
  published
    property Name : string read FName write FName;
    property RemindTime : integer read FRemindTime write FRemindTime;
    property Date : TDateTime read FDate write FDate;
    property Notified : boolean read FNotified write FNotified;
  end;

var

  CalendarForm: TCalendarForm;
  RemindDays  : TElObjectList;
  KillBmp     : TBitmap;

{$R 'killdate.res'}

const OrigHeight = -11;
      OrigWidth  = 193;

implementation

{$R *.DFM}

uses frmDateProp;

procedure TCalendarForm.SetNames;
var
  i : integer;
begin
  for i := 1 to 12 do
    MonthCombo.Items.Add(LongMonthNames[i]);
end;

procedure TCalendarForm.UpdateLabel;
begin
  MonthCombo.ItemIndex := Calendar.Month - 1;
  YearSpin.Value := Calendar.Year;
end;

procedure TCalendarForm.PrevYearBtnClick(Sender: TObject);
begin
  Calendar.Year := Calendar.Year - 1;
  UpdateLabel;
end;

procedure TCalendarForm.NextYearBtnClick(Sender: TObject);
begin
  Calendar.Year := Calendar.Year + 1;
  UpdateLabel;
end;

procedure TCalendarForm.NextMonBtnClick(Sender: TObject);
begin
  if Calendar.Month = 12 then
  begin
    Calendar.Month := 1;
    Calendar.Year := Calendar.Year + 1;
  end
  else
    Calendar.Month := Calendar.Month + 1;
  UpdateLabel;
end;

procedure TCalendarForm.PrevMonBtnClick(Sender: TObject);
begin
  if Calendar.Month = 1 then
  begin
    Calendar.Month := 12;
    Calendar.Year := Calendar.Year - 1;
  end
  else
    Calendar.Month := Calendar.Month - 1;
  UpdateLabel;
end;

procedure TCalendarForm.MonthComboChange(Sender: TObject);
begin
  if (MonthCombo.ItemIndex >= 0) and (MonthCombo.ItemIndex < 12) then
    Calendar.Month := MonthCombo.ItemIndex + 1;
end;

procedure TCalendarForm.YearSpinChange(Sender: TObject);
var
  FSaveYear : integer;
begin
  FSaveYear := Calendar.Year;
  try
    Calendar.Year := Trunc(YearSpin.Value);
  except
    Calendar.Year := FSaveYear;
  end;
end;

procedure TCalendarForm.CalendarChange(Sender: TObject);
begin
  UpdateLabel;
end;

procedure TCalendarForm.CreateParams(var Params: TCreateParams);  { protected }
begin
  inherited;
  Params.Style := WS_POPUP or WS_BORDER or WS_THICKFRAME;
end;  { CreateParams }

procedure TCalendarForm.FormCreate(Sender: TObject);
begin
  //SetNames;
end;

procedure TCalendarForm.FormShow(Sender: TObject);
begin
  UpdateLabel;
end;

procedure TCalendarForm.FormResize(Sender: TObject);
begin
  Calendar.Font.Height := MulDiv(OrigHeight, ClientWidth, OrigWidth);
end;

function TCalendarForm.ConfigureRemindDay(ADay : TElRemindDay) : boolean;
begin
  result := false;
  with TDatePropForm.Create(nil) do
  try
    SetData(ADay);
    if ShowModal = mrOk then
    begin
      GetData(ADay);
      result := true; 
    end;
  finally
    free;
  end;
end;

procedure TCalendarForm.UpdateRemindDays;
var s   : string;
    Day : TElRemindDay;
    Today: TDateTime;
    i   : integer;
begin
  S := '';
  Today := Trunc(Now);
  for i := 0 to RemindDays.Count - 1 do
  begin
    Day := TElRemindDay(RemindDays[i]);
    if Day.Date < Today then
       S := S + Format('%d days after <a href="%d">%s</a> ', [Trunc(Today - Day.Date), Integer(Pointer(Day)), Day.Name])
    else
    if Day.Date > Today then
       S := S + Format('%d days until <a href="%d">%s</a> ', [Trunc(Day.Date - Today), Integer(Pointer(Day)), Day.Name])
    else
       S := S + Format('Today is <a href="%d">%s</a> ', [Integer(Pointer(Day)), Day.Name]);
    S := S + Format('<a href="-%d"><img src="killimage"></a><br>', [Integer(Pointer(Day))]);
  end;
  ReminderView.Caption := S;
end;

procedure TCalendarForm.ReminderViewLinkClick(Sender: TObject;
  HRef: TElFString);
var i : integer;
begin
  i := StrToIntDef(HRef, 0);
  if i > 0 then
  begin
    if ConfigureRemindDay(TElRemindDay(Pointer(i))) then
      UpdateRemindDays;
  end
  else
  if i < 0 then
  begin
    if ElMessageDlg(Format('Do you want to delete %s?', [TElRemindDay(Pointer(-i)).Name]), mtWarning, [mbYes, mbNo], 0) = mrYes then
    begin
      RemindDays.Remove(Pointer(-i));
      UpdateRemindDays;
    end;
  end;
end;

procedure TCalendarForm.ReminderViewImageNeeded(Sender: TObject;
  Src: TElFString; var Image: TBitmap);
begin
  if src = 'killimage' then
     Image := KillBmp
  else
     Image := nil;
end;

procedure TCalendarForm.btnAddDayClick(Sender: TObject);
var ADay : TElRemindDay;
begin
  ADay := TElRemindDay(RemindDays.Add);
  ADay.Date := Trunc(Now + 1);
  if ConfigureRemindDay(ADay) then
  begin
    UpdateRemindDays;
  end else
  begin
    RemindDays.Remove(ADay);
  end;
end;

initialization

  RegisterClass(TCalendarForm);

  RemindDays := TElObjectList.Create(nil, TElRemindDay);
  RemindDays.AutoClearObjects := true;
  KillBmp := TBitmap.Create;
  KillBmp.LoadFromResourceName(HInstance, 'KILLDATEIMAGE');

finalization

  KillBmp.Free;
  RemindDays.Free;

end.

