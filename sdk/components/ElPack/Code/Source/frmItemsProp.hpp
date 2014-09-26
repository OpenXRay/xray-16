// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmItemsProp.pas' rev: 6.00

#ifndef frmItemsPropHPP
#define frmItemsPropHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTreeCombo.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <frmItemCol.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <DsnConst.hpp>	// Pascal unit
#include <DesignWindows.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
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

namespace Frmitemsprop
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TItemsPropDlg;
class PASCALIMPLEMENTATION TItemsPropDlg : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Stdctrls::TGroupBox* ItemsGB;
	Dialogs::TOpenDialog* OpenDlg;
	Dialogs::TSaveDialog* SaveDlg;
	Eltree::TElTree* Tree;
	Extctrls::TPanel* Panel1;
	Stdctrls::TButton* OKBtn;
	Stdctrls::TButton* CancelBtn;
	Stdctrls::TButton* ApplyBtn;
	Extctrls::TPanel* Panel2;
	Stdctrls::TButton* NewItemBtn;
	Stdctrls::TButton* SubitemBtn;
	Stdctrls::TButton* DeleteBtn;
	Stdctrls::TButton* SaveBtn;
	Stdctrls::TButton* LoadBtn;
	Stdctrls::TButton* EditBtn;
	Stdctrls::TButton* MoveRightBtn;
	Stdctrls::TButton* MoveLeftBtn;
	Stdctrls::TButton* MoveDownBtn;
	Stdctrls::TButton* MoveUpBtn;
	Stdctrls::TButton* DuplicateBtn;
	void __fastcall DeleteBtnClick(System::TObject* Sender);
	void __fastcall SubitemBtnClick(System::TObject* Sender);
	void __fastcall TreeItemFocused(System::TObject* Sender);
	void __fastcall NewItemBtnClick(System::TObject* Sender);
	void __fastcall EditBtnClick(System::TObject* Sender);
	void __fastcall OKBtnClick(System::TObject* Sender);
	void __fastcall SaveBtnClick(System::TObject* Sender);
	void __fastcall LoadBtnClick(System::TObject* Sender);
	void __fastcall TreeStartDrag(System::TObject* Sender, Controls::TDragObject* &DragObject);
	void __fastcall TreeDragOver(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	void __fastcall TreeDragDrop(System::TObject* Sender, System::TObject* Source, int X, int Y);
	void __fastcall FormCreate(System::TObject* Sender);
	void __fastcall TreeKeyDown(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall MoveRightBtnClick(System::TObject* Sender);
	void __fastcall MoveLeftBtnClick(System::TObject* Sender);
	void __fastcall MoveUpBtnClick(System::TObject* Sender);
	void __fastcall MoveDownBtnClick(System::TObject* Sender);
	void __fastcall TreeDblClick(System::TObject* Sender);
	void __fastcall DuplicateBtnClick(System::TObject* Sender);
	
private:
	Eltree::TElTreeItem* FDragItem;
	
public:
	Classes::TComponent* AComp;
	Eltree::TElTreeItems* DTreeItems;
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TItemsPropDlg(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TItemsPropDlg(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TItemsPropDlg(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TItemsPropDlg(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreeItemsProperty;
class PASCALIMPLEMENTATION TElTreeItemsProperty : public Designeditors::TClassProperty 
{
	typedef Designeditors::TClassProperty inherited;
	
public:
	virtual void __fastcall Edit(void);
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
	virtual AnsiString __fastcall GetValue();
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TElTreeItemsProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TClassProperty(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TElTreeItemsProperty(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TItemsPropDlg* ItemsPropDlg;

}	/* namespace Frmitemsprop */
using namespace Frmitemsprop;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmItemsProp
