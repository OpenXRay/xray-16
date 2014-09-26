// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElIPEdit.pas' rev: 6.00

#ifndef ElIPEditHPP
#define ElIPEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElImgFrm.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <WinSock.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elipedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElIPPartEdit;
class PASCALIMPLEMENTATION TElIPPartEdit : public Eledits::TElEdit 
{
	typedef Eledits::TElEdit inherited;
	
private:
	Classes::TNotifyEvent OnPoint;
	Classes::TNotifyEvent OnLeftPoint;
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMGetDlgCode(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	
protected:
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
public:
	#pragma option push -w-inl
	/* TCustomElEdit.Create */ inline __fastcall virtual TElIPPartEdit(Classes::TComponent* AOwner) : Eledits::TElEdit(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElEdit.Destroy */ inline __fastcall virtual ~TElIPPartEdit(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElIPPartEdit(HWND ParentWindow) : Eledits::TElEdit(ParentWindow) { }
	#pragma option pop
	
};


typedef TElIPPartEdit IntEditClass;
;

class DELPHICLASS TElIPEdit;
class PASCALIMPLEMENTATION TElIPEdit : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	Forms::TFormBorderStyle FBorderStyle;
	bool FFlat;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FMouseOver;
	bool FModified;
	Classes::TNotifyEvent FOnChange;
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	bool FPartCanEdit[4];
	TElIPPartEdit* FPartEditors[4];
	Byte FParts[4];
	Elvclutils::TElBorderSides FBorderSides;
	WideString FHint;
	int __fastcall FindPart(TElIPPartEdit* Editor);
	Byte __fastcall GetPart(int Index);
	bool __fastcall GetPartCanEdit(int Index);
	void __fastcall SetPart(int Index, Byte Value);
	void __fastcall SetPartCanEdit(int Index, bool Value);
	unsigned __fastcall GetIPAddress(void);
	void __fastcall SetIPAddress(unsigned Value);
	void __fastcall SetFlat(const bool Value);
	void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetBorderStyle(Forms::TBorderStyle Value);
	void __fastcall UpdateFrame(void);
	void __fastcall SetModified(bool Value);
	void __fastcall SetIPString(AnsiString value);
	AnsiString __fastcall GetIPString();
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	void __fastcall ClickHandler(System::TObject* Sender);
	void __fastcall ContextPopupHandler(System::TObject* Sender, const Types::TPoint &MousePos, bool &Handled);
	void __fastcall DblClickHandler(System::TObject* Sender);
	void __fastcall DragDropHandler(System::TObject* Sender, System::TObject* Source, int X, int Y);
	void __fastcall DragOverHandler(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	void __fastcall EndDragHandler(System::TObject* Sender, System::TObject* Target, int X, int Y);
	void __fastcall KeyDownHandler(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall KeyPressHandler(System::TObject* Sender, char &Key);
	void __fastcall KeyUpHandler(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall MouseDownHandler(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall MouseEnterHandler(System::TObject* Sender);
	void __fastcall MouseLeaveHandler(System::TObject* Sender);
	void __fastcall MouseMoveHandler(System::TObject* Sender, Classes::TShiftState Shift, int X, int Y);
	void __fastcall MouseUpHandler(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall StartDragHandler(System::TObject* Sender, Controls::TDragObject* &DragObject);
	
protected:
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	void __fastcall DrawFlatBorder(HDC DC);
	virtual void __fastcall AdjustEditorPositions(void);
	HIDESBASE MESSAGE void __fastcall CMColorChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	DYNAMIC void __fastcall DoMouseEnter(void);
	DYNAMIC void __fastcall DoMouseLeave(void);
	virtual void __fastcall CreateWindowHandle(const Controls::TCreateParams &Params);
	void __fastcall OnEditorChange(System::TObject* Sender);
	void __fastcall OnEditorEnter(System::TObject* Sender);
	void __fastcall OnEditorExit(System::TObject* Sender);
	void __fastcall OnEditorPoint(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TMessage &Message);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	void __fastcall OnEditorLeftPoint(System::TObject* Sender);
	virtual void __fastcall TriggerChangeEvent(void);
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall Loaded(void);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElIPEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElIPEdit(void);
	virtual void __fastcall Paint(void);
	__property unsigned IPAddress = {read=GetIPAddress, write=SetIPAddress, nodefault};
	__property bool MouseOver = {read=FMouseOver, nodefault};
	__property bool Modified = {read=FModified, write=SetModified, nodefault};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, default=1};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=0};
	__property Byte Part1 = {read=GetPart, write=SetPart, index=1, default=0};
	__property Byte Part2 = {read=GetPart, write=SetPart, index=2, default=0};
	__property Byte Part3 = {read=GetPart, write=SetPart, index=3, default=0};
	__property Byte Part4 = {read=GetPart, write=SetPart, index=4, default=0};
	__property bool Part1CanEdit = {read=GetPartCanEdit, write=SetPartCanEdit, index=1, default=1};
	__property bool Part2CanEdit = {read=GetPartCanEdit, write=SetPartCanEdit, index=2, default=1};
	__property bool Part3CanEdit = {read=GetPartCanEdit, write=SetPartCanEdit, index=3, default=1};
	__property bool Part4CanEdit = {read=GetPartCanEdit, write=SetPartCanEdit, index=4, default=1};
	__property AnsiString IPString = {read=GetIPString, write=SetIPString};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
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
	__property TabStop  = {default=0};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property OnClick ;
	__property OnContextPopup ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDock ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDock ;
	__property OnStartDrag ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElIPEdit(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elipedit */
using namespace Elipedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElIPEdit
