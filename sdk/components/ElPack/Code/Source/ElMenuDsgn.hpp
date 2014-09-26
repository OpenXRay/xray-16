// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElMenuDsgn.pas' rev: 6.00

#ifndef ElMenuDsgnHPP
#define ElMenuDsgnHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <DesignIntf.hpp>	// Pascal unit
#include <DesignConst.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElMenus.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
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

namespace Elmenudsgn
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElDesignMenu;
class PASCALIMPLEMENTATION TElDesignMenu : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Stdctrls::TGroupBox* GroupBox1;
	Eltree::TElTree* MenuEdit;
	Extctrls::TPanel* Panel2;
	Stdctrls::TButton* NewSubItemBtn;
	Stdctrls::TButton* NewItemBtn;
	Stdctrls::TButton* DeleteItemBtn;
	Stdctrls::TButton* Load;
	Stdctrls::TButton* Save;
	Extctrls::TBevel* Bevel1;
	Dialogs::TOpenDialog* OpenMenuDlg;
	Dialogs::TSaveDialog* SaveMenuDlg;
	Extctrls::TBevel* Bevel2;
	Stdctrls::TButton* MoveUp;
	Stdctrls::TButton* MoveDown;
	Stdctrls::TButton* LevelUp;
	Stdctrls::TButton* LevelDown;
	void __fastcall NewItemBtnClick(System::TObject* Sender);
	void __fastcall NewSubItemBtnClick(System::TObject* Sender);
	void __fastcall DeleteItemBtnClick(System::TObject* Sender);
	void __fastcall FormClose(System::TObject* Sender, Forms::TCloseAction &Action);
	void __fastcall MenuEditItemFocused(System::TObject* Sender);
	void __fastcall SaveClick(System::TObject* Sender);
	void __fastcall MoveUpClick(System::TObject* Sender);
	void __fastcall MoveDownClick(System::TObject* Sender);
	void __fastcall LevelUpClick(System::TObject* Sender);
	void __fastcall LevelDownClick(System::TObject* Sender);
	void __fastcall LoadClick(System::TObject* Sender);
	void __fastcall MenuEditStartDrag(System::TObject* Sender, Controls::TDragObject* &DragObject);
	void __fastcall MenuEditDragOver(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	void __fastcall MenuEditDragDrop(System::TObject* Sender, System::TObject* Source, int X, int Y);
	
private:
	Menus::TMenu* FMenu;
	Eltree::TElTreeItem* FDragItem;
	Elmenus::TElMenuItem* FElMenuItem;
	void __fastcall SetElMenu(const Menus::TMenu* Value);
	void __fastcall MenuChanged(System::TObject* Sender, Elmenus::TElMenuItem* Source, bool Rebuild);
	
protected:
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	Classes::TComponent* AComp;
	Designintf::_di_IDesigner ADesigner;
	__fastcall virtual ~TElDesignMenu(void);
	__property Menus::TMenu* Menu = {read=FMenu, write=SetElMenu};
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TElDesignMenu(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TElDesignMenu(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDesignMenu(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElMenuEditor;
class PASCALIMPLEMENTATION TElMenuEditor : public Designeditors::TComponentEditor 
{
	typedef Designeditors::TComponentEditor inherited;
	
public:
	virtual void __fastcall ExecuteVerb(int Index);
	virtual AnsiString __fastcall GetVerb(int Index);
	virtual int __fastcall GetVerbCount(void);
public:
	#pragma option push -w-inl
	/* TComponentEditor.Create */ inline __fastcall virtual TElMenuEditor(Classes::TComponent* AComponent, Designintf::_di_IDesigner ADesigner) : Designeditors::TComponentEditor(AComponent, ADesigner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElMenuEditor(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElMenuItemsProperty;
class PASCALIMPLEMENTATION TElMenuItemsProperty : public Designeditors::TClassProperty 
{
	typedef Designeditors::TClassProperty inherited;
	
public:
	virtual void __fastcall Edit(void);
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
	virtual AnsiString __fastcall GetValue();
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TElMenuItemsProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TClassProperty(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TElMenuItemsProperty(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TElDesignMenu* ElDesignMenu;

}	/* namespace Elmenudsgn */
using namespace Elmenudsgn;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElMenuDsgn
