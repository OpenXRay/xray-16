// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTray.pas' rev: 6.00

#ifndef ElTrayHPP
#define ElTrayHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ShellAPI.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElTimers.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElHook.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElBaseComp.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltray
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElTrayExtHintShowEvent)(System::TObject* Sender, bool &DoShow);

typedef void __fastcall (__closure *TElTrayExtHintCreateEvent)(System::TObject* Sender, AnsiString FormClass, Forms::TForm* &Form);

typedef void __fastcall (__closure *TQueryEndSessionEvent)(System::TObject* Sender, int Action, bool &CanClose);

#pragma pack(push, 4)
struct TClickInfo
{
	Controls::TMouseButton Button;
	Classes::TShiftState Shift;
	int X;
	int Y;
} ;
#pragma pack(pop)

class DELPHICLASS TElTrayIcon;
class PASCALIMPLEMENTATION TElTrayIcon : public Elbasecomp::TElBaseComponent 
{
	typedef Elbasecomp::TElBaseComponent inherited;
	
private:
	unsigned FExtHintWndStyle;
	unsigned FExtHintWndExStyle;
	TElTrayExtHintShowEvent FOnBeforeExtendedHintShow;
	TElTrayExtHintCreateEvent FOnExtHintFormCreate;
	bool FHideTaskBarIcon;
	unsigned FExtendedHintDelay;
	TQueryEndSessionEvent FOnQueryEndSession;
	bool FPopupWhenModal;
	AnsiString FHint;
	Graphics::TIcon* FStaticIcon;
	bool FAnimated;
	Graphics::TIcon* FIcon;
	Controls::TImageList* FIcons;
	int FInterval;
	Menus::TPopupMenu* FPopupMenu;
	TClickInfo FClickInfo;
	Eltimers::TElTimerPoolItem* FClickTimer;
	Eltimers::TElTimerPoolItem* FHintTimer;
	Eltimers::TElTimerPoolItem* FTimer;
	Eltimers::TElTimerPool* FTimerPool;
	int FImgIdx;
	Controls::TMouseEvent FOnClick;
	Classes::TNotifyEvent FOnDblClick;
	Controls::TMouseEvent FOnMouseDown;
	Controls::TMouseEvent FOnMouseUp;
	Controls::TMouseMoveEvent FOnMouseMove;
	Imglist::TChangeLink* MyChangeLink;
	bool FSet;
	_NOTIFYICONDATAA FIconData;
	bool FLClick;
	bool FMClick;
	bool FRClick;
	Forms::TForm* FExtForm;
	AnsiString FExtFormName;
	#pragma pack(push, 1)
	Types::TPoint FExtFormPt;
	#pragma pack(pop)
	
	bool FHideForm;
	bool FExtFormInt;
	bool FNoShow;
	Controls::TMouseEvent FOnDblClickEx;
	void *NewMenuProc;
	void *OldMenuProc;
	void __fastcall SetExtForm(AnsiString newValue);
	void __fastcall SetAnimated(bool newValue);
	void __fastcall SetDesignActive(bool newValue);
	void __fastcall SetIcons(Controls::TImageList* newValue);
	void __fastcall SetInterval(int newValue);
	void __fastcall SetPopupMenu(Menus::TPopupMenu* newValue);
	void __fastcall SetStaticIcon(Graphics::TIcon* newValue);
	void __fastcall OnClickTimer(System::TObject* Sender);
	void __fastcall OnTimer(System::TObject* Sender);
	void __fastcall OnHintTimer(System::TObject* Sender);
	void __fastcall OnImagesChange(System::TObject* Sender);
	void __fastcall SetHint(AnsiString newValue);
	bool __fastcall DoPopupMenu(int X, int Y);
	bool __fastcall DoDblClick(bool Perform);
	
protected:
	bool FAnimateOnce;
	virtual void __fastcall TriggerClickEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerDblClickEvent(void);
	virtual void __fastcall TriggerMouseDownEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerMouseUpEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerMouseMoveEvent(Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall PopupMenuProc(Messages::TMessage &Message);
	bool __fastcall HookWndProc(Messages::TMessage &Message);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	virtual void __fastcall DoSetEnabled(bool AEnabled);
	virtual void __fastcall UpdateIconData(bool remove);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall TriggerQueryEndSessionEvent(int Action, bool &CanClose);
	virtual void __fastcall SetExtendedHintDelay(unsigned newValue);
	virtual void __fastcall SetHideTaskBarIcon(bool newValue);
	void __fastcall DoHideTaskBarIcon(void);
	void __fastcall DoShowTaskBarIcon(void);
	virtual void __fastcall TriggerBeforeExtendedHintShowEvent(bool &DoShow);
	TMetaClass* __fastcall FindExtForm(AnsiString Name);
	virtual void __fastcall TriggerDblClickExEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	
public:
	__fastcall virtual TElTrayIcon(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTrayIcon(void);
	void __fastcall ShowExtForm(int X, int Y);
	__property Handle ;
	__property Forms::TForm* ExtendedHintForm = {read=FExtForm};
	__property unsigned ExtHintWndStyle = {read=FExtHintWndStyle, write=FExtHintWndStyle, nodefault};
	__property unsigned ExtHintWndExStyle = {read=FExtHintWndExStyle, write=FExtHintWndExStyle, nodefault};
	
__published:
	__property Controls::TImageList* Icons = {read=FIcons, write=SetIcons};
	__property bool Animated = {read=FAnimated, write=SetAnimated, default=0};
	__property bool DesignActive = {read=FDesignActive, write=SetDesignActive, default=0};
	__property int Interval = {read=FInterval, write=SetInterval, default=200};
	__property Menus::TPopupMenu* PopupMenu = {read=FPopupMenu, write=SetPopupMenu};
	__property Controls::TMouseEvent OnClick = {read=FOnClick, write=FOnClick};
	__property Classes::TNotifyEvent OnDblClick = {read=FOnDblClick, write=FOnDblClick};
	__property Controls::TMouseEvent OnMouseDown = {read=FOnMouseDown, write=FOnMouseDown};
	__property Controls::TMouseEvent OnMouseUp = {read=FOnMouseUp, write=FOnMouseUp};
	__property Controls::TMouseMoveEvent OnMouseMove = {read=FOnMouseMove, write=FOnMouseMove};
	__property Graphics::TIcon* StaticIcon = {read=FStaticIcon, write=SetStaticIcon};
	__property AnsiString Hint = {read=FHint, write=SetHint};
	__property Enabled  = {default=0};
	__property AnsiString ExtendedHint = {read=FExtFormName, write=SetExtForm};
	__property bool ExtendedHintInteractive = {read=FExtFormInt, write=FExtFormInt, default=0};
	__property bool PopupWhenModal = {read=FPopupWhenModal, write=FPopupWhenModal, default=0};
	__property TQueryEndSessionEvent OnQueryEndSession = {read=FOnQueryEndSession, write=FOnQueryEndSession};
	__property unsigned ExtendedHintDelay = {read=FExtendedHintDelay, write=SetExtendedHintDelay, nodefault};
	__property bool HideTaskBarIcon = {read=FHideTaskBarIcon, write=SetHideTaskBarIcon, default=0};
	__property TElTrayExtHintShowEvent OnBeforeExtendedHintShow = {read=FOnBeforeExtendedHintShow, write=FOnBeforeExtendedHintShow};
	__property TElTrayExtHintCreateEvent OnExtHintFormCreate = {read=FOnExtHintFormCreate, write=FOnExtHintFormCreate};
	__property Controls::TMouseEvent OnDblClickEx = {read=FOnDblClickEx, write=FOnDblClickEx};
	__property bool AnimateOnce = {read=FAnimateOnce, write=FAnimateOnce, default=0};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE int TrackInterval;
extern PACKAGE int HideInterval;
extern PACKAGE bool FInMenu;

}	/* namespace Eltray */
using namespace Eltray;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTray
