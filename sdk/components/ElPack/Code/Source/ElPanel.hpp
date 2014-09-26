// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElPanel.pas' rev: 6.00

#ifndef ElPanelHPP
#define ElPanelHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElExtBkgnd.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elpanel
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElPanelGrabHandleKind { ghkNone, ghkMove, ghkResize, ghkMoveParent };
#pragma option pop

class DELPHICLASS TElPanelGrabHandle;
class DELPHICLASS TCustomElPanel;
class PASCALIMPLEMENTATION TCustomElPanel : public Extctrls::TCustomPanel 
{
	typedef Extctrls::TCustomPanel inherited;
	
private:
	Controls::TBevelCut FBevelInner;
	Controls::TBevelCut FBevelOuter;
	Controls::TBevelWidth FBevelWidth;
	HIDESBASE void __fastcall SetBevelInner(Controls::TBevelCut Value);
	HIDESBASE void __fastcall SetBevelOuter(Controls::TBevelCut Value);
	HIDESBASE void __fastcall SetBevelWidth(Controls::TBevelWidth Value);
	
protected:
	bool FOwnerDraw;
	bool FAlwaysPaintBackground;
	Stdctrls::TTextLayout FLayout;
	Classes::TNotifyEvent FOnPaint;
	int FGradientSteps;
	Graphics::TColor FGradientStartColor;
	Graphics::TColor FGradientEndColor;
	Graphics::TBitmap* FTmpBmp;
	bool FTransparent;
	Graphics::TBitmap* FBackground;
	Elvclutils::TElBkGndType FBackgroundType;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elimgfrm::TElImageForm* FImgForm;
	HWND FSaveCapture;
	Classes::TNotifyEvent FOnMove;
	bool FResizable;
	bool FMovable;
	#pragma pack(push, 1)
	Types::TRect FSizeMoveRect;
	#pragma pack(pop)
	
	TElPanelGrabHandle* FGrabHandles[4];
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	bool FMouseInControl;
	unsigned FTheme;
	bool FUseXPThemes;
	bool FPressed;
	bool FIntPaint;
	WideString FCaption;
	Byte FAlphaLevel;
	bool FTransparentXPThemes;
	bool FSizeGrip;
	#pragma pack(push, 1)
	Types::TRect SizeGripRect;
	#pragma pack(pop)
	
	Graphics::TColor FBevelSpaceColor;
	bool FShowFocus;
	WideString FHint;
	void __fastcall SetBackground(Graphics::TBitmap* newValue);
	void __fastcall ImageChange(System::TObject* Sender);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetResizable(bool newValue);
	MESSAGE void __fastcall WMEnterSizeMove(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMExitSizeMove(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkGnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMMove(Messages::TWMMove &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	void __fastcall RedoTmpBmp(void);
	void __fastcall SetGradientStartColor(Graphics::TColor newValue);
	void __fastcall SetGradientEndColor(Graphics::TColor newValue);
	void __fastcall SetGradientSteps(int newValue);
	void __fastcall SetLayout(Stdctrls::TTextLayout newValue);
	void __fastcall SetOwnerDraw(bool newValue);
	TElPanelGrabHandle* __fastcall GetTopGrabHandle(void);
	void __fastcall SetTopGrabHandle(TElPanelGrabHandle* newValue);
	TElPanelGrabHandle* __fastcall GetRightGrabHandle(void);
	void __fastcall SetRightGrabHandle(TElPanelGrabHandle* newValue);
	TElPanelGrabHandle* __fastcall GetLeftGrabHandle(void);
	void __fastcall SetLeftGrabHandle(TElPanelGrabHandle* newValue);
	TElPanelGrabHandle* __fastcall GetBottomGrabHandle(void);
	void __fastcall SetBottomGrabHandle(TElPanelGrabHandle* newValue);
	void __fastcall SetAlwaysPaintBackground(bool Value);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	virtual void __fastcall SetBackgroundType(Elvclutils::TElBkGndType newValue);
	virtual void __fastcall SetTransparent(bool newValue);
	virtual void __fastcall SetMovable(bool newValue);
	virtual void __fastcall AlignControls(Controls::TControl* AControl, Types::TRect &Rect);
	virtual void __fastcall Paint(void);
	virtual void __fastcall TriggerMoveEvent(void);
	virtual void __fastcall TriggerPaintEvent(void);
	virtual void __fastcall SetCaption(WideString newValue);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Message);
	virtual void __fastcall AdjustClientRect(Types::TRect &Rect);
	TElPanelGrabHandle* __fastcall InGrabHandle(int X, int Y, const Types::TRect &Rect);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall DoMouseEnter(void);
	virtual void __fastcall DoMouseLeave(void);
	virtual WideString __fastcall GetCaption();
	void __fastcall SetAlphaLevel(Byte Value);
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual void __fastcall CreateThemeHandle(void);
	virtual void __fastcall FreeThemeHandle(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	virtual void __fastcall DrawThemedBackground(void);
	virtual Types::TRect __fastcall GetBackgroundClientRect();
	virtual void __fastcall SetTransparentXPThemes(bool Value);
	void __fastcall SetSizeGrip(bool Value);
	virtual void __fastcall UpdateInterior(void);
	void __fastcall SetBevelSpaceColor(Graphics::TColor Value);
	void __fastcall SetShowFocus(bool Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	__property bool AlwaysPaintBackground = {read=FAlwaysPaintBackground, write=SetAlwaysPaintBackground, default=0};
	__property Stdctrls::TTextLayout Layout = {read=FLayout, write=SetLayout, default=1};
	__property bool OwnerDraw = {read=FOwnerDraw, write=SetOwnerDraw, default=0};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property Elvclutils::TElBkGndType BackgroundType = {read=FBackgroundType, write=SetBackgroundType, default=2};
	__property bool Resizable = {read=FResizable, write=SetResizable, default=0};
	__property bool Movable = {read=FMovable, write=SetMovable, default=0};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property WideString Caption = {read=GetCaption, write=SetCaption};
	__property Graphics::TColor GradientStartColor = {read=FGradientStartColor, write=SetGradientStartColor, default=0};
	__property Graphics::TColor GradientEndColor = {read=FGradientEndColor, write=SetGradientEndColor, default=0};
	__property int GradientSteps = {read=FGradientSteps, write=SetGradientSteps, default=16};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property TElPanelGrabHandle* TopGrabHandle = {read=GetTopGrabHandle, write=SetTopGrabHandle};
	__property TElPanelGrabHandle* RightGrabHandle = {read=GetRightGrabHandle, write=SetRightGrabHandle};
	__property TElPanelGrabHandle* LeftGrabHandle = {read=GetLeftGrabHandle, write=SetLeftGrabHandle};
	__property TElPanelGrabHandle* BottomGrabHandle = {read=GetBottomGrabHandle, write=SetBottomGrabHandle};
	__property Classes::TNotifyEvent OnMove = {read=FOnMove, write=FOnMove};
	__property Classes::TNotifyEvent OnPaint = {read=FOnPaint, write=FOnPaint};
	__property Classes::TNotifyEvent OnMouseEnter = {read=FOnMouseEnter, write=FOnMouseEnter};
	__property Classes::TNotifyEvent OnMouseLeave = {read=FOnMouseLeave, write=FOnMouseLeave};
	__property Byte AlphaLevel = {read=FAlphaLevel, write=SetAlphaLevel, default=0};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property bool TransparentXPThemes = {read=FTransparentXPThemes, write=SetTransparentXPThemes, default=1};
	__property TabStop  = {default=1};
	__property bool SizeGrip = {read=FSizeGrip, write=SetSizeGrip, default=0};
	__property Controls::TBevelCut BevelInner = {read=FBevelInner, write=SetBevelInner, default=0};
	__property Controls::TBevelCut BevelOuter = {read=FBevelOuter, write=SetBevelOuter, default=2};
	__property Controls::TBevelWidth BevelWidth = {read=FBevelWidth, write=SetBevelWidth, default=1};
	__property Graphics::TColor BevelSpaceColor = {read=FBevelSpaceColor, write=SetBevelSpaceColor, default=-2147483633};
	__property bool ShowFocus = {read=FShowFocus, write=SetShowFocus, default=0};
	
public:
	virtual void __fastcall Update(void);
	__fastcall virtual TCustomElPanel(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElPanel(void);
	bool __fastcall IsThemeApplied(void);
	__property bool MouseInControl = {read=FMouseInControl, nodefault};
	__property Canvas ;
	__property unsigned Theme = {read=FTheme, nodefault};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElPanel(HWND ParentWindow) : Extctrls::TCustomPanel(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElPanelGrabHandle : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
private:
	bool FEnabled;
	int FSize;
	TElPanelGrabHandleKind FKind;
	Controls::TBevelCut FBevelKind;
	TCustomElPanel* FOwner;
	bool FVisible;
	void __fastcall SetVisible(bool newValue);
	void __fastcall SetEnabled(bool newValue);
	void __fastcall SetSize(int newValue);
	void __fastcall SetBevelKind(Controls::TBevelCut newValue);
	
public:
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	
__published:
	__property bool Enabled = {read=FEnabled, write=SetEnabled, default=0};
	__property int Size = {read=FSize, write=SetSize, default=0};
	__property TElPanelGrabHandleKind Kind = {read=FKind, write=FKind, default=0};
	__property Controls::TBevelCut BevelKind = {read=FBevelKind, write=SetBevelKind, default=0};
	__property bool Visible = {read=FVisible, write=SetVisible, default=0};
public:
	#pragma option push -w-inl
	/* TPersistent.Destroy */ inline __fastcall virtual ~TElPanelGrabHandle(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElPanelGrabHandle(void) : Classes::TPersistent() { }
	#pragma option pop
	
};


class DELPHICLASS TElPanel;
class PASCALIMPLEMENTATION TElPanel : public TCustomElPanel 
{
	typedef TCustomElPanel inherited;
	
__published:
	__property AlphaLevel  = {default=0};
	__property AlwaysPaintBackground  = {default=0};
	__property Background ;
	__property BackgroundType  = {default=2};
	__property GradientEndColor  = {default=0};
	__property GradientStartColor  = {default=0};
	__property GradientSteps  = {default=16};
	__property Alignment  = {default=2};
	__property Layout  = {default=1};
	__property OwnerDraw  = {default=0};
	__property ImageForm ;
	__property TopGrabHandle ;
	__property RightGrabHandle ;
	__property LeftGrabHandle ;
	__property BottomGrabHandle ;
	__property Resizable  = {default=0};
	__property Movable  = {default=0};
	__property OnMove ;
	__property OnPaint ;
	__property SizeGrip  = {default=0};
	__property Align ;
	__property BevelInner  = {default=0};
	__property BevelOuter  = {default=2};
	__property BevelSpaceColor  = {default=-2147483633};
	__property BevelWidth  = {default=1};
	__property BorderStyle  = {default=0};
	__property BorderWidth  = {default=0};
	__property TransparentXPThemes  = {default=1};
	__property UseXPThemes  = {default=1};
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
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property Visible  = {default=1};
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
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElPanel.Create */ inline __fastcall virtual TElPanel(Classes::TComponent* AOwner) : TCustomElPanel(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElPanel.Destroy */ inline __fastcall virtual ~TElPanel(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElPanel(HWND ParentWindow) : TCustomElPanel(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elpanel */
using namespace Elpanel;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElPanel
