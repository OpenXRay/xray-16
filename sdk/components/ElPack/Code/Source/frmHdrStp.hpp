// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmHdrStp.pas' rev: 6.00

#ifndef frmHdrStpHPP
#define frmHdrStpHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
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

namespace Frmhdrstp
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TfrmHeaderSetup;
class PASCALIMPLEMENTATION TfrmHeaderSetup : public Forms::TForm 
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
	void __fastcall LoadHeaderSections(Elheader::TElHeaderSections* ASections);
	void __fastcall SaveHeaderSections(Elheader::TElHeaderSections* ASections);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TfrmHeaderSetup(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TfrmHeaderSetup(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TfrmHeaderSetup(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TfrmHeaderSetup(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Frmhdrstp */
using namespace Frmhdrstp;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmHdrStp
