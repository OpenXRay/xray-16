// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElClock.pas' rev: 6.00

#ifndef ElClockHPP
#define ElClockHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Forms.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elclock
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElClock;
class PASCALIMPLEMENTATION TElClock : public Elpanel::TElPanel 
{
	typedef Elpanel::TElPanel inherited;
	
protected:
	bool FTimerPaused;
	System::TDateTime FStartTime;
	System::TDateTime FPauseTime;
	bool FIsTimer;
	bool FTimerActive;
	bool FShowDate;
	bool FShowHint;
	Extctrls::TTimer* FTimer;
	_TIME_ZONE_INFORMATION FTZone;
	bool FLocal;
	bool FSeconds;
	bool FAMPM;
	AnsiString FCaption;
	bool FUseBias;
	int FBias;
	bool FShowWeekDay;
	bool FUseCustomFormat;
	AnsiString FCustomFormat;
	bool FShowDaysInTimer;
	bool FCountdownActive;
	Classes::TNotifyEvent FOnCountdownDone;
	Classes::TNotifyEvent FOnCountdownTick;
	bool FCountdownPaused;
	int FCountdownTime;
	int FSaveCDTime;
	bool FIsCountdown;
	WideString FDummyStr;
	void __fastcall SetIsCountdown(bool newValue);
	void __fastcall SetCountdownTime(int newValue);
	void __fastcall SetCountdownPaused(bool newValue);
	void __fastcall SetCountdownActive(bool newValue);
	void __fastcall SetShowDaysInTimer(bool newValue);
	void __fastcall SetUseCustomFormat(bool newValue);
	void __fastcall SetCustomFormat(AnsiString newValue);
	void __fastcall SetShowWeekDay(bool newValue);
	void __fastcall SetUseBias(bool newValue);
	void __fastcall SetBias(int newValue);
	bool __fastcall GetTimer(void);
	void __fastcall SetTimer(bool value);
	void __fastcall OnTimer(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Msg);
	void __fastcall SetShowDate(bool newValue);
	HIDESBASE void __fastcall SetShowHint(bool newValue);
	void __fastcall SetIsTimer(bool newValue);
	System::TDateTime __fastcall GetTimeElapsed(void);
	void __fastcall SetTimerActive(bool newValue);
	void __fastcall SetTimerPaused(bool newValue);
	void __fastcall CreateTimer(void);
	void __fastcall PaintBorders(Graphics::TCanvas* Canvas, Types::TRect &R);
	virtual void __fastcall Paint(void);
	void __fastcall InheritedPaint(void);
	virtual void __fastcall TriggerCountdownDoneEvent(void);
	virtual void __fastcall TriggerCountdownTickEvent(void);
	
public:
	__fastcall virtual TElClock(Classes::TComponent* AOwner);
	__fastcall virtual ~TElClock(void);
	virtual void __fastcall Kick(void);
	virtual void __fastcall GetTime(_SYSTEMTIME &Time);
	void __fastcall ResetTimer(void);
	__property System::TDateTime TimeElapsed = {read=GetTimeElapsed};
	__property _TIME_ZONE_INFORMATION TimeZone = {read=FTZone, write=FTZone};
	
__published:
	__property WideString Caption = {read=FDummyStr};
	__property bool LocalTime = {read=FLocal, write=FLocal, default=1};
	__property bool ShowWeekDay = {read=FShowWeekDay, write=SetShowWeekDay, nodefault};
	__property bool ShowSeconds = {read=FSeconds, write=FSeconds, nodefault};
	__property bool ShowDate = {read=FShowDate, write=SetShowDate, nodefault};
	__property bool AM_PM = {read=FAMPM, write=FAMPM, nodefault};
	__property bool Labels = {read=FShowHint, write=SetShowHint, nodefault};
	__property bool UseBias = {read=FUseBias, write=SetUseBias, nodefault};
	__property int Bias = {read=FBias, write=SetBias, nodefault};
	__property bool UseCustomFormat = {read=FUseCustomFormat, write=SetUseCustomFormat, nodefault};
	__property AnsiString CustomFormat = {read=FCustomFormat, write=SetCustomFormat};
	__property bool IsTimer = {read=FIsTimer, write=SetIsTimer, nodefault};
	__property bool TimerActive = {read=FTimerActive, write=SetTimerActive, nodefault};
	__property bool TimerPaused = {read=FTimerPaused, write=SetTimerPaused, default=0};
	__property bool ShowDaysInTimer = {read=FShowDaysInTimer, write=SetShowDaysInTimer, nodefault};
	__property bool IsCountdown = {read=FIsCountdown, write=SetIsCountdown, nodefault};
	__property int CountdownTime = {read=FCountdownTime, write=SetCountdownTime, nodefault};
	__property bool CountdownActive = {read=FCountdownActive, write=SetCountdownActive, nodefault};
	__property bool CountdownPaused = {read=FCountdownPaused, write=SetCountdownPaused, nodefault};
	__property bool UseTimer = {read=GetTimer, write=SetTimer, nodefault};
	__property Classes::TNotifyEvent OnCountdownDone = {read=FOnCountdownDone, write=FOnCountdownDone};
	__property Classes::TNotifyEvent OnCountdownTick = {read=FOnCountdownTick, write=FOnCountdownTick};
	__property Align ;
	__property Alignment  = {default=2};
	__property BevelInner  = {default=1};
	__property BevelOuter  = {default=2};
	__property BevelWidth  = {default=1};
	__property BorderWidth  = {default=0};
	__property BorderStyle  = {default=0};
	__property Color  = {default=-2147483633};
	__property Ctl3D ;
	__property Cursor  = {default=0};
	__property Font ;
	__property Hint ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseMove ;
	__property OnMouseDown ;
	__property OnMouseUp ;
	__property OnDragOver ;
	__property OnDragDrop ;
	__property OnEndDrag ;
	__property OnStartDrag ;
	__property OnResize ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElClock(HWND ParentWindow) : Elpanel::TElPanel(ParentWindow) { }
	#pragma option pop
	
};


struct TShortTZ;
typedef TShortTZ *PShortTZ;

#pragma pack(push, 1)
struct TShortTZ
{
	int Bias;
	int StandardBias;
	int DayLightBias;
	Word wReserved1;
	Word StdMonth;
	Word StdDayOfWeek;
	Word StdDay;
	Word StdHour;
	Word StdMinute;
	Word StdSecond;
	Word wReserved2;
	Word wReserved3;
	Word DLMonth;
	Word DLDayOfWeek;
	Word DLDay;
	Word DLHour;
	Word DLMinute;
	Word DLSecond;
	Word wReserved4;
} ;
#pragma pack(pop)

struct TTimeZoneInfo;
typedef TTimeZoneInfo *PTimeZoneInfo;

#pragma pack(push, 4)
struct TTimeZoneInfo
{
	AnsiString KeyName;
	AnsiString DisplayName;
	AnsiString DltName;
	AnsiString StdName;
	AnsiString MapID;
	_TIME_ZONE_INFORMATION TimeZone;
	TShortTZ STimeZone;
} ;
#pragma pack(pop)

//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool FDL;
extern PACKAGE Ellist::TElList* SysTimeZones;
extern PACKAGE void __fastcall ShortTZToTimeZoneInfo(const TShortTZ &ShortTZ, TTimeZoneInfo &TZInfo);
extern PACKAGE AnsiString __fastcall TranslateTZDate(const _SYSTEMTIME &ADate);
extern PACKAGE bool __fastcall RetrieveTimeZoneInfo(Ellist::TElList* TimeZoneInfoList);

}	/* namespace Elclock */
using namespace Elclock;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElClock
