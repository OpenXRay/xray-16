unit frmCalConfig;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ComCtrls, StdCtrls, ElTree, ElPopBtn, ElACtrls,
  ElCalendar, frmHolidayProp, ElBtnCtl, ElCheckCtl, ElSpin, ElFlatCtl,
  ElClrCmb, CalOptions, ElCalendarDefs, ElPgCtl, ElXPThemedControl;

type                                           
  TCalConfigForm = class(TForm)
    OpenDlg: TOpenDialog;
    SaveDlg: TSaveDialog;
    OkBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    Pages: TElPageControl;
    GeneralPage: TElTabSheet;
    HolidaysPage: TElTabSheet;
    Label1: TLabel;
    Label2: TLabel;
    Label4: TLabel;
    WeekendColorCombo: TElColorCombo;
    WeekNumsCB: TElCheckBox;
    WeekEndList: TElAdvancedListBox;
    StartDayCombo: TElAdvancedComboBox;
    Label3: TLabel;
    HoliColorCombo: TElColorCombo;
    HolidaysList: TElTree;
    HolidayAddBtn: TElPopupButton;
    HolidayRemoveBtn: TElPopupButton;
    HolidayModifyBtn: TElPopupButton;
    HolidaysSaveBtn: TElPopupButton;
    HolidaysLoadBtn: TElPopupButton;
    HolidaysCB: TElCheckBox;
    procedure HolidayAddBtnClick(Sender: TObject);
    procedure HolidayModifyBtnClick(Sender: TObject);
    procedure HolidaysListItemFocused(Sender: TObject);
    procedure HolidayRemoveBtnClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure HolidaysSaveBtnClick(Sender: TObject);
    procedure HolidaysLoadBtnClick(Sender: TObject);
    procedure HolidaysListItemDeletion(Sender: TObject; Item: TElTreeItem);
  private
    { Private declarations }
    Holidays : TElHolidays;
  public
    procedure AddHolidayToList(AHoliday : TElHoliday);
    procedure GetData;
    procedure SetData;
  end;

var
  CalConfigForm: TCalConfigForm;

implementation

{$R *.DFM}

procedure TCalConfigForm.GetData;
var I   : Integer;
    WED : TElWeekEndDays;
begin
  with Options do
  begin
    Holidays.Assign(Self.Holidays);
    Options.ShowWeekNums := WeekNumsCB.Checked;
    ShowHolidays := HolidaysCB.Checked;
    Options.WeekStart  := TDayOfWeek(StartDayCombo.ItemIndex);
    WeekEndColor := WeekendColorCombo.SelectedColor;
    HolidayColor := HoliColorCombo.SelectedColor;
    Wed := [];
    for i := 0 to 6 do    // Iterate
    begin
      if WeekEndList.Selected[i]
         then Include(WED, TElWeekEndDay(i))
         else Exclude(WED, TElWeekEndDay(i));
    end;    // for
    Options.WeekEnds := WED;
  end;
end;                        

procedure TCalConfigForm.SetData;
var I: Integer;
begin
  Holidays.Assign(Options.Holidays);
  for i := 0 to 6 do    // Iterate
    if TElWeekEndDay(i) in Options.WeekEnds then WeekEndList.Selected[i] := true;
  for i := 0 to Holidays.Count - 1 do AddHolidayToList(Holidays.Items[i]);
  WeekNumsCB.Checked    := Options.ShowWeekNums;
  HolidaysCB.Checked    := Options.ShowHolidays;

  StartDayCombo.ItemIndex         := Integer(Options.WeekStart);
  WeekendColorCombo.SelectedColor := Options.WeekEndColor;
  HoliColorCombo.SelectedColor    := Options.HolidayColor;
end;

procedure TCalConfigForm.AddHolidayToList(AHoliday : TElHoliday);
var Item : TElTreeItem;
begin
  Item := HolidaysList.Items.AddItem(nil);
  Item.ColumnText.Add(AHoliday.Description);
  if AHoliday.FixedDate
     then Item.Text := IntToStr(AHoliday.Day) + ' ' + LongMonthNames[AHoliday.Month]
     else Item.Text := IntToStr(AHoliday.Day) + ' ' + LongDayNames[AHoliday.DayOfWeek  + 1] +
                                                ' ' + LongMonthNames[AHoliday.Month];
  Item.Data := AHoliday;
end;

procedure TCalConfigForm.HolidayAddBtnClick(Sender: TObject);
var AHoliday : TELHoliday;
begin
  AHoliday := Holidays.Add;
  HolidayPropForm := THolidayPropForm.Create(nil);
  HolidayPropForm.AHoliday := AHoliday;
  HolidayPropForm.SetData;
  if HolidayPropForm.ShowModal = mrOk then
  begin
    HolidayPropForm.GetData;
    AddHolidayToList(AHoliday);
  end
  else AHoliday.Free;
  HolidayPropForm.Free;
end;

procedure TCalConfigForm.HolidayModifyBtnClick(Sender: TObject);
var AHoliday : TELHoliday;
begin
  if HolidaysList.ItemFocused = nil then Exit;
  AHoliday := TElHoliday(HolidaysList.ItemFocused.Data);
  HolidayPropForm := THolidayPropForm.Create(nil);
  HolidayPropForm.AHoliday := AHoliday;
  HolidayPropForm.SetData;
  if HolidayPropForm.ShowModal = mrOk then
  begin
    HolidayPropForm.GetData;
    HolidaysList.ItemFocused.ColumnText.Clear;
    HolidaysList.ItemFocused.ColumnText.Add(AHoliday.Description);
    if AHoliday.FixedDate
       then HolidaysList.ItemFocused.Text := IntToStr(AHoliday.Day) + ' ' + LongMonthNames[AHoliday.Month]
       else HolidaysList.ItemFocused.Text := IntToStr(AHoliday.Day) + ' ' + LongDayNames[AHoliday.DayOfWeek + 1] +
                                                                      ' ' + LongMonthNames[AHoliday.Month];
  end;
  HolidayPropForm.Free;
end;

procedure TCalConfigForm.HolidaysListItemFocused(Sender: TObject);
begin
  HolidayModifyBtn.Enabled := HolidaysList.ItemFocused <> nil;
  HolidayRemoveBtn.Enabled := HolidaysList.ItemFocused <> nil;
end;

procedure TCalConfigForm.HolidayRemoveBtnClick(Sender: TObject);
begin
  if HolidaysList.ItemFocused = nil then Exit;
  HolidaysList.Items.DeleteItem(HolidaysList.ItemFocused);
end;

procedure TCalConfigForm.FormCreate(Sender: TObject);
begin
  Holidays := TElHolidays.Create(Self);
end;

procedure TCalConfigForm.FormDestroy(Sender: TObject);
begin
  Holidays.Free;
  Holidays := nil;
end;

procedure TCalConfigForm.HolidaysSaveBtnClick(Sender: TObject);
var Stream : TFileStream;
begin
  if SaveDlg.Execute then
  begin
    Stream := nil;
    try
      try
        Stream := TFileStream.Create(SaveDlg.FileName, fmCreate or fmShareDenyWrite);
        Holidays.SaveToStream(Stream);
      finally // wrap up
        Stream.Free;
      end;    // try/finally
    except
      MessageDlg('Failed to save holidays', mtError, [mbOk], 0);
    end;
  end;
end;

procedure TCalConfigForm.HolidaysLoadBtnClick(Sender: TObject);
var Stream : TFileStream;
    i : integer;
begin
  if OpenDlg.Execute then
  begin
    Stream := nil;
    try
      try
        Stream := TFileStream.Create(OpenDlg.FileName, fmOpenRead or fmShareDenyWrite);
        Holidays.LoadFromStream(Stream);
        HolidaysList.Items.Clear;
        for i := 0 to Holidays.Count - 1 do AddHolidayToList(Holidays.Items[i]);
      finally // wrap up
        Stream.Free;
      end;    // try/finally
    except
      MessageDlg('Failed to load holidays', mtError, [mbOk], 0);
    end;
  end;
end;

procedure TCalConfigForm.HolidaysListItemDeletion(Sender: TObject;
  Item: TElTreeItem);
begin
  if Assigned(Item) and Assigned(Holidays) then TElHoliday(Item.Data).Free;
end;

end.

