// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ExtBtn.pas' rev: 6.00

#ifndef ExtBtnHPP
#define ExtBtnHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <CommCtrl.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Extbtn
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TButtonLayout { blGlyphLeft, blGlyphRight, blGlyphTop, blGlyphBottom };
#pragma option pop

#pragma option push -b-
enum TButtonState { bsUp, bsDisabled, bsDown, bsExclusive };
#pragma option pop

#pragma option push -b-
enum TButtonStyle { bsAutoDetect, bsWin31, bsNew };
#pragma option pop

#pragma option push -b-
enum TKind { knClose, knMinimize, knNone };
#pragma option pop

typedef Shortint TNumGlyphs;

class DELPHICLASS TExtBtn;
class PASCALIMPLEMENTATION TExtBtn : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	int FGroupIndex;
	void *FGlyph;
	void *FExtGlyph;
	bool FDown;
	bool FDragging;
	bool FAllowAllUp;
	TButtonLayout FLayout;
	int FSpacing;
	bool FTransparent;
	int FMargin;
	int FExtMargin;
	bool FFlat;
	bool FMouseInControl;
	bool FExtButton;
	int FExtWidth;
	bool FExtTransparent;
	Classes::TNotifyEvent FOnExtBtnClick;
	TKind FKind;
	bool FBevelShow;
	bool FThinBorder;
	bool FHotTrack;
	Graphics::TColor FNormalColor;
	Graphics::TColor FHotColor;
	Graphics::TColor FDownColor;
	Graphics::TColor FBtnColor;
	bool FNeedManualMouseUp;
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall UpdateExclusive(void);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	TNumGlyphs __fastcall GetNumGlyphs(void);
	void __fastcall SetNumGlyphs(TNumGlyphs Value);
	void __fastcall SetDown(bool Value);
	void __fastcall SetHotTrack(bool Value);
	void __fastcall SetFlat(bool Value);
	void __fastcall SetAllowAllUp(bool Value);
	void __fastcall SetGroupIndex(int Value);
	void __fastcall SetLayout(TButtonLayout Value);
	void __fastcall SetSpacing(int Value);
	void __fastcall SetTransparent(bool Value);
	void __fastcall SetThinBorder(bool Value);
	void __fastcall SetMargin(int Value);
	void __fastcall SetExtMargin(int Value);
	void __fastcall SetExtButton(bool Value);
	void __fastcall SetBevelShow(bool Value);
	void __fastcall SetExtWidth(int Value);
	void __fastcall SetExtTransparent(bool Value);
	void __fastcall SetKind(TKind Value);
	void __fastcall SetBtnColor(Graphics::TColor Value);
	void __fastcall SetNormalColor(Graphics::TColor Value);
	void __fastcall SetDownColor(Graphics::TColor Value);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMButtonPressed(Messages::TMessage &Message);
	MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	
protected:
	TButtonState FState;
	TButtonState FExtState;
	DYNAMIC void __fastcall ActionChange(System::TObject* Sender, bool CheckDefaults);
	DYNAMIC HPALETTE __fastcall GetPalette(void);
	virtual void __fastcall Loaded(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Paint(void);
	__property bool MouseInControl = {read=FMouseInControl, nodefault};
	void __fastcall UpdateTracking(void);
	
public:
	void __fastcall MouseManualUp(void);
	__fastcall virtual TExtBtn(Classes::TComponent* AOwner);
	__fastcall virtual ~TExtBtn(void);
	DYNAMIC void __fastcall Click(void);
	void __fastcall UpdateMouseInControl(void);
	
__published:
	__property Align  = {default=3};
	__property Action ;
	__property bool AllowAllUp = {read=FAllowAllUp, write=SetAllowAllUp, default=0};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property bool BevelShow = {read=FBevelShow, write=SetBevelShow, default=1};
	__property bool HotTrack = {read=FHotTrack, write=SetHotTrack, default=0};
	__property Graphics::TColor HotColor = {read=FHotColor, write=FHotColor, default=16711680};
	__property Graphics::TColor NormalColor = {read=FNormalColor, write=SetNormalColor, default=0};
	__property Graphics::TColor BtnColor = {read=FBtnColor, write=SetBtnColor, default=-2147483633};
	__property Graphics::TColor DownColor = {read=FDownColor, write=SetDownColor, default=-2147483628};
	__property bool ExtButton = {read=FExtButton, write=SetExtButton, default=0};
	__property int ExtWidth = {read=FExtWidth, write=SetExtWidth, default=14};
	__property bool ExtTransparent = {read=FExtTransparent, write=SetExtTransparent, default=0};
	__property int GroupIndex = {read=FGroupIndex, write=SetGroupIndex, default=0};
	__property bool Down = {read=FDown, write=SetDown, default=0};
	__property Caption ;
	__property Enabled  = {default=1};
	__property bool Flat = {read=FFlat, write=SetFlat, default=1};
	__property Font ;
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property TKind Kind = {read=FKind, write=SetKind, default=0};
	__property TButtonLayout Layout = {read=FLayout, write=SetLayout, default=0};
	__property int Margin = {read=FMargin, write=SetMargin, default=-1};
	__property int ExtMargin = {read=FExtMargin, write=SetExtMargin, default=-1};
	__property TNumGlyphs NumGlyphs = {read=GetNumGlyphs, write=SetNumGlyphs, default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ParentBiDiMode  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property int Spacing = {read=FSpacing, write=SetSpacing, default=4};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=1};
	__property bool FlatAlwaysEdge = {read=FThinBorder, write=SetThinBorder, default=0};
	__property Visible  = {default=1};
	__property Classes::TNotifyEvent OnExtBtnClick = {read=FOnExtBtnClick, write=FOnExtBtnClick};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnStartDrag ;
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE Classes::TList* ExtButtonList;

}	/* namespace Extbtn */
using namespace Extbtn;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ExtBtn
