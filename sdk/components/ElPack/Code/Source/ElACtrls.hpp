// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElACtrls.pas' rev: 6.00

#ifndef ElACtrlsHPP
#define ElACtrlsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elactrls
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElAdvancedMemo;
class PASCALIMPLEMENTATION TElAdvancedMemo : public Stdctrls::TMemo 
{
	typedef Stdctrls::TMemo inherited;
	
private:
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Graphics::TBitmap* FBackground;
	bool FFlat;
	bool FFlatFocusedScrollBars;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FMouseOver;
	bool FPainting;
	bool FPaintingTo;
	bool FTransparent;
	bool FUseBackground;
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elvclutils::TElBorderSides FBorderSides;
	bool FHandleDialogKeys;
	unsigned FTheme;
	WideString FHint;
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall BackgroundChanged(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMParentColorChanged(Messages::TMessage &Msg);
	MESSAGE void __fastcall CNCtlColorEdit(Messages::TWMCtlColor &Msg);
	MESSAGE void __fastcall CNCtlColorStatic(Messages::TWMCtlColor &Msg);
	HIDESBASE MESSAGE void __fastcall WMGetDlgCode(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMove(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	void __fastcall DrawBackground(HDC DC, const Types::TRect &R);
	void __fastcall DrawFlatBorder(HDC DC);
	void __fastcall DrawParentControl(HDC DC);
	void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetBackground(const Graphics::TBitmap* Value);
	void __fastcall SetFlat(const bool Value);
	void __fastcall SetFlatFocusedScrollBars(const bool Value);
	void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetTransparent(const bool Value);
	void __fastcall SetUseBackground(const bool Value);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	
protected:
	bool FUseXPThemes;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	DYNAMIC void __fastcall Change(void);
	DYNAMIC void __fastcall DoMouseEnter(void);
	DYNAMIC void __fastcall DoMouseLeave(void);
	DYNAMIC void __fastcall DoPaint(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	void __fastcall SetUseXPThemes(bool Value);
	bool __fastcall IsThemeApplied(void);
	virtual void __fastcall FreeThemeHandle(void);
	virtual void __fastcall CreateThemeHandle(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	WideString __fastcall GetThemedClassName();
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElAdvancedMemo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElAdvancedMemo(void);
	__property unsigned Theme = {read=FTheme, nodefault};
	
__published:
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Align  = {default=0};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property bool FlatFocusedScrollBars = {read=FFlatFocusedScrollBars, write=SetFlatFocusedScrollBars, default=0};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property bool UseBackground = {read=FUseBackground, write=SetUseBackground, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Classes::TNotifyEvent OnMouseEnter = {read=FOnMouseEnter, write=FOnMouseEnter};
	__property Classes::TNotifyEvent OnMouseLeave = {read=FOnMouseLeave, write=FOnMouseLeave};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property bool HandleDialogKeys = {read=FHandleDialogKeys, write=FHandleDialogKeys, default=0};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElAdvancedMemo(HWND ParentWindow) : Stdctrls::TMemo(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TCustomElAdvancedEdit;
class PASCALIMPLEMENTATION TCustomElAdvancedEdit : public Stdctrls::TCustomEdit 
{
	typedef Stdctrls::TCustomEdit inherited;
	
private:
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Classes::TAlignment FAlignment;
	Graphics::TBitmap* FBackground;
	bool FFlat;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FMouseOver;
	bool FPainting;
	bool FPaintingTo;
	bool FReturnPressed;
	bool FTransparent;
	bool FUseBackground;
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	bool FHandleDialogKeys;
	Elvclutils::TElBorderSides FBorderSides;
	unsigned FTheme;
	WideString FHint;
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall BackgroundChanged(System::TObject* Sender);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMParentColorChanged(Messages::TMessage &Msg);
	MESSAGE void __fastcall CNCtlColorEdit(Messages::TWMCtlColor &Msg);
	MESSAGE void __fastcall CNCtlColorStatic(Messages::TWMCtlColor &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMove(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	void __fastcall DrawBackground(HDC DC, const Types::TRect &R);
	void __fastcall DrawFlatBorder(HDC DC);
	void __fastcall DrawParentControl(HDC DC);
	void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetAlignment(const Classes::TAlignment Value);
	void __fastcall SetBackground(const Graphics::TBitmap* Value);
	void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetTransparent(const bool Value);
	void __fastcall SetUseBackground(const bool Value);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	
protected:
	bool FNoHandleEnter;
	char FPasswordChar;
	bool FUseXPThemes;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	virtual void __fastcall SetFlat(const bool Value);
	DYNAMIC void __fastcall Change(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	DYNAMIC void __fastcall DoMouseEnter(void);
	DYNAMIC void __fastcall DoMouseLeave(void);
	DYNAMIC void __fastcall DoPaint(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, default=0};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property bool UseBackground = {read=FUseBackground, write=SetUseBackground, default=0};
	__property Classes::TNotifyEvent OnMouseEnter = {read=FOnMouseEnter, write=FOnMouseEnter};
	__property Classes::TNotifyEvent OnMouseLeave = {read=FOnMouseLeave, write=FOnMouseLeave};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	HIDESBASE void __fastcall SetPasswordChar(char Value);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	virtual void __fastcall SetUseXPThemes(bool Value);
	bool __fastcall IsThemeApplied(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	virtual void __fastcall FreeThemeHandle(void);
	virtual void __fastcall CreateThemeHandle(void);
	WideString __fastcall GetThemedClassName();
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall WMPaste(Messages::TMessage &Message);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TCustomElAdvancedEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElAdvancedEdit(void);
	__property bool MouseOver = {read=FMouseOver, nodefault};
	__property unsigned Theme = {read=FTheme, nodefault};
	
__published:
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property bool HandleDialogKeys = {read=FHandleDialogKeys, write=FHandleDialogKeys, default=0};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property char PasswordChar = {read=FPasswordChar, write=SetPasswordChar, stored=false, default=0};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElAdvancedEdit(HWND ParentWindow) : Stdctrls::TCustomEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElAdvancedEdit;
class PASCALIMPLEMENTATION TElAdvancedEdit : public TCustomElAdvancedEdit 
{
	typedef TCustomElAdvancedEdit inherited;
	
__published:
	__property ActiveBorderType  = {default=1};
	__property Align  = {default=0};
	__property Alignment  = {default=0};
	__property Anchors  = {default=3};
	__property AutoSelect  = {default=1};
	__property AutoSize  = {default=1};
	__property Background ;
	__property BiDiMode ;
	__property BorderStyle  = {default=1};
	__property CharCase  = {default=0};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Cursor  = {default=0};
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Flat  = {default=0};
	__property Font ;
	__property HideSelection  = {default=1};
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property MaxLength  = {default=0};
	__property OEMConvert  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PasswordChar  = {default=0};
	__property PopupMenu ;
	__property ReadOnly  = {default=0};
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property Text ;
	__property Transparent  = {default=0};
	__property UseBackground  = {default=0};
	__property Visible  = {default=1};
	__property OnChange ;
	__property OnClick ;
	__property OnContextPopup ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDock ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMouseDown ;
	__property OnMouseEnter ;
	__property OnMouseMove ;
	__property OnMouseLeave ;
	__property OnMouseUp ;
	__property OnStartDock ;
	__property OnStartDrag ;
public:
	#pragma option push -w-inl
	/* TCustomElAdvancedEdit.Create */ inline __fastcall virtual TElAdvancedEdit(Classes::TComponent* AOwner) : TCustomElAdvancedEdit(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElAdvancedEdit.Destroy */ inline __fastcall virtual ~TElAdvancedEdit(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElAdvancedEdit(HWND ParentWindow) : TCustomElAdvancedEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElAdvancedListBox;
class PASCALIMPLEMENTATION TElAdvancedListBox : public Stdctrls::TListBox 
{
	typedef Stdctrls::TListBox inherited;
	
private:
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Graphics::TBitmap* FBackground;
	bool FFlat;
	bool FFlatFocusedScrollBars;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FInvertSelection;
	int FLastTopIndex;
	bool FMouseOver;
	Graphics::TColor FSelectedColor;
	Graphics::TFont* FSelectedFont;
	bool FTransparent;
	bool FUseBackground;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	bool FInVScroll;
	bool FInHScroll;
	bool FTransparentSelection;
	Elvclutils::TElBorderSides FBorderSides;
	bool FShowLineHint;
	int FCurHintItem;
	Stdctrls::TListBoxStyle FStyle;
	int FMaxWidth;
	bool FHorizontalScroll;
	Extctrls::TTimer* FHintTimer;
	Controls::THintWindow* FHintWnd;
	Classes::TWndMethod FHintWndProc;
	unsigned FTheme;
	WideString FHint;
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall BackgroundChanged(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMParentColorChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMParentFontChanged(Messages::TMessage &Msg);
	MESSAGE void __fastcall LBGetTopIndex(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMVScroll(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMHScroll(Messages::TMessage &Message);
	MESSAGE void __fastcall WMNCMouseMove(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	void __fastcall DrawBackground(HDC DC, const Types::TRect &R);
	void __fastcall DrawBackgroundEx(HDC DC, const Types::TRect &R, const Types::TRect &SubR);
	void __fastcall DrawFlatBorder(HDC DC, bool HDragging, bool VDragging);
	void __fastcall DrawParentControl(HDC DC);
	void __fastcall DrawParentControlEx(HDC DC, const Types::TRect &R);
	void __fastcall IntMouseMove(short XPos, short YPos);
	void __fastcall SelectedFontChanged(System::TObject* Sender);
	void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetBackground(const Graphics::TBitmap* Value);
	void __fastcall SetFlat(const bool Value);
	void __fastcall SetFlatFocusedScrollBars(const bool Value);
	void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetInvertSelection(const bool Value);
	void __fastcall SetSelectedColor(const Graphics::TColor Value);
	void __fastcall SetSelectedFont(const Graphics::TFont* Value);
	void __fastcall SetTransparent(const bool Value);
	void __fastcall SetUseBackground(const bool Value);
	void __fastcall SetTransparentSelection(bool Value);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	HIDESBASE void __fastcall SetStyle(Stdctrls::TListBoxStyle Value);
	void __fastcall SetHorizontalScroll(bool Value);
	void __fastcall ResetHorizontalExtent(void);
	void __fastcall SetHorizontalExtent(void);
	void __fastcall CancelLineHint(void);
	void __fastcall OnLineHintTimer(System::TObject* Sender);
	void __fastcall HintWndProc(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	
protected:
	bool FUseXPThemes;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	virtual int __fastcall GetItemWidth(int Index);
	virtual int __fastcall GetParentCtlWidth(void);
	virtual int __fastcall GetParentCtlHeight(void);
	virtual Types::TPoint __fastcall RealScreenToClient(const Types::TPoint &APoint);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &R, Windows::TOwnerDrawState State);
	virtual Graphics::TBitmap* __fastcall GetBackground(void);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual Controls::THintWindow* __fastcall CreateHintWindow(void);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	void __fastcall SetUseXPThemes(bool Value);
	bool __fastcall IsThemeApplied(void);
	virtual void __fastcall FreeThemeHandle(void);
	virtual void __fastcall CreateThemeHandle(void);
	WideString __fastcall GetThemedClassName();
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElAdvancedListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElAdvancedListBox(void);
	__property unsigned Theme = {read=FTheme, nodefault};
	
__published:
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Graphics::TBitmap* Background = {read=GetBackground, write=SetBackground};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property bool FlatFocusedScrollBars = {read=FFlatFocusedScrollBars, write=SetFlatFocusedScrollBars, default=0};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
	__property bool InvertSelection = {read=FInvertSelection, write=SetInvertSelection, default=0};
	__property Graphics::TColor SelectedColor = {read=FSelectedColor, write=SetSelectedColor, default=-2147483635};
	__property Graphics::TFont* SelectedFont = {read=FSelectedFont, write=SetSelectedFont};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property bool UseBackground = {read=FUseBackground, write=SetUseBackground, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property bool TransparentSelection = {read=FTransparentSelection, write=SetTransparentSelection, default=0};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property bool ShowLineHint = {read=FShowLineHint, write=FShowLineHint, default=0};
	__property Stdctrls::TListBoxStyle Style = {read=FStyle, write=SetStyle, default=0};
	__property bool HorizontalScroll = {read=FHorizontalScroll, write=SetHorizontalScroll, nodefault};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElAdvancedListBox(HWND ParentWindow) : Stdctrls::TListBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElAdvancedComboBox;
class PASCALIMPLEMENTATION TElAdvancedComboBox : public Stdctrls::TComboBox 
{
	typedef Stdctrls::TComboBox inherited;
	
private:
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FFlat;
	Graphics::TCanvas* BtnCanvas;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	bool FAutoCompletion;
	void *FListInstance;
	void *FEditInstance;
	int FSaveEditWndProc;
	int FSaveListWndProc;
	Classes::TWndMethod FListWindowProc;
	Classes::TWndMethod FEditWindowProc;
	bool FHorizontalScroll;
	bool FInHScroll;
	bool FInVScroll;
	unsigned FTheme;
	WideString FHint;
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetFlat(bool newValue);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Message);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TMessage &Message);
	bool __fastcall IsFocused(void);
	void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetHorizontalScroll(bool Value);
	void __fastcall SetHorizontalExtent(void);
	void __fastcall ResetHorizontalExtent(void);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	
protected:
	HWND FListHandle;
	HWND FEditHandle;
	bool FMouseOver;
	int FHorzPos;
	int FMaxWidth;
	bool FBtnFlat;
	bool FBtnTransparent;
	bool FUseXPThemes;
	bool FBtnThinFrame;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	void __fastcall DrawFlatBorder(bool DrawButton);
	void __fastcall UpdateFrame(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	HIDESBASE virtual void __fastcall ListWndProc(Messages::TMessage &Message);
	HIDESBASE virtual void __fastcall EditWndProc(Messages::TMessage &Message);
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall SetBtnFlat(bool Value);
	void __fastcall SetBtnTransparent(bool Value);
	virtual int __fastcall GetItemWidth(int Index);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	void __fastcall SetUseXPThemes(bool Value);
	bool __fastcall IsThemeApplied(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	virtual void __fastcall FreeThemeHandle(void);
	virtual void __fastcall CreateThemeHandle(void);
	WideString __fastcall GetThemedClassName();
	void __fastcall SetBtnThinFrame(bool Value);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	__property bool AutoCompletion = {read=FAutoCompletion, write=FAutoCompletion, nodefault};
	
public:
	__fastcall virtual TElAdvancedComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElAdvancedComboBox(void);
	__property HWND ListHandle = {read=FListHandle, nodefault};
	__property HWND EditHandle = {read=FEditHandle, nodefault};
	__property Classes::TWndMethod ListWindowProc = {read=FListWindowProc, write=FListWindowProc};
	__property Classes::TWndMethod EditWindowProc = {read=FEditWindowProc, write=FEditWindowProc};
	__property unsigned Theme = {read=FTheme, nodefault};
	
__published:
	__property Align  = {default=0};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
	__property bool BtnFlat = {read=FBtnFlat, write=SetBtnFlat, default=0};
	__property bool BtnTransparent = {read=FBtnTransparent, write=SetBtnTransparent, default=0};
	__property bool HorizontalScroll = {read=FHorizontalScroll, write=SetHorizontalScroll, nodefault};
	__property ItemIndex  = {default=-1};
	__property ItemHeight ;
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property bool BtnThinFrame = {read=FBtnThinFrame, write=SetBtnThinFrame, default=1};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElAdvancedComboBox(HWND ParentWindow) : Stdctrls::TComboBox(ParentWindow) { }
	#pragma option pop
	
};


typedef TCustomElAdvancedEdit TCustomElFlatEdit;
;

typedef TElAdvancedEdit TElFlatEdit;
;

typedef TElAdvancedMemo TElFlatMemo;
;

typedef TElAdvancedListBox TElFlatListBox;
;

typedef TElAdvancedComboBox TElFlatComboBox;
;

//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elactrls */
using namespace Elactrls;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElACtrls
