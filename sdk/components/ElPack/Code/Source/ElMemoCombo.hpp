// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElMemoCombo.pas' rev: 6.00

#ifndef ElMemoComboHPP
#define ElMemoComboHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElSndMap.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElBtnEdit.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elmemocombo
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElMemoComboDropEvent)(System::TObject* Sender, bool &Dropped, AnsiString &Text);

typedef TElAdvancedMemo TElIntMemo;
;

class DELPHICLASS TElMemoCombo;
class PASCALIMPLEMENTATION TElMemoCombo : public Elbtnedit::TCustomElButtonEdit 
{
	typedef Elbtnedit::TCustomElButtonEdit inherited;
	
private:
	Classes::TNotifyEvent FOnMemoChange;
	Classes::TNotifyEvent FOnMemoClick;
	Classes::TNotifyEvent FOnMemoDblClick;
	Controls::TKeyEvent FOnMemoKeyDown;
	Controls::TKeyPressEvent FOnMemoKeyPress;
	Controls::TKeyEvent FOnMemoKeyUp;
	Controls::TMouseEvent FOnMemoMouseDown;
	Controls::TMouseMoveEvent FOnMemoMouseMove;
	Controls::TMouseEvent FOnMemoMouseUp;
	Elactrls::TElAdvancedMemo* FMemo;
	Classes::TAlignment FMemoAlignment;
	bool FDropAutoWidth;
	int FDropHeight;
	int FDropWidth;
	bool FDropped;
	TElMemoComboDropEvent FOnDrop;
	bool FIgnoreBtn;
	void __fastcall SetDropHeight(int newValue);
	void __fastcall SetDropWidth(int newValue);
	void __fastcall SetDropped(bool newValue);
	void __fastcall SetMemoAlignment(Classes::TAlignment newValue);
	void __fastcall SetMemoColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetMemoColor(void);
	void __fastcall SetMemoFont(Graphics::TFont* newValue);
	Graphics::TFont* __fastcall GetMemoFont(void);
	void __fastcall SetMemoMaxLength(int newValue);
	int __fastcall GetMemoMaxLength(void);
	void __fastcall SetMemoPopupMenu(Menus::TPopupMenu* newValue);
	Menus::TPopupMenu* __fastcall GetMemoPopupMenu(void);
	void __fastcall SetMemoScrollBars(Stdctrls::TScrollStyle newValue);
	Stdctrls::TScrollStyle __fastcall GetMemoScrollBars(void);
	void __fastcall SetMemoWordWrap(bool newValue);
	bool __fastcall GetMemoWordWrap(void);
	void __fastcall MemoDeactivate(System::TObject* Sender);
	void __fastcall ButtonClick(System::TObject* Sender);
	void __fastcall SetMemoFlatScrollBars(bool newValue);
	bool __fastcall GetMemoFlatScrollBars(void);
	void __fastcall SetMemoActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	Elvclutils::TElFlatBorderType __fastcall GetMemoActiveBorderType(void);
	void __fastcall MemoChangeTransfer(System::TObject* Sender);
	void __fastcall MemoClickTransfer(System::TObject* Sender);
	void __fastcall MemoDblClickTransfer(System::TObject* Sender);
	void __fastcall MemoKeyDownTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall MemoKeyPressTransfer(System::TObject* Sender, char &Key);
	void __fastcall MemoKeyUpTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall MemoMouseDownTransfer(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall MemoMouseMoveTransfer(System::TObject* Sender, Classes::TShiftState Shift, int X, int Y);
	void __fastcall MemoMouseUpTransfer(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	void __fastcall SetDropAutoWidth(const bool Value);
	
protected:
	virtual void __fastcall TriggerDropEvent(bool &Dropped, AnsiString &Text);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Message);
	
public:
	__fastcall virtual TElMemoCombo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElMemoCombo(void);
	virtual void __fastcall CreateWnd(void);
	void __fastcall Drop(bool Dropped);
	Elactrls::TElAdvancedMemo* __fastcall GetMemo(void);
	
__published:
	__property bool DropAutoWidth = {read=FDropAutoWidth, write=SetDropAutoWidth, default=1};
	__property int DropHeight = {read=FDropHeight, write=SetDropHeight, nodefault};
	__property int DropWidth = {read=FDropWidth, write=SetDropWidth, nodefault};
	__property bool Dropped = {read=FDropped, write=SetDropped, nodefault};
	__property TElMemoComboDropEvent OnDrop = {read=FOnDrop, write=FOnDrop};
	__property Classes::TAlignment MemoAlignment = {read=FMemoAlignment, write=SetMemoAlignment, nodefault};
	__property Graphics::TColor MemoColor = {read=GetMemoColor, write=SetMemoColor, nodefault};
	__property Graphics::TFont* MemoFont = {read=GetMemoFont, write=SetMemoFont};
	__property int MemoMaxLength = {read=GetMemoMaxLength, write=SetMemoMaxLength, nodefault};
	__property Menus::TPopupMenu* MemoPopupMenu = {read=GetMemoPopupMenu, write=SetMemoPopupMenu};
	__property Stdctrls::TScrollStyle MemoScrollBars = {read=GetMemoScrollBars, write=SetMemoScrollBars, nodefault};
	__property bool MemoWordWrap = {read=GetMemoWordWrap, write=SetMemoWordWrap, nodefault};
	__property Elvclutils::TElFlatBorderType MemoActiveBorderType = {read=GetMemoActiveBorderType, write=SetMemoActiveBorderType, nodefault};
	__property bool MemoFlatScrollBars = {read=GetMemoFlatScrollBars, write=SetMemoFlatScrollBars, nodefault};
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property AutoSize  = {default=1};
	__property RTLContent ;
	__property BorderSides ;
	__property PasswordChar ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property FlatFocusedScrollBars  = {default=0};
	__property WantTabs  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property Lines  = {stored=false};
	__property ImageForm ;
	__property WordWrap  = {default=1};
	__property ScrollBars  = {default=0};
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Text ;
	__property Multiline ;
	__property Align  = {default=0};
	__property ActiveBorderType  = {default=1};
	__property AutoSelect  = {default=0};
	__property BorderStyle ;
	__property ButtonClickSound ;
	__property ButtonDownSound ;
	__property ButtonUpSound ;
	__property ButtonSoundMap ;
	__property ButtonColor ;
	__property ButtonHint ;
	__property ButtonWidth ;
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Flat  = {default=0};
	__property Font ;
	__property InactiveBorderType  = {default=3};
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ReadOnly  = {default=0};
	__property ShowHint ;
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
	__property Classes::TNotifyEvent OnMemoChange = {read=FOnMemoChange, write=FOnMemoChange};
	__property Classes::TNotifyEvent OnMemoClick = {read=FOnMemoClick, write=FOnMemoClick};
	__property Classes::TNotifyEvent OnMemoDblClick = {read=FOnMemoDblClick, write=FOnMemoDblClick};
	__property Controls::TKeyEvent OnMemoKeyDown = {read=FOnMemoKeyDown, write=FOnMemoKeyDown};
	__property Controls::TKeyPressEvent OnMemoKeyPress = {read=FOnMemoKeyPress, write=FOnMemoKeyPress};
	__property Controls::TKeyEvent OnMemoKeyUp = {read=FOnMemoKeyUp, write=FOnMemoKeyUp};
	__property Controls::TMouseEvent OnMemoMouseDown = {read=FOnMemoMouseDown, write=FOnMemoMouseDown};
	__property Controls::TMouseMoveEvent OnMemoMouseMove = {read=FOnMemoMouseMove, write=FOnMemoMouseMove};
	__property Controls::TMouseEvent OnMemoMouseUp = {read=FOnMemoMouseUp, write=FOnMemoMouseUp};
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElMemoCombo(HWND ParentWindow) : Elbtnedit::TCustomElButtonEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElHackMemo;
class PASCALIMPLEMENTATION TElHackMemo : public Elactrls::TElAdvancedMemo 
{
	typedef Elactrls::TElAdvancedMemo inherited;
	
private:
	Classes::TComponent* Owner;
	Classes::TNotifyEvent FOnDeactivate;
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMSysKeyDown(Messages::TWMKey &Msg);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Msg);
	MESSAGE void __fastcall WMActivate(Messages::TWMActivate &Msg);
	
protected:
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	__property Classes::TNotifyEvent OnDeactivate = {read=FOnDeactivate, write=FOnDeactivate};
	
public:
	__fastcall virtual TElHackMemo(Classes::TComponent* Owner);
public:
	#pragma option push -w-inl
	/* TElAdvancedMemo.Destroy */ inline __fastcall virtual ~TElHackMemo(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHackMemo(HWND ParentWindow) : Elactrls::TElAdvancedMemo(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elmemocombo */
using namespace Elmemocombo;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElMemoCombo
