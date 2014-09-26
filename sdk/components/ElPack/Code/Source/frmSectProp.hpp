// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmSectProp.pas' rev: 6.00

#ifndef frmSectPropHPP
#define frmSectPropHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <frmSectEdit.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <DsnConst.hpp>	// Pascal unit
#include <DesignWindows.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
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

namespace Frmsectprop
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElSectionsPropDlg;
class PASCALIMPLEMENTATION TElSectionsPropDlg : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Dialogs::TOpenDialog* OpenDlg;
	Dialogs::TSaveDialog* SaveDlg;
	Extctrls::TPanel* Panel3;
	Stdctrls::TButton* Button1;
	Stdctrls::TButton* Button2;
	Stdctrls::TButton* Button3;
	Stdctrls::TGroupBox* GroupBox1;
	Extctrls::TPanel* Panel1;
	Stdctrls::TListBox* SecList;
	Extctrls::TPanel* Panel2;
	Stdctrls::TButton* AddBtn;
	Stdctrls::TButton* DeleteBtn;
	Stdctrls::TButton* EditBtn;
	Stdctrls::TButton* UpBtn;
	Stdctrls::TButton* DownBtn;
	Stdctrls::TButton* LoadBtn;
	Stdctrls::TButton* SaveBtn;
	Elheader::TElHeader* TestHeader;
	Stdctrls::TButton* DuplicateBtn;
	void __fastcall LoadBtnClick(System::TObject* Sender);
	void __fastcall SaveBtnClick(System::TObject* Sender);
	void __fastcall EditBtnClick(System::TObject* Sender);
	void __fastcall AddBtnClick(System::TObject* Sender);
	void __fastcall DeleteBtnClick(System::TObject* Sender);
	void __fastcall UpBtnClick(System::TObject* Sender);
	void __fastcall DownBtnClick(System::TObject* Sender);
	void __fastcall SecListClick(System::TObject* Sender);
	void __fastcall FormCreate(System::TObject* Sender);
	void __fastcall SecListKeyPress(System::TObject* Sender, char &Key);
	void __fastcall SecListDblClick(System::TObject* Sender);
	void __fastcall DuplicateBtnClick(System::TObject* Sender);
	void __fastcall Button3Click(System::TObject* Sender);
	
public:
	Elheader::TElHeaderSections* ASect;
	void __fastcall SetData(void);
	void __fastcall GetData(void);
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TElSectionsPropDlg(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TElSectionsPropDlg(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TElSectionsPropDlg(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElSectionsPropDlg(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElSectionsProperty;
class PASCALIMPLEMENTATION TElSectionsProperty : public Designeditors::TClassProperty 
{
	typedef Designeditors::TClassProperty inherited;
	
public:
	virtual void __fastcall Edit(void);
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
	virtual AnsiString __fastcall GetValue();
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TElSectionsProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TClassProperty(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TElSectionsProperty(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElHeaderEditor;
class PASCALIMPLEMENTATION TElHeaderEditor : public Designeditors::TDefaultEditor 
{
	typedef Designeditors::TDefaultEditor inherited;
	
public:
	virtual void __fastcall EditProperty(const Designintf::_di_IProperty Prop, bool &Continue);
	virtual void __fastcall ExecuteVerb(int Index);
	virtual AnsiString __fastcall GetVerb(int Index);
	virtual int __fastcall GetVerbCount(void);
public:
	#pragma option push -w-inl
	/* TComponentEditor.Create */ inline __fastcall virtual TElHeaderEditor(Classes::TComponent* AComponent, Designintf::_di_IDesigner ADesigner) : Designeditors::TDefaultEditor(AComponent, ADesigner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElHeaderEditor(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TElSectionsPropDlg* ElSectionsPropDlg;

}	/* namespace Frmsectprop */
using namespace Frmsectprop;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmSectProp
