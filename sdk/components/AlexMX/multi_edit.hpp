// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'multi_edit.pas' rev: 6.00

#ifndef multi_editHPP
#define multi_editHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <CommCtrl.hpp>	// Pascal unit
#include <ComCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Multi_edit
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TSpinButtonState { sbNotDown, sbTopDown, sbBottomDown };
#pragma option pop

#pragma option push -b-
enum TValueType { vtInt, vtFloat, vtHex };
#pragma option pop

#pragma option push -b-
enum TSpinButtonKind { bkStandard, bkDiagonal, bkLightWave };
#pragma option pop

#pragma option push -b-
enum TLWButtonState { sbLWNotDown, sbLWDown };
#pragma option pop

class DELPHICLASS TMultiObjSpinButton;
class PASCALIMPLEMENTATION TMultiObjSpinButton : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	TSpinButtonState FDown;
	Graphics::TBitmap* FUpBitmap;
	Graphics::TBitmap* FDownBitmap;
	bool FDragging;
	bool FInvalidate;
	Graphics::TBitmap* FTopDownBtn;
	Graphics::TBitmap* FBottomDownBtn;
	Extctrls::TTimer* FRepeatTimer;
	Graphics::TBitmap* FNotDownBtn;
	TSpinButtonState FLastDown;
	Controls::TWinControl* FFocusControl;
	Classes::TNotifyEvent FOnTopClick;
	Classes::TNotifyEvent FOnBottomClick;
	void __fastcall TopClick(void);
	void __fastcall BottomClick(void);
	void __fastcall GlyphChanged(System::TObject* Sender);
	Graphics::TBitmap* __fastcall GetUpGlyph(void);
	Graphics::TBitmap* __fastcall GetDownGlyph(void);
	void __fastcall SetUpGlyph(Graphics::TBitmap* Value);
	void __fastcall SetDownGlyph(Graphics::TBitmap* Value);
	void __fastcall SetDown(TSpinButtonState Value);
	void __fastcall SetFocusControl(Controls::TWinControl* Value);
	void __fastcall DrawAllBitmap(void);
	void __fastcall DrawBitmap(Graphics::TBitmap* ABitmap, TSpinButtonState ADownState);
	void __fastcall TimerExpired(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	
protected:
	virtual void __fastcall Paint(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TMultiObjSpinButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TMultiObjSpinButton(void);
	__property TSpinButtonState Down = {read=FDown, write=SetDown, default=0};
	
__published:
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Visible  = {default=1};
	__property Graphics::TBitmap* DownGlyph = {read=GetDownGlyph, write=SetDownGlyph};
	__property Graphics::TBitmap* UpGlyph = {read=GetUpGlyph, write=SetUpGlyph};
	__property Controls::TWinControl* FocusControl = {read=FFocusControl, write=SetFocusControl};
	__property ShowHint ;
	__property ParentShowHint  = {default=1};
	__property Classes::TNotifyEvent OnBottomClick = {read=FOnBottomClick, write=FOnBottomClick};
	__property Classes::TNotifyEvent OnTopClick = {read=FOnTopClick, write=FOnTopClick};
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnStartDrag ;
};


typedef void __fastcall (__closure *TLWNotifyEvent)(System::TObject* Sender, int Val);

class DELPHICLASS TMultiObjLWButton;
class PASCALIMPLEMENTATION TMultiObjLWButton : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	TLWButtonState FDown;
	Graphics::TBitmap* FLWBitmap;
	bool FDragging;
	bool FInvalidate;
	Graphics::TBitmap* FLWDownBtn;
	Graphics::TBitmap* FLWNotDownBtn;
	Controls::TWinControl* FFocusControl;
	TLWNotifyEvent FOnLWChange;
	double FSens;
	double FAccum;
	void __fastcall LWChange(System::TObject* Sender, int Val);
	void __fastcall GlyphChanged(System::TObject* Sender);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	void __fastcall SetDown(TLWButtonState Value);
	void __fastcall SetFocusControl(Controls::TWinControl* Value);
	void __fastcall DrawAllBitmap(void);
	void __fastcall DrawBitmap(Graphics::TBitmap* ABitmap, TLWButtonState ADownState);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	
protected:
	virtual void __fastcall Paint(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall SetSens(double Value);
	
public:
	__fastcall virtual TMultiObjLWButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TMultiObjLWButton(void);
	__property TLWButtonState Down = {read=FDown, write=SetDown, default=0};
	virtual void __fastcall Invalidate(void);
	
__published:
	__property double LWSensitivity = {read=FSens, write=SetSens};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Visible  = {default=1};
	__property Graphics::TBitmap* LWGlyph = {read=GetGlyph, write=SetGlyph};
	__property Controls::TWinControl* FocusControl = {read=FFocusControl, write=SetFocusControl};
	__property TLWNotifyEvent OnLWChange = {read=FOnLWChange, write=FOnLWChange};
	__property ShowHint ;
	__property ParentShowHint  = {default=1};
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnStartDrag ;
};


class DELPHICLASS TMultiObjSpinEdit;
class PASCALIMPLEMENTATION TMultiObjSpinEdit : public Stdctrls::TCustomEdit 
{
	typedef Stdctrls::TCustomEdit inherited;
	
private:
	bool bIsMulti;
	bool bChanged;
	Extended BeforeValue;
	Graphics::TColor StartColor;
	Graphics::TColor FBtnColor;
	Classes::TAlignment FAlignment;
	Extended FMinValue;
	Extended FMaxValue;
	Extended FIncrement;
	int FButtonWidth;
	Byte FDecimal;
	bool FChanging;
	bool FEditorEnabled;
	TValueType FValueType;
	TMultiObjSpinButton* FButton;
	Controls::TWinControl* FBtnWindow;
	bool FArrowKeys;
	Classes::TNotifyEvent FOnTopClick;
	Classes::TNotifyEvent FOnBottomClick;
	TSpinButtonKind FButtonKind;
	Comctrls::TCustomUpDown* FUpDown;
	TMultiObjLWButton* FLWButton;
	TLWNotifyEvent FOnLWChange;
	double FSens;
	TSpinButtonKind __fastcall GetButtonKind(void);
	void __fastcall SetButtonKind(TSpinButtonKind Value);
	void __fastcall UpDownClick(System::TObject* Sender, Comctrls::TUDBtnType Button);
	void __fastcall LWChange(System::TObject* Sender, int Val);
	Extended __fastcall GetValue(void);
	Extended __fastcall CheckValue(Extended NewValue);
	int __fastcall GetAsInteger(void);
	bool __fastcall IsIncrementStored(void);
	bool __fastcall IsMaxStored(void);
	bool __fastcall IsMinStored(void);
	bool __fastcall IsValueStored(void);
	void __fastcall SetArrowKeys(bool Value);
	void __fastcall SetAsInteger(int NewValue);
	void __fastcall SetValue(Extended NewValue);
	void __fastcall SetValueType(TValueType NewType);
	void __fastcall SetButtonWidth(int NewValue);
	void __fastcall SetDecimal(Byte NewValue);
	int __fastcall GetButtonWidth(void);
	void __fastcall RecreateButton(void);
	void __fastcall ResizeButton(void);
	void __fastcall SetEditRect(void);
	void __fastcall SetAlignment(Classes::TAlignment Value);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Message);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall WMPaste(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall WMCut(Messages::TWMNoParams &Message);
	HIDESBASE MESSAGE void __fastcall CMCtl3DChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	void __fastcall SetBtnColor(Graphics::TColor Value);
	
protected:
	DYNAMIC void __fastcall Change(void);
	virtual bool __fastcall IsValidChar(char Key);
	virtual void __fastcall UpClick(System::TObject* Sender);
	virtual void __fastcall DownClick(System::TObject* Sender);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWnd(void);
	void __fastcall SetSens(double Val);
	double __fastcall GetSens(void);
	
public:
	__fastcall virtual TMultiObjSpinEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TMultiObjSpinEdit(void);
	__property int AsInteger = {read=GetAsInteger, write=SetAsInteger, default=0};
	__property Text ;
	void __fastcall ObjFirstInit(float v);
	void __fastcall ObjNextInit(float v);
	void __fastcall ObjApplyFloat(float &_to);
	void __fastcall ObjApplyInt(int &_to);
	
__published:
	__property double LWSensitivity = {read=GetSens, write=SetSens};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, default=0};
	__property bool ArrowKeys = {read=FArrowKeys, write=SetArrowKeys, default=1};
	__property Graphics::TColor BtnColor = {read=FBtnColor, write=SetBtnColor, default=-2147483633};
	__property TSpinButtonKind ButtonKind = {read=FButtonKind, write=SetButtonKind, default=0};
	__property Byte Decimal = {read=FDecimal, write=SetDecimal, default=2};
	__property int ButtonWidth = {read=FButtonWidth, write=SetButtonWidth, default=14};
	__property bool EditorEnabled = {read=FEditorEnabled, write=FEditorEnabled, default=1};
	__property Extended Increment = {read=FIncrement, write=FIncrement, stored=IsIncrementStored};
	__property Extended MaxValue = {read=FMaxValue, write=FMaxValue, stored=IsMaxStored};
	__property Extended MinValue = {read=FMinValue, write=FMinValue, stored=IsMinStored};
	__property TValueType ValueType = {read=FValueType, write=SetValueType, default=0};
	__property Extended Value = {read=GetValue, write=SetValue, stored=IsValueStored};
	__property AutoSelect  = {default=1};
	__property AutoSize  = {default=1};
	__property BorderStyle  = {default=1};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property MaxLength  = {default=0};
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ReadOnly  = {default=0};
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property TLWNotifyEvent OnLWChange = {read=FOnLWChange, write=FOnLWChange};
	__property Classes::TNotifyEvent OnBottomClick = {read=FOnBottomClick, write=FOnBottomClick};
	__property Classes::TNotifyEvent OnTopClick = {read=FOnTopClick, write=FOnTopClick};
	__property OnChange ;
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
	__property OnContextPopup ;
	__property OnMouseWheelDown ;
	__property OnMouseWheelUp ;
	__property OnEndDock ;
	__property OnStartDock ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TMultiObjSpinEdit(HWND ParentWindow) : Stdctrls::TCustomEdit(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Multi_edit */
using namespace Multi_edit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// multi_edit
