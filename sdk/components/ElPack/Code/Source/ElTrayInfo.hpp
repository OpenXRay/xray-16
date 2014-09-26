// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTrayInfo.pas' rev: 6.00

#ifndef ElTrayInfoHPP
#define ElTrayInfoHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ExtCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElFrmPers.hpp>	// Pascal unit
#include <ElHTMLLbl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltrayinfo
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElTrayInfoType { titInformation, titWarning, titError };
#pragma option pop

class DELPHICLASS TTrayInfoForm;
class PASCALIMPLEMENTATION TTrayInfoForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Extctrls::TPanel* Panel1;
	Extctrls::TImage* Image1;
	Extctrls::TTimer* Timer;
	Elhtmllbl::TElHTMLLabel* InfoLabel;
	Elfrmpers::TElFormPersist* ElFormPersist1;
	void __fastcall TimerTimer(System::TObject* Sender);
	void __fastcall ClickHandler(System::TObject* Sender);
	void __fastcall DblClickHandler(System::TObject* Sender);
	void __fastcall ShowHandler(System::TObject* Sender);
	void __fastcall HideHandler(System::TObject* Sender);
	
private:
	MESSAGE void __fastcall WMMouseActivate(Messages::TMessage &Msg);
	
protected:
	Classes::TNotifyEvent FOnShow;
	Classes::TNotifyEvent FOnHide;
	Classes::TNotifyEvent FOnClick;
	Classes::TNotifyEvent FOnDblClick;
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TTrayInfoForm(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TTrayInfoForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TTrayInfoForm(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TTrayInfoForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElTrayInfo;
class PASCALIMPLEMENTATION TElTrayInfo : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FIsHTML;
	TTrayInfoForm* FInfoForm;
	unsigned FShowTime;
	TElTrayInfoType FInfoType;
	WideString FMessage;
	Graphics::TColor FColor;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Classes::TNotifyEvent FOnShow;
	Classes::TNotifyEvent FOnHide;
	Classes::TNotifyEvent FOnClick;
	Classes::TNotifyEvent FOnDblClick;
	
protected:
	virtual void __fastcall SetShowTime(unsigned newValue);
	virtual void __fastcall SetInfoType(TElTrayInfoType newValue);
	virtual void __fastcall SetMessage(WideString newValue);
	void __fastcall AdjustFormIcon(void);
	void __fastcall AdjustFormSize(int X, int Y);
	int __fastcall SuggestedHeight(void);
	int __fastcall SuggestedWidth(void);
	virtual void __fastcall SetIsHTML(bool newValue);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall ClickHandler(System::TObject* Sender);
	void __fastcall DblClickHandler(System::TObject* Sender);
	void __fastcall ShowHandler(System::TObject* Sender);
	void __fastcall HideHandler(System::TObject* Sender);
	void __fastcall SetColor(Graphics::TColor Value);
	
public:
	void __fastcall Show(void);
	void __fastcall Hide(void);
	__fastcall virtual TElTrayInfo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTrayInfo(void);
	
__published:
	__property Graphics::TColor Color = {read=FColor, write=SetColor, default=-2147483624};
	__property unsigned ShowTime = {read=FShowTime, write=SetShowTime, nodefault};
	__property TElTrayInfoType InfoType = {read=FInfoType, write=SetInfoType, nodefault};
	__property WideString Message = {read=FMessage, write=SetMessage};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, nodefault};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property Classes::TNotifyEvent OnShow = {read=FOnShow, write=FOnShow};
	__property Classes::TNotifyEvent OnHide = {read=FOnHide, write=FOnHide};
	__property Classes::TNotifyEvent OnClick = {read=FOnClick, write=FOnClick};
	__property Classes::TNotifyEvent OnDblClick = {read=FOnDblClick, write=FOnDblClick};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TTrayInfoForm* TrayInfoForm;

}	/* namespace Eltrayinfo */
using namespace Eltrayinfo;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTrayInfo
