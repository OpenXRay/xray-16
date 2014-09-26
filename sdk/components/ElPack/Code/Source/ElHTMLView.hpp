// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHTMLView.pas' rev: 6.00

#ifndef ElHTMLViewHPP
#define ElHTMLViewHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElScrollBar.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhtmlview
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElHTMLView;
class PASCALIMPLEMENTATION TElHTMLView : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	bool FFlatFocusedScrollBars;
	bool FUseCustomScrollBars;
	Menus::TPopupMenu* FLinkPopupMenu;
	Controls::TCursor FCursor;
	Graphics::TColor FHighlightColor;
	Graphics::TColor FHighlightBkColor;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Graphics::TColor FLinkColor;
	Graphics::TFontStyles FLinkStyle;
	Elimgfrm::TElImageForm* FImgForm;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FFlat;
	bool FWordWrap;
	bool FMouseOver;
	Forms::TFormBorderStyle FBorderStyle;
	Htmlrender::TElHTMLRender* FRender;
	HDC TmpDC;
	int FGradientSteps;
	Graphics::TColor FGradientStartColor;
	Graphics::TColor FGradientEndColor;
	Graphics::TBitmap* FTmpBmp;
	bool FTransparent;
	Graphics::TBitmap* FBackground;
	Elvclutils::TElBkGndType FBackgroundType;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	AnsiString FDummyString;
	#pragma pack(push, 1)
	Types::TPoint FViewPos;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	Types::TRect FTextRect;
	#pragma pack(pop)
	
	int FScrollStep;
	Elscrollbar::TElScrollBar* FVertScrollBar;
	Elscrollbar::TElScrollBar* FHorzScrollBar;
	bool FVScrollVisible;
	bool FHScrollVisible;
	Elscrollbar::TElScrollBarStyles* FVertScrollBarStyles;
	Elscrollbar::TElScrollBarStyles* FHorzScrollBarStyles;
	Elvclutils::TElBorderSides FBorderSides;
	WideString FHint;
	HIDESBASE MESSAGE void __fastcall WMSysColorChange(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Msg);
	void __fastcall SBChanged(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
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
	void __fastcall DrawFlatBorder(bool HorzTracking, bool VertTracking);
	void __fastcall DrawFlatBorderEx(HDC DC, bool HorzTracking, bool VertTracking);
	void __fastcall OnHScroll(System::TObject* Sender, Elscrollbar::TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);
	void __fastcall OnVScroll(System::TObject* Sender, Elscrollbar::TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TWMMouseWheel &Msg);
	void __fastcall SetLinkPopupMenu(Menus::TPopupMenu* newValue);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMVScroll(Messages::TWMScroll &Msg);
	HIDESBASE MESSAGE void __fastcall WMHScroll(Messages::TWMScroll &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseWheel(Controls::TCMMouseWheel &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TWMContextMenu &Message);
	
protected:
	WideString FCaption;
	bool FUseXPThemes;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	#pragma pack(push, 1)
	Types::TRect FViewRect;
	#pragma pack(pop)
	
	int FMargin;
	virtual void __fastcall SetVertScrollBarStyles(Elscrollbar::TElScrollBarStyles* newValue);
	virtual void __fastcall SetHorzScrollBarStyles(Elscrollbar::TElScrollBarStyles* newValue);
	virtual void __fastcall PrepareText(void);
	virtual void __fastcall SetViewPos(const Types::TPoint &newValue);
	virtual void __fastcall SetWordWrap(bool newValue);
	void __fastcall AdjustScrollBars(void);
	virtual void __fastcall Paint(void);
	void __fastcall UpdateFrame(void);
	virtual void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	virtual void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	virtual void __fastcall SetFlat(bool newValue);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWindowHandle(const Controls::TCreateParams &Params);
	virtual void __fastcall SetLinkColor(Graphics::TColor newValue);
	virtual void __fastcall SetLinkStyle(Graphics::TFontStyles newValue);
	virtual void __fastcall SetHighlightColor(Graphics::TColor newValue);
	virtual void __fastcall SetHighlightBkColor(Graphics::TColor newValue);
	virtual void __fastcall TriggerLinkClickEvent(AnsiString HRef);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	HIDESBASE virtual void __fastcall SetCursor(Controls::TCursor newValue);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall SetUseCustomScrollBars(bool newValue);
	virtual void __fastcall SetFlatFocusedScrollBars(bool newValue);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall SetCaption(WideString newValue);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual WideString __fastcall GetThemedClassName();
	void __fastcall DoLinkPopup(const Types::TPoint &MousePos);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetViewRect(const Types::TRect &Value);
	void __fastcall SetMargin(int Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	__property Types::TRect ViewRect = {read=FViewRect, write=SetViewRect};
	
public:
	__fastcall virtual TElHTMLView(Classes::TComponent* AOwner);
	__fastcall virtual ~TElHTMLView(void);
	DYNAMIC void __fastcall Click(void);
	__property bool VertScrollBarVisible = {read=FVScrollVisible, nodefault};
	__property bool HorzScrollBarVisible = {read=FHScrollVisible, nodefault};
	__property Types::TPoint Position = {read=FViewPos, write=SetViewPos};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property Elscrollbar::TElScrollBarStyles* VertScrollBarStyles = {read=FVertScrollBarStyles, write=SetVertScrollBarStyles};
	__property Elscrollbar::TElScrollBarStyles* HorzScrollBarStyles = {read=FHorzScrollBarStyles, write=SetHorzScrollBarStyles};
	__property bool WordWrap = {read=FWordWrap, write=SetWordWrap, nodefault};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, nodefault};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, nodefault};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, nodefault};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, nodefault};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Graphics::TColor GradientStartColor = {read=FGradientStartColor, write=SetGradientStartColor, nodefault};
	__property Graphics::TColor GradientEndColor = {read=FGradientEndColor, write=SetGradientEndColor, nodefault};
	__property int GradientSteps = {read=FGradientSteps, write=SetGradientSteps, nodefault};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property Elvclutils::TElBkGndType BackgroundType = {read=FBackgroundType, write=SetBackgroundType, nodefault};
	__property Controls::TCursor Cursor = {read=FCursor, write=SetCursor, nodefault};
	__property Graphics::TColor LinkColor = {read=FLinkColor, write=SetLinkColor, nodefault};
	__property Graphics::TFontStyles LinkStyle = {read=FLinkStyle, write=SetLinkStyle, nodefault};
	__property Menus::TPopupMenu* LinkPopupMenu = {read=FLinkPopupMenu, write=SetLinkPopupMenu};
	__property Graphics::TColor HighlightColor = {read=FHighlightColor, write=SetHighlightColor, nodefault};
	__property Graphics::TColor HighlightBkColor = {read=FHighlightBkColor, write=SetHighlightBkColor, nodefault};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property bool UseCustomScrollBars = {read=FUseCustomScrollBars, write=SetUseCustomScrollBars, nodefault};
	__property bool FlatFocusedScrollBars = {read=FFlatFocusedScrollBars, write=SetFlatFocusedScrollBars, nodefault};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property AnsiString Text = {read=FDummyString};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property int Margin = {read=FMargin, write=SetMargin, default=4};
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property Align  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
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
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHTMLView(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elhtmlview */
using namespace Elhtmlview;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHTMLView
