// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTools.pas' rev: 6.00

#ifndef ElToolsHPP
#define ElToolsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <MMSystem.hpp>	// Pascal unit
#include <ActiveX.hpp>	// Pascal unit
#include <ShellAPI.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltools
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TDirectMemoryStream;
class PASCALIMPLEMENTATION TDirectMemoryStream : public Classes::TMemoryStream 
{
	typedef Classes::TMemoryStream inherited;
	
public:
	HIDESBASE void __fastcall SetPointer(void * Ptr, int Size);
public:
	#pragma option push -w-inl
	/* TMemoryStream.Destroy */ inline __fastcall virtual ~TDirectMemoryStream(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TDirectMemoryStream(void) : Classes::TMemoryStream() { }
	#pragma option pop
	
};


typedef TDirectMemoryStream TElMemoryStream;
;

class DELPHICLASS TNamedFileStream;
class PASCALIMPLEMENTATION TNamedFileStream : public Classes::TFileStream 
{
	typedef Classes::TFileStream inherited;
	
private:
	AnsiString FFileName;
	
public:
	__fastcall TNamedFileStream(const AnsiString FileName, Word Mode);
	__property AnsiString FileName = {read=FFileName};
public:
	#pragma option push -w-inl
	/* TFileStream.Destroy */ inline __fastcall virtual ~TNamedFileStream(void) { }
	#pragma option pop
	
};


#pragma pack(push, 2)
struct TReducedDateTime
{
	Word Year;
	Word Month;
	Word DOW;
	Word Day;
	Word Hour;
	Word Min;
} ;
#pragma pack(pop)

typedef void __fastcall (__closure *TMsgPumpRoutineEvent)(void);

typedef void __fastcall (__closure *TElWndMethod)(Messages::TMessage &Message);

typedef AnsiString ElTools__3[38];

//-- var, const, procedure ---------------------------------------------------
static const Word MAXPATHLEN = 0x400;
extern PACKAGE TMsgPumpRoutineEvent OnMessagePump;
extern PACKAGE bool IsLinux;
extern PACKAGE bool IsWin95;
extern PACKAGE bool IsWinNT;
extern PACKAGE bool IsWin2000;
extern PACKAGE bool IsWinNTUp;
extern PACKAGE bool IsWin2000Up;
extern PACKAGE bool IsWinXP;
extern PACKAGE bool IsWinXPUp;
extern PACKAGE bool IsWin95OSR2;
extern PACKAGE bool IsWin98;
extern PACKAGE bool IsWinME;
extern PACKAGE bool IsWin98Up;
extern PACKAGE HWND LastWin;
extern PACKAGE unsigned LastProcessID;
extern PACKAGE AnsiString ElementFormatList[38];
extern PACKAGE void __fastcall PlaySound(char * Name, unsigned Flags1, unsigned Flags2);
extern PACKAGE Word __fastcall swapInt16(Word w);
extern PACKAGE int __fastcall swapInt32(int i);
extern PACKAGE double __fastcall SwapDouble(double d);
extern PACKAGE int __fastcall GetSysStartDayOfWeek(void);
extern PACKAGE System::TDateTime __fastcall GetTime(System::TDateTime DateTime);
extern PACKAGE AnsiString __fastcall GetCommonAppDataFolder();
extern PACKAGE AnsiString __fastcall GetUserAppDataFolder();
extern PACKAGE AnsiString __fastcall GetUserLocalAppDataFolder();
extern PACKAGE AnsiString __fastcall GetSpecialFolder(const int CSIDL);
extern PACKAGE AnsiString __fastcall IncludeTrailingBackslash2(const AnsiString Path);
extern PACKAGE AnsiString __fastcall GetTempFile();
extern PACKAGE bool __fastcall TimeInMask(AnsiString CronMask, const TReducedDateTime &T);
extern PACKAGE AnsiString __fastcall GetSystemDir();
extern PACKAGE AnsiString __fastcall GetShortPath(AnsiString Path);
extern PACKAGE AnsiString __fastcall GetTempDir();
extern PACKAGE AnsiString __fastcall GetWindowsDir();
extern PACKAGE AnsiString __fastcall GetFormattedTimeString(System::TDateTime ADate, AnsiString Format);
extern PACKAGE int __fastcall DayNumber(int AYear, int AMonth, int ADay);
extern PACKAGE int __fastcall WeekNumber(int AYear, int AMonth, int ADay);
extern PACKAGE System::TDateTime __fastcall ExtractTime(System::TDateTime ATime);
extern PACKAGE System::TDateTime __fastcall ExtractDate(System::TDateTime ATime);
extern PACKAGE System::TDateTime __fastcall IncTime(System::TDateTime ATime, int Hours, int Minutes, int Seconds, int MSecs);
extern PACKAGE void __fastcall CenterRects(int WS, int WT, int HS, int HT, Types::TRect &R);
extern PACKAGE bool __fastcall ReadTextFromStream(Classes::TStream* S, AnsiString &Data);
extern PACKAGE void __fastcall WriteTextToStream(Classes::TStream* S, AnsiString Data);
extern PACKAGE AnsiString __fastcall encode_line(const void *buf, int size);
extern PACKAGE bool __fastcall FileNameValid(AnsiString FileName);
extern PACKAGE __int64 __fastcall GetFileSize(const AnsiString FileName);
extern PACKAGE System::TDateTime __fastcall FileDateTime(const AnsiString FileName);
extern PACKAGE bool __fastcall CreateFile(AnsiString FileName);
extern PACKAGE void __fastcall EnsureDirExists(AnsiString RootName, AnsiString DirName);
extern PACKAGE bool __fastcall DirExists(AnsiString DirName);
extern PACKAGE bool __fastcall PurgeDir(AnsiString DirName);
extern PACKAGE unsigned __fastcall RunProgram(AnsiString StartName, AnsiString Params, AnsiString StartDir);
extern PACKAGE TReducedDateTime __fastcall MakeReducedDT(Word Year, Word Month, Word Day, Word DOW, Word Hour, Word Min);
extern PACKAGE bool __fastcall CompareReducedDT(const TReducedDateTime &T1, const TReducedDateTime &T2);
extern PACKAGE TReducedDateTime __fastcall DateTimeToReduced(System::TDateTime T);
extern PACKAGE System::TDateTime __fastcall ReducedToDateTime(const TReducedDateTime &T);
extern PACKAGE bool __fastcall IsBIn(int index, Byte storage);
extern PACKAGE int __fastcall Sign(int a);
extern PACKAGE bool __fastcall InRangeF(double L, double R, double x);
extern PACKAGE bool __fastcall InRange(int L, int R, int x);
extern PACKAGE int __fastcall Max(int a, int b);
extern PACKAGE int __fastcall Min(int a, int b);
extern PACKAGE System::TDateTime __fastcall SubtractTimes(System::TDateTime Time1, System::TDateTime Time2);
extern PACKAGE bool __fastcall RangesIntersect(int L1, int R1, int L2, int R2);
extern PACKAGE bool __fastcall WriteStringToStream(Classes::TStream* S, AnsiString Str);
extern PACKAGE bool __fastcall ReadStringFromStream(Classes::TStream* S, AnsiString &Str);
extern PACKAGE bool __fastcall WriteWideStringToStream(Classes::TStream* S, WideString Str);
extern PACKAGE bool __fastcall ReadWideStringFromStream(Classes::TStream* S, WideString &Str);
extern PACKAGE bool __fastcall WriteFStringToStream(Classes::TStream* S, WideString Str);
extern PACKAGE bool __fastcall ReadFStringFromStream(Classes::TStream* S, WideString &Str);
extern PACKAGE unsigned __fastcall ElDateTimeToSeconds(int ADay, int AMonth, int AYear, int AHours, int AMinute, int ASecond);
extern PACKAGE void __fastcall ElSecondsToDateTime(unsigned Seconds, int &ADay, int &AMonth, int &AYear, int &AHours, int &AMinute, int &ASecond);
extern PACKAGE int __fastcall DateToJulianDays(int ADay, int AMonth, int AYear);
extern PACKAGE void __fastcall JulianDaysToDate(int &ADay, int &AMonth, int &AYear, int JulianDate);
extern PACKAGE int __fastcall ElDayOfWeek(int ADay, int AMonth, int AYear);
extern PACKAGE int __fastcall DaysPerMonth(int AYear, int AMonth);
extern PACKAGE System::TDateTime __fastcall IncDate(System::TDateTime ADate, int Days, int Months, int Years);
extern PACKAGE System::TDateTime __fastcall NowToUTC(void);
extern PACKAGE void __fastcall UTCToZoneLocal(Windows::PTimeZoneInformation lpTimeZoneInformation, const _SYSTEMTIME &lpUniversalTime, _SYSTEMTIME &lpLocalTime);
extern PACKAGE void __fastcall ZoneLocalToUTC(Windows::PTimeZoneInformation lpTimeZoneInformation, _SYSTEMTIME &lpUniversalTime, const _SYSTEMTIME &lpLocalTime);
extern PACKAGE void __fastcall ElSystemTimeToTzSpecificLocalTime(Windows::PTimeZoneInformation lpTimeZoneInformation, _SYSTEMTIME &lpUniversalTime, _SYSTEMTIME &lpLocalTime);
extern PACKAGE int __fastcall ZoneIDtoBias(AnsiString ZoneID);
extern PACKAGE bool __fastcall SetPrivilege(AnsiString sPrivilegeName, bool bEnabled);
extern PACKAGE HWND __fastcall WindowExists(AnsiString ClassName, AnsiString Caption, bool ExactMatch);
extern PACKAGE HWND __fastcall TopWindowExists(AnsiString ClassName, AnsiString Caption, bool ExactMatch);
extern PACKAGE AnsiString __fastcall AppendSlash(const AnsiString PathName);
extern PACKAGE AnsiString __fastcall GetModulePath();
extern PACKAGE AnsiString __fastcall GetComputerName();
extern PACKAGE AnsiString __fastcall RectToString(const Types::TRect &Rect);
extern PACKAGE Types::TRect __fastcall StringToRect(AnsiString AString);
extern PACKAGE void __fastcall ValFloat(AnsiString Value, double Result, int &Error);
extern PACKAGE AnsiString __fastcall StrFloat(double Value);
extern PACKAGE HWND __fastcall XAllocateHWND(System::TObject* Obj, TElWndMethod WndMethod);
extern PACKAGE void __fastcall XDeallocateHWND(HWND Wnd);
extern PACKAGE int __fastcall GetKeysState(void);
extern PACKAGE Classes::TShiftState __fastcall GetShiftState(void);

}	/* namespace Eltools */
using namespace Eltools;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTools
