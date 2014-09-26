// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElScrollBar.pas' rev: 6.00

#ifndef ElScrollBarHPP
#define ElScrollBarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElHintWnd.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elscrollbar
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElScrollHintNeededEvent)(System::TObject* Sender, int TrackPosition, WideString &Hint);

#pragma option push -b-
enum TElScrollBarPart { sbpNowhere, sbpLeftArrow, sbpRightSndArrow, sbpRightArrow, sbpLeftSndArrow, sbpThumb, sbpLeftTop, sbpRightBottom };
#pragma option pop

typedef void __fastcall (__closure *TElScrollHitTestEvent)(System::TObject* Sender, int X, int Y, TElScrollBarPart &Part, bool &DefaultTest);

typedef void __fastcall (__closure *TElScrollDrawPartEvent)(System::TObject* Sender, Graphics::TCanvas* Canvas, const Types::TRect &R, TElScrollBarPart Part, bool Enabled, bool Focused, bool Pressed, bool &DefaultDraw);

#pragma option push -b-
enum TElScrollCode { escLineUp, escLineDown, escPageUp, escPageDown, escPosition, escTrack, escTop, escBottom, escEndScroll, escSndLineUp, escSndLineDown };
#pragma option pop

typedef void __fastcall (__closure *TElScrollEvent)(System::TObject* Sender, TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);

#pragma option push -b-
enum TElSecButtonsKind { sbkOpposite, sbkPage, sbkCustom };
#pragma option pop

#pragma option push -b-
enum TElScrollThumbMode { etmFixed, etmAuto };
#pragma option pop

class DELPHICLASS TCustomElScrollBar;
class PASCALIMPLEMENTATION TCustomElScrollBar : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	bool FThinFrames;
	bool FDrawFrames;
	bool FDrawArrowFrames;
	bool FDrawBars;
	unsigned FBarOffset;
	Graphics::TColor FBarColor;
	Graphics::TColor FArrowColor;
	Graphics::TColor FArrowHotTrackColor;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elimgfrm::TElImageForm* FImgForm;
	bool FUseSystemMetrics;
	bool FNoSunkenThumb;
	bool FShowLeftArrows;
	bool FShowRightArrows;
	bool FChangeColorsOnScroll;
	bool FBitmapOwner;
	bool FBlendBackground;
	bool FShowTrackHint;
	bool FNoDisableButtons;
	bool FOwnerDraw;
	bool FSecondaryButtons;
	TElSecButtonsKind FSecondBtnKind;
	int FPage;
	int FMinThumbSize;
	TElScrollThumbMode FThumbMode;
	int FThumbSize;
	int FButtonSize;
	Forms::TScrollBarKind FKind;
	int FPosition;
	int FMax;
	int FMin;
	bool FFlat;
	bool FActiveFlat;
	bool FMouseInControl;
	bool FIsHTML;
	Classes::TNotifyEvent FOnChange;
	TElScrollEvent FOnScroll;
	TElScrollDrawPartEvent FOnDrawPart;
	TElScrollHintNeededEvent FOnScrollHintNeeded;
	Extctrls::TTimer* FScrollTimer;
	int FThumbOffset;
	int FOrigPos;
	int FOrigCoord;
	HWND FSaveCapture;
	int FTrackPos;
	int FThumbPos;
	TElScrollBarPart FPressedIn;
	TElScrollBarPart FOrigPressedIn;
	TElScrollBarPart FMouseOver;
	Elhintwnd::TElHintWindow* FHintWnd;
	#pragma pack(push, 1)
	Types::TPoint FPressedPos;
	#pragma pack(pop)
	
	bool FPressed;
	bool FTracking;
	bool FNoScrollMessages;
	bool FAltDisablingArrows;
	TElScrollHitTestEvent FOnHitTest;
	void __fastcall SetKind(Forms::TScrollBarKind newValue);
	void __fastcall SetPosition(int newValue);
	void __fastcall SetMax(int newValue);
	void __fastcall SetMin(int newValue);
	void __fastcall SetPage(int newValue);
	void __fastcall IntMouseButton(bool Pressed, Controls::TMouseButton Btn, short XPos, short YPos);
	void __fastcall IntMouseMove(short XPos, short YPos);
	void __fastcall IntMouseEnter(void);
	void __fastcall IntMouseLeave(void);
	void __fastcall IntDoEnter(void);
	void __fastcall IntDoExit(void);
	void __fastcall IntColorChanged(void);
	void __fastcall DoSetPosition(int newValue, bool Redraw);
	void __fastcall DoSetMax(int newValue, bool Redraw);
	void __fastcall DoSetMin(int newValue, bool Redraw);
	void __fastcall DoSetPage(int newValue, bool Redraw);
	Graphics::TColor __fastcall ShadowColor(void);
	Graphics::TColor __fastcall LighterColor(void);
	void __fastcall SetFlat(bool newValue);
	void __fastcall SetActiveFlat(bool newValue);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Msg);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	MESSAGE void __fastcall SBMSetScrollInfo(Messages::TMessage &Msg);
	MESSAGE void __fastcall SBMGetScrollInfo(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMSysColorChange(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMColorChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TMessage &Msg);
	void __fastcall OnScrollTimer(System::TObject* Sender);
	void __fastcall SetButtonSize(int newValue);
	void __fastcall SetThumbMode(TElScrollThumbMode newValue);
	void __fastcall SetThumbSize(int newValue);
	int __fastcall GetAutoThumbSize(void);
	int __fastcall GetThumbPos(void);
	int __fastcall GetTopBtns(void);
	int __fastcall GetBottomBtns(void);
	int __fastcall AdjustThumbPos(void);
	int __fastcall UpdateThumbPos(void);
	void __fastcall SetMinThumbSize(int newValue);
	void __fastcall SetSecondaryButtons(bool newValue);
	void __fastcall SetOwnerDraw(bool newValue);
	void __fastcall SetSecondBtnKind(TElSecButtonsKind newValue);
	void __fastcall SetNoDisableButtons(bool newValue);
	void __fastcall UpdateScrollingRegion(void);
	void __fastcall ShowHintAt(int APosition, int X, int Y);
	int __fastcall GetButtonSize(void);
	void __fastcall SetIsDesigning(bool newValue);
	bool __fastcall GetIsDesigning(void);
	void __fastcall SetBlendBackground(bool newValue);
	void __fastcall SetShowLeftArrows(bool newValue);
	void __fastcall SetShowRightArrows(bool newValue);
	void __fastcall SetNoSunkenThumb(bool newValue);
	void __fastcall SetUseSystemMetrics(bool value);
	void __fastcall SetArrowColor(Graphics::TColor newValue);
	void __fastcall SetArrowHotTrackColor(Graphics::TColor newValue);
	void __fastcall SetDrawFrames(bool newValue);
	void __fastcall SetDrawBars(const bool Value);
	void __fastcall SetDrawArrowFrames(const bool Value);
	void __fastcall SetThinFrames(bool newValue);
	void __fastcall SetBarColor(const Graphics::TColor Value);
	void __fastcall SetBarOffset(const unsigned Value);
	void __fastcall ImageFormChange(System::TObject* Sender);
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	bool __fastcall RightBtnsEnabled(void);
	bool __fastcall LeftBtnsEnabled(void);
	void __fastcall RebuildBackground(void);
	void __fastcall SetAltDisablingArrows(const bool Value);
	int __fastcall GetThumbSize(void);
	
protected:
	Graphics::TColor FHintColor;
	Graphics::TColor FHintTextColor;
	WideString FHint;
	bool FSysBkColor;
	int FStep;
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall TriggerChangeEvent(void);
	virtual void __fastcall TriggerScrollEvent(TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);
	virtual void __fastcall TriggerScrollHintNeededEvent(int TrackPosition, WideString &Hint);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall SetSysBkColor(bool Value);
	virtual void __fastcall TriggerHitTestEvent(int X, int Y, TElScrollBarPart &Part, bool &DefaultTest);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	__property Color ;
	__property ParentColor  = {default=0};
	__property Forms::TScrollBarKind Kind = {read=FKind, write=SetKind, default=0};
	__property int Position = {read=FPosition, write=SetPosition, default=0};
	__property int Max = {read=FMax, write=SetMax, default=100};
	__property int Min = {read=FMin, write=SetMin, default=0};
	__property int Step = {read=FStep, write=FStep, default=1};
	__property bool Flat = {read=FFlat, write=SetFlat, default=1};
	__property bool ActiveFlat = {read=FActiveFlat, write=SetActiveFlat, default=1};
	__property int ButtonSize = {read=GetButtonSize, write=SetButtonSize, nodefault};
	__property TElScrollThumbMode ThumbMode = {read=FThumbMode, write=SetThumbMode, default=1};
	__property int ThumbSize = {read=GetThumbSize, write=SetThumbSize, default=0};
	__property int MinThumbSize = {read=FMinThumbSize, write=SetMinThumbSize, default=4};
	__property int Page = {read=FPage, write=SetPage, default=1};
	__property bool SecondaryButtons = {read=FSecondaryButtons, write=SetSecondaryButtons, default=0};
	__property TElSecButtonsKind SecondBtnKind = {read=FSecondBtnKind, write=SetSecondBtnKind, default=0};
	__property bool NoDisableButtons = {read=FNoDisableButtons, write=SetNoDisableButtons, default=0};
	__property bool ShowTrackHint = {read=FShowTrackHint, write=FShowTrackHint, default=0};
	__property bool IsDesigning = {read=GetIsDesigning, write=SetIsDesigning, default=0};
	__property bool BlendBackground = {read=FBlendBackground, write=SetBlendBackground, default=1};
	__property bool ShowLeftArrows = {read=FShowLeftArrows, write=SetShowLeftArrows, default=1};
	__property bool ShowRightArrows = {read=FShowRightArrows, write=SetShowRightArrows, default=1};
	__property bool ChangeColorsOnScroll = {read=FChangeColorsOnScroll, write=FChangeColorsOnScroll, default=1};
	__property bool NoScrollMessages = {read=FNoScrollMessages, write=FNoScrollMessages, default=0};
	__property bool NoSunkenThumb = {read=FNoSunkenThumb, write=SetNoSunkenThumb, default=0};
	__property bool UseSystemMetrics = {read=FUseSystemMetrics, write=SetUseSystemMetrics, default=1};
	__property bool DrawFrames = {read=FDrawFrames, write=SetDrawFrames, default=1};
	__property bool DrawArrowFrames = {read=FDrawArrowFrames, write=SetDrawArrowFrames, default=1};
	__property bool DrawBars = {read=FDrawBars, write=SetDrawBars, default=1};
	__property unsigned BarOffset = {read=FBarOffset, write=SetBarOffset, default=2};
	__property Graphics::TColor BarColor = {read=FBarColor, write=SetBarColor, default=-2147483632};
	__property bool IsHTML = {read=FIsHTML, write=FIsHTML, default=0};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property TElScrollHitTestEvent OnHitTest = {read=FOnHitTest, write=FOnHitTest};
	__property TElScrollEvent OnScroll = {read=FOnScroll, write=FOnScroll};
	__property TElScrollHintNeededEvent OnScrollHintNeeded = {read=FOnScrollHintNeeded, write=FOnScrollHintNeeded};
	__property bool OwnerDraw = {read=FOwnerDraw, write=SetOwnerDraw, nodefault};
	__property TElScrollDrawPartEvent OnDrawPart = {read=FOnDrawPart, write=FOnDrawPart};
	__property Graphics::TColor ArrowColor = {read=FArrowColor, write=SetArrowColor, default=-2147483630};
	__property Graphics::TColor ArrowHotTrackColor = {read=FArrowHotTrackColor, write=SetArrowHotTrackColor, default=-2147483635};
	__property bool ThinFrames = {read=FThinFrames, write=SetThinFrames, default=0};
	__property Graphics::TColor HintColor = {read=FHintColor, write=FHintColor, default=-2147483624};
	__property Graphics::TColor HintTextColor = {read=FHintTextColor, write=FHintTextColor, default=-2147483625};
	__property bool SystemBkColor = {read=FSysBkColor, write=SetSysBkColor, default=1};
	__property bool AltDisablingArrows = {read=FAltDisablingArrows, write=SetAltDisablingArrows, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	
public:
	__fastcall virtual TCustomElScrollBar(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElScrollBar(void);
	virtual void __fastcall Loaded(void);
	TElScrollBarPart __fastcall GetHitTest(int X, int Y);
	virtual void __fastcall Paint(void);
	DYNAMIC void __fastcall Resize(void);
	void __fastcall EndScroll(void);
	int __fastcall SetScrollInfo(const tagSCROLLINFO &ScrollInfo, BOOL Redraw);
	BOOL __fastcall GetScrollInfo(tagSCROLLINFO &ScrollInfo);
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElScrollBar(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElScrollBarStyles;
class PASCALIMPLEMENTATION TElScrollBarStyles : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
private:
	TCustomElScrollBar* FElScrollBar;
	Classes::TNotifyEvent FOnChange;
	int __fastcall GetButtonSize(void);
	void __fastcall SetButtonSize(int newValue);
	bool __fastcall GetNoDisableButtons(void);
	void __fastcall SetNoDisableButtons(bool newValue);
	bool __fastcall GetNoSunkenThumb(void);
	void __fastcall SetNoSunkenThumb(bool newValue);
	void __fastcall SetActiveFlat(bool newValue);
	bool __fastcall GetActiveFlat(void);
	void __fastcall SetColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetColor(void);
	void __fastcall SetFlat(bool newValue);
	bool __fastcall GetFlat(void);
	void __fastcall SetMinThumbSize(int newValue);
	int __fastcall GetMinThumbSize(void);
	void __fastcall SetOwnerDraw(bool newValue);
	bool __fastcall GetOwnerDraw(void);
	void __fastcall SetSecondaryButtons(bool newValue);
	bool __fastcall GetSecondaryButtons(void);
	void __fastcall SetSecondBtnKind(TElSecButtonsKind newValue);
	TElSecButtonsKind __fastcall GetSecondBtnKind(void);
	void __fastcall SetShowTrackHint(bool newValue);
	bool __fastcall GetShowTrackHint(void);
	void __fastcall SetThumbMode(TElScrollThumbMode newValue);
	TElScrollThumbMode __fastcall GetThumbMode(void);
	void __fastcall SetThumbSize(int newValue);
	int __fastcall GetThumbSize(void);
	bool __fastcall GetBlendBackground(void);
	void __fastcall SetBlendBackground(bool newValue);
	int __fastcall GetWidth(void);
	void __fastcall SetWidth(int newValue);
	bool __fastcall GetShowLeftArrows(void);
	void __fastcall SetShowLeftArrows(bool newValue);
	bool __fastcall GetShowRightArrows(void);
	void __fastcall SetShowRightArrows(bool newValue);
	bool __fastcall GetUseSystemMetrics(void);
	void __fastcall SetUseSystemMetrics(bool Value);
	Graphics::TColor __fastcall GetArrowColor(void);
	void __fastcall SetArrowColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetArrowHotTrackColor(void);
	void __fastcall SetArrowHotTrackColor(Graphics::TColor newValue);
	bool __fastcall GetDrawFrames(void);
	void __fastcall SetDrawFrames(bool newValue);
	bool __fastcall GetThinFrames(void);
	void __fastcall SetThinFrames(bool newValue);
	Graphics::TColor __fastcall GetHintColor(void);
	void __fastcall SetHintColor(Graphics::TColor Value);
	Graphics::TColor __fastcall GetHintTextColor(void);
	void __fastcall SetHintTextColor(Graphics::TColor Value);
	bool __fastcall GetDrawBars(void);
	void __fastcall SetDrawBars(const bool Value);
	Graphics::TColor __fastcall GetBarColor(void);
	void __fastcall SetBarColor(const Graphics::TColor Value);
	unsigned __fastcall GetBarOffset(void);
	void __fastcall SetBarOffset(const unsigned Value);
	Elimgfrm::TElImageForm* __fastcall GetImageForm(void);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	bool __fastcall GetDrawArrowFrames(void);
	void __fastcall SetDrawArrowFrames(const bool Value);
	bool __fastcall GetUseXPThemes(void);
	void __fastcall SetUseXPThemes(const bool Value);
	
protected:
	bool __fastcall GetSysBkColor(void);
	void __fastcall SetSysBkColor(bool Value);
	
public:
	__fastcall TElScrollBarStyles(TCustomElScrollBar* AControl);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	
__published:
	__property bool ActiveFlat = {read=GetActiveFlat, write=SetActiveFlat, default=0};
	__property bool BlendBackground = {read=GetBlendBackground, write=SetBlendBackground, default=1};
	__property Graphics::TColor Color = {read=GetColor, write=SetColor, default=-2147483633};
	__property bool Flat = {read=GetFlat, write=SetFlat, default=1};
	__property int MinThumbSize = {read=GetMinThumbSize, write=SetMinThumbSize, default=4};
	__property bool NoDisableButtons = {read=GetNoDisableButtons, write=SetNoDisableButtons, default=0};
	__property bool NoSunkenThumb = {read=GetNoSunkenThumb, write=SetNoSunkenThumb, default=0};
	__property bool OwnerDraw = {read=GetOwnerDraw, write=SetOwnerDraw, default=0};
	__property bool SecondaryButtons = {read=GetSecondaryButtons, write=SetSecondaryButtons, default=0};
	__property TElSecButtonsKind SecondBtnKind = {read=GetSecondBtnKind, write=SetSecondBtnKind, default=0};
	__property bool ShowLeftArrows = {read=GetShowLeftArrows, write=SetShowLeftArrows, default=1};
	__property bool ShowRightArrows = {read=GetShowRightArrows, write=SetShowRightArrows, default=1};
	__property bool ShowTrackHint = {read=GetShowTrackHint, write=SetShowTrackHint, nodefault};
	__property TElScrollThumbMode ThumbMode = {read=GetThumbMode, write=SetThumbMode, default=1};
	__property int ThumbSize = {read=GetThumbSize, write=SetThumbSize, default=0};
	__property int Width = {read=GetWidth, write=SetWidth, nodefault};
	__property int ButtonSize = {read=GetButtonSize, write=SetButtonSize, nodefault};
	__property bool UseSystemMetrics = {read=GetUseSystemMetrics, write=SetUseSystemMetrics, default=1};
	__property Graphics::TColor ArrowColor = {read=GetArrowColor, write=SetArrowColor, default=-2147483630};
	__property Graphics::TColor ArrowHotTrackColor = {read=GetArrowHotTrackColor, write=SetArrowHotTrackColor, default=-2147483635};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property bool DrawFrames = {read=GetDrawFrames, write=SetDrawFrames, default=1};
	__property bool DrawArrowFrames = {read=GetDrawArrowFrames, write=SetDrawArrowFrames, default=1};
	__property bool DrawBars = {read=GetDrawBars, write=SetDrawBars, default=1};
	__property unsigned BarOffset = {read=GetBarOffset, write=SetBarOffset, default=2};
	__property Graphics::TColor BarColor = {read=GetBarColor, write=SetBarColor, default=-2147483632};
	__property bool ThinFrames = {read=GetThinFrames, write=SetThinFrames, default=0};
	__property Graphics::TColor HintColor = {read=GetHintColor, write=SetHintColor, default=-2147483624};
	__property Graphics::TColor HintTextColor = {read=GetHintTextColor, write=SetHintTextColor, default=-2147483625};
	__property bool SystemBkColor = {read=GetSysBkColor, write=SetSysBkColor, default=1};
	__property Elimgfrm::TElImageForm* ImageForm = {read=GetImageForm, write=SetImageForm};
	__property bool UseXPThemes = {read=GetUseXPThemes, write=SetUseXPThemes, default=1};
public:
	#pragma option push -w-inl
	/* TPersistent.Destroy */ inline __fastcall virtual ~TElScrollBarStyles(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElScrollBar;
class PASCALIMPLEMENTATION TElScrollBar : public TCustomElScrollBar 
{
	typedef TCustomElScrollBar inherited;
	
__published:
	__property AltDisablingArrows  = {default=0};
	__property ArrowColor  = {default=-2147483630};
	__property ArrowHotTrackColor  = {default=-2147483635};
	__property Kind  = {default=0};
	__property Position  = {default=0};
	__property Max  = {default=100};
	__property Min  = {default=0};
	__property Flat  = {default=1};
	__property ActiveFlat  = {default=1};
	__property BlendBackground  = {default=1};
	__property SystemBkColor  = {default=1};
	__property ButtonSize ;
	__property ChangeColorsOnScroll  = {default=1};
	__property DrawFrames  = {default=1};
	__property DrawArrowFrames  = {default=1};
	__property DrawBars  = {default=1};
	__property BarOffset  = {default=2};
	__property BarColor  = {default=-2147483632};
	__property ImageForm ;
	__property IsHTML  = {default=0};
	__property MinThumbSize  = {default=4};
	__property NoDisableButtons  = {default=0};
	__property NoSunkenThumb  = {default=0};
	__property Page  = {default=1};
	__property SecondaryButtons  = {default=0};
	__property SecondBtnKind  = {default=0};
	__property ShowLeftArrows  = {default=1};
	__property ShowRightArrows  = {default=1};
	__property ShowTrackHint  = {default=0};
	__property Step  = {default=1};
	__property ThinFrames  = {default=0};
	__property ThumbMode  = {default=1};
	__property ThumbSize  = {default=0};
	__property UseSystemMetrics  = {default=1};
	__property UseXPThemes  = {default=1};
	__property Align  = {default=0};
	__property Color ;
	__property Ctl3D ;
	__property Enabled  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
	__property Visible  = {default=1};
	__property OnHitTest ;
	__property OnChange ;
	__property OnScroll ;
	__property OnScrollHintNeeded ;
	__property OwnerDraw ;
	__property OnDrawPart ;
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnMouseDown ;
	__property OnMouseUp ;
	__property OnMouseMove ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
public:
	#pragma option push -w-inl
	/* TCustomElScrollBar.Create */ inline __fastcall virtual TElScrollBar(Classes::TComponent* AOwner) : TCustomElScrollBar(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElScrollBar.Destroy */ inline __fastcall virtual ~TElScrollBar(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElScrollBar(HWND ParentWindow) : TCustomElScrollBar(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elscrollbar */
using namespace Elscrollbar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElScrollBar
