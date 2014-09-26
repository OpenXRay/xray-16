// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxDateUtil.pas' rev: 6.00

#ifndef mxDateUtilHPP
#define mxDateUtilHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxdateutil
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TDateOrder { doMDY, doDMY, doYMD };
#pragma option pop

#pragma option push -b-
enum TDayOfWeekName { Sun, Mon, Tue, Wed, Thu, Fri, Sat };
#pragma option pop

typedef Set<TDayOfWeekName, Sun, Sat>  TDaysOfWeek;

//-- var, const, procedure ---------------------------------------------------
#define DefaultDateOrder (TDateOrder)(1)
extern PACKAGE bool FourDigitYear;
extern PACKAGE Byte CenturyOffset;
extern PACKAGE System::TDateTime NullDate;
extern PACKAGE bool __fastcall IsLeapYear(int AYear);
extern PACKAGE int __fastcall DaysPerMonth(int AYear, int AMonth);
extern PACKAGE System::TDateTime __fastcall FirstDayOfNextMonth(void);
extern PACKAGE System::TDateTime __fastcall FirstDayOfPrevMonth(void);
extern PACKAGE System::TDateTime __fastcall LastDayOfPrevMonth(void);
extern PACKAGE Word __fastcall ExtractDay(System::TDateTime ADate);
extern PACKAGE Word __fastcall ExtractMonth(System::TDateTime ADate);
extern PACKAGE Word __fastcall ExtractYear(System::TDateTime ADate);
extern PACKAGE System::TDateTime __fastcall IncDate(System::TDateTime ADate, int Days, int Months, int Years);
extern PACKAGE void __fastcall DateDiff(System::TDateTime Date1, System::TDateTime Date2, Word &Days, Word &Months, Word &Years);
extern PACKAGE System::TDateTime __fastcall IncDay(System::TDateTime ADate, int Delta);
extern PACKAGE System::TDateTime __fastcall IncMonth(System::TDateTime ADate, int Delta);
extern PACKAGE System::TDateTime __fastcall IncYear(System::TDateTime ADate, int Delta);
extern PACKAGE double __fastcall MonthsBetween(System::TDateTime Date1, System::TDateTime Date2);
extern PACKAGE bool __fastcall ValidDate(System::TDateTime ADate);
extern PACKAGE int __fastcall DaysInPeriod(System::TDateTime Date1, System::TDateTime Date2);
extern PACKAGE int __fastcall DaysBetween(System::TDateTime Date1, System::TDateTime Date2);
extern PACKAGE System::TDateTime __fastcall IncTime(System::TDateTime ATime, int Hours, int Minutes, int Seconds, int MSecs);
extern PACKAGE System::TDateTime __fastcall IncHour(System::TDateTime ATime, int Delta);
extern PACKAGE System::TDateTime __fastcall IncMinute(System::TDateTime ATime, int Delta);
extern PACKAGE System::TDateTime __fastcall IncSecond(System::TDateTime ATime, int Delta);
extern PACKAGE System::TDateTime __fastcall IncMSec(System::TDateTime ATime, int Delta);
extern PACKAGE System::TDateTime __fastcall CutTime(System::TDateTime ADate);
extern PACKAGE Word __fastcall CurrentYear(void);
extern PACKAGE TDateOrder __fastcall GetDateOrder(const AnsiString DateFormat);
extern PACKAGE Byte __fastcall MonthFromName(const AnsiString S, Byte MaxLen);
extern PACKAGE System::TDateTime __fastcall StrToDateFmt(const AnsiString DateFormat, const AnsiString S);
extern PACKAGE System::TDateTime __fastcall StrToDateDef(const AnsiString S, System::TDateTime Default);
extern PACKAGE System::TDateTime __fastcall StrToDateFmtDef(const AnsiString DateFormat, const AnsiString S, System::TDateTime Default);
extern PACKAGE AnsiString __fastcall DefDateFormat(bool FourDigitYear);
extern PACKAGE AnsiString __fastcall DefDateMask(char BlanksChar, bool FourDigitYear);
extern PACKAGE AnsiString __fastcall FormatLongDate(System::TDateTime Value);
extern PACKAGE AnsiString __fastcall FormatLongDateTime(System::TDateTime Value);

}	/* namespace Mxdateutil */
using namespace Mxdateutil;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxDateUtil
