// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHTMLPanel.pas' rev: 6.00

#ifndef ElHTMLPanelHPP
#define ElHTMLPanelHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElImgFrm.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhtmlpanel
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TCustomElHTMLPanel;
class PASCALIMPLEMENTATION TCustomElHTMLPanel : public Elpanel::TCustomElPanel 
{
	typedef Elpanel::TCustomElPanel inherited;
	
private:
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Controls::TCursor FCursor;
	Graphics::TColor FLinkColor;
	Menus::TPopupMenu* FLinkPopupMenu;
	Graphics::TFontStyles FLinkStyle;
	#pragma pack(push, 1)
	Types::TRect FTextRect;
	#pragma pack(pop)
	
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	
protected:
	Htmlrender::TElHTMLRender* FRender;
	virtual void __fastcall TriggerPaintEvent(void);
	void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	virtual void __fastcall TriggerLinkClickEvent(WideString HRef);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall Click(void);
	HIDESBASE virtual void __fastcall SetCursor(Controls::TCursor newValue);
	virtual void __fastcall SetLinkPopupMenu(Menus::TPopupMenu* newValue);
	virtual void __fastcall SetLinkColor(Graphics::TColor newValue);
	virtual void __fastcall SetLinkStyle(Graphics::TFontStyles newValue);
	virtual void __fastcall SetCaption(WideString newValue);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TWMContextMenu &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Message);
	void __fastcall DoLinkPopup(const Types::TPoint &MousePos);
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property Controls::TCursor Cursor = {read=FCursor, write=SetCursor, nodefault};
	__property Graphics::TColor LinkColor = {read=FLinkColor, write=SetLinkColor, nodefault};
	__property Menus::TPopupMenu* LinkPopupMenu = {read=FLinkPopupMenu, write=SetLinkPopupMenu};
	__property Graphics::TFontStyles LinkStyle = {read=FLinkStyle, write=SetLinkStyle, nodefault};
	
public:
	__fastcall virtual TCustomElHTMLPanel(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElHTMLPanel(void);
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElHTMLPanel(HWND ParentWindow) : Elpanel::TCustomElPanel(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElHTMLPanel;
class PASCALIMPLEMENTATION TElHTMLPanel : public TCustomElHTMLPanel 
{
	typedef TCustomElHTMLPanel inherited;
	
__published:
	__property OnImageNeeded ;
	__property OnLinkClick ;
	__property Cursor ;
	__property LinkColor ;
	__property LinkPopupMenu ;
	__property LinkStyle ;
	__property Background ;
	__property BackgroundType  = {default=2};
	__property GradientEndColor  = {default=0};
	__property GradientStartColor  = {default=0};
	__property GradientSteps  = {default=16};
	__property Alignment  = {default=2};
	__property Layout  = {default=1};
	__property ImageForm ;
	__property TopGrabHandle ;
	__property RightGrabHandle ;
	__property LeftGrabHandle ;
	__property BottomGrabHandle ;
	__property Resizable  = {default=0};
	__property Movable  = {default=0};
	__property OnMove ;
	__property Align ;
	__property BevelInner  = {default=0};
	__property BevelOuter  = {default=2};
	__property BevelWidth  = {default=1};
	__property BorderStyle  = {default=0};
	__property BorderWidth  = {default=0};
	__property Canvas ;
	__property Color  = {default=-2147483633};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property Locked  = {default=0};
	__property MouseCapture ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property TransparentXPThemes  = {default=1};
	__property UseXPThemes  = {default=1};
	__property Visible  = {default=1};
	__property SizeGrip  = {default=0};
	__property Caption ;
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
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TCustomElHTMLPanel.Create */ inline __fastcall virtual TElHTMLPanel(Classes::TComponent* AOwner) : TCustomElHTMLPanel(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElHTMLPanel.Destroy */ inline __fastcall virtual ~TElHTMLPanel(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHTMLPanel(HWND ParentWindow) : TCustomElHTMLPanel(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elhtmlpanel */
using namespace Elhtmlpanel;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHTMLPanel
