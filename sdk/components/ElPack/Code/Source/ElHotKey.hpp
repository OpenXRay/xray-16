// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHotKey.pas' rev: 6.00

#ifndef ElHotKeyHPP
#define ElHotKeyHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhotkey
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElHKInvalidKey { hcShift, hcAlt, hcCtrl, hcAltShift, hcAltCtrl, hcCtrlShift, hcCtrlAlftShift };
#pragma option pop

typedef Set<TElHKInvalidKey, hcShift, hcCtrlAlftShift>  TElHKInvalidKeys;

#pragma option push -b-
enum TElHKModifier { hkShift, hkAlt, hkCtrl };
#pragma option pop

typedef Set<TElHKModifier, hkShift, hkCtrl>  TElHKModifiers;

class DELPHICLASS TElHotKey;
class PASCALIMPLEMENTATION TElHotKey : public Eledits::TCustomElEdit 
{
	typedef Eledits::TCustomElEdit inherited;
	
private:
	WideString FText;
	bool FKeyPressed;
	TElHKInvalidKeys FInvalidKeys;
	TElHKModifiers FModifiers;
	Classes::TShiftState FShiftState;
	void __fastcall SetShortCut(Classes::TShortCut newValue);
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyUp(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall WMSysKeyDown(Messages::TWMKey &Message);
	void __fastcall SetInvalidKeys(TElHKInvalidKeys Value);
	void __fastcall SetModifiers(TElHKModifiers Value);
	WideString __fastcall ShiftStateToText(Classes::TShiftState state);
	Classes::TShortCut __fastcall GetShortCut(void);
	Classes::TShiftState __fastcall GetShiftState(Classes::TShiftState state);
	
protected:
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyUp(Word &Key, Classes::TShiftState Shift);
	
public:
	__fastcall virtual TElHotKey(Classes::TComponent* AOwner);
	
__published:
	__property Classes::TShortCut HotKey = {read=GetShortCut, write=SetShortCut, nodefault};
	__property TElHKInvalidKeys InvalidKeys = {read=FInvalidKeys, write=SetInvalidKeys, nodefault};
	__property TElHKModifiers Modifiers = {read=FModifiers, write=SetModifiers, nodefault};
	__property AutoSize  = {default=1};
	__property Alignment ;
	__property Background ;
	__property BorderSides ;
	__property UseBackground  = {default=0};
	__property RTLContent ;
	__property Transparent ;
	__property ReadOnly  = {default=0};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property TopMargin  = {default=1};
	__property BorderStyle ;
	__property HideSelection  = {default=1};
	__property ActiveBorderType  = {default=1};
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property Align  = {default=0};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
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
public:
	#pragma option push -w-inl
	/* TCustomElEdit.Destroy */ inline __fastcall virtual ~TElHotKey(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHotKey(HWND ParentWindow) : Eledits::TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


typedef AnsiString ElHotKey__2[7];

//-- var, const, procedure ---------------------------------------------------
extern PACKAGE System::ResourceString _rsShiftP;
#define Elhotkey_rsShiftP System::LoadResourceString(&Elhotkey::_rsShiftP)
extern PACKAGE System::ResourceString _rsAltP;
#define Elhotkey_rsAltP System::LoadResourceString(&Elhotkey::_rsAltP)
extern PACKAGE System::ResourceString _rsCtrlP;
#define Elhotkey_rsCtrlP System::LoadResourceString(&Elhotkey::_rsCtrlP)
extern PACKAGE System::ResourceString _rsLeftP;
#define Elhotkey_rsLeftP System::LoadResourceString(&Elhotkey::_rsLeftP)
extern PACKAGE System::ResourceString _rsRightP;
#define Elhotkey_rsRightP System::LoadResourceString(&Elhotkey::_rsRightP)
extern PACKAGE System::ResourceString _rsMiddleP;
#define Elhotkey_rsMiddleP System::LoadResourceString(&Elhotkey::_rsMiddleP)
extern PACKAGE System::ResourceString _rsDoubleP;
#define Elhotkey_rsDoubleP System::LoadResourceString(&Elhotkey::_rsDoubleP)
extern PACKAGE AnsiString nshift[7];

}	/* namespace Elhotkey */
using namespace Elhotkey;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHotKey
