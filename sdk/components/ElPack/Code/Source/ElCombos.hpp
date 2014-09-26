// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCombos.pas' rev: 6.00

#ifndef ElCombosHPP
#define ElCombosHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElScrollBar.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElFrmPers.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElListBox.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcombos
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElComboButton;
class PASCALIMPLEMENTATION TElComboButton : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	bool FFlat;
	bool FFocused;
	bool FMouseOver;
	bool FTransparent;
	Graphics::TColor FArrowColor;
	Elpopbtn::TElButtonGlyph* FGlyph;
	bool FDrawFrame;
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	void __fastcall SetFocused(const bool Value);
	void __fastcall SetDown(const bool Value);
	void __fastcall SetTransparent(const bool Value);
	void __fastcall SetDrawFrame(bool Value);
	
protected:
	bool ExtGlyph;
	bool FDown;
	bool KeepColor;
	virtual void __fastcall DrawArrow(const Types::TRect &R);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Paint(void);
	void __fastcall GlyphChanged(System::TObject* Sender);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	
public:
	__fastcall virtual TElComboButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TElComboButton(void);
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property bool Flat = {read=FFlat, write=FFlat, nodefault};
	__property bool Down = {read=FDown, write=SetDown, nodefault};
	__property bool Focused = {read=FFocused, write=SetFocused, nodefault};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, nodefault};
	__property bool DrawFrame = {read=FDrawFrame, write=SetDrawFrame, nodefault};
};


class DELPHICLASS TElComboListBox;
class DELPHICLASS TElComboBox;
class PASCALIMPLEMENTATION TElComboBox : public Eledits::TCustomElEdit 
{
	typedef Eledits::TCustomElEdit inherited;
	
protected:
	bool ChangeAllowed;
	int FDropDownWidth;
	bool FAutoCompletion;
	Classes::TShortCut FAltButtonShortcut;
	Classes::TShortCut FButtonShortcut;
	Classes::TAlignment FAltBtnAlign;
	int FAltBtnWidth;
	bool FBtnFlat;
	bool FBtnTransparent;
	TElComboButton* FAltButton;
	TElComboButton* FButton;
	int FDropDownCount;
	Forms::TForm* FForm;
	int FItemIndex;
	TElComboListBox* FListBox;
	Classes::TNotifyEvent FOnAltBtnClick;
	Classes::TNotifyEvent FOnDropDown;
	Graphics::TColor FSaveColor;
	bool FSaveFlat;
	bool FForcedText;
	bool FIgnoreItemIdx;
	bool FCanDrop;
	bool FDroppedDown;
	bool FAdjustDropDownPos;
	bool __fastcall GetListTransparentSelection(void);
	void __fastcall SetListTransparentSelection(bool Value);
	void __fastcall SetDropDownWidth(const int Value);
	bool __fastcall GetBtnDrawFrame(void);
	void __fastcall SetBtnDrawFrame(bool Value);
	bool __fastcall GetAltBtnDrawFrame(void);
	void __fastcall SetAltBtnDrawFrame(bool Value);
	MESSAGE void __fastcall CMCancelMode(Controls::TCMCancelMode &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetCursor(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall WMMButtonDblClk(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TWMMouseWheel &Msg);
	MESSAGE void __fastcall EMSetReadOnly(Messages::TMessage &Msg);
	MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Message);
	void __fastcall ButtonClick(System::TObject* Sender);
	void __fastcall AltButtonClick(System::TObject* Sender);
	Graphics::TColor __fastcall GetBtnColor(void);
	void __fastcall GetDropDownValue(void);
	bool __fastcall GetDroppedDown(void);
	Elunicodestrings::TElWideStrings* __fastcall GetItems(void);
	Graphics::TColor __fastcall GetListColor(void);
	bool __fastcall GetListInvertSelection(void);
	bool __fastcall GetSorted(void);
	bool __fastcall GetUseBackground(void);
	void __fastcall ListBoxClick(System::TObject* Sender);
	void __fastcall SetBtnColor(const Graphics::TColor Value);
	void __fastcall SetBtnTransparent(const bool Value);
	void __fastcall SetDropDownCount(const int Value);
	void __fastcall SetDroppedDown(const bool Value);
	void __fastcall SetCanDrop(const bool Value);
	HIDESBASE void __fastcall SetEditRect(void);
	void __fastcall SetItemIndex(const int Value);
	void __fastcall SetItems(const Elunicodestrings::TElWideStrings* Value);
	void __fastcall SetListColor(const Graphics::TColor Value);
	void __fastcall SetListInvertSelection(const bool Value);
	void __fastcall SetSorted(const bool Value);
	HIDESBASE void __fastcall SetUseBackground(const bool Value);
	int __fastcall GetDroppedIndex(void);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TMessage &Msg);
	Elimgfrm::TElImageForm* __fastcall GetListImageForm(void);
	void __fastcall SetListImageForm(Elimgfrm::TElImageForm* newValue);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	HIDESBASE MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	virtual void __fastcall DoDropDown(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall SetBtnFlat(bool newValue);
	virtual Graphics::TColor __fastcall GetBtnArrowColor(void);
	virtual void __fastcall SetBtnArrowColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetListSelectedColor(void);
	void __fastcall SetListSelectedColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetAltBtnColor(void);
	void __fastcall SetAltBtnColor(Graphics::TColor Value);
	bool __fastcall GetAltBtnTransparent(void);
	void __fastcall SetAltBtnTransparent(bool Value);
	bool __fastcall GetAltBtnFlat(void);
	void __fastcall SetAltBtnFlat(bool Value);
	Graphics::TBitmap* __fastcall GetAltBtnGlyph(void);
	void __fastcall SetAltBtnGlyph(Graphics::TBitmap* Value);
	void __fastcall SetAltBtnWidth(int Value);
	bool __fastcall GetAltBtnVisible(void);
	void __fastcall SetAltBtnVisible(bool Value);
	void __fastcall SetAltBtnAlign(Classes::TLeftRight Value);
	void __fastcall DoAutoComplete(void);
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TWMKey &Message);
	HIDESBASE void __fastcall SetReadOnly(bool Value);
	virtual void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	virtual void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	virtual void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	virtual void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	virtual void __fastcall SetFlat(const bool Value);
	HIDESBASE MESSAGE void __fastcall CMDialogKey(Messages::TWMKey &Message);
	virtual void __fastcall DestroyWnd(void);
	
public:
	__fastcall virtual TElComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElComboBox(void);
	DYNAMIC void __fastcall Click(void);
	void __fastcall CloseUp(bool AcceptValue);
	void __fastcall DropDown(void);
	__property bool DroppedDown = {read=GetDroppedDown, write=SetDroppedDown, nodefault};
	__property int DroppedIndex = {read=GetDroppedIndex, nodefault};
	
__published:
	__property ActiveBorderType  = {default=1};
	__property Align  = {default=0};
	__property Alignment ;
	__property Anchors  = {default=3};
	__property Background ;
	__property BiDiMode ;
	__property BorderStyle ;
	__property BorderSides ;
	__property VertScrollBarStyles ;
	__property HorzScrollBarStyles ;
	__property UseCustomScrollBars ;
	__property Graphics::TColor BtnColor = {read=GetBtnColor, write=SetBtnColor, default=-2147483633};
	__property bool BtnTransparent = {read=FBtnTransparent, write=SetBtnTransparent, default=0};
	__property bool BtnFlat = {read=FBtnFlat, write=SetBtnFlat, default=0};
	__property Graphics::TColor BtnArrowColor = {read=GetBtnArrowColor, write=SetBtnArrowColor, nodefault};
	__property Graphics::TColor AltBtnColor = {read=GetAltBtnColor, write=SetAltBtnColor, default=-2147483633};
	__property bool AltBtnTransparent = {read=GetAltBtnTransparent, write=SetAltBtnTransparent, default=0};
	__property bool AltBtnFlat = {read=GetAltBtnFlat, write=SetAltBtnFlat, default=0};
	__property Graphics::TBitmap* AltBtnGlyph = {read=GetAltBtnGlyph, write=SetAltBtnGlyph};
	__property bool AltBtnVisible = {read=GetAltBtnVisible, write=SetAltBtnVisible, default=0};
	__property int AltBtnWidth = {read=FAltBtnWidth, write=SetAltBtnWidth, nodefault};
	__property Classes::TLeftRight AltBtnPosition = {read=FAltBtnAlign, write=SetAltBtnAlign, default=1};
	__property Classes::TNotifyEvent OnAltButtonClick = {read=FOnAltBtnClick, write=FOnAltBtnClick};
	__property bool CanDrop = {read=FCanDrop, write=SetCanDrop, default=1};
	__property int DropDownCount = {read=FDropDownCount, write=SetDropDownCount, default=8};
	__property int DropDownWidth = {read=FDropDownWidth, write=SetDropDownWidth, default=-1};
	__property bool ListTransparentSelection = {read=GetListTransparentSelection, write=SetListTransparentSelection, default=0};
	__property bool BtnDrawFrame = {read=GetBtnDrawFrame, write=SetBtnDrawFrame, default=1};
	__property bool AutoCompletion = {read=FAutoCompletion, write=FAutoCompletion, nodefault};
	__property bool AltBtnDrawFrame = {read=GetAltBtnDrawFrame, write=SetAltBtnDrawFrame, default=1};
	__property Elunicodestrings::TElWideStrings* Items = {read=GetItems, write=SetItems};
	__property Graphics::TColor ListColor = {read=GetListColor, write=SetListColor, default=-2147483643};
	__property Elimgfrm::TElImageForm* ListImageForm = {read=GetListImageForm, write=SetListImageForm};
	__property bool ListInvertSelection = {read=GetListInvertSelection, write=SetListInvertSelection, default=0};
	__property Graphics::TColor ListSelectedColor = {read=GetListSelectedColor, write=SetListSelectedColor, nodefault};
	__property bool Sorted = {read=GetSorted, write=SetSorted, default=0};
	__property bool UseBackground = {read=GetUseBackground, write=SetUseBackground, default=0};
	__property Classes::TNotifyEvent OnDropDown = {read=FOnDropDown, write=FOnDropDown};
	__property bool AdjustDropDownPos = {read=FAdjustDropDownPos, write=FAdjustDropDownPos, default=1};
	__property int ItemIndex = {read=FItemIndex, write=SetItemIndex, default=-1};
	__property AutoSize  = {default=1};
	__property CharCase  = {default=0};
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property RTLContent ;
	__property PasswordChar ;
	__property Multiline  = {default=0};
	__property WantTabs  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property Lines  = {stored=false};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Flat  = {default=0};
	__property Font ;
	__property ImageForm ;
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property MaxLength  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property ParentColor  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ReadOnly  = {write=SetReadOnly, default=0};
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Text ;
	__property Transparent ;
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
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnStartDock ;
	__property OnStartDrag ;
	__property Classes::TShortCut AltButtonShortcut = {read=FAltButtonShortcut, write=FAltButtonShortcut, nodefault};
	__property Classes::TShortCut ButtonShortcut = {read=FButtonShortcut, write=FButtonShortcut, nodefault};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElComboBox(HWND ParentWindow) : Eledits::TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElComboListBox : public Ellistbox::TElListBox 
{
	typedef Ellistbox::TElListBox inherited;
	
private:
	TElComboBox* FCombo;
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Msg);
	MESSAGE void __fastcall WMMouseActivate(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMRButtonDown(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CNCommand(Messages::TMessage &Msg);
	
protected:
	DYNAMIC void __fastcall ResetContent(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual Graphics::TBitmap* __fastcall GetBackground(void);
	
public:
	__fastcall virtual TElComboListBox(Classes::TComponent* AOwner);
public:
	#pragma option push -w-inl
	/* TCustomElListBox.Destroy */ inline __fastcall virtual ~TElComboListBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElComboListBox(HWND ParentWindow) : Ellistbox::TElListBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcombos */
using namespace Elcombos;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCombos
