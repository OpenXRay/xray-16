// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCalendar.pas' rev: 6.00

#ifndef ElCalendarHPP
#define ElCalendarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElCalendarDefs.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Grids.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcalendar
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElCalendar;
class PASCALIMPLEMENTATION TElCalendar : public Grids::TCustomGrid 
{
	typedef Grids::TCustomGrid inherited;
	
private:
	Graphics::TColor FHolidayColor;
	bool FShowPeriods;
	System::TDateTime FPeriodStart;
	Word FPeriodLength;
	Word FPeriodInterval;
	Graphics::TColor FPeriodColor;
	bool FShowHolidays;
	Elcalendardefs::TElHolidays* FHolidays;
	Graphics::TColor FWeekEndColor;
	Elcalendardefs::TElWeekEndDays FWeekEndDays;
	bool FShowWeekNum;
	System::TDateTime FDate;
	int FMonthOffset;
	Classes::TNotifyEvent FOnChange;
	bool FReadOnly;
	Elcalendardefs::TDayOfWeek FStartOfWeek;
	bool FUpdating;
	bool FUserNavigation;
	bool FUseCurrentDate;
	bool FTranslateDays;
	bool FMouseOver;
	bool FFlat;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FUseLineColors;
	Elvclutils::TElBorderSides FBorderSides;
	Elvclutils::TElFlatBorderType FSelectionBorder;
	Elvclutils::TElFlatBorderType FDayCellBorder;
	Elvclutils::TElFlatBorderType FCurrentDayBorder;
	void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	AnsiString __fastcall GetCellText(int ACol, int ARow);
	int __fastcall GetDateElement(int Index);
	void __fastcall SetCalendarDate(System::TDateTime Value);
	void __fastcall SetDateElement(int Index, int Value);
	void __fastcall SetStartOfWeek(Elcalendardefs::TDayOfWeek Value);
	void __fastcall SetUseCurrentDate(bool Value);
	bool __fastcall StoreCalendarDate(void);
	void __fastcall SetShowWeekNum(bool newValue);
	void __fastcall SetWeekEndDays(Elcalendardefs::TElWeekEndDays newValue);
	void __fastcall SetWeekEndColor(Graphics::TColor newValue);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Msg);
	void __fastcall SetHolidays(Elcalendardefs::TElHolidays* newValue);
	void __fastcall FixHolidayDate(Elcalendardefs::TElHoliday* AHoliday, System::TDateTime &Date);
	void __fastcall SetShowHolidays(bool newValue);
	void __fastcall SetShowPeriods(bool newValue);
	void __fastcall SetPeriodStart(System::TDateTime newValue);
	void __fastcall SetPeriodLength(Word newValue);
	void __fastcall SetPeriodInterval(Word newValue);
	void __fastcall SetPeriodColor(Graphics::TColor newValue);
	void __fastcall SetHolidayColor(Graphics::TColor newValue);
	void __fastcall SetDate(System::TDateTime newValue);
	void __fastcall SetFlat(bool newValue);
	void __fastcall SetTranslateDays(bool value);
	void __fastcall DrawFlatBorder(void);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMVScroll(Messages::TWMScroll &Msg);
	HIDESBASE MESSAGE void __fastcall WMHScroll(Messages::TWMScroll &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Message);
	bool __fastcall StoreDate(void);
	void __fastcall SetLineColorLight(Graphics::TColor Value);
	void __fastcall SetLineColorDark(Graphics::TColor Value);
	void __fastcall SetUseLineColors(bool Value);
	void __fastcall SetSelectionBorder(Elvclutils::TElFlatBorderType Value);
	void __fastcall SetDayCellBorder(Elvclutils::TElFlatBorderType Value);
	void __fastcall SetCurrentDayBorder(Elvclutils::TElFlatBorderType Value);
	
protected:
	Graphics::TColor FLineColorLight;
	Graphics::TColor FLineColorDark;
	bool FUseSystemStartOfWeek;
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	DYNAMIC void __fastcall Change(void);
	void __fastcall ChangeMonth(int Delta);
	DYNAMIC void __fastcall Click(void);
	virtual int __fastcall DaysThisMonth(void);
	virtual void __fastcall DrawCell(int ACol, int ARow, const Types::TRect &ARect, Grids::TGridDrawState AState);
	virtual bool __fastcall IsLeapYear(int AYear);
	virtual bool __fastcall SelectCell(int ACol, int ARow);
	virtual void __fastcall Loaded(void);
	void __fastcall UpdateFrame(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	void __fastcall SetUseSystemStartOfWeek(bool Value);
	
public:
	__fastcall virtual TElCalendar(Classes::TComponent* AOwner);
	__fastcall virtual ~TElCalendar(void);
	void __fastcall NextMonth(void);
	void __fastcall NextYear(void);
	void __fastcall PrevMonth(void);
	void __fastcall PrevYear(void);
	virtual void __fastcall UpdateCalendar(void);
	void __fastcall MouseToCell(int X, int Y, int &ACol, int &ARow);
	__property System::TDateTime CalendarDate = {read=FDate, write=SetCalendarDate, stored=StoreCalendarDate};
	__property AnsiString CellText[int ACol][int ARow] = {read=GetCellText};
	bool __fastcall IsHoliday(int AYear, int AMonth, int ADay);
	bool __fastcall IsInPeriod(Word AYear, Word AMonth, Word ADay);
	bool __fastcall IsRestHoliday(Word AYear, Word AMonth, Word ADay);
	
__published:
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property System::TDateTime Date = {read=FDate, write=SetDate, stored=StoreDate};
	__property int Day = {read=GetDateElement, write=SetDateElement, stored=false, index=3, nodefault};
	__property Elcalendardefs::TElHolidays* Holidays = {read=FHolidays, write=SetHolidays};
	__property int Month = {read=GetDateElement, write=SetDateElement, stored=false, index=2, nodefault};
	__property bool ReadOnly = {read=FReadOnly, write=FReadOnly, default=0};
	__property Elcalendardefs::TDayOfWeek StartOfWeek = {read=FStartOfWeek, write=SetStartOfWeek, nodefault};
	__property bool TranslateDays = {read=FTranslateDays, write=SetTranslateDays, default=1};
	__property bool UseCurrentDate = {read=FUseCurrentDate, write=SetUseCurrentDate, default=1};
	__property int Year = {read=GetDateElement, write=SetDateElement, stored=false, index=1, nodefault};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property bool ShowWeekNum = {read=FShowWeekNum, write=SetShowWeekNum, default=0};
	__property Elcalendardefs::TElWeekEndDays WeekEndDays = {read=FWeekEndDays, write=SetWeekEndDays, nodefault};
	__property Graphics::TColor WeekEndColor = {read=FWeekEndColor, write=SetWeekEndColor, default=-2147483643};
	__property bool ShowHolidays = {read=FShowHolidays, write=SetShowHolidays, default=1};
	__property bool ShowPeriods = {read=FShowPeriods, write=SetShowPeriods, nodefault};
	__property System::TDateTime PeriodStart = {read=FPeriodStart, write=SetPeriodStart};
	__property Word PeriodLength = {read=FPeriodLength, write=SetPeriodLength, default=1};
	__property Word PeriodInterval = {read=FPeriodInterval, write=SetPeriodInterval, default=28};
	__property Graphics::TColor PeriodColor = {read=FPeriodColor, write=SetPeriodColor, default=16776960};
	__property Graphics::TColor HolidayColor = {read=FHolidayColor, write=SetHolidayColor, nodefault};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, nodefault};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, nodefault};
	__property bool UserNavigation = {read=FUserNavigation, write=FUserNavigation, nodefault};
	__property Graphics::TColor LineColorLight = {read=FLineColorLight, write=SetLineColorLight, stored=FUseLineColors, default=-2147483643};
	__property Graphics::TColor LineColorDark = {read=FLineColorDark, write=SetLineColorDark, stored=FUseLineColors, default=-2147483632};
	__property bool UseSystemStartOfWeek = {read=FUseSystemStartOfWeek, write=SetUseSystemStartOfWeek, default=0};
	__property bool UseLineColors = {read=FUseLineColors, write=SetUseLineColors, default=1};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property Elvclutils::TElFlatBorderType SelectionBorder = {read=FSelectionBorder, write=SetSelectionBorder, nodefault};
	__property Elvclutils::TElFlatBorderType DayCellBorder = {read=FDayCellBorder, write=SetDayCellBorder, nodefault};
	__property Elvclutils::TElFlatBorderType CurrentDayBorder = {read=FCurrentDayBorder, write=SetCurrentDayBorder, nodefault};
	__property Align  = {default=0};
	__property BorderStyle  = {default=1};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property Enabled  = {default=1};
	__property Font ;
	__property GridLineWidth  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
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
	/* TWinControl.CreateParented */ inline __fastcall TElCalendar(HWND ParentWindow) : Grids::TCustomGrid(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcalendar */
using namespace Elcalendar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCalendar
