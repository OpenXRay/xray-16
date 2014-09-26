// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHTMLLbl.pas' rev: 6.00

#ifndef ElHTMLLblHPP
#define ElHTMLLblHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElHandPt.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhtmllbl
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElHTMLLabel;
class PASCALIMPLEMENTATION TElHTMLLabel : public Stdctrls::TCustomLabel 
{
	typedef Stdctrls::TCustomLabel inherited;
	
private:
	Controls::TCursor FCursor;
	Htmlrender::TElHTMLRender* FRender;
	bool FIsHTML;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Graphics::TColor FLinkColor;
	Menus::TPopupMenu* FLinkPopupMenu;
	Graphics::TFontStyles FLinkStyle;
	WideString FHint;
	HIDESBASE MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	void __fastcall SetLinkPopupMenu(Menus::TPopupMenu* newValue);
	
protected:
	WideString FCaption;
	DYNAMIC void __fastcall AdjustBounds(void);
	virtual void __fastcall SetIsHTML(bool newValue);
	virtual void __fastcall TriggerLinkClickEvent(WideString HRef);
	void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	virtual void __fastcall Loaded(void);
	HIDESBASE virtual void __fastcall SetCursor(Controls::TCursor newValue);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall Click(void);
	virtual bool __fastcall GetWordWrap(void);
	HIDESBASE virtual void __fastcall SetWordWrap(bool newValue);
	virtual void __fastcall SetAutoSize(bool newValue);
	Types::TRect __fastcall GetTextRect();
	void __fastcall SetCaption(WideString Value);
	DYNAMIC void __fastcall DoDrawText(Types::TRect &Rect, int Flags);
	virtual void __fastcall SetLinkColor(Graphics::TColor newValue);
	virtual void __fastcall SetLinkStyle(Graphics::TFontStyles newValue);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Message);
	void __fastcall DoLinkPopup(const Types::TPoint &MousePos);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TWMContextMenu &Message);
	Types::TRect __fastcall CalcTextRect();
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	virtual void __fastcall SetName(const AnsiString Value);
	
public:
	__fastcall virtual TElHTMLLabel(Classes::TComponent* AOwner);
	__fastcall virtual ~TElHTMLLabel(void);
	virtual void __fastcall Paint(void);
	virtual void __fastcall SetBounds(int ALeft, int ATop, int AWidth, int AHeight);
	__property Types::TRect TextRect = {read=GetTextRect};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
	__property Controls::TCursor Cursor = {read=FCursor, write=SetCursor, nodefault};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, nodefault};
	__property bool WordWrap = {read=GetWordWrap, write=SetWordWrap, nodefault};
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property Graphics::TColor LinkColor = {read=FLinkColor, write=SetLinkColor, nodefault};
	__property Menus::TPopupMenu* LinkPopupMenu = {read=FLinkPopupMenu, write=SetLinkPopupMenu};
	__property Graphics::TFontStyles LinkStyle = {read=FLinkStyle, write=SetLinkStyle, nodefault};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Align  = {default=0};
	__property Alignment  = {default=0};
	__property AutoSize  = {default=1};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Color ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property FocusControl ;
	__property Font ;
	__property ParentBiDiMode  = {default=1};
	__property ParentColor  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Transparent  = {default=0};
	__property Layout  = {default=0};
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnContextPopup ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnEndDock ;
	__property OnStartDock ;
	__property OnStartDrag ;
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elhtmllbl */
using namespace Elhtmllbl;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHTMLLbl
