// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHTMLHint.pas' rev: 6.00

#ifndef ElHTMLHintHPP
#define ElHTMLHintHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <HTMLRender.hpp>	// Pascal unit
#include <ElHintWnd.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhtmlhint
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElHTMLHint;
class PASCALIMPLEMENTATION TElHTMLHint : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FEnabled;
	TMetaClass*FHintClass;
	Classes::TNotifyEvent FOnShow;
	Classes::TNotifyEvent FOnHide;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	
protected:
	AnsiString FFontName;
	virtual void __fastcall SetEnabled(bool Value);
	void __fastcall SetOnHide(Classes::TNotifyEvent Value);
	void __fastcall SetOnShow(Classes::TNotifyEvent Value);
	void __fastcall SetOnImageNeeded(Htmlrender::TElHTMLImageNeededEvent Value);
	void __fastcall SetFontName(const AnsiString Value);
	
public:
	__fastcall virtual ~TElHTMLHint(void);
	
__published:
	__property bool Enabled = {read=FEnabled, write=SetEnabled, nodefault};
	__property Classes::TNotifyEvent OnShow = {read=FOnShow, write=SetOnShow};
	__property Classes::TNotifyEvent OnHide = {read=FOnHide, write=SetOnHide};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=SetOnImageNeeded};
	__property AnsiString FontName = {read=FFontName, write=SetFontName};
public:
	#pragma option push -w-inl
	/* TComponent.Create */ inline __fastcall virtual TElHTMLHint(Classes::TComponent* AOwner) : Classes::TComponent(AOwner) { }
	#pragma option pop
	
};


class DELPHICLASS TElHTMLHintWindow;
class PASCALIMPLEMENTATION TElHTMLHintWindow : public Elhintwnd::TElHintWindow 
{
	typedef Elhintwnd::TElHintWindow inherited;
	
protected:
	void __fastcall OnShow(void);
	void __fastcall OnHide(void);
	MESSAGE void __fastcall WMShowWindow(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TMessage &Message);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	void __fastcall SetupRightCaption(AnsiString Caption);
	
public:
	__fastcall virtual TElHTMLHintWindow(Classes::TComponent* AOwner);
	__fastcall virtual ~TElHTMLHintWindow(void);
	virtual Types::TRect __fastcall CalcHintRect(int MaxWidth, const AnsiString AHint, void * AData);
	virtual void __fastcall ActivateHint(const Types::TRect &Rect, const AnsiString AHint);
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHTMLHintWindow(HWND ParentWindow) : Elhintwnd::TElHintWindow(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elhtmlhint */
using namespace Elhtmlhint;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHTMLHint
