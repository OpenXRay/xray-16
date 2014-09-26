unit Fake;

interface

uses
  Forms,
  ElTray,
  SysUtils,
  Controls,
  Classes,
  Windows,
  Dialogs,
  Graphics,
  Messages,
  Menus,
  ElCalendar,
  ElCalendarDefs,
  ElTimers,
  ElTools,
  ElIni,
  ElStrUtils,
  ElShutdownWatcher,
  CalOptions,
  ElPromptDlg,
  frmCalend, (* in 'frmCalend.pas' {CalendarForm}, *)
  frmHolidayProp, (* in 'frmHolidayProp.pas' {HolidayPropForm}, *)
  frmCalConfig, (* in 'frmCalConfig.pas' {CalConfigForm}, *)
  frmDateProp; (* in  'frmDateProp.pas' {DatePropForm}; *)

type

  TFakeClass = class
  public
    FTrayIcon : TElTrayIcon;
    FTrayMenu : TPopupMenu;
    FTimer    : TElTimer;
    FLastDate : integer;
    FConfigIni: TElIniFile;
    FBmp      : TBitmap;
    constructor Create;
    destructor Destroy; override;
    procedure OnSettingsItemClick(Sender : TObject);
    procedure OnExitItemClick(Sender : TObject);
    procedure TimerTimer(Sender : TObject);
    procedure Run;
    procedure UpdateCalendar;
    procedure SaveOptions;
    procedure LoadOptions;
    procedure CheckRemindDays;
  end;

var FakeClass : TFakeClass;

implementation

procedure TFakeClass.CheckRemindDays;
var i : integer;
    aDay : TElRemindDay;
    Today : TDateTime;
begin
  ToDay := Date;
  for i := 0 to RemindDays.Count - 1 do
  begin
    aDay := TElRemindDay(RemindDays[i]);
    if (not aDay.Notified) and (aDay.Date >= Today) and (aDay.Date - Today <= aDay.RemindTime) then
    begin
      aDay.Notified := true;
      with TElPromptDialog.Create(nil) do
      try
        DialogCaption := 'EldoS TrayDays';
        Message := Format('%s is in %d days', [aDay.Name, aDay.RemindTime]);
        Buttons := [mbOk];
        Show;
      finally
        Free;
      end;
    end;
  end;
end;

procedure TFakeClass.UpdateCalendar;
var CalForm : TCalendarForm;
begin
  CalForm := TCalendarForm(FTrayIcon.ExtendedHintForm);

  with CalForm do
  begin
    Calendar.ShowPeriods    := Options.ShowPeriods;
    Calendar.StartOfWeek    := Options.WeekStart;
    Calendar.PeriodStart    := Options.PeriodStart;
    Calendar.PeriodLength   := Options.PeriodLength;
    Calendar.PeriodInterval := Options.PeriodInterval;
    Calendar.WeekEndColor   := Options.WeekEndColor;
    Calendar.HolidayColor   := Options.HolidayColor;
    Calendar.PeriodColor    := Options.PeriodsColor;
    Calendar.WeekEndDays    := Options.WeekEnds;
    Calendar.Holidays       := Options.Holidays;
    Calendar.ShowWeekNum    := Options.ShowWeekNums;
    UpdateRemindDays;
  end;
end;

procedure TFakeClass.OnSettingsItemClick(Sender : TObject);
begin
  with TCalConfigForm.Create(nil) do
  try
    SetData;
    if ShowModal = mrOk then
    begin
      GetData;
      UpdateCalendar;
    end;
  finally
    Free;
  end;
end;

procedure TFakeClass.OnExitItemClick(Sender : TObject);
begin
  PostMessage(Application.Handle, WM_QUIT, 0, 0);
end;

procedure TFakeClass.Run;
begin
  repeat
    Application.HandleMessage;
  until
    Application.Terminated;
end;

procedure TFakeClass.SaveOptions;
var St : TMemoryStream;
    S  : String;
    wed : TElWeekEndDay;
begin
  FConfigIni.ClearKey('\RemindDays');
  FConfigIni.WriteObject('\RemindDays', RemindDays);

  FConfigIni.WriteColor('\Settings', 'WeekEndColor', Options.WeekEndColor);
  FConfigIni.WriteColor('\Settings', 'HolidayColor', Options.HolidayColor);
  FConfigIni.WriteBool('\Settings', 'ShowHolidays', Options.ShowHolidays);
  FConfigIni.WriteBool('\Settings', 'ShowWeekNums', Options.ShowWeekNums);
  FConfigIni.WriteInteger('\Settings', 'WeekStart', Options.WeekStart);

  S := '';
  for wed := Sun to Sat do
        if wed in Options.WeekEnds then S := S + char(Smallint(wed)+65);

  FConfigIni.WriteString('\Settings', 'WeekEnds', S);

  St := TMemoryStream.Create;
  try
    Options.Holidays.SaveToStream(St);
    S := Data2Str(St.Memory, St.Size);
    FConfigIni.WriteString('\Settings', 'Holidays', S);
  finally
    St.Free;
  end;
end;

procedure TFakeClass.LoadOptions;
var i : integer;
    p1: Pointer;
    wed : TElWeekEndDay;
    Ss  : TElMemoryStream;
    S   : String;
begin
  Options.Holidays := TElHolidays.Create(nil);

  FConfigIni.ReadObject('\RemindDays', RemindDays);

  FConfigIni.ReadColor('\Settings', 'WeekEndColor', clRed, Options.WeekEndColor);
  FConfigIni.ReadColor('\Settings', 'HolidayColor', clFuchsia, Options.HolidayColor);
  FConfigIni.ReadBool('\Settings', 'ShowHolidays', false, Options.ShowHolidays);
  FConfigIni.ReadBool('\Settings', 'ShowWeekNums', false, Options.ShowWeekNums);
  FConfigIni.ReadInteger('\Settings', 'WeekStart', 0, Options.WeekStart);

  FConfigIni.ReadString('\Settings', 'WeekEnds', 'AG', S);

  Options.WeekEnds := [];
  for wed := Sun to Sat do
      if Pos(char(SmallInt(Wed)+65), S) > 0 then Include(Options.WeekEnds, Wed);

  FConfigIni.ReadString('\Settings', 'Holidays', '', S);

  if Str2Data(S, p1, i) then
  begin
    try
      SS := nil;
      try
        SS := TElMemoryStream.Create;
        SS.SetPointer(P1, i);
        Options.Holidays.LoadFromStream(SS);
      finally
        SS.Free;
      end;
    except
    end;
  end;
end;

procedure TFakeClass.TimerTimer;
var ST : TSystemTime;
    Bmp: TBitmap;
    Msk: TBitmap;
    II: TIconInfo;
    Icon: TIcon;
    C : TColor;
    R : TRect;
    Font: TBitmap;
    i, j: integer;
begin
  GetLocalTime(ST);
  if (FLastDate <> ST.wDay) then
  begin
    // generate new icon
    Font := TBitmap.Create;
    Font.Assign(FBmp);
    FConfigIni.ReadColor('\Settings', 'TrayIconColor', clBtnText, C);
    for i := 0 to Font.Width -1 do
    begin
      for j := 0 to Font.Height - 1 do
        if Font.Canvas.Pixels[i, j] = clBlack then
           Font.Canvas.Pixels[i, j] := C;
    end;

    Bmp := TBitmap.Create;
    try
      Bmp.Width := 16;
      Bmp.Height := 16;
      Bmp.PixelFormat := pf4bit;

      Msk := TBitmap.Create;
      try
        Msk.Width := 16;
        Msk.Height := 16;
        Msk.PixelFormat := pf1bit;
        Msk.Canvas.Brush.Color := clWhite;
        Msk.Canvas.FillRect(Rect(0,0,16, 16));


        R := Rect(0, 0, 16, 16);
        Msk.Canvas.Brush.Color := clWhite;
        Msk.Canvas.FillRect(R);
        R := Rect(0, 0, 15, 15);

        if ST.wDay > 9 then
        begin
          R := Rect(3, 1, 8, 7);
          bitblt(Bmp.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wDay div 10 * 5, 0, SRCCOPY);
        end;
        R := Rect(10, 1, 15, 7);
        bitblt(Bmp.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wDay mod 10 * 5, 0, SRCCOPY);

        R := Rect(3, 9, 8, 7);
        bitblt(Bmp.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wMonth div 10 * 5, 0, SRCCOPY);

        R := Rect(10, 9, 15, 7);
        bitblt(Bmp.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wMonth mod 10 * 5, 0, SRCCOPY);

        if ST.wDay > 9 then
        begin
          R := Rect(3, 1, 8, 7);
          bitblt(Msk.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wDay div 10 * 5, 0, SRCCOPY);
        end;
        R := Rect(10, 1, 15, 7);
        bitblt(Msk.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wDay mod 10 * 5, 0, SRCCOPY);

        R := Rect(3, 9, 8, 7);
        bitblt(Msk.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wMonth div 10 * 5, 0, SRCCOPY);

        R := Rect(10, 9, 15, 7);
        bitblt(Msk.Canvas.Handle, R.Left, R.Top, 5, 6, Font.Canvas.Handle, St.wMonth mod 10 * 5, 0, SRCCOPY);

        II.fIcon := true;
        II.xHotspot := 0;
        II.yHotspot := 0;
        II.hbmMask := Msk.Handle;
        II.hbmColor := Bmp.Handle;
        Icon := TIcon.Create;
        Icon.Handle := CreateIconIndirect(II);
        FTrayIcon.StaticIcon := Icon;
        Icon.Free;

      finally
        Msk.Free;
      end;
    finally
      Bmp.Free;
    end;
    CheckRemindDays;
  end;
  FLastDate := ST.wDay;
end;

constructor TFakeClass.Create;
var MI   : TMenuItem;
    R    : TRect;
    i    : integer;
begin
  inherited;
  FBmp      := TBitmap.Create;
  FBmp.LoadFromResourceName(HInstance, 'FONT');
  FConfigIni:= TElIniFile.Create(nil);
  FConfigIni.UseRegistry := true;
  FConfigIni.Path := 'SOFTWARE\EldoS\TrayDays';
  LoadOptions;

  FTimer    := TElTimer.Create;
  FTimer.OnTimer := TimerTimer;

  FTrayMenu := TPopupMenu.Create(nil);
  MI := Menus.NewItem('&Settings', 0, false, true, OnSettingsItemClick, 0, 'SettingsItem');
  FTrayMenu.Items.Add(MI);
  MI := Menus.NewItem('E&xit', 0, false, true, OnExitItemClick, 0, 'ExitItem');
  FTrayMenu.Items.Add(MI);

  FTrayIcon := TElTrayIcon.Create(nil);
  FTrayIcon.ExtendedHintInteractive := true;
  FTrayIcon.ExtendedHint := 'TCalendarForm';
  FTrayIcon.ExtendedHintDelay := 500;
  FTrayIcon.ExtHintWndStyle := Cardinal(WS_POPUP or WS_BORDER  or WS_THICKFRAME);
  FTrayIcon.ExtHintWndExStyle := WS_EX_TOPMOST or WS_EX_TOOLWINDOW;

  SendMessage(FTrayIcon.ExtendedHintForm.Handle, CM_ShowingChanged, 0, 0);
  R := FTrayIcon.ExtendedHintForm.BoundsRect;
  FConfigIni.ReadRect('\Settings', 'Size', R, R);
  with FTrayIcon.ExtendedHintForm do
       SetBounds(Left, Top, R.Right - R.Left, R.Bottom - R.Top);
  ShowWindow(Application.Handle, SW_HIDE);
  UpdateCalendar;
  if FConfigIni.ReadInteger('\Settings', 'BottomPanelHeight', TCalendarForm(FTrayIcon.ExtendedHintForm).BottomPanel.Height, i) then
     TCalendarForm(FTrayIcon.ExtendedHintForm).BottomPanel.Height := i;

  FTrayIcon.PopupMenu := FTrayMenu;
  TimerTimer(Self);
  FTrayIcon.Enabled := true;
  FTimer.Enabled := true;
end;

destructor TFakeClass.Destroy;
begin
  FConfigIni.WriteInteger('\Settings', 'BottomPanelHeight', TCalendarForm(FTrayIcon.ExtendedHintForm).BottomPanel.Height);
  FConfigIni.WriteRect('\Settings', 'Size', FTrayIcon.ExtendedHintForm.BoundsRect);
  SaveOptions;
  FTrayIcon.Free;
  FTrayMenu.Free;
  FConfigIni.Free;
  FBmp.Free;
  inherited;
end;

end.
