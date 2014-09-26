// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'MxMenus.pas' rev: 6.00

#ifndef MxMenusHPP
#define MxMenusHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxmenus
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TMxMenuStyle { msStandard, msOwnerDraw, msBtnLowered, msBtnRaised };
#pragma option pop

#pragma option push -b-
enum MxMenus__1 { mdSelected, mdGrayed, mdDisabled, mdChecked, mdFocused, mdDefault };
#pragma option pop

typedef Set<MxMenus__1, mdSelected, mdDefault>  TMenuOwnerDrawState;

typedef void __fastcall (__closure *TDrawMenuItemEvent)(Menus::TMenu* Sender, Menus::TMenuItem* Item, const Types::TRect &Rect, TMenuOwnerDrawState State);

typedef void __fastcall (__closure *TMeasureMenuItemEvent)(Menus::TMenu* Sender, Menus::TMenuItem* Item, int &Width, int &Height);

typedef void __fastcall (__closure *TDrawMarginEvent)(Menus::TMenu* Sender, const Types::TRect &Rect);

typedef void __fastcall (__closure *TItemParamsEvent)(Menus::TMenu* Sender, Menus::TMenuItem* Item, TMenuOwnerDrawState State, Graphics::TFont* AFont, Graphics::TColor &Color, Graphics::TGraphic* &Graphic, int &NumGlyphs);

typedef void __fastcall (__closure *TItemImageEvent)(Menus::TMenu* Sender, Menus::TMenuItem* Item, TMenuOwnerDrawState State, int &ImageIndex);

class DELPHICLASS TMxPopupMenu;
class PASCALIMPLEMENTATION TMxPopupMenu : public Menus::TPopupMenu 
{
	typedef Menus::TPopupMenu inherited;
	
private:
	Graphics::TColor FMarginStartColor;
	Graphics::TColor FMarginEndColor;
	Graphics::TColor FSepHColor;
	Graphics::TColor FSepLColor;
	Graphics::TColor FBKColor;
	Graphics::TColor FSelColor;
	Graphics::TColor FSelFontColor;
	Graphics::TColor FFontColor;
	TMxMenuStyle FStyle;
	Graphics::TCanvas* FCanvas;
	bool FShowCheckMarks;
	unsigned FMinTextOffset;
	unsigned FLeftMargin;
	Controls::TCursor FCursor;
	TDrawMenuItemEvent FOnDrawItem;
	TMeasureMenuItemEvent FOnMeasureItem;
	TDrawMarginEvent FOnDrawMargin;
	TItemParamsEvent FOnGetItemParams;
	#pragma pack(push, 1)
	Types::TPoint FPopupPoint;
	#pragma pack(pop)
	
	bool FParentBiDiMode;
	Controls::TImageList* FImages;
	Imglist::TChangeLink* FImageChangeLink;
	TItemImageEvent FOnGetImageIndex;
	HIDESBASE void __fastcall SetImages(Controls::TImageList* Value);
	HIDESBASE void __fastcall ImageListChange(System::TObject* Sender);
	void __fastcall SetStyle(TMxMenuStyle Value);
	void __fastcall WndMessage(System::TObject* Sender, Messages::TMessage &AMsg, bool &Handled);
	MESSAGE void __fastcall WMDrawItem(Messages::TWMDrawItem &Message);
	MESSAGE void __fastcall WMMeasureItem(Messages::TWMMeasureItem &Message);
	HIDESBASE void __fastcall SetBiDiModeFromPopupControl(void);
	
protected:
	virtual void __fastcall Loaded(void);
	HIDESBASE bool __fastcall UseRightToLeftAlignment(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	DYNAMIC void __fastcall GetImageIndex(Menus::TMenuItem* Item, TMenuOwnerDrawState State, int &ImageIndex);
	virtual void __fastcall DrawItem(Menus::TMenuItem* Item, const Types::TRect &Rect, TMenuOwnerDrawState State);
	virtual void __fastcall DrawMargin(const Types::TRect &ARect);
	DYNAMIC void __fastcall GetItemParams(Menus::TMenuItem* Item, TMenuOwnerDrawState State, Graphics::TFont* AFont, Graphics::TColor &Color, Graphics::TGraphic* &Graphic, int &NumGlyphs);
	DYNAMIC void __fastcall MeasureItem(Menus::TMenuItem* Item, int &Width, int &Height);
	virtual void __fastcall RefreshMenu(bool AOwnerDraw);
	bool __fastcall IsOwnerDrawMenu(void);
	
public:
	__fastcall virtual TMxPopupMenu(Classes::TComponent* AOwner);
	__fastcall virtual ~TMxPopupMenu(void);
	void __fastcall Refresh(void);
	virtual void __fastcall Popup(int X, int Y);
	void __fastcall DefaultDrawItem(Menus::TMenuItem* Item, const Types::TRect &Rect, TMenuOwnerDrawState State);
	void __fastcall DefaultDrawMargin(const Types::TRect &ARect, Graphics::TColor StartColor, Graphics::TColor EndColor);
	__property Graphics::TCanvas* Canvas = {read=FCanvas};
	
__published:
	__property Controls::TCursor Cursor = {read=FCursor, write=FCursor, default=0};
	__property Graphics::TColor MarginStartColor = {read=FMarginStartColor, write=FMarginStartColor, default=16711680};
	__property Graphics::TColor MarginEndColor = {read=FMarginEndColor, write=FMarginEndColor, default=16711680};
	__property Graphics::TColor BKColor = {read=FBKColor, write=FBKColor, default=-2147483633};
	__property Graphics::TColor SelColor = {read=FSelColor, write=FSelColor, default=-2147483632};
	__property Graphics::TColor SelFontColor = {read=FSelFontColor, write=FSelFontColor, default=-2147483634};
	__property Graphics::TColor FontColor = {read=FFontColor, write=FFontColor, default=-2147483641};
	__property Graphics::TColor SepHColor = {read=FSepHColor, write=FSepHColor, default=-2147483632};
	__property Graphics::TColor SepLColor = {read=FSepLColor, write=FSepLColor, default=-2147483628};
	__property unsigned LeftMargin = {read=FLeftMargin, write=FLeftMargin, default=0};
	__property unsigned MinTextOffset = {read=FMinTextOffset, write=FMinTextOffset, default=0};
	__property TMxMenuStyle Style = {read=FStyle, write=SetStyle, default=0};
	__property bool ShowCheckMarks = {read=FShowCheckMarks, write=FShowCheckMarks, default=1};
	__property OwnerDraw  = {stored=false, default=0};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property TItemImageEvent OnGetImageIndex = {read=FOnGetImageIndex, write=FOnGetImageIndex};
	__property TDrawMenuItemEvent OnDrawItem = {read=FOnDrawItem, write=FOnDrawItem};
	__property TDrawMarginEvent OnDrawMargin = {read=FOnDrawMargin, write=FOnDrawMargin};
	__property TItemParamsEvent OnGetItemParams = {read=FOnGetItemParams, write=FOnGetItemParams};
	__property TMeasureMenuItemEvent OnMeasureItem = {read=FOnMeasureItem, write=FOnMeasureItem};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool __fastcall IsItemPopup(Menus::TMenuItem* Item);
extern PACKAGE void __fastcall SetDefaultMenuFont(Graphics::TFont* AFont);

}	/* namespace Mxmenus */
using namespace Mxmenus;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// MxMenus
