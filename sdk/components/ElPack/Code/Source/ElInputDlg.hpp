// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElInputDlg.pas' rev: 6.00

#ifndef ElInputDlgHPP
#define ElInputDlgHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElPopBtn.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElHTMLLbl.hpp>	// Pascal unit
#include <ElCheckCtl.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
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

namespace Elinputdlg
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElInputDialog;
class PASCALIMPLEMENTATION TElInputDialog : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	
protected:
	AnsiString FPrompt;
	AnsiString FCaption;
	bool FIsHTML;
	AnsiString FValue;
	
public:
	bool __fastcall Execute(void);
	
__published:
	__property AnsiString Prompt = {read=FPrompt, write=FPrompt};
	__property AnsiString Caption = {read=FCaption, write=FCaption};
	__property bool IsHTML = {read=FIsHTML, write=FIsHTML, nodefault};
	__property AnsiString Value = {read=FValue, write=FValue};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
public:
	#pragma option push -w-inl
	/* TComponent.Create */ inline __fastcall virtual TElInputDialog(Classes::TComponent* AOwner) : Classes::TComponent(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TComponent.Destroy */ inline __fastcall virtual ~TElInputDialog(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool __fastcall InputQuery(const AnsiString ACaption, const AnsiString APrompt, AnsiString &AValue, bool AIsHTML);

}	/* namespace Elinputdlg */
using namespace Elinputdlg;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElInputDlg
