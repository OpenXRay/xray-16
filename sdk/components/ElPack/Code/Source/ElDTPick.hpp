// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDTPick.pas' rev: 6.00

#ifndef ElDTPickHPP
#define ElDTPickHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElCalendDlg.hpp>	// Pascal unit
#include <ElCalendarDefs.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElExtBkgnd.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElSpinBtn.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eldtpick
{
//-- type declarations -------------------------------------------------------
#pragma pack(push, 4)
struct TDTFPart
{
	AnsiString Text;
	int DPart;
	int TSPos;
	int TEPos;
	int SPos;
	int EPos;
} ;
#pragma pack(pop)

typedef TDTFPart *PDTFPart;

#pragma option push -b-
enum TElDatePickerFormat { edfShortDateLongTime, edfLongDate, edfShortDate, edfLongTime, edfShortTime, edfCustom };
#pragma option pop

class DELPHICLASS TElDateTimePicker;
class PASCALIMPLEMENTATION TElDateTimePicker : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
protected:
	bool FHandleDialogKeys;
	bool FModified;
	bool FShowCheckBox;
	bool FChecked;
	bool FAltChangeMethod;
	bool FShowPopupCalendar;
	bool FNavigationInPopup;
	Forms::TFormBorderStyle FBorderStyle;
	Classes::TNotifyEvent FOnChange;
	bool Use12Hours;
	Elpopbtn::TElGraphicButton* FCalButton;
	Elspinbtn::TElSpinButton* FButton;
	int FBtnWidth;
	Ellist::TElList* DTFParts;
	AnsiString FFormatStr;
	TElDatePickerFormat FFormat;
	System::TDateTime FDate;
	bool FFocused;
	int FCurPart;
	Classes::TAlignment FAlignment;
	bool FMouseOver;
	Elcalenddlg::TElCalendarForm* FForm;
	HDC TmpDC;
	Classes::TNotifyEvent FOnDropDown;
	int FGradientSteps;
	Graphics::TColor FGradientStartColor;
	Graphics::TColor FGradientEndColor;
	Graphics::TBitmap* FTmpBmp;
	bool FTransparent;
	Graphics::TBitmap* FBackground;
	Elvclutils::TElBkGndType FBackgroundType;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elimgfrm::TElImageForm* FImgForm;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FFlat;
	System::TDateTime FMinDate;
	System::TDateTime FMaxDate;
	AnsiString FDI;
	WideString FText;
	bool FDroppedDown;
	bool InDblClick;
	Classes::TNotifyEvent FOnCheckBoxChange;
	Elvclutils::TElBorderSides FBorderSides;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	bool FButtonVisible;
	bool FUnassigned;
	Graphics::TColor FUnassignedColor;
	bool FUnassignedAllowed;
	bool FReadOnly;
	bool FButtonShowOnFocus;
	bool FUseCurrentDate;
	bool FButtonThinFrame;
	bool FAutoSize;
	WideString FHint;
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall WMSysKeyDown(Messages::TWMKey &Msg);
	MESSAGE void __fastcall CMCancelMode(Controls::TCMCancelMode &Msg);
	HIDESBASE MESSAGE void __fastcall CMCtl3DChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	void __fastcall SetBorderStyle(Forms::TBorderStyle Value);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall SetTransparent(bool newValue);
	void __fastcall SetBackground(Graphics::TBitmap* newValue);
	void __fastcall SetBackgroundType(Elvclutils::TElBkGndType newValue);
	void __fastcall ImageChange(System::TObject* Sender);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetGradientStartColor(Graphics::TColor newValue);
	void __fastcall SetGradientEndColor(Graphics::TColor newValue);
	void __fastcall SetGradientSteps(int newValue);
	void __fastcall RedoTmpBmp(void);
	void __fastcall DrawFlatBorder(void);
	void __fastcall InvalidateEdit(void);
	virtual void __fastcall CloseUp(bool AcceptValue);
	void __fastcall CalendarDeactivate(System::TObject* Sender);
	void __fastcall SpinUpClick(System::TObject* Sender, double Increment);
	void __fastcall SpinDownClick(System::TObject* Sender, double Increment);
	void __fastcall CalBtnClick(System::TObject* Sender);
	void __fastcall IncValue(int Increment);
	void __fastcall ParseFormat(AnsiString AFormat);
	void __fastcall UpdatePart(void);
	void __fastcall UpdateFrame(void);
	void __fastcall UpdateText(void);
	void __fastcall OnDTFPartDelete(System::TObject* Sender, void * Item);
	AnsiString __fastcall GetStdFormat(TElDatePickerFormat Fmt);
	void __fastcall TuneupCalendarControls(void);
	bool __fastcall GetCalendarUseLineColors(void);
	void __fastcall SetCalendarUseLineColors(bool Value);
	bool __fastcall StoreStartOfWeek(void);
	Graphics::TColor __fastcall GetCalendarWeekendColor(void);
	void __fastcall SetCalendarWeekendColor(Graphics::TColor Value);
	void __fastcall DrawFlatFrame(Graphics::TCanvas* Canvas, const Types::TRect &R);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	virtual void __fastcall SetFormat(TElDatePickerFormat newValue);
	virtual void __fastcall SetFormatStr(AnsiString newValue);
	virtual void __fastcall SetDateTime(System::TDateTime newValue);
	virtual void __fastcall SetAlignment(Classes::TAlignment Value);
	virtual int __fastcall GetBtnWidth(void);
	virtual int __fastcall GetCheckDims(void);
	virtual int __fastcall GetCheckWidth(void);
	void __fastcall SetEditRect(void);
	virtual void __fastcall DropDown(void);
	virtual void __fastcall Paint(void);
	virtual void __fastcall DoDropDown(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerChangeEvent(void);
	virtual void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	virtual void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	virtual void __fastcall SetFlat(bool newValue);
	virtual void __fastcall SetNavigationInPopup(bool newValue);
	virtual void __fastcall SetDate(System::TDateTime newValue);
	virtual System::TDateTime __fastcall GetDate(void);
	virtual void __fastcall SetTime(System::TDateTime newValue);
	virtual System::TDateTime __fastcall GetTime(void);
	bool __fastcall GetDroppedDown(void);
	virtual void __fastcall SetDroppedDown(bool newValue);
	Elcalendardefs::TDayOfWeek __fastcall GetStartOfWeek(void);
	void __fastcall SetStartOfWeek(Elcalendardefs::TDayOfWeek Value);
	Elcalendardefs::TElWeekEndDays __fastcall GetWeekEndDays(void);
	void __fastcall SetWeekEndDays(Elcalendardefs::TElWeekEndDays Value);
	void __fastcall SetCalendarLineColorLight(Graphics::TColor Value);
	Graphics::TColor __fastcall GetCalendarLineColorDark(void);
	void __fastcall SetCalendarLineColorDark(Graphics::TColor Value);
	Graphics::TColor __fastcall GetCalendarLineColorLight(void);
	Graphics::TColor __fastcall GetCalendarBackColor(void);
	void __fastcall SetCalendarBackColor(Graphics::TColor Value);
	bool __fastcall GetUseSystemStartOfWeek(void);
	void __fastcall SetUseSystemStartOfWeek(bool Value);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall CreateWindowHandle(const Controls::TCreateParams &Params);
	void __fastcall AdjustHeight(void);
	void __fastcall UpdateHeight(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall SetShowPopupCalendar(bool newValue);
	void __fastcall CalendarKeyDown(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall CalendarChange(System::TObject* Sender);
	DYNAMIC void __fastcall DblClick(void);
	__property int BtnWidth = {read=GetBtnWidth, nodefault};
	virtual void __fastcall SetShowCheckBox(bool newValue);
	virtual void __fastcall SetChecked(bool newValue);
	virtual void __fastcall SetModified(bool newValue);
	void __fastcall TriggerCheckboxChangeEvent(void);
	Elvclutils::TElFlatBorderType __fastcall GetCalendarCurrentDayBorder(void);
	void __fastcall SetCalendarCurrentDayBorder(Elvclutils::TElFlatBorderType Value);
	Elvclutils::TElFlatBorderType __fastcall GetCalendarDayCellBorder(void);
	void __fastcall SetCalendarDayCellBorder(Elvclutils::TElFlatBorderType Value);
	Elvclutils::TElFlatBorderType __fastcall GetCalendarSelectionBorder(void);
	void __fastcall SetCalendarSelectionBorder(Elvclutils::TElFlatBorderType Value);
	void __fastcall SetMinDate(System::TDateTime Value);
	void __fastcall SetMaxDate(System::TDateTime Value);
	void __fastcall DoSetDateTime(System::TDateTime ADate);
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall SetUseXPThemes(const bool Value);
	Elspinbtn::TElSpinBtnDir __fastcall GetButtonDir(void);
	void __fastcall SetButtonDir(Elspinbtn::TElSpinBtnDir Value);
	Elspinbtn::TElSpinBtnType __fastcall GetButtonType(void);
	void __fastcall SetButtonType(Elspinbtn::TElSpinBtnType Value);
	void __fastcall SetButtonWidth(int Value);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall UpdateButtonStyles(void);
	void __fastcall SetButtonVisible(bool Value);
	void __fastcall SetUnassigned(bool Value);
	void __fastcall SetUnassignedColor(Graphics::TColor Value);
	void __fastcall SetUnassignedAllowed(bool Value);
	virtual bool __fastcall GetReadOnly(void);
	virtual void __fastcall SetReadOnly(bool Value);
	void __fastcall SetButtonShowOnFocus(bool Value);
	bool __fastcall IsDateTimeStored(void);
	void __fastcall SetButtonThinFrame(bool Value);
	HIDESBASE void __fastcall SetAutoSize(bool Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElDateTimePicker(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDateTimePicker(void);
	__property bool MouseOver = {read=FMouseOver, nodefault};
	__property bool DroppedDown = {read=GetDroppedDown, write=SetDroppedDown, nodefault};
	__property System::TDateTime Date = {read=GetDate, write=SetDate};
	__property System::TDateTime Time = {read=GetTime, write=SetTime};
	
__published:
	__property TElDatePickerFormat Format = {read=FFormat, write=SetFormat, default=0};
	__property AnsiString FormatString = {read=FFormatStr, write=SetFormatStr};
	__property System::TDateTime DateTime = {read=FDate, write=SetDateTime, stored=IsDateTimeStored};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=1};
	__property Graphics::TColor GradientStartColor = {read=FGradientStartColor, write=SetGradientStartColor, default=0};
	__property Graphics::TColor GradientEndColor = {read=FGradientEndColor, write=SetGradientEndColor, default=0};
	__property int GradientSteps = {read=FGradientSteps, write=SetGradientSteps, default=16};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property Elvclutils::TElBkGndType BackgroundType = {read=FBackgroundType, write=SetBackgroundType, nodefault};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, default=1};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, nodefault};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, nodefault};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property bool ShowPopupCalendar = {read=FShowPopupCalendar, write=SetShowPopupCalendar, default=0};
	__property bool NavigationInPopup = {read=FNavigationInPopup, write=SetNavigationInPopup, default=1};
	__property bool AltChangeMethod = {read=FAltChangeMethod, write=FAltChangeMethod, nodefault};
	__property bool ShowCheckBox = {read=FShowCheckBox, write=SetShowCheckBox, nodefault};
	__property bool Checked = {read=FChecked, write=SetChecked, default=1};
	__property bool Modified = {read=FModified, write=SetModified, nodefault};
	__property Graphics::TColor CalendarLineColorDark = {read=GetCalendarLineColorDark, write=SetCalendarLineColorDark, default=-2147483632};
	__property Graphics::TColor CalendarLineColorLight = {read=GetCalendarLineColorLight, write=SetCalendarLineColorLight, default=-2147483643};
	__property Graphics::TColor CalendarBackColor = {read=GetCalendarBackColor, write=SetCalendarBackColor, default=-2147483643};
	__property Elcalendardefs::TDayOfWeek StartOfWeek = {read=GetStartOfWeek, write=SetStartOfWeek, stored=StoreStartOfWeek, nodefault};
	__property bool UseSystemStartOfWeek = {read=GetUseSystemStartOfWeek, write=SetUseSystemStartOfWeek, nodefault};
	__property Elcalendardefs::TElWeekEndDays WeekEndDays = {read=GetWeekEndDays, write=SetWeekEndDays, nodefault};
	__property bool CalendarUseLineColors = {read=GetCalendarUseLineColors, write=SetCalendarUseLineColors, default=1};
	__property Graphics::TColor CalendarWeekendColor = {read=GetCalendarWeekendColor, write=SetCalendarWeekendColor, nodefault};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property Elvclutils::TElFlatBorderType CalendarCurrentDayBorder = {read=GetCalendarCurrentDayBorder, write=SetCalendarCurrentDayBorder, nodefault};
	__property Elvclutils::TElFlatBorderType CalendarDayCellBorder = {read=GetCalendarDayCellBorder, write=SetCalendarDayCellBorder, nodefault};
	__property Elvclutils::TElFlatBorderType CalendarSelectionBorder = {read=GetCalendarSelectionBorder, write=SetCalendarSelectionBorder, nodefault};
	__property System::TDateTime MinDate = {read=FMinDate, write=SetMinDate};
	__property System::TDateTime MaxDate = {read=FMaxDate, write=SetMaxDate};
	__property bool HandleDialogKeys = {read=FHandleDialogKeys, write=FHandleDialogKeys, default=0};
	__property Elspinbtn::TElSpinBtnDir ButtonDir = {read=GetButtonDir, write=SetButtonDir, nodefault};
	__property Elspinbtn::TElSpinBtnType ButtonType = {read=GetButtonType, write=SetButtonType, nodefault};
	__property int ButtonWidth = {read=FBtnWidth, write=SetButtonWidth, nodefault};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Classes::TNotifyEvent OnDropDown = {read=FOnDropDown, write=FOnDropDown};
	__property Classes::TNotifyEvent OnCheckBoxChange = {read=FOnCheckBoxChange, write=FOnCheckBoxChange};
	__property Anchors  = {default=3};
	__property Align  = {default=0};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
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
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property bool ButtonVisible = {read=FButtonVisible, write=SetButtonVisible, default=1};
	__property bool Unassigned = {read=FUnassigned, write=SetUnassigned, default=0};
	__property Graphics::TColor UnassignedColor = {read=FUnassignedColor, write=SetUnassignedColor, default=255};
	__property bool UnassignedAllowed = {read=FUnassignedAllowed, write=SetUnassignedAllowed, default=0};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property bool ButtonShowOnFocus = {read=FButtonShowOnFocus, write=SetButtonShowOnFocus, default=0};
	__property bool UseCurrentDate = {read=FUseCurrentDate, write=FUseCurrentDate, default=1};
	__property bool ButtonThinFrame = {read=FButtonThinFrame, write=SetButtonThinFrame, default=1};
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDateTimePicker(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldtpick */
using namespace Eldtpick;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDTPick
