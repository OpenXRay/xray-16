// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmColorMapItems.pas' rev: 6.00

#ifndef frmColorMapItemsHPP
#define frmColorMapItemsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElClrCmb.hpp>	// Pascal unit
#include <ElCheckCtl.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElColorMap.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
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

namespace Frmcolormapitems
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TColorMapItemsForm;
class PASCALIMPLEMENTATION TColorMapItemsForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Stdctrls::TLabel* Label1;
	Stdctrls::TLabel* Label2;
	Elclrcmb::TElColorCombo* FgColor;
	Elclrcmb::TElColorCombo* BkColor;
	Stdctrls::TLabel* Label3;
	Stdctrls::TLabel* Label4;
	Stdctrls::TLabel* Label5;
	Stdctrls::TLabel* IDLbl;
	Elpopbtn::TElPopupButton* OkBtn;
	Elpopbtn::TElPopupButton* CancelBtn;
	Elpopbtn::TElPopupButton* AddBtn;
	Elpopbtn::TElPopupButton* DelBtn;
	Elpopbtn::TElPopupButton* AddGroupBtn;
	Elpopbtn::TElPopupButton* DelGroupBtn;
	Elactrls::TElAdvancedListBox* EntryLB;
	Elactrls::TElAdvancedListBox* GroupLB;
	Elcheckctl::TElCheckBox* UseBkCB;
	Elcheckctl::TElCheckBox* UseFgCB;
	void __fastcall UseFgCBClick(System::TObject* Sender);
	void __fastcall CustomFgCBClick(System::TObject* Sender);
	void __fastcall AddGroupBtnClick(System::TObject* Sender);
	void __fastcall GroupLBClick(System::TObject* Sender);
	void __fastcall FormCreate(System::TObject* Sender);
	void __fastcall FormDestroy(System::TObject* Sender);
	void __fastcall FormShow(System::TObject* Sender);
	void __fastcall UseBkCBClick(System::TObject* Sender);
	void __fastcall DelGroupBtnClick(System::TObject* Sender);
	void __fastcall AddBtnClick(System::TObject* Sender);
	void __fastcall DelBtnClick(System::TObject* Sender);
	void __fastcall EntryLBClick(System::TObject* Sender);
	void __fastcall FgColorChange(System::TObject* Sender);
	void __fastcall BkColorChange(System::TObject* Sender);
	
protected:
	int GrSel;
	int MapSel;
	int EntSel;
	AnsiString SaveVal;
	void __fastcall RefreshEntriesList(void);
	
public:
	bool Runtime;
	Elcolormap::TElColorMap* Map;
	void __fastcall RefreshColors(void);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TColorMapItemsForm(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TColorMapItemsForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TColorMapItemsForm(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TColorMapItemsForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TColorMapItemsForm* ColorMapItemsForm;

}	/* namespace Frmcolormapitems */
using namespace Frmcolormapitems;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmColorMapItems
