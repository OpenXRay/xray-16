// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElSpin.pas' rev: 6.00

#ifndef ElSpinHPP
#define ElSpinHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElSpinBtn.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elspin
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElSpinEditError;
class PASCALIMPLEMENTATION TElSpinEditError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall TElSpinEditError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall TElSpinEditError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall TElSpinEditError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall TElSpinEditError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall TElSpinEditError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall TElSpinEditError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall TElSpinEditError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall TElSpinEditError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElSpinEditError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElSpinEdit;
class PASCALIMPLEMENTATION TElSpinEdit : public Eledits::TCustomElEdit 
{
	typedef Eledits::TCustomElEdit inherited;
	
protected:
	bool FModified;
	int FBtnWidth;
	bool FMouseOver;
	int FLargeIncrement;
	int FIncrement;
	int FSaveValue;
	int FSavePos;
	int FSaveLen;
	bool FChanging;
	int FValue;
	int FMaxValue;
	int FMinValue;
	bool FAllowEdit;
	Elspinbtn::TElSpinButton* FButton;
	Classes::TNotifyEvent FOnUpClick;
	Classes::TNotifyEvent FOnDownClick;
	int FButtonWidth;
	bool FUseButtonWidth;
	bool FValueUndefined;
	bool FReadOnly;
	bool FButtonThinFrame;
	bool FButtonFlat;
	void __fastcall SetValue(int newValue);
	void __fastcall SetMaxValue(int newValue);
	void __fastcall SetMinValue(int newValue);
	void __fastcall SetAllowEdit(bool newValue);
	void __fastcall SetIncrement(int newValue);
	HIDESBASE void __fastcall SetEditRect(void);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	MESSAGE void __fastcall WMCreate(Messages::TWMCreate &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TWMMouseWheel &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMCut(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaste(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMButtonDown(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TMessage &Msg);
	virtual void __fastcall SpinUpClick(System::TObject* Sender, double Increment);
	virtual void __fastcall SpinDownClick(System::TObject* Sender, double Increment);
	virtual void __fastcall SpinDrag(System::TObject* Sender, double NewValue);
	void __fastcall SpinStart(System::TObject* Sender, double &StartValue);
	HIDESBASE void __fastcall SetReadOnly(bool Value);
	virtual void __fastcall SetFlat(const bool Value);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	DYNAMIC void __fastcall KeyPress(char &Key);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall Click(void);
	DYNAMIC Menus::TPopupMenu* __fastcall GetPopupMenu(void);
	HIDESBASE virtual void __fastcall SetModified(bool newValue);
	void __fastcall SetButtonWidth(const int Value);
	void __fastcall SetUseButtonWidth(bool Value);
	Elspinbtn::TElSpinBtnType __fastcall GetButtonType(void);
	void __fastcall SetButtonType(Elspinbtn::TElSpinBtnType Value);
	Elspinbtn::TElSpinBtnDir __fastcall GetButtonDirection(void);
	void __fastcall SetButtonDirection(Elspinbtn::TElSpinBtnDir Value);
	void __fastcall SetValueUndefined(bool Value);
	bool __fastcall GetUseXPThemes(void);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	void __fastcall SetButtonThinFrame(bool Value);
	virtual void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	virtual void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetButtonFlat(bool Value);
	
public:
	__fastcall virtual TElSpinEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElSpinEdit(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall Change(void);
	virtual void __fastcall Loaded(void);
	__property bool MouseOver = {read=FMouseOver, nodefault};
	__property bool Modified = {read=FModified, write=SetModified, nodefault};
	
__published:
	__property int Value = {read=FValue, write=SetValue, default=0};
	__property int MaxValue = {read=FMaxValue, write=SetMaxValue, default=100};
	__property int MinValue = {read=FMinValue, write=SetMinValue, default=0};
	__property bool AllowEdit = {read=FAllowEdit, write=SetAllowEdit, default=1};
	__property int Increment = {read=FIncrement, write=SetIncrement, nodefault};
	__property int LargeIncrement = {read=FLargeIncrement, write=FLargeIncrement, nodefault};
	__property int ButtonWidth = {read=FButtonWidth, write=SetButtonWidth, stored=FUseButtonWidth, nodefault};
	__property bool UseButtonWidth = {read=FUseButtonWidth, write=SetUseButtonWidth, default=0};
	__property Elspinbtn::TElSpinBtnType ButtonType = {read=GetButtonType, write=SetButtonType, default=0};
	__property Elspinbtn::TElSpinBtnDir ButtonDirection = {read=GetButtonDirection, write=SetButtonDirection, default=0};
	__property bool ValueUndefined = {read=FValueUndefined, write=SetValueUndefined, default=0};
	__property bool ReadOnly = {read=FReadOnly, write=SetReadOnly, default=0};
	__property bool UseXPThemes = {read=GetUseXPThemes, write=SetUseXPThemes, default=1};
	__property bool ButtonThinFrame = {read=FButtonThinFrame, write=SetButtonThinFrame, default=1};
	__property Classes::TNotifyEvent OnUpClick = {read=FOnUpClick, write=FOnUpClick};
	__property Classes::TNotifyEvent OnDownClick = {read=FOnDownClick, write=FOnDownClick};
	__property Alignment ;
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property BorderSides ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property ImageForm ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DragKind  = {default=0};
	__property BiDiMode ;
	__property ActiveBorderType  = {default=1};
	__property Align  = {default=0};
	__property AutoSelect  = {default=0};
	__property AutoSize  = {default=1};
	__property Background ;
	__property BorderStyle ;
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property ShowHint ;
	__property UseBackground  = {default=0};
	__property Visible  = {default=1};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
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
	__property OnStartDrag ;
	__property bool ButtonFlat = {read=FButtonFlat, write=SetButtonFlat, default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElSpinEdit(HWND ParentWindow) : Eledits::TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElFloatSpinEdit;
class PASCALIMPLEMENTATION TElFloatSpinEdit : public Eledits::TCustomElEdit 
{
	typedef Eledits::TCustomElEdit inherited;
	
protected:
	bool FAllowEdit;
	int FBtnWidth;
	Elspinbtn::TElSpinButton* FButton;
	int FButtonWidth;
	bool FChanging;
	double FIncrement;
	double FLargeIncrement;
	double FMaxValue;
	double FMinValue;
	bool FModified;
	bool FMouseOver;
	Classes::TNotifyEvent FOnUpClick;
	Classes::TNotifyEvent FOnDownClick;
	bool FReadOnly;
	int FSaveLen;
	int FSavePos;
	double FSaveValue;
	bool FUseButtonWidth;
	double FValue;
	bool FValueUndefined;
	bool FButtonThinFrame;
	void __fastcall SetAllowEdit(bool newValue);
	HIDESBASE void __fastcall SetEditRect(void);
	void __fastcall SetIncrement(double newValue);
	void __fastcall SetMaxValue(double newValue);
	void __fastcall SetMinValue(double newValue);
	HIDESBASE void __fastcall SetReadOnly(bool Value);
	void __fastcall SetValue(double newValue);
	virtual void __fastcall SpinDownClick(System::TObject* Sender, double Increment);
	virtual void __fastcall SpinDrag(System::TObject* Sender, double NewValue);
	void __fastcall SpinStart(System::TObject* Sender, double &StartValue);
	virtual void __fastcall SpinUpClick(System::TObject* Sender, double Increment);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	MESSAGE void __fastcall WMCreate(Messages::TWMCreate &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TWMMouseWheel &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMCut(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaste(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMButtonDown(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TMessage &Msg);
	DYNAMIC void __fastcall Click(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	Elspinbtn::TElSpinBtnDir __fastcall GetButtonDirection(void);
	Elspinbtn::TElSpinBtnType __fastcall GetButtonType(void);
	DYNAMIC Menus::TPopupMenu* __fastcall GetPopupMenu(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	void __fastcall SetButtonDirection(Elspinbtn::TElSpinBtnDir Value);
	void __fastcall SetButtonType(Elspinbtn::TElSpinBtnType Value);
	void __fastcall SetButtonWidth(const int Value);
	virtual void __fastcall SetFlat(const bool Value);
	HIDESBASE virtual void __fastcall SetModified(bool newValue);
	void __fastcall SetUseButtonWidth(bool Value);
	void __fastcall SetValueUndefined(bool Value);
	bool __fastcall GetUseXPThemes(void);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	void __fastcall SetButtonThinFrame(bool Value);
	virtual void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	virtual void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	
public:
	__fastcall virtual TElFloatSpinEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFloatSpinEdit(void);
	virtual void __fastcall Change(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall Loaded(void);
	__property bool Modified = {read=FModified, write=SetModified, nodefault};
	__property bool MouseOver = {read=FMouseOver, nodefault};
	
__published:
	__property bool AllowEdit = {read=FAllowEdit, write=SetAllowEdit, default=1};
	__property Elspinbtn::TElSpinBtnDir ButtonDirection = {read=GetButtonDirection, write=SetButtonDirection, default=0};
	__property Elspinbtn::TElSpinBtnType ButtonType = {read=GetButtonType, write=SetButtonType, default=0};
	__property int ButtonWidth = {read=FButtonWidth, write=SetButtonWidth, stored=FUseButtonWidth, nodefault};
	__property double Increment = {read=FIncrement, write=SetIncrement};
	__property double LargeIncrement = {read=FLargeIncrement, write=FLargeIncrement};
	__property double MaxValue = {read=FMaxValue, write=SetMaxValue};
	__property double MinValue = {read=FMinValue, write=SetMinValue};
	__property Classes::TNotifyEvent OnDownClick = {read=FOnDownClick, write=FOnDownClick};
	__property Classes::TNotifyEvent OnUpClick = {read=FOnUpClick, write=FOnUpClick};
	__property bool ReadOnly = {read=FReadOnly, write=SetReadOnly, default=0};
	__property bool UseButtonWidth = {read=FUseButtonWidth, write=SetUseButtonWidth, default=0};
	__property double Value = {read=FValue, write=SetValue};
	__property bool ValueUndefined = {read=FValueUndefined, write=SetValueUndefined, default=0};
	__property Alignment ;
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property BorderSides ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property ImageForm ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DragKind  = {default=0};
	__property BiDiMode ;
	__property ActiveBorderType  = {default=1};
	__property Align  = {default=0};
	__property AutoSelect  = {default=0};
	__property AutoSize  = {default=1};
	__property Background ;
	__property BorderStyle ;
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property ShowHint ;
	__property UseBackground  = {default=0};
	__property Visible  = {default=1};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
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
	__property OnStartDrag ;
	__property bool UseXPThemes = {read=GetUseXPThemes, write=SetUseXPThemes, default=1};
	__property bool ButtonThinFrame = {read=FButtonThinFrame, write=SetButtonThinFrame, default=1};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElFloatSpinEdit(HWND ParentWindow) : Eledits::TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elspin */
using namespace Elspin;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElSpin
