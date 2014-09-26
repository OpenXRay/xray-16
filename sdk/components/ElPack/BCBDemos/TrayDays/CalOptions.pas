unit CalOptions;

interface

uses
  ElCalendar, Graphics, SysUtils, ElCalendarDefs;

type TCalOpts = record
       ShowWarning : boolean;
       WeekEndColor,
       HolidayColor,
       PeriodsColor: TColor;
       ShowHolidays,
       ShowPeriods : boolean;
       PeriodInterval,
       PeriodLength: integer;
       PeriodStart : TDateTime;
       ShowWeekNums: boolean;
       WeekStart   : integer;
       WeekEnds    : TElWeekendDays;
       Holidays    : TElHolidays;
     end;

var Options : TCalOpts;

implementation

initialization
  with Options do
  begin
    ShowWarning := true;
    WeekEndColor := clRed;
    HolidayColor := clRed;
    PeriodsColor := clAqua;
    ShowHolidays := true;
    ShowPeriods := false;
    PeriodInterval := 28;
    PeriodLength := 5;
    PeriodStart := EncodeDate(1999, 1, 1);
    ShowWeekNums := false;
    WeekStart := 0;
    WeekEnds := [Sat, Sun];
    Holidays := TElHolidays.Create(nil);
  end;

finalization
  Options.Holidays.Free;

end.

