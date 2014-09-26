// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmSoundMap.pas' rev: 6.00

#ifndef frmSoundMapHPP
#define frmSoundMapHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTreeModalEdit.hpp>	// Pascal unit
#include <ElTreeAdvEdit.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <DsnConst.hpp>	// Pascal unit
#include <DesignWindows.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
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

namespace Frmsoundmap
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TSoundMapForm;
class PASCALIMPLEMENTATION TSoundMapForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Elpopbtn::TElPopupButton* OkBtn;
	Eltree::TElTree* List;
	Elpopbtn::TElPopupButton* AddBtn;
	Elpopbtn::TElPopupButton* RemoveBtn;
	Menus::TPopupMenu* PopupMenu;
	Menus::TMenuItem* AddItem;
	Menus::TMenuItem* RemoveItem;
	Elpopbtn::TElPopupButton* CancelBtn;
	Dialogs::TOpenDialog* SoundDialog;
	Menus::TMenuItem* PlayItem;
	void __fastcall AddItemClick(System::TObject* Sender);
	void __fastcall RemoveItemClick(System::TObject* Sender);
	void __fastcall ListItemFocused(System::TObject* Sender);
	void __fastcall ListEditRequest(System::TObject* Sender, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section);
	void __fastcall PlayItemClick(System::TObject* Sender);
	void __fastcall FormCreate(System::TObject* Sender);
	
private:
	Elsndmap::TElSoundMap* FMap;
	Eltreeadvedit::TElTreeInplaceAdvancedEdit* AdvEditor;
	Eltreemodaledit::TElTreeInplaceModalEdit* AnEditor;
	void __fastcall ModalEditorExecute(System::TObject* Sender, bool &Accepted);
	
public:
	void __fastcall SetData(Elsndmap::TElSoundMap* AMap);
	void __fastcall GetData(Elsndmap::TElSoundMap* AMap);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TSoundMapForm(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TSoundMapForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TSoundMapForm(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TSoundMapForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TSoundNameProperty;
class PASCALIMPLEMENTATION TSoundNameProperty : public Designeditors::TPropertyEditor 
{
	typedef Designeditors::TPropertyEditor inherited;
	
public:
	virtual void __fastcall GetValues(Classes::TGetStrProc Proc);
	virtual AnsiString __fastcall GetValue();
	virtual void __fastcall SetValue(const AnsiString Value);
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TSoundNameProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TPropertyEditor(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TSoundNameProperty(void) { }
	#pragma option pop
	
};


class DELPHICLASS TSoundMapEditor;
class PASCALIMPLEMENTATION TSoundMapEditor : public Designeditors::TComponentEditor 
{
	typedef Designeditors::TComponentEditor inherited;
	
public:
	virtual void __fastcall ExecuteVerb(int Index);
	virtual AnsiString __fastcall GetVerb(int Index);
	virtual int __fastcall GetVerbCount(void);
public:
	#pragma option push -w-inl
	/* TComponentEditor.Create */ inline __fastcall virtual TSoundMapEditor(Classes::TComponent* AComponent, Designintf::_di_IDesigner ADesigner) : Designeditors::TComponentEditor(AComponent, ADesigner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TSoundMapEditor(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TSoundMapForm* SoundMapForm;

}	/* namespace Frmsoundmap */
using namespace Frmsoundmap;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmSoundMap
