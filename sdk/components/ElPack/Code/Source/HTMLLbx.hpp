// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'HTMLLbx.pas' rev: 6.00

#ifndef HTMLLbxHPP
#define HTMLLbxHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElHintWnd.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Htmllbx
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TCustomElHTMLListBox;
class PASCALIMPLEMENTATION TCustomElHTMLListBox : public Elactrls::TElAdvancedListBox 
{
	typedef Elactrls::TElAdvancedListBox inherited;
	
private:
	Classes::TNotifyEvent FDummyEvent;
	Htmlrender::TElHTMLRender* FRender;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	bool FIsHTML;
	Controls::TCursor FCursor;
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	
protected:
	Stdctrls::TListBoxStyle FStyle;
	virtual int __fastcall GetItemWidth(int Index);
	virtual void __fastcall MeasureItem(int Index, int &Height);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	virtual void __fastcall TriggerLinkClickEvent(System::TObject* Sender, WideString HRef);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	HIDESBASE virtual void __fastcall SetCursor(Controls::TCursor newValue);
	virtual Controls::THintWindow* __fastcall CreateHintWindow(void);
	HIDESBASE void __fastcall SetStyle(Stdctrls::TListBoxStyle Value);
	virtual void __fastcall SetIsHTML(bool Value);
	
public:
	__fastcall virtual TCustomElHTMLListBox(Classes::TComponent* AOwner);
	virtual void __fastcall Loaded(void);
	__fastcall virtual ~TCustomElHTMLListBox(void);
	
__published:
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Classes::TNotifyEvent OnDrawItem = {read=FDummyEvent};
	__property Classes::TNotifyEvent OnMeasureItem = {read=FDummyEvent};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, nodefault};
	__property Controls::TCursor Cursor = {read=FCursor, write=SetCursor, nodefault};
	__property Stdctrls::TListBoxStyle Style = {read=FStyle, write=SetStyle, default=2};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElHTMLListBox(HWND ParentWindow) : Elactrls::TElAdvancedListBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TCustomElHTMLComboBox;
class PASCALIMPLEMENTATION TCustomElHTMLComboBox : public Elactrls::TElAdvancedComboBox 
{
	typedef Elactrls::TElAdvancedComboBox inherited;
	
private:
	Classes::TNotifyEvent FDummyEvent;
	Htmlrender::TElHTMLRender* FRender;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	bool FIsHTML;
	
protected:
	Stdctrls::TComboBoxStyle FStyle;
	virtual void __fastcall MeasureItem(int Index, int &Height);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	virtual void __fastcall TriggerLinkClickEvent(System::TObject* Sender, WideString HRef);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	HIDESBASE void __fastcall SetStyle(Stdctrls::TComboBoxStyle Value);
	virtual void __fastcall SetIsHTML(bool Value);
	
public:
	__fastcall virtual TCustomElHTMLComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElHTMLComboBox(void);
	virtual void __fastcall Loaded(void);
	
__published:
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Classes::TNotifyEvent OnDrawItem = {read=FDummyEvent};
	__property Classes::TNotifyEvent OnMeasureItem = {read=FDummyEvent};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, nodefault};
	__property Stdctrls::TComboBoxStyle Style = {read=FStyle, write=SetStyle, default=4};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElHTMLComboBox(HWND ParentWindow) : Elactrls::TElAdvancedComboBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElHTMLListBox;
class PASCALIMPLEMENTATION TElHTMLListBox : public TCustomElHTMLListBox 
{
	typedef TCustomElHTMLListBox inherited;
	
public:
	#pragma option push -w-inl
	/* TCustomElHTMLListBox.Create */ inline __fastcall virtual TElHTMLListBox(Classes::TComponent* AOwner) : TCustomElHTMLListBox(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElHTMLListBox.Destroy */ inline __fastcall virtual ~TElHTMLListBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHTMLListBox(HWND ParentWindow) : TCustomElHTMLListBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElHTMLComboBox;
class PASCALIMPLEMENTATION TElHTMLComboBox : public TCustomElHTMLComboBox 
{
	typedef TCustomElHTMLComboBox inherited;
	
public:
	#pragma option push -w-inl
	/* TCustomElHTMLComboBox.Create */ inline __fastcall virtual TElHTMLComboBox(Classes::TComponent* AOwner) : TCustomElHTMLComboBox(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElHTMLComboBox.Destroy */ inline __fastcall virtual ~TElHTMLComboBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHTMLComboBox(HWND ParentWindow) : TCustomElHTMLComboBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Htmllbx */
using namespace Htmllbx;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// HTMLLbx
