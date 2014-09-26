// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElAppBar.pas' rev: 6.00

#ifndef ElAppBarHPP
#define ElAppBarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ShellAPI.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elappbar
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TAppBarMessage { abmNew, abmRemove, abmQueryPos, abmSetPos, abmGetState, abmGetTaskBarPos, abmActivate, abmGetAutoHideBar, abmSetAutoHideBar, abmWindowPosChanged };
#pragma option pop

#pragma option push -b-
enum TAppBarEdge { abeLeft, abeTop, abeRight, abeBottom, abeUnknown, abeFloat };
#pragma option pop

#pragma option push -b-
enum TAppBarFlag { abfAllowLeft, abfAllowTop, abfAllowRight, abfAllowBottom, abfAllowFloat };
#pragma option pop

typedef Set<TAppBarFlag, abfAllowLeft, abfAllowFloat>  TAppBarFlags;

#pragma option push -b-
enum TAppBarTaskEntry { abtShow, abtHide, abtFloatDependent };
#pragma option pop

#pragma pack(push, 4)
struct TAppBarSettings
{
	TAppBarEdge abEdge;
	bool bAutohide;
	bool bAlwaysOnTop;
	bool bSlideEffect;
	int nTimerInterval;
	Types::TRect rcDockDims;
	Types::TRect rcFloat;
	int nMinWidth;
	int nMinHeight;
	int nMaxWidth;
	int nMaxHeight;
	TAppBarTaskEntry abTaskEntry;
} ;
#pragma pack(pop)

class DELPHICLASS TElAppBar;
class PASCALIMPLEMENTATION TElAppBar : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
private:
	bool FInPosChanged;
	
protected:
	bool FForceReHide;
	bool FPreventOffScreen;
	bool FKeepSize;
	#pragma pack(push, 1)
	Types::TPoint FDraggingOffset;
	#pragma pack(pop)
	
	TAppBarSettings FABS;
	TAppBarFlags FabFlags;
	#pragma pack(push, 1)
	tagSIZE FszSizeInc;
	#pragma pack(pop)
	
	TAppBarEdge FabEdgeProposedPrev;
	bool FbFullScreenAppOpen;
	bool FbAutoHideIsVisible;
	Extctrls::TTimer* FTimer;
	Classes::TNotifyEvent FOnEdgeChanged;
	unsigned __fastcall AppBarMessage(TAppBarMessage abMessage, TAppBarEdge abEdge, int lParam, bool bRect, Types::TRect &rc);
	unsigned __fastcall AppBarMessage1(TAppBarMessage abMessage);
	unsigned __fastcall AppBarMessage2(TAppBarMessage abMessage, TAppBarEdge abEdge);
	unsigned __fastcall AppBarMessage3(TAppBarMessage abMessage, TAppBarEdge abEdge, int lParam);
	unsigned __fastcall AppBarMessage4(TAppBarMessage abMessage, TAppBarEdge abEdge, int lParam, Types::TRect &rc);
	TAppBarEdge __fastcall CalcProposedState(Types::TSmallPoint &pt);
	bool __fastcall AdjustLocationForAutohide(bool bShow, Types::TRect &rc);
	void __fastcall SlideWindow(Types::TRect &rcEnd);
	TAppBarEdge __fastcall GetAutohideEdge(void);
	Types::TSmallPoint __fastcall GetMessagePosition(void);
	void __fastcall SetKeepSize(bool newValue);
	void __fastcall SetPreventOffScreen(bool newValue);
	void __fastcall SetHorzInc(int newValue);
	void __fastcall SetVertInc(int newValue);
	int __fastcall GetVertInc(void);
	int __fastcall GetHorzInc(void);
	void __fastcall DoEdgeChanged(void);
	virtual void __fastcall GetRect(TAppBarEdge abEdgeProposed, Types::TRect &rcProposed);
	TAppBarEdge __fastcall GetEdge(void);
	void __fastcall SetEdge(TAppBarEdge abEdge);
	void __fastcall SetSlideTime(int nInterval);
	bool __fastcall IsAutoHide(void);
	void __fastcall SetAutoHide(bool bAutoHide);
	bool __fastcall IsAlwaysOnTop(void);
	void __fastcall SetAlwaysOnTop(bool bAlwaysOnTop);
	void __fastcall SetFlags(TAppBarFlags newValue);
	virtual void __fastcall OnAppBarForcedToDocked(void);
	virtual void __fastcall OnABNFullScreenApp(bool bOpen);
	virtual void __fastcall OnABNPosChanged(void);
	virtual void __fastcall OnABNWindowArrange(bool bBeginning);
	MESSAGE void __fastcall OnAppBarCallbackMsg(Messages::TMessage &Msg);
	MESSAGE void __fastcall WmCreate(Messages::TWMCreate &Msg);
	HIDESBASE MESSAGE void __fastcall WmDestroy(Messages::TWMNoParams &Msg);
	MESSAGE void __fastcall OnWindowPosChanged(Messages::TWMWindowPosMsg &Msg);
	HIDESBASE MESSAGE void __fastcall OnActivate(Messages::TWMActivate &Msg);
	void __fastcall OnAppBarTimer(System::TObject* Sender);
	MESSAGE void __fastcall OnNcMouseMove(Messages::TWMNCHitMessage &Msg);
	HIDESBASE MESSAGE void __fastcall OnMouseMove(Messages::TWMMouse &Msg);
	MESSAGE void __fastcall OnNcHitTest(Messages::TWMNCHitTest &Msg);
	MESSAGE void __fastcall OnEnterSizeMove(Messages::TMessage &Msg);
	MESSAGE void __fastcall OnExitSizeMove(Messages::TMessage &Msg);
	MESSAGE void __fastcall OnMoving(Messages::TMessage &Msg);
	MESSAGE void __fastcall OnSizing(Messages::TMessage &Msg);
	MESSAGE void __fastcall OnGetMinMaxInfo(Messages::TWMGetMinMaxInfo &Msg);
	bool __fastcall IsEdgeLeftOrRight(TAppBarEdge abEdge);
	bool __fastcall IsEdgeTopOrBottom(TAppBarEdge abEdge);
	bool __fastcall IsFloating(TAppBarEdge abEdge);
	bool __fastcall IsDockable(TAppBarFlags abFlags);
	bool __fastcall IsDockableVertically(TAppBarFlags abFlags);
	bool __fastcall IsDockableHorizontally(TAppBarFlags abFlags);
	void __fastcall ResetSystemKnowledge(void);
	TAppBarEdge __fastcall GetEdgeFromPoint(TAppBarFlags abFlags, Types::TSmallPoint pt);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	
public:
	void __fastcall GetDockDims(Types::TRect &rc);
	void __fastcall SetDockDims(const Types::TRect &rc);
	void __fastcall GetFloatRect(Types::TRect &rc);
	void __fastcall SetFloatRect(const Types::TRect &rc);
	__fastcall virtual TElAppBar(Classes::TComponent* Owner);
	__fastcall virtual ~TElAppBar(void);
	void __fastcall UpdateBar(void);
	virtual void __fastcall ShowHiddenAppBar(bool bShow);
	__property bool AutoHideIsVisible = {read=FbAutoHideIsVisible, nodefault};
	
__published:
	__property TAppBarFlags Flags = {read=FabFlags, write=SetFlags, nodefault};
	__property int HorzSizeInc = {read=GetHorzInc, write=SetHorzInc, nodefault};
	__property int VertSizeInc = {read=GetVertInc, write=SetVertInc, nodefault};
	__property TAppBarEdge Edge = {read=GetEdge, write=SetEdge, nodefault};
	__property bool AutoHide = {read=IsAutoHide, write=SetAutoHide, nodefault};
	__property bool AlwaysOnTop = {read=IsAlwaysOnTop, write=SetAlwaysOnTop, nodefault};
	__property int MinWidth = {read=FABS.nMinWidth, write=FABS.nMinWidth, nodefault};
	__property int MinHeight = {read=FABS.nMinHeight, write=FABS.nMinHeight, nodefault};
	__property int MaxWidth = {read=FABS.nMaxWidth, write=FABS.nMaxWidth, nodefault};
	__property int MaxHeight = {read=FABS.nMaxHeight, write=FABS.nMaxHeight, nodefault};
	__property TAppBarTaskEntry TaskEntry = {read=FABS.abTaskEntry, write=FABS.abTaskEntry, nodefault};
	__property bool KeepSize = {read=FKeepSize, write=SetKeepSize, nodefault};
	__property bool PreventOffScreen = {read=FPreventOffScreen, write=SetPreventOffScreen, default=0};
	__property Classes::TNotifyEvent OnEdgeChanged = {read=FOnEdgeChanged, write=FOnEdgeChanged};
public:
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TElAppBar(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElAppBar(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Word WM_APPBARNOTIFY = 0x464;
static const Word SLIDE_DEF_TIMER_INTERVAL = 0x190;
static const Shortint AB_DEF_SIZE_INC = 0x1;
static const Shortint AB_DEF_DOCK_DIM = 0x20;

}	/* namespace Elappbar */
using namespace Elappbar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElAppBar
