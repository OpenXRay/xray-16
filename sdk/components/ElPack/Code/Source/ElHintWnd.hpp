// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHintWnd.pas' rev: 6.00

#ifndef ElHintWndHPP
#define ElHintWndHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhintwnd
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElHintWindow;
class PASCALIMPLEMENTATION TElHintWindow : public Controls::THintWindow 
{
	typedef Controls::THintWindow inherited;
	
protected:
	Graphics::TFont* FFont;
	bool FActivating;
	WideString FWideCaption;
	Htmlrender::TElHTMLRender* FRender;
	bool FIsHTML;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	HIDESBASE void __fastcall SetFont(Graphics::TFont* newValue);
	bool FWordWrap;
	virtual void __fastcall TriggerLinkClickEvent(WideString HRef);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	HIDESBASE MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Message);
	void __fastcall SetWordWrap(bool Value);
	
public:
	__fastcall virtual TElHintWindow(Classes::TComponent* AOwner);
	__fastcall virtual ~TElHintWindow(void);
	virtual void __fastcall Paint(void);
	virtual Types::TRect __fastcall CalcHintRect(int MaxWidth, const AnsiString AHint, void * AData);
	Types::TRect __fastcall CalcHintRectW(int MaxWidth, const WideString AHint, void * AData);
	virtual void __fastcall ActivateHintW(const Types::TRect &Rect, const WideString AHint);
	__property Canvas ;
	
__published:
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property bool WordWrap = {read=FWordWrap, write=SetWordWrap, default=0};
	__property WideString WideCaption = {read=FWideCaption, write=FWideCaption};
	__property bool IsHTML = {read=FIsHTML, write=FIsHTML, nodefault};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHintWindow(HWND ParentWindow) : Controls::THintWindow(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE WideString __fastcall GetUnicodeHint(AnsiString Hint);

}	/* namespace Elhintwnd */
using namespace Elhintwnd;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHintWnd
