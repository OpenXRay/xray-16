// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmStrPoolEdit.pas' rev: 6.00

#ifndef frmStrPoolEditHPP
#define frmStrPoolEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElEdits.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ElFrmPers.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElStrPool.hpp>	// Pascal unit
#include <DsnConst.hpp>	// Pascal unit
#include <DesignWindows.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
#include <ElStrArray.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElSplit.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
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

namespace Frmstrpooledit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TStrPoolEditForm;
class PASCALIMPLEMENTATION TStrPoolEditForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Elpanel::TElPanel* ElPanel1;
	Elpanel::TElPanel* ElPanel2;
	Elpanel::TElPanel* ElPanel3;
	Elsplit::TElSplitter* ElSplitter1;
	Elpanel::TElPanel* ElPanel4;
	Eltree::TElTree* List;
	Elpopbtn::TElPopupButton* OkBtn;
	Elpopbtn::TElPopupButton* CancelBtn;
	Elpopbtn::TElPopupButton* AddBtn;
	Elpopbtn::TElPopupButton* InsertBtn;
	Elpopbtn::TElPopupButton* DeleteBtn;
	Menus::TPopupMenu* PopupMenu;
	Menus::TMenuItem* AddItem;
	Menus::TMenuItem* InsertItem;
	Menus::TMenuItem* DeleteItem;
	Menus::TMainMenu* MainMenu;
	Menus::TMenuItem* Pool1;
	Menus::TMenuItem* Clear1;
	Menus::TMenuItem* Open1;
	Menus::TMenuItem* Save1;
	Menus::TMenuItem* Text1;
	Menus::TMenuItem* Open2;
	Menus::TMenuItem* Save2;
	Elfrmpers::TElFormPersist* ElFormPersist1;
	Elini::TElIniFile* ElIniFile1;
	Elpopbtn::TElPopupButton* UpBtn;
	Elpopbtn::TElPopupButton* DownBtn;
	Elpopbtn::TElPopupButton* CopyBtn;
	Eledits::TElEdit* Memo;
	void __fastcall ListItemFocused(System::TObject* Sender);
	void __fastcall AddBtnClick(System::TObject* Sender);
	void __fastcall InsertBtnClick(System::TObject* Sender);
	void __fastcall DeleteBtnClick(System::TObject* Sender);
	void __fastcall ListItemDeletion(System::TObject* Sender, Eltree::TElTreeItem* Item);
	void __fastcall FormCreate(System::TObject* Sender);
	void __fastcall FormDestroy(System::TObject* Sender);
	void __fastcall ListVirtualTextNeeded(System::TObject* Sender, Eltree::TElTreeItem* Item, int SectionIndex, WideString &Text);
	void __fastcall ListVirtualHintNeeded(System::TObject* Sender, Eltree::TElTreeItem* Item, WideString &Hint);
	void __fastcall UpBtnClick(System::TObject* Sender);
	void __fastcall DownBtnClick(System::TObject* Sender);
	void __fastcall CopyBtnClick(System::TObject* Sender);
	void __fastcall OkBtnClick(System::TObject* Sender);
	
private:
	int CurIndex;
	Elunicodestrings::TElWideStringArray* StrArray;
	
public:
	void __fastcall GetData(Elunicodestrings::TElWideStringArray* anArray);
	void __fastcall SetData(Elunicodestrings::TElWideStringArray* anArray);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TStrPoolEditForm(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TStrPoolEditForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TStrPoolEditForm(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TStrPoolEditForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TStrPoolItemsProperty;
class PASCALIMPLEMENTATION TStrPoolItemsProperty : public Designeditors::TPropertyEditor 
{
	typedef Designeditors::TPropertyEditor inherited;
	
public:
	virtual void __fastcall Edit(void);
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
	virtual AnsiString __fastcall GetValue();
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TStrPoolItemsProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TPropertyEditor(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TStrPoolItemsProperty(void) { }
	#pragma option pop
	
};


class DELPHICLASS TStrPoolItemsEditor;
class PASCALIMPLEMENTATION TStrPoolItemsEditor : public Designeditors::TComponentEditor 
{
	typedef Designeditors::TComponentEditor inherited;
	
public:
	virtual void __fastcall ExecuteVerb(int Index);
	virtual AnsiString __fastcall GetVerb(int Index);
	virtual int __fastcall GetVerbCount(void);
public:
	#pragma option push -w-inl
	/* TComponentEditor.Create */ inline __fastcall virtual TStrPoolItemsEditor(Classes::TComponent* AComponent, Designintf::_di_IDesigner ADesigner) : Designeditors::TComponentEditor(AComponent, ADesigner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TStrPoolItemsEditor(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TStrPoolEditForm* StrPoolEditForm;

}	/* namespace Frmstrpooledit */
using namespace Frmstrpooledit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmStrPoolEdit
