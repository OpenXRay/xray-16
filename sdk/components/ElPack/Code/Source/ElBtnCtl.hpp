// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElBtnCtl.pas' rev: 6.00

#ifndef ElBtnCtlHPP
#define ElBtnCtlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ActnList.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elbtnctl
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElButtonControl;
class PASCALIMPLEMENTATION TElButtonControl : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	HIDESBASE bool __fastcall IsCaptionStored(void);
	HIDESBASE MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Message);
	
protected:
	bool FTransparent;
	Elvclutils::TElTextDrawType FTextDrawType;
	bool F2000DrawFocus;
	bool F2000DrawAccel;
	HIDESBASE MESSAGE void __fastcall WMMove(Messages::TWMMove &Msg);
	void __fastcall SetTextDrawType(Elvclutils::TElTextDrawType newValue);
	bool ClicksDisabled;
	WideString FCaption;
	WideString FHint;
	bool FMoneyFlat;
	Graphics::TColor FMoneyFlatActiveColor;
	Graphics::TColor FMoneyFlatInactiveColor;
	Graphics::TColor FMoneyFlatDownColor;
	virtual bool __fastcall GetChecked(void) = 0 ;
	virtual void __fastcall SetChecked(bool newValue) = 0 ;
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	virtual void __fastcall SetTransparent(bool newValue);
	DYNAMIC TMetaClass* __fastcall GetActionLinkClass(void);
	DYNAMIC void __fastcall ActionChange(System::TObject* Sender, bool CheckDefaults);
	__property bool Checked = {read=GetChecked, write=SetChecked, default=0};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property Elvclutils::TElTextDrawType TextDrawType = {read=FTextDrawType, write=SetTextDrawType, default=0};
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	void __fastcall SetCaption(WideString Value);
	virtual WideString __fastcall GetThemedClassName();
	bool __fastcall GetUIStateDrawFocus(void);
	bool __fastcall GetUIStateDrawAccel(void);
	MESSAGE void __fastcall WMUpdateUIState(Messages::TMessage &Message);
	MESSAGE void __fastcall WMChangeUIState(Messages::TMessage &Message);
	void __fastcall SetMoneyFlat(bool Value);
	void __fastcall SetMoneyFlatActiveColor(Graphics::TColor Value);
	void __fastcall SetMoneyFlatInactiveColor(Graphics::TColor Value);
	void __fastcall SetMoneyFlatDownColor(Graphics::TColor Value);
	bool __fastcall GetMoneyFlat(void);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	__property Color ;
	__property WideString Caption = {read=FCaption, write=SetCaption, stored=IsCaptionStored};
	__property bool MoneyFlat = {read=GetMoneyFlat, write=SetMoneyFlat, default=0};
	__property Graphics::TColor MoneyFlatActiveColor = {read=FMoneyFlatActiveColor, write=SetMoneyFlatActiveColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatInactiveColor = {read=FMoneyFlatInactiveColor, write=SetMoneyFlatInactiveColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatDownColor = {read=FMoneyFlatDownColor, write=SetMoneyFlatDownColor, stored=GetMoneyFlat, nodefault};
	
public:
	__fastcall virtual TElButtonControl(Classes::TComponent* Owner);
	__property bool UIStateDrawFocus = {read=GetUIStateDrawFocus, nodefault};
	__property bool UIStateDrawAccel = {read=GetUIStateDrawAccel, nodefault};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TCustomControl.Destroy */ inline __fastcall virtual ~TElButtonControl(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElButtonControl(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElButtonActionLink;
class PASCALIMPLEMENTATION TElButtonActionLink : public Controls::TWinControlActionLink 
{
	typedef Controls::TWinControlActionLink inherited;
	
protected:
	TElButtonControl* FClient;
	virtual void __fastcall SetCaption(const AnsiString Value);
	virtual void __fastcall AssignClient(System::TObject* AClient);
	virtual bool __fastcall IsCheckedLinked(void);
	virtual bool __fastcall IsImageIndexLinked(void);
	virtual void __fastcall SetChecked(bool Value);
	virtual void __fastcall SetHint(const AnsiString Value);
public:
	#pragma option push -w-inl
	/* TBasicActionLink.Create */ inline __fastcall virtual TElButtonActionLink(System::TObject* AClient) : Controls::TWinControlActionLink(AClient) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TBasicActionLink.Destroy */ inline __fastcall virtual ~TElButtonActionLink(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elbtnctl */
using namespace Elbtnctl;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElBtnCtl
