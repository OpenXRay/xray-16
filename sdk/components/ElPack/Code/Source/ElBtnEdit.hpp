// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElBtnEdit.pas' rev: 6.00

#ifndef ElBtnEditHPP
#define ElBtnEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElScrollBar.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elbtnedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TCustomElButtonEdit;
class PASCALIMPLEMENTATION TCustomElButtonEdit : public Eledits::TCustomElEdit 
{
	typedef Eledits::TCustomElEdit inherited;
	
private:
	Classes::TNotifyEvent FOnAltButtonClick;
	Classes::TShortCut FButtonShortcut;
	Classes::TShortCut FAltButtonShortcut;
	Classes::TNotifyEvent FOnButtonClick;
	void __fastcall SetButtonClickSound(AnsiString newValue);
	AnsiString __fastcall GetButtonClickSound();
	void __fastcall SetButtonDownSound(AnsiString newValue);
	AnsiString __fastcall GetButtonDownSound();
	void __fastcall SetButtonSoundMap(Elsndmap::TElSoundMap* newValue);
	Elsndmap::TElSoundMap* __fastcall GetButtonSoundMap(void);
	void __fastcall SetButtonUpSound(AnsiString newValue);
	AnsiString __fastcall GetButtonUpSound();
	void __fastcall SetAltButtonClickSound(AnsiString newValue);
	AnsiString __fastcall GetAltButtonClickSound();
	void __fastcall SetAltButtonUpSound(AnsiString newValue);
	AnsiString __fastcall GetAltButtonUpSound();
	void __fastcall SetAltButtonSoundMap(Elsndmap::TElSoundMap* newValue);
	Elsndmap::TElSoundMap* __fastcall GetAltButtonSoundMap(void);
	void __fastcall SetButtonColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetButtonColor(void);
	void __fastcall SetButtonDown(bool newValue);
	bool __fastcall GetButtonDown(void);
	void __fastcall SetButtonGlyph(Graphics::TBitmap* newValue);
	Graphics::TBitmap* __fastcall GetButtonGlyph(void);
	void __fastcall SetButtonHint(AnsiString newValue);
	AnsiString __fastcall GetButtonHint();
	void __fastcall SetButtonIcon(Graphics::TIcon* newValue);
	Graphics::TIcon* __fastcall GetButtonIcon(void);
	void __fastcall SetButtonNumGlyphs(int newValue);
	int __fastcall GetButtonNumGlyphs(void);
	void __fastcall SetButtonUseIcon(bool newValue);
	bool __fastcall GetButtonUseIcon(void);
	void __fastcall SetButtonWidth(int newValue);
	int __fastcall GetButtonWidth(void);
	void __fastcall ButtonClickTransfer(System::TObject* Sender);
	void __fastcall SetButtonVisible(bool newValue);
	bool __fastcall GetButtonVisible(void);
	void __fastcall SetAltButtonDown(bool newValue);
	bool __fastcall GetAltButtonDown(void);
	void __fastcall SetAltButtonDownSound(AnsiString newValue);
	AnsiString __fastcall GetAltButtonDownSound();
	void __fastcall SetAltButtonFlat(bool newValue);
	bool __fastcall GetAltButtonFlat(void);
	void __fastcall SetAltButtonGlyph(Graphics::TBitmap* newValue);
	Graphics::TBitmap* __fastcall GetAltButtonGlyph(void);
	void __fastcall SetAltButtonIcon(Graphics::TIcon* newValue);
	Graphics::TIcon* __fastcall GetAltButtonIcon(void);
	void __fastcall SetAltButtonNumGlyphs(int newValue);
	int __fastcall GetAltButtonNumGlyphs(void);
	void __fastcall SetAltButtonUseIcon(bool newValue);
	bool __fastcall GetAltButtonUseIcon(void);
	void __fastcall SetAltButtonVisible(bool newValue);
	bool __fastcall GetAltButtonVisible(void);
	void __fastcall SetAltButtonWidth(int newValue);
	int __fastcall GetAltButtonWidth(void);
	void __fastcall AltButtonClickTransfer(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMCtl3DChanged(Messages::TMessage &Msg);
	void __fastcall SetButtonFlat(bool newValue);
	bool __fastcall GetButtonFlat(void);
	void __fastcall SetAltButtonEnabled(bool newValue);
	bool __fastcall GetAltButtonEnabled(void);
	void __fastcall SetButtonEnabled(bool newValue);
	bool __fastcall GetButtonEnabled(void);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	void __fastcall SetAltButtonHint(AnsiString newValue);
	AnsiString __fastcall GetAltButtonHint();
	void __fastcall SetAltButtonPopupPlace(Elpopbtn::TPopupPlace newValue);
	Elpopbtn::TPopupPlace __fastcall GetAltButtonPopupPlace(void);
	void __fastcall SetAltButtonPullDownMenu(Menus::TPopupMenu* newValue);
	Menus::TPopupMenu* __fastcall GetAltButtonPullDownMenu(void);
	void __fastcall SetButtonPopupPlace(Elpopbtn::TPopupPlace newValue);
	Elpopbtn::TPopupPlace __fastcall GetButtonPopupPlace(void);
	void __fastcall SetButtonPullDownMenu(Menus::TPopupMenu* newValue);
	Menus::TPopupMenu* __fastcall GetButtonPullDownMenu(void);
	void __fastcall SetAltButtonCaption(AnsiString newValue);
	AnsiString __fastcall GetAltButtonCaption();
	void __fastcall SetButtonCaption(AnsiString newValue);
	AnsiString __fastcall GetButtonCaption();
	HIDESBASE void __fastcall SetMultiline(bool newValue);
	void __fastcall SetAltBtnAlign(Classes::TLeftRight newValue);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	
protected:
	Elpopbtn::TCustomElGraphicButton* FAltButton;
	Elpopbtn::TCustomElGraphicButton* FButton;
	Classes::TAlignment FAltBtnAlign;
	bool FMultiline;
	TMetaClass*ButtonClass;
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	DYNAMIC void __fastcall KeyPress(char &Key);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall Loaded(void);
	HIDESBASE void __fastcall SetEditRect(void);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	Graphics::TColor __fastcall GetAltButtonColor(void);
	void __fastcall SetAltButtonColor(Graphics::TColor Value);
	bool __fastcall GetButtonThinFrame(void);
	void __fastcall SetButtonThinFrame(bool Value);
	bool __fastcall GetAltButtonThinFrame(void);
	void __fastcall SetAltButtonThinFrame(bool Value);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	void __fastcall UpdateButtonStyles(void);
	virtual void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	virtual void __fastcall SetFlat(const bool Value);
	virtual void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	virtual void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	virtual void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	bool __fastcall GetButtonTransparent(void);
	void __fastcall SetButtonTransparent(bool Value);
	bool __fastcall GetAltButtonTransparent(void);
	void __fastcall SetAltButtonTransparent(bool Value);
	__property AnsiString ButtonCaption = {read=GetButtonCaption, write=SetButtonCaption};
	__property AnsiString ButtonClickSound = {read=GetButtonClickSound, write=SetButtonClickSound};
	__property Graphics::TColor ButtonColor = {read=GetButtonColor, write=SetButtonColor, nodefault};
	__property bool ButtonDown = {read=GetButtonDown, write=SetButtonDown, nodefault};
	__property AnsiString ButtonDownSound = {read=GetButtonDownSound, write=SetButtonDownSound};
	__property bool ButtonFlat = {read=GetButtonFlat, write=SetButtonFlat, nodefault};
	__property Graphics::TBitmap* ButtonGlyph = {read=GetButtonGlyph, write=SetButtonGlyph};
	__property AnsiString ButtonHint = {read=GetButtonHint, write=SetButtonHint};
	__property Graphics::TIcon* ButtonIcon = {read=GetButtonIcon, write=SetButtonIcon};
	__property int ButtonNumGlyphs = {read=GetButtonNumGlyphs, write=SetButtonNumGlyphs, nodefault};
	__property Elpopbtn::TPopupPlace ButtonPopupPlace = {read=GetButtonPopupPlace, write=SetButtonPopupPlace, nodefault};
	__property Menus::TPopupMenu* ButtonPullDownMenu = {read=GetButtonPullDownMenu, write=SetButtonPullDownMenu};
	__property Elsndmap::TElSoundMap* ButtonSoundMap = {read=GetButtonSoundMap, write=SetButtonSoundMap};
	__property AnsiString ButtonUpSound = {read=GetButtonUpSound, write=SetButtonUpSound};
	__property bool ButtonUseIcon = {read=GetButtonUseIcon, write=SetButtonUseIcon, nodefault};
	__property int ButtonWidth = {read=GetButtonWidth, write=SetButtonWidth, nodefault};
	__property bool ButtonEnabled = {read=GetButtonEnabled, write=SetButtonEnabled, nodefault};
	__property Classes::TNotifyEvent OnButtonClick = {read=FOnButtonClick, write=FOnButtonClick};
	__property Classes::TShortCut ButtonShortcut = {read=FButtonShortcut, write=FButtonShortcut, nodefault};
	__property Classes::TShortCut AltButtonShortcut = {read=FAltButtonShortcut, write=FAltButtonShortcut, nodefault};
	__property bool ButtonVisible = {read=GetButtonVisible, write=SetButtonVisible, nodefault};
	__property AnsiString AltButtonCaption = {read=GetAltButtonCaption, write=SetAltButtonCaption};
	__property AnsiString AltButtonClickSound = {read=GetAltButtonClickSound, write=SetAltButtonClickSound};
	__property AnsiString AltButtonDownSound = {read=GetAltButtonDownSound, write=SetAltButtonDownSound};
	__property Elsndmap::TElSoundMap* AltButtonSoundMap = {read=GetAltButtonSoundMap, write=SetAltButtonSoundMap};
	__property AnsiString AltButtonUpSound = {read=GetAltButtonUpSound, write=SetAltButtonUpSound};
	__property Graphics::TColor AltButtonColor = {read=GetAltButtonColor, write=SetAltButtonColor, nodefault};
	__property bool AltButtonDown = {read=GetAltButtonDown, write=SetAltButtonDown, nodefault};
	__property bool AltButtonFlat = {read=GetAltButtonFlat, write=SetAltButtonFlat, nodefault};
	__property Graphics::TBitmap* AltButtonGlyph = {read=GetAltButtonGlyph, write=SetAltButtonGlyph};
	__property AnsiString AltButtonHint = {read=GetAltButtonHint, write=SetAltButtonHint};
	__property Graphics::TIcon* AltButtonIcon = {read=GetAltButtonIcon, write=SetAltButtonIcon};
	__property int AltButtonNumGlyphs = {read=GetAltButtonNumGlyphs, write=SetAltButtonNumGlyphs, nodefault};
	__property Elpopbtn::TPopupPlace AltButtonPopupPlace = {read=GetAltButtonPopupPlace, write=SetAltButtonPopupPlace, nodefault};
	__property Classes::TLeftRight AltButtonPosition = {read=FAltBtnAlign, write=SetAltBtnAlign, default=1};
	__property Menus::TPopupMenu* AltButtonPullDownMenu = {read=GetAltButtonPullDownMenu, write=SetAltButtonPullDownMenu};
	__property bool AltButtonUseIcon = {read=GetAltButtonUseIcon, write=SetAltButtonUseIcon, nodefault};
	__property bool AltButtonVisible = {read=GetAltButtonVisible, write=SetAltButtonVisible, nodefault};
	__property int AltButtonWidth = {read=GetAltButtonWidth, write=SetAltButtonWidth, nodefault};
	__property bool AltButtonEnabled = {read=GetAltButtonEnabled, write=SetAltButtonEnabled, nodefault};
	__property bool Multiline = {read=FMultiline, write=SetMultiline, nodefault};
	__property Classes::TNotifyEvent OnAltButtonClick = {read=FOnAltButtonClick, write=FOnAltButtonClick};
	
public:
	__fastcall virtual TCustomElButtonEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElButtonEdit(void);
	virtual void __fastcall CreateWnd(void);
	
__published:
	__property bool ButtonThinFrame = {read=GetButtonThinFrame, write=SetButtonThinFrame, default=0};
	__property bool AltButtonThinFrame = {read=GetAltButtonThinFrame, write=SetAltButtonThinFrame, default=0};
	__property bool ButtonTransparent = {read=GetButtonTransparent, write=SetButtonTransparent, default=0};
	__property bool AltButtonTransparent = {read=GetAltButtonTransparent, write=SetAltButtonTransparent, default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElButtonEdit(HWND ParentWindow) : Eledits::TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElButtonEdit;
class PASCALIMPLEMENTATION TElButtonEdit : public TCustomElButtonEdit 
{
	typedef TCustomElButtonEdit inherited;
	
__published:
	__property AlignBottom  = {default=1};
	__property CharCase  = {default=0};
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property AutoSize  = {default=1};
	__property RTLContent ;
	__property BorderSides ;
	__property PasswordChar ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property FlatFocusedScrollBars  = {default=0};
	__property WantTabs  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property Lines  = {stored=false};
	__property ImageForm ;
	__property WordWrap  = {default=1};
	__property ScrollBars  = {default=0};
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Text ;
	__property Multiline ;
	__property VertScrollBarStyles ;
	__property HorzScrollBarStyles ;
	__property UseCustomScrollBars ;
	__property Flat  = {default=0};
	__property ActiveBorderType  = {default=1};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property UseBackground  = {default=0};
	__property Alignment ;
	__property AutoSelect  = {default=0};
	__property Background ;
	__property ButtonCaption ;
	__property ButtonClickSound ;
	__property ButtonDownSound ;
	__property ButtonUpSound ;
	__property ButtonSoundMap ;
	__property ButtonColor ;
	__property ButtonDown ;
	__property ButtonEnabled ;
	__property ButtonFlat ;
	__property ButtonGlyph ;
	__property ButtonHint ;
	__property ButtonIcon ;
	__property ButtonNumGlyphs ;
	__property ButtonPopupPlace ;
	__property ButtonPullDownMenu ;
	__property ButtonShortcut ;
	__property ButtonUseIcon ;
	__property ButtonVisible ;
	__property ButtonWidth ;
	__property OnButtonClick ;
	__property AltButtonCaption ;
	__property AltButtonClickSound ;
	__property AltButtonDownSound ;
	__property AltButtonUpSound ;
	__property AltButtonSoundMap ;
	__property AltButtonColor ;
	__property AltButtonDown ;
	__property AltButtonEnabled ;
	__property AltButtonFlat ;
	__property AltButtonGlyph ;
	__property AltButtonHint ;
	__property AltButtonIcon ;
	__property AltButtonNumGlyphs ;
	__property AltButtonPopupPlace ;
	__property AltButtonPosition  = {default=1};
	__property AltButtonPullDownMenu ;
	__property AltButtonShortcut ;
	__property AltButtonUseIcon ;
	__property AltButtonVisible ;
	__property AltButtonWidth ;
	__property OnAltButtonClick ;
	__property BorderStyle ;
	__property Ctl3D ;
	__property ParentCtl3D  = {default=1};
	__property Enabled  = {default=1};
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property PopupMenu ;
	__property Color  = {default=-2147483643};
	__property ParentColor  = {default=1};
	__property Align  = {default=0};
	__property Font ;
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property ReadOnly  = {default=0};
	__property OnEnter ;
	__property OnExit ;
	__property OnClick ;
	__property OnDblClick ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDock ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDock ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TCustomElButtonEdit.Create */ inline __fastcall virtual TElButtonEdit(Classes::TComponent* AOwner) : TCustomElButtonEdit(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElButtonEdit.Destroy */ inline __fastcall virtual ~TElButtonEdit(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElButtonEdit(HWND ParentWindow) : TCustomElButtonEdit(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elbtnedit */
using namespace Elbtnedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElBtnEdit
