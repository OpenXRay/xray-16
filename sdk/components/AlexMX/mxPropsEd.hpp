// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxPropsEd.pas' rev: 6.00

#ifndef mxPropsEdHPP
#define mxPropsEdHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <MXProps.hpp>	// Pascal unit
#include <MXCtrls.hpp>	// Pascal unit
#include <mxVCLUtils.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
#include <mxPlacemnt.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxpropsed
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TFormPropsDlg;
class PASCALIMPLEMENTATION TFormPropsDlg : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Extctrls::TBevel* Bevel1;
	Stdctrls::TLabel* Label30;
	Stdctrls::TLabel* Label31;
	Stdctrls::TLabel* Label2;
	Buttons::TSpeedButton* UpBtn;
	Buttons::TSpeedButton* DownBtn;
	Mxctrls::TTextListBox* StoredList;
	Mxctrls::TTextListBox* PropertiesList;
	Mxctrls::TTextListBox* ComponentsList;
	Stdctrls::TGroupBox* FormBox;
	Stdctrls::TCheckBox* ActiveCtrlBox;
	Stdctrls::TCheckBox* PositionBox;
	Stdctrls::TCheckBox* StateBox;
	Stdctrls::TButton* AddButton;
	Stdctrls::TButton* DeleteButton;
	Stdctrls::TButton* ClearButton;
	Stdctrls::TButton* OkBtn;
	Stdctrls::TButton* CancelBtn;
	void __fastcall AddButtonClick(System::TObject* Sender);
	void __fastcall ClearButtonClick(System::TObject* Sender);
	void __fastcall ListClick(System::TObject* Sender);
	void __fastcall FormDestroy(System::TObject* Sender);
	void __fastcall DeleteButtonClick(System::TObject* Sender);
	void __fastcall StoredListClick(System::TObject* Sender);
	void __fastcall UpBtnClick(System::TObject* Sender);
	void __fastcall DownBtnClick(System::TObject* Sender);
	void __fastcall StoredListDragOver(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	void __fastcall StoredListDragDrop(System::TObject* Sender, System::TObject* Source, int X, int Y);
	void __fastcall PropertiesListDblClick(System::TObject* Sender);
	
private:
	Classes::TComponent* FCompOwner;
	Designintf::_di_IDesigner FDesigner;
	void __fastcall ListToIndex(Stdctrls::TCustomListBox* List, int Idx);
	void __fastcall UpdateCurrent(void);
	void __fastcall DeleteProp(int I);
	bool __fastcall FindProp(const AnsiString CompName, const AnsiString PropName, int &IdxComp, int &IdxProp);
	void __fastcall ClearLists(void);
	void __fastcall CheckAddItem(const AnsiString CompName, const AnsiString PropName);
	void __fastcall AddItem(int IdxComp, int IdxProp, bool AUpdate);
	void __fastcall BuildLists(Classes::TStrings* StoredProps);
	void __fastcall CheckButtons(void);
	void __fastcall SetStoredList(Classes::TStrings* AList);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TFormPropsDlg(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TFormPropsDlg(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TFormPropsDlg(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TFormPropsDlg(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TFormStorageEditor;
class PASCALIMPLEMENTATION TFormStorageEditor : public Designeditors::TComponentEditor 
{
	typedef Designeditors::TComponentEditor inherited;
	
public:
	virtual void __fastcall ExecuteVerb(int Index);
	virtual AnsiString __fastcall GetVerb(int Index);
	virtual int __fastcall GetVerbCount(void);
public:
	#pragma option push -w-inl
	/* TComponentEditor.Create */ inline __fastcall virtual TFormStorageEditor(Classes::TComponent* AComponent, Designintf::_di_IDesigner ADesigner) : Designeditors::TComponentEditor(AComponent, ADesigner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFormStorageEditor(void) { }
	#pragma option pop
	
};


class DELPHICLASS TStoredPropsProperty;
class PASCALIMPLEMENTATION TStoredPropsProperty : public Designeditors::TClassProperty 
{
	typedef Designeditors::TClassProperty inherited;
	
public:
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
	virtual AnsiString __fastcall GetValue();
	virtual void __fastcall Edit(void);
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TStoredPropsProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TClassProperty(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TStoredPropsProperty(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool __fastcall ShowStorageDesigner(Classes::TComponent* ACompOwner, Designintf::_di_IDesigner ADesigner, Classes::TStrings* AStoredList, Mxplacemnt::TPlacementOptions &Options);

}	/* namespace Mxpropsed */
using namespace Mxpropsed;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxPropsEd
