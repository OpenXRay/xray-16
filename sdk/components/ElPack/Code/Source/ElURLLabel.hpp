// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElURLLabel.pas' rev: 6.00

#ifndef ElURLLabelHPP
#define ElURLLabelHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ShellAPI.hpp>	// Pascal unit
#include <ElCLabel.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <ElHandPt.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elurllabel
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElURLLabel;
class PASCALIMPLEMENTATION TElURLLabel : public Elclabel::TElCustomLabel 
{
	typedef Elclabel::TElCustomLabel inherited;
	
private:
	bool FShowURLInHint;
	bool FVisited;
	Graphics::TColor FVisitedColor;
	AnsiString FURI;
	Graphics::TColor FHyperLinkColour;
	Graphics::TColor FOldColour;
	Graphics::TFontStyles FHyperLinkStyle;
	Graphics::TFontStyles FOldStyle;
	WideString FHint;
	void __fastcall SetHyperLinkStyle(const Graphics::TFontStyles Value);
	void __fastcall SetHyperLinkColour(const Graphics::TColor Value);
	void __fastcall SetVisitedColor(Graphics::TColor newValue);
	void __fastcall SetVisited(bool newValue);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Message);
	
protected:
	DYNAMIC Menus::TPopupMenu* __fastcall GetPopupMenu(void);
	void __fastcall OnOpen(System::TObject* Sender);
	void __fastcall OnCopy(System::TObject* Sender);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElURLLabel(Classes::TComponent* AOwner);
	DYNAMIC void __fastcall Click(void);
	void __fastcall GotoURL(void);
	void __fastcall CopyURL(void);
	
__published:
	__property AnsiString URL = {read=FURI, write=FURI};
	__property Graphics::TColor VisitedColor = {read=FVisitedColor, write=SetVisitedColor, nodefault};
	__property bool Visited = {read=FVisited, write=SetVisited, nodefault};
	__property Graphics::TColor HyperLinkColor = {read=FHyperLinkColour, write=SetHyperLinkColour, nodefault};
	__property Graphics::TFontStyles HyperLinkStyle = {read=FHyperLinkStyle, write=SetHyperLinkStyle, nodefault};
	__property bool ShowURLInHint = {read=FShowURLInHint, write=FShowURLInHint, default=1};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property Align  = {default=0};
	__property Alignment  = {default=0};
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
	__property AutoSize  = {default=1};
	__property Caption ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property FocusControl ;
	__property Font ;
	__property ShowHint ;
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowAccelChar  = {default=1};
	__property Transparent  = {default=0};
	__property Visible  = {default=1};
	__property WordWrap  = {default=0};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDrag ;
public:
	#pragma option push -w-inl
	/* TGraphicControl.Destroy */ inline __fastcall virtual ~TElURLLabel(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elurllabel */
using namespace Elurllabel;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElURLLabel
