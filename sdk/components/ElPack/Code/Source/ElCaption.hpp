// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCaption.pas' rev: 6.00

#ifndef ElCaptionHPP
#define ElCaptionHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElHook.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
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

namespace Elcaption
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElCaptionText;
class PASCALIMPLEMENTATION TElCaptionText : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	Graphics::TColor FActiveColor;
	Graphics::TColor FInactiveColor;
	Graphics::TFont* FFont;
	bool FVisible;
	WideString FCaption;
	bool FOwnerStyle;
	Buttons::TButtonLayout FLayout;
	Elpopbtn::TElButtonGlyph* FGlyph;
	Classes::TAlignment FAlign;
	void __fastcall SetOwnerStyle(bool newValue);
	void __fastcall SetActiveColor(Graphics::TColor newValue);
	void __fastcall SetInactiveColor(Graphics::TColor newValue);
	void __fastcall SetFont(Graphics::TFont* newValue);
	void __fastcall SetVisible(bool newValue);
	void __fastcall SetCaption(WideString newValue);
	void __fastcall FontChange(System::TObject* Sender);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* newValue);
	void __fastcall SetLayout(Buttons::TButtonLayout newValue);
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall SetAlign(Classes::TAlignment newValue);
	
protected:
	virtual void __fastcall Paint(Graphics::TCanvas* Canvas, const Types::TRect &R);
	int __fastcall GetWidth(Graphics::TCanvas* Canvas, int Height);
	
public:
	__fastcall virtual TElCaptionText(Classes::TCollection* Collection);
	__fastcall virtual ~TElCaptionText(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	
__published:
	__property Graphics::TColor ActiveColor = {read=FActiveColor, write=SetActiveColor, default=-2147483639};
	__property Graphics::TColor InactiveColor = {read=FInactiveColor, write=SetInactiveColor, default=-2147483629};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property bool Visible = {read=FVisible, write=SetVisible, default=1};
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property bool OwnerStyle = {read=FOwnerStyle, write=SetOwnerStyle, default=1};
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property Buttons::TButtonLayout Layout = {read=FLayout, write=SetLayout, nodefault};
	__property Classes::TAlignment Align = {read=FAlign, write=SetAlign, nodefault};
};


class DELPHICLASS TElCaptionTexts;
class DELPHICLASS TElFormCaption;
class DELPHICLASS TElCaptionButtons;
class DELPHICLASS TElCaptionButton;
class PASCALIMPLEMENTATION TElCaptionButtons : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElCaptionButton* operator[](int index) { return Items[index]; }
	
private:
	TElFormCaption* FCaption;
	
protected:
	TElCaptionButton* __fastcall GetItems(int Index);
	void __fastcall SetItems(int Index, TElCaptionButton* newValue);
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	
public:
	HIDESBASE TElCaptionButton* __fastcall Add(void);
	__property TElCaptionButton* Items[int index] = {read=GetItems, write=SetItems/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Create */ inline __fastcall TElCaptionButtons(TMetaClass* ItemClass) : Classes::TCollection(ItemClass) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElCaptionButtons(void) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TElPaintBkgndType { pbtActive, pbtInactive, pbtAlways };
#pragma option pop

typedef void __fastcall (__closure *TElCaptionDrawEvent)(System::TObject* Sender, Graphics::TCanvas* Canvas, Types::TRect &Rect);

typedef void __fastcall (__closure *TCaptionButtonEvent)(System::TObject* Sender, TElCaptionButton* Button);

class PASCALIMPLEMENTATION TElFormCaption : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	TElCaptionTexts* FTexts;
	Graphics::TBitmap* FInactiveBitmap;
	TElCaptionButtons* FButtons;
	TElPaintBkgndType FPaintBkgnd;
	Menus::TPopupMenu* FPopupMenu;
	Graphics::TBitmap* FBitmap;
	Elhook::TElHook* FHook;
	bool FActive;
	Graphics::TColor FActiveLeftColor;
	Graphics::TColor FActiveRightColor;
	Graphics::TColor FInactiveLeftColor;
	Graphics::TColor FInactiveRightColor;
	Elvclutils::TElBkGndType FBackgroundType;
	int FNumColors;
	Classes::TAlignment FAlignment;
	Forms::TForm* FForm;
	bool FSystemFont;
	Graphics::TFont* FFont;
	Graphics::TFont* Font2;
	bool FWndActive;
	HRGN FRegion;
	HRGN FSaveRegion;
	bool FClicked;
	bool FInBtn;
	TElCaptionButton* FBtnPressed;
	#pragma pack(push, 1)
	Types::TRect FBtnsRect;
	#pragma pack(pop)
	
	TElCaptionDrawEvent FOnDrawCaption;
	TCaptionButtonEvent FOnButtonClick;
	TCaptionButtonEvent FOnButtonDblClick;
	bool FPreventUpdate;
	unsigned FTheme;
	void __fastcall SetActive(bool newValue);
	void __fastcall SetActiveLeftColor(Graphics::TColor newValue);
	void __fastcall SetActiveRightColor(Graphics::TColor newValue);
	void __fastcall SetInactiveLeftColor(Graphics::TColor newValue);
	void __fastcall SetBackgroundType(Elvclutils::TElBkGndType newValue);
	void __fastcall SetPopupMenu(Menus::TPopupMenu* newValue);
	void __fastcall SetNumColors(int newValue);
	void __fastcall SetAlignment(Classes::TAlignment newValue);
	void __fastcall SetBitmap(Graphics::TBitmap* newValue);
	void __fastcall BitmapChange(System::TObject* Sender);
	void __fastcall OnBeforeHook(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall OnAfterHook(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall SetPaintBkgnd(TElPaintBkgndType newValue);
	void __fastcall SetInactiveRightColor(Graphics::TColor newValue);
	void __fastcall SetButtons(TElCaptionButtons* newValue);
	void __fastcall SetInactiveBitmap(Graphics::TBitmap* newValue);
	void __fastcall SetSystemFont(bool newValue);
	void __fastcall SetFont(Graphics::TFont* newValue);
	void __fastcall FontChange(System::TObject* Sender);
	void __fastcall GetSystemFont(void);
	void __fastcall SetTexts(TElCaptionTexts* newValue);
	
protected:
	bool FUseXPThemes;
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	void __fastcall Update(void);
	virtual void __fastcall PaintCaption(Messages::TMessage &Msg, int Step);
	virtual void __fastcall TriggerDrawCaptionEvent(Graphics::TCanvas* Canvas, Types::TRect &Rect);
	virtual void __fastcall TriggerButtonClickEvent(TElCaptionButton* Button);
	virtual void __fastcall TriggerButtonDblClickEvent(TElCaptionButton* Button);
	void __fastcall SetUseXPThemes(bool Value);
	virtual void __fastcall CreateThemeHandle(void);
	virtual void __fastcall FreeThemeHandle(void);
	void __fastcall AllocThemes(void);
	
public:
	__fastcall virtual TElFormCaption(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFormCaption(void);
	TElCaptionButton* __fastcall ButtonAtPos(int X, int Y);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	bool __fastcall IsThemeApplied(void);
	
__published:
	__property bool Active = {read=FActive, write=SetActive, nodefault};
	__property Graphics::TColor ActiveLeftColor = {read=FActiveLeftColor, write=SetActiveLeftColor, default=0};
	__property Graphics::TColor ActiveRightColor = {read=FActiveRightColor, write=SetActiveRightColor, default=-2147483646};
	__property Graphics::TColor InactiveLeftColor = {read=FInactiveLeftColor, write=SetInactiveLeftColor, default=0};
	__property Graphics::TColor InactiveRightColor = {read=FInactiveRightColor, write=SetInactiveRightColor, default=-2147483645};
	__property Elvclutils::TElBkGndType BackgroundType = {read=FBackgroundType, write=SetBackgroundType, default=2};
	__property Menus::TPopupMenu* PopupMenu = {read=FPopupMenu, write=SetPopupMenu};
	__property int NumColors = {read=FNumColors, write=SetNumColors, default=64};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, nodefault};
	__property Graphics::TBitmap* ActiveBitmap = {read=FBitmap, write=SetBitmap};
	__property TElPaintBkgndType PaintBkgnd = {read=FPaintBkgnd, write=SetPaintBkgnd, nodefault};
	__property TElCaptionButtons* Buttons = {read=FButtons, write=SetButtons};
	__property Graphics::TBitmap* InactiveBitmap = {read=FInactiveBitmap, write=SetInactiveBitmap};
	__property TElCaptionDrawEvent OnDrawCaption = {read=FOnDrawCaption, write=FOnDrawCaption};
	__property TCaptionButtonEvent OnButtonClick = {read=FOnButtonClick, write=FOnButtonClick};
	__property TCaptionButtonEvent OnButtonDblClick = {read=FOnButtonDblClick, write=FOnButtonDblClick};
	__property bool SystemFont = {read=FSystemFont, write=SetSystemFont, nodefault};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property TElCaptionTexts* Texts = {read=FTexts, write=SetTexts};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
};


class PASCALIMPLEMENTATION TElCaptionTexts : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElCaptionText* operator[](int index) { return Items[index]; }
	
private:
	TElFormCaption* FCaption;
	
protected:
	TElCaptionText* __fastcall GetItems(int Index);
	void __fastcall SetItems(int Index, TElCaptionText* newValue);
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	
public:
	HIDESBASE TElCaptionText* __fastcall Add(void);
	__property TElCaptionText* Items[int index] = {read=GetItems, write=SetItems/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Create */ inline __fastcall TElCaptionTexts(TMetaClass* ItemClass) : Classes::TCollection(ItemClass) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElCaptionTexts(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElCaptionButton : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	Classes::TAlignment FAlign;
	WideString FCaption;
	Elpopbtn::TElButtonGlyph* FGlyph;
	TElCaptionButtons* FButtons;
	bool FEnabled;
	bool FFixClick;
	bool FDown;
	bool FVisible;
	bool FOwnerStyle;
	Graphics::TColor FActiveColor;
	Graphics::TColor FInactiveColor;
	Graphics::TFont* FFont;
	Buttons::TButtonLayout FLayout;
	#pragma pack(push, 1)
	Types::TRect FBtnRect;
	#pragma pack(pop)
	
	Classes::TNotifyEvent FOnClick;
	Classes::TNotifyEvent FOnDblClick;
	void __fastcall SetLayout(Buttons::TButtonLayout newValue);
	void __fastcall SetFont(Graphics::TFont* newValue);
	void __fastcall SetOwnerStyle(bool newValue);
	void __fastcall SetActiveColor(Graphics::TColor newValue);
	void __fastcall SetInactiveColor(Graphics::TColor newValue);
	void __fastcall SetVisible(bool newValue);
	void __fastcall SetEnabled(bool newValue);
	void __fastcall SetDown(bool newValue);
	void __fastcall SetAlign(Classes::TAlignment newValue);
	void __fastcall SetCaption(WideString newValue);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* newValue);
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall FontChange(System::TObject* Sender);
	
protected:
	virtual void __fastcall Paint(Graphics::TCanvas* Canvas, const Types::TRect &R);
	int __fastcall GetWidth(Graphics::TCanvas* Canvas, int Height);
	
public:
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	__fastcall virtual TElCaptionButton(Classes::TCollection* Collection);
	__fastcall virtual ~TElCaptionButton(void);
	
__published:
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property Buttons::TButtonLayout Layout = {read=FLayout, write=SetLayout, nodefault};
	__property Classes::TAlignment Align = {read=FAlign, write=SetAlign, nodefault};
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property bool Enabled = {read=FEnabled, write=SetEnabled, default=1};
	__property bool FixClick = {read=FFixClick, write=FFixClick, nodefault};
	__property bool Down = {read=FDown, write=SetDown, default=0};
	__property bool Visible = {read=FVisible, write=SetVisible, default=1};
	__property bool OwnerStyle = {read=FOwnerStyle, write=SetOwnerStyle, default=1};
	__property Graphics::TColor ActiveColor = {read=FActiveColor, write=SetActiveColor, nodefault};
	__property Graphics::TColor InactiveColor = {read=FInactiveColor, write=SetInactiveColor, nodefault};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property Classes::TNotifyEvent OnClick = {read=FOnClick, write=FOnClick};
	__property Classes::TNotifyEvent OnDblClick = {read=FOnDblClick, write=FOnDblClick};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcaption */
using namespace Elcaption;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCaption
