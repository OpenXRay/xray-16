// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCalendDlg.pas' rev: 6.00

#ifndef ElCalendDlgHPP
#define ElCalendDlgHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElCombos.hpp>	// Pascal unit
#include <ElSpin.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElCalendar.hpp>	// Pascal unit
#include <ElCalendarDefs.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcalenddlg
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElCalendarForm;
class PASCALIMPLEMENTATION TElCalendarForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Extctrls::TPanel* Panel2;
	Elpopbtn::TElPopupButton* TodayBtn;
	Elpopbtn::TElPopupButton* OkBtn;
	Elpopbtn::TElPopupButton* CancelBtn;
	void __fastcall FormCreate(System::TObject* Sender);
	void __fastcall FormShow(System::TObject* Sender);
	void __fastcall TodayBtnClick(System::TObject* Sender);
	void __fastcall OkBtnClick(System::TObject* Sender);
	void __fastcall FormClose(System::TObject* Sender, Forms::TCloseAction &Action);
	void __fastcall CancelBtnClick(System::TObject* Sender);
	void __fastcall CalendarChange(System::TObject* Sender);
	void __fastcall CalendarClick(System::TObject* Sender);
	
private:
	Classes::TNotifyEvent FOnDeactivate;
	Classes::TNotifyEvent FOnChange;
	HIDESBASE MESSAGE void __fastcall WMQueryEndSession(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMActivate(Messages::TWMActivate &Msg);
	
protected:
	virtual void __fastcall TriggerChangeEvent(void);
	
public:
	bool IsModal;
	Elcalendar::TElCalendar* Calendar;
	Elpanel::TElPanel* Panel1;
	Elpopbtn::TElPopupButton* PrevMonBtn;
	Elpopbtn::TElPopupButton* PrevYearBtn;
	Elpopbtn::TElPopupButton* NextMonBtn;
	Elpopbtn::TElPopupButton* NextYearBtn;
	Elspin::TElSpinEdit* YearSpin;
	Elactrls::TElAdvancedComboBox* MonthCombo;
	void __fastcall PrevYearBtnClick(System::TObject* Sender);
	void __fastcall PrevMonBtnClick(System::TObject* Sender);
	void __fastcall MonthComboChange(System::TObject* Sender);
	void __fastcall NextMonBtnClick(System::TObject* Sender);
	void __fastcall NextYearBtnClick(System::TObject* Sender);
	void __fastcall YearSpinChange(System::TObject* Sender);
	void __fastcall SetNames(void);
	void __fastcall UpdateLabel(void);
	__fastcall virtual ~TElCalendarForm(void);
	
__published:
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Classes::TNotifyEvent OnDeactivate = {read=FOnDeactivate, write=FOnDeactivate};
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TElCalendarForm(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TElCalendarForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElCalendarForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElCalendarDialog;
class PASCALIMPLEMENTATION TElCalendarDialog : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	System::TDateTime FDate;
	bool FShowHolidays;
	bool FShowPeriods;
	Elcalendardefs::TDayOfWeek FStartOfWeek;
	bool FUseCurrentDate;
	int FGridLineWidth;
	bool FShowWeekNum;
	Elcalendardefs::TElWeekEndDays FWeekEndDays;
	System::TDateTime FPeriodStart;
	int FPeriodLength;
	int FPeriodInterval;
	Graphics::TColor FPeriodColor;
	Graphics::TColor FHolidayColor;
	Graphics::TColor FWeekEndColor;
	bool FUseSystemStartOfWeek;
	Elcalendardefs::TElHolidays* FHolidays;
	Classes::TNotifyEvent FOnChange;
	void __fastcall PrepareDialog(TElCalendarForm* FrmDialogComponent);
	
protected:
	Elvclutils::TElFlatBorderType FSelectionBorder;
	Elvclutils::TElFlatBorderType FDayCellBorder;
	Elvclutils::TElFlatBorderType FCurrentDayBorder;
	bool FUseLineColors;
	Graphics::TColor FLineColorDark;
	Graphics::TColor FLineColorLight;
	
public:
	__fastcall virtual TElCalendarDialog(Classes::TComponent* AOwner);
	__fastcall virtual ~TElCalendarDialog(void);
	bool __fastcall Execute(void);
	
__published:
	__property System::TDateTime Date = {read=FDate, write=FDate};
	__property bool ShowHolidays = {read=FShowHolidays, write=FShowHolidays, nodefault};
	__property bool ShowPeriods = {read=FShowPeriods, write=FShowPeriods, nodefault};
	__property Elcalendardefs::TDayOfWeek StartOfWeek = {read=FStartOfWeek, write=FStartOfWeek, nodefault};
	__property bool UseCurrentDate = {read=FUseCurrentDate, write=FUseCurrentDate, nodefault};
	__property int GridLineWidth = {read=FGridLineWidth, write=FGridLineWidth, nodefault};
	__property bool ShowWeekNum = {read=FShowWeekNum, write=FShowWeekNum, nodefault};
	__property Elcalendardefs::TElWeekEndDays WeekEndDays = {read=FWeekEndDays, write=FWeekEndDays, nodefault};
	__property System::TDateTime PeriodStart = {read=FPeriodStart, write=FPeriodStart};
	__property int PeriodLength = {read=FPeriodLength, write=FPeriodLength, nodefault};
	__property int PeriodInterval = {read=FPeriodInterval, write=FPeriodInterval, nodefault};
	__property Graphics::TColor PeriodColor = {read=FPeriodColor, write=FPeriodColor, nodefault};
	__property Graphics::TColor HolidayColor = {read=FHolidayColor, write=FHolidayColor, nodefault};
	__property Graphics::TColor WeekEndColor = {read=FWeekEndColor, write=FWeekEndColor, nodefault};
	__property Elcalendardefs::TElHolidays* Holidays = {read=FHolidays};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property bool UseSystemStartOfWeek = {read=FUseSystemStartOfWeek, write=FUseSystemStartOfWeek, nodefault};
	__property Elvclutils::TElFlatBorderType SelectionBorder = {read=FSelectionBorder, write=FSelectionBorder, nodefault};
	__property Elvclutils::TElFlatBorderType DayCellBorder = {read=FDayCellBorder, write=FDayCellBorder, nodefault};
	__property Elvclutils::TElFlatBorderType CurrentDayBorder = {read=FCurrentDayBorder, write=FCurrentDayBorder, nodefault};
	__property bool UseLineColors = {read=FUseLineColors, write=FUseLineColors, nodefault};
	__property Graphics::TColor LineColorDark = {read=FLineColorDark, write=FLineColorDark, nodefault};
	__property Graphics::TColor LineColorLight = {read=FLineColorLight, write=FLineColorLight, nodefault};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE Ellist::TElList* FormList;

}	/* namespace Elcalenddlg */
using namespace Elcalenddlg;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCalendDlg
