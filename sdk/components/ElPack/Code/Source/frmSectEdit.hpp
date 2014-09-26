// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmSectEdit.pas' rev: 6.00

#ifndef frmSectEditHPP
#define frmSectEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
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

namespace Frmsectedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TSectEdit;
class PASCALIMPLEMENTATION TSectEdit : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Stdctrls::TLabel* Label6;
	Stdctrls::TEdit* ImIndexEdit;
	Stdctrls::TGroupBox* GroupBox1;
	Stdctrls::TLabel* Label1;
	Stdctrls::TEdit* TextEB;
	Stdctrls::TLabel* Label5;
	Stdctrls::TEdit* FieldEdit;
	Stdctrls::TLabel* Label11;
	Stdctrls::TEdit* HintEdit;
	Stdctrls::TLabel* Label8;
	Stdctrls::TGroupBox* GroupBox2;
	Stdctrls::TLabel* Label2;
	Stdctrls::TLabel* Label3;
	Stdctrls::TEdit* WidthEB;
	Stdctrls::TEdit* MinWidthEB;
	Stdctrls::TEdit* MaxWidthEB;
	Stdctrls::TLabel* Label7;
	Stdctrls::TLabel* Label9;
	Stdctrls::TLabel* Label10;
	Stdctrls::TGroupBox* GroupBox3;
	Extctrls::TPanel* Panel1;
	Stdctrls::TLabel* Label4;
	Extctrls::TBevel* Bevel1;
	Stdctrls::TComboBox* StyleCombo;
	Stdctrls::TComboBox* ColTypeCB;
	Stdctrls::TComboBox* PopupCombo;
	Stdctrls::TComboBox* ParentCombo;
	Stdctrls::TCheckBox* ExpandableCB;
	Stdctrls::TCheckBox* ExpandedCB;
	Stdctrls::TCheckBox* FilterCB;
	Stdctrls::TCheckBox* LookupCB;
	Stdctrls::TCheckBox* ClickCB;
	Stdctrls::TCheckBox* ClickSelCB;
	Stdctrls::TCheckBox* ResizeCB;
	Stdctrls::TCheckBox* PswCB;
	Stdctrls::TCheckBox* EditCB;
	Extctrls::TRadioGroup* AlignRG;
	Extctrls::TRadioGroup* LayoutRG;
	Extctrls::TRadioGroup* ImAlignRG;
	Extctrls::TRadioGroup* SortRG;
	Stdctrls::TButton* ElPopupButton1;
	Stdctrls::TButton* ElPopupButton2;
	Stdctrls::TCheckBox* VisCB;
	Stdctrls::TCheckBox* AutosizeCB;
	void __fastcall ExpandableCBClick(System::TObject* Sender);
	void __fastcall FilterCBClick(System::TObject* Sender);
	void __fastcall LookupCBClick(System::TObject* Sender);
	
public:
	Elheader::TElHeaderSection* Item;
	Elheader::TElHeaderSections* Items;
	Forms::TCustomForm* Form;
	void __fastcall SetData(void);
	void __fastcall GetData(void);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TSectEdit(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TSectEdit(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TSectEdit(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TSectEdit(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TSectEdit* SectEdit;

}	/* namespace Frmsectedit */
using namespace Frmsectedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmSectEdit
