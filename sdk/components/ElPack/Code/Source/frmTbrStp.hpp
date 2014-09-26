// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmTbrStp.pas' rev: 6.00

#ifndef frmTbrStpHPP
#define frmTbrStpHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElToolbar.hpp>	// Pascal unit
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

namespace Frmtbrstp
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TfrmToolbarSetup;
class PASCALIMPLEMENTATION TfrmToolbarSetup : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Extctrls::TPanel* pnlSections;
	Elactrls::TElAdvancedListBox* lbxAvailable;
	Elactrls::TElAdvancedListBox* lbxVisible;
	Stdctrls::TLabel* lblAvailable;
	Stdctrls::TLabel* lblVisible;
	Elpopbtn::TElPopupButton* btnAdd;
	Elpopbtn::TElPopupButton* btnDelete;
	Elpopbtn::TElPopupButton* btnUp;
	Elpopbtn::TElPopupButton* btnDown;
	Elpopbtn::TElPopupButton* btnOk;
	Elpopbtn::TElPopupButton* btnCancel;
	Stdctrls::TLabel* TextOptionsLabel;
	Elactrls::TElAdvancedComboBox* TextOptionsCombo;
	Stdctrls::TLabel* IconOptionsLabel;
	Elactrls::TElAdvancedComboBox* IconOptionsCombo;
	void __fastcall FormShow(System::TObject* Sender);
	void __fastcall lbxVisibleEnter(System::TObject* Sender);
	void __fastcall lbxAvailableEnter(System::TObject* Sender);
	void __fastcall btnAddClick(System::TObject* Sender);
	void __fastcall btnDeleteClick(System::TObject* Sender);
	void __fastcall btnUpClick(System::TObject* Sender);
	void __fastcall btnDownClick(System::TObject* Sender);
	void __fastcall lbxVisibleMouseDown(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall lbxVisibleDragOver(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	void __fastcall lbxVisibleDragDrop(System::TObject* Sender, System::TObject* Source, int X, int Y);
	void __fastcall lbxAvailableDragOver(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	void __fastcall lbxAvailableMouseDown(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall lbxAvailableDragDrop(System::TObject* Sender, System::TObject* Source, int X, int Y);
	
private:
	void __fastcall UpdateButtons(void);
	
public:
	void __fastcall LoadToolbarControls(Eltoolbar::TElToolBar* Toolbar);
	void __fastcall SaveToolbarControls(Eltoolbar::TElToolBar* Toolbar);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TfrmToolbarSetup(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TfrmToolbarSetup(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TfrmToolbarSetup(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TfrmToolbarSetup(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Frmtbrstp */
using namespace Frmtbrstp;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmTbrStp
