// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDailyTip.pas' rev: 6.00

#ifndef ElDailyTipHPP
#define ElDailyTipHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <HTMLRender.hpp>	// Pascal unit
#include <ElHTMLLbl.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElStrPool.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eldailytip
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElDailyTipForm;
class PASCALIMPLEMENTATION TElDailyTipForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Elpopbtn::TElPopupButton* OkBtn;
	Stdctrls::TCheckBox* NextTimeCB;
	Elpopbtn::TElPopupButton* NextBtn;
	Extctrls::TPanel* Panel1;
	Extctrls::TPanel* Panel2;
	Extctrls::TImage* Image1;
	Extctrls::TPanel* Panel3;
	Extctrls::TPanel* Panel4;
	Stdctrls::TLabel* Label1;
	Extctrls::TPanel* Panel5;
	Stdctrls::TLabel* TipNumLabel;
	Elhtmllbl::TElHTMLLabel* TipText;
	void __fastcall NextBtnClick(System::TObject* Sender);
	
private:
	int MinNum;
	int CurNum;
	int MaxNum;
	Elstrpool::TElStringPool* FStringPool;
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TElDailyTipForm(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TElDailyTipForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TElDailyTipForm(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDailyTipForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElDailyTipDialog;
class PASCALIMPLEMENTATION TElDailyTipDialog : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FShowNextTime;
	int FStartID;
	int FEndID;
	bool FShowTipNumber;
	Elstrpool::TElStringPool* FStringPool;
	bool FIsHTML;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Graphics::TColor FLinkColor;
	Graphics::TFontStyles FLinkStyle;
	void __fastcall SetStringPool(Elstrpool::TElStringPool* newValue);
	void __fastcall SetStartID(int newValue);
	void __fastcall SetEndID(int newValue);
	void __fastcall SetIsHTML(bool newValue);
	
protected:
	virtual void __fastcall SetLinkColor(Graphics::TColor newValue);
	virtual void __fastcall SetLinkStyle(Graphics::TFontStyles newValue);
	
public:
	void __fastcall Execute(void);
	__fastcall virtual TElDailyTipDialog(Classes::TComponent* AOwner);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	
__published:
	__property bool ShowNextTime = {read=FShowNextTime, write=FShowNextTime, nodefault};
	__property int StartID = {read=FStartID, write=SetStartID, default=10001};
	__property int EndID = {read=FEndID, write=SetEndID, default=10001};
	__property bool ShowTipNumber = {read=FShowTipNumber, write=FShowTipNumber, nodefault};
	__property Elstrpool::TElStringPool* StringPool = {read=FStringPool, write=SetStringPool};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, nodefault};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property Graphics::TColor LinkColor = {read=FLinkColor, write=SetLinkColor, nodefault};
	__property Graphics::TFontStyles LinkStyle = {read=FLinkStyle, write=SetLinkStyle, nodefault};
public:
	#pragma option push -w-inl
	/* TComponent.Destroy */ inline __fastcall virtual ~TElDailyTipDialog(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TElDailyTipForm* ElDailyTipForm;

}	/* namespace Eldailytip */
using namespace Eldailytip;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDailyTip
