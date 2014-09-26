// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmItemCol.pas' rev: 6.00

#ifndef frmItemColHPP
#define frmItemColHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ComCtrls.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <DsnConst.hpp>	// Pascal unit
#include <DesignWindows.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Frmitemcol
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TItemColDlg;
class PASCALIMPLEMENTATION TItemColDlg : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Stdctrls::TButton* OKBtn;
	Stdctrls::TButton* CancelBtn;
	Comctrls::TPageControl* PageControl1;
	Comctrls::TTabSheet* TabSheet1;
	Comctrls::TTabSheet* TabSheet2;
	Stdctrls::TLabel* Label2;
	Stdctrls::TMemo* TextEdit;
	Stdctrls::TLabel* Label13;
	Stdctrls::TMemo* HintEdit;
	Stdctrls::TMemo* ColTextMemo;
	Stdctrls::TLabel* Label1;
	Stdctrls::TCheckBox* StylesCB;
	Stdctrls::TGroupBox* StylesGB;
	Stdctrls::TCheckBox* BoldCB;
	Stdctrls::TCheckBox* ItCB;
	Stdctrls::TCheckBox* ULCB;
	Stdctrls::TCheckBox* StrikeCB;
	Stdctrls::TCheckBox* ColorsCB;
	Stdctrls::TGroupBox* ColorsGB;
	Stdctrls::TLabel* Label4;
	Stdctrls::TLabel* Label5;
	Stdctrls::TLabel* Label12;
	Stdctrls::TComboBox* ColorCombo;
	Stdctrls::TComboBox* BkColorCombo;
	Stdctrls::TComboBox* RowBkColorCombo;
	Stdctrls::TCheckBox* UseBkColorCB;
	Stdctrls::TCheckBox* ShowChecksCB;
	Stdctrls::TGroupBox* CBGroup;
	Stdctrls::TLabel* Label8;
	Stdctrls::TLabel* Label9;
	Stdctrls::TComboBox* CBTypeCombo;
	Stdctrls::TComboBox* CBStateCombo;
	Stdctrls::TCheckBox* CBEnabledCB;
	Stdctrls::TLabel* Label3;
	Stdctrls::TLabel* Label6;
	Stdctrls::TLabel* Label7;
	Stdctrls::TEdit* StIndexEdit;
	Stdctrls::TEdit* IndexEdit;
	Stdctrls::TLabel* Label10;
	Stdctrls::TLabel* Label11;
	Stdctrls::TEdit* Index2Edit;
	Stdctrls::TEdit* StIndex2Edit;
	Comctrls::TTabSheet* TabSheet3;
	Stdctrls::TCheckBox* ForcedBtnsCB;
	Stdctrls::TCheckBox* EnabledCB;
	Stdctrls::TCheckBox* HiddenCB;
	Stdctrls::TCheckBox* HtmlCB;
	Stdctrls::TLabel* Label14;
	Stdctrls::TEdit* TagEdit;
	Stdctrls::TCheckBox* StrikeOutCB;
	Stdctrls::TComboBox* StrikeLineColorCB;
	Stdctrls::TCheckBox* HorZlineCB;
	Stdctrls::TCheckBox* AllowEditCB;
	Stdctrls::TCheckBox* SuppressButtonsCB;
	Stdctrls::TCheckBox* MultilineCB;
	Stdctrls::TCheckBox* OwnHeightCB;
	Stdctrls::TEdit* HeightEdit;
	Stdctrls::TEdit* IndentEdit;
	Stdctrls::TCheckBox* IndentAdjustCB;
	Stdctrls::TComboBox* BorderStyleCombo;
	Stdctrls::TLabel* Label15;
	Stdctrls::TCheckBox* SuppressLinesCB;
	Stdctrls::TLabel* Label16;
	Stdctrls::TEdit* OvIndexEdit;
	Stdctrls::TEdit* OvIndex2Edit;
	void __fastcall ColorsCBClick(System::TObject* Sender);
	void __fastcall StylesCBClick(System::TObject* Sender);
	void __fastcall FormCloseQuery(System::TObject* Sender, bool &CanClose);
	void __fastcall OKBtnClick(System::TObject* Sender);
	void __fastcall FormCreate(System::TObject* Sender);
	void __fastcall CancelBtnClick(System::TObject* Sender);
	void __fastcall ShowChecksCBClick(System::TObject* Sender);
	void __fastcall CBTypeComboChange(System::TObject* Sender);
	void __fastcall StrikeOutCBClick(System::TObject* Sender);
	void __fastcall OwnHeightCBClick(System::TObject* Sender);
	void __fastcall IndentAdjustCBClick(System::TObject* Sender);
	
public:
	Eltree::TElTreeItem* Item;
	bool ByCancel;
	void __fastcall SetData(void);
	void __fastcall GetData(void);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TItemColDlg(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TItemColDlg(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TItemColDlg(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TItemColDlg(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TItemColDlg* ItemColDlg;

}	/* namespace Frmitemcol */
using namespace Frmitemcol;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmItemCol
