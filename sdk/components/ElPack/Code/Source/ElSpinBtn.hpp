// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElSpinBtn.pas' rev: 6.00

#ifndef ElSpinBtnHPP
#define ElSpinBtnHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTools.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elspinbtn
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElSpinEvent)(System::TObject* Sender, double Distance);

typedef void __fastcall (__closure *TElSpinStartEvent)(System::TObject* Sender, double &InitialDistance);

#pragma option push -b-
enum TElSpinBtnDir { sbdUpDown, sbdLeftRight };
#pragma option pop

#pragma option push -b-
enum TElSpinBtnType { sbtUpDown, sbtLeftRight };
#pragma option pop

class DELPHICLASS TElSpinButton;
class PASCALIMPLEMENTATION TElSpinButton : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	bool FFlat;
	bool FUseDrag;
	TElSpinEvent FOnUpClick;
	TElSpinEvent FOnDownClick;
	TElSpinEvent FOnSpinDrag;
	TElSpinStartEvent FOnSpinStart;
	bool FMouseInUpPart;
	bool FMouseInDownPart;
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	
protected:
	bool FTopBtnDown;
	bool FBottomBtnDown;
	Extctrls::TTimer* FTimer;
	Controls::TCursor SaveCursor;
	bool FCaptured;
	bool FDragging;
	double FStartValue;
	double FValue;
	double FIncrement;
	TElSpinBtnDir FButtonDirection;
	TElSpinBtnType FButtonType;
	bool FUpArrowEnabled;
	bool FDownArrowEnabled;
	bool FMoneyFlat;
	Graphics::TColor FMoneyFlatActiveColor;
	Graphics::TColor FMoneyFlatDownColor;
	Graphics::TColor FMoneyFlatInactiveColor;
	bool FOldStyled;
	WideString FHint;
	void __fastcall IntMouseMove(short XPos, short YPos);
	virtual void __fastcall Paint(void);
	void __fastcall OnTimer(System::TObject* Sender);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TWMMouseWheel &Msg);
	DYNAMIC Menus::TPopupMenu* __fastcall GetPopupMenu(void);
	virtual void __fastcall SetUseDrag(bool newValue);
	virtual void __fastcall SetFlat(bool newValue);
	void __fastcall SetButtonDirection(TElSpinBtnDir Value);
	void __fastcall SetButtonType(TElSpinBtnType Value);
	virtual WideString __fastcall GetThemedClassName();
	void __fastcall SetUpArrowEnabled(bool Value);
	void __fastcall SetDownArrowEnabled(bool Value);
	void __fastcall SetMoneyFlat(bool Value);
	void __fastcall SetMoneyFlatActiveColor(Graphics::TColor Value);
	void __fastcall SetMoneyFlatDownColor(Graphics::TColor Value);
	void __fastcall SetMoneyFlatInactiveColor(Graphics::TColor Value);
	bool __fastcall GetMoneyFlat(void);
	void __fastcall SetOldStyled(bool Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElSpinButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TElSpinButton(void);
	void __fastcall StopDragging(void);
	__property bool SpinDragging = {read=FDragging, nodefault};
	
__published:
	__property double Increment = {read=FIncrement, write=FIncrement};
	__property bool UseDrag = {read=FUseDrag, write=SetUseDrag, nodefault};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property TElSpinBtnDir ButtonDirection = {read=FButtonDirection, write=SetButtonDirection, default=0};
	__property TElSpinBtnType ButtonType = {read=FButtonType, write=SetButtonType, nodefault};
	__property bool UpArrowEnabled = {read=FUpArrowEnabled, write=SetUpArrowEnabled, default=1};
	__property bool DownArrowEnabled = {read=FDownArrowEnabled, write=SetDownArrowEnabled, default=1};
	__property bool MoneyFlat = {read=GetMoneyFlat, write=SetMoneyFlat, default=0};
	__property Graphics::TColor MoneyFlatActiveColor = {read=FMoneyFlatActiveColor, write=SetMoneyFlatActiveColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatDownColor = {read=FMoneyFlatDownColor, write=SetMoneyFlatDownColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatInactiveColor = {read=FMoneyFlatInactiveColor, write=SetMoneyFlatInactiveColor, stored=GetMoneyFlat, nodefault};
	__property bool OldStyled = {read=FOldStyled, write=SetOldStyled, default=0};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property TElSpinEvent OnUpClick = {read=FOnUpClick, write=FOnUpClick};
	__property TElSpinEvent OnDownClick = {read=FOnDownClick, write=FOnDownClick};
	__property TElSpinEvent OnSpinDrag = {read=FOnSpinDrag, write=FOnSpinDrag};
	__property TElSpinStartEvent OnSpinStart = {read=FOnSpinStart, write=FOnSpinStart};
	__property Caption ;
	__property Enabled  = {default=1};
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property PopupMenu ;
	__property Color  = {default=-2147483643};
	__property ParentColor  = {default=1};
	__property Align  = {default=0};
	__property Font ;
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnKeyDown ;
	__property OnKeyUp ;
	__property OnKeyPress ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnStartDrag ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElSpinButton(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elspinbtn */
using namespace Elspinbtn;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElSpinBtn
