// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'frmFormPers.pas' rev: 6.00

#ifndef frmFormPersHPP
#define frmFormPersHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ElMTree.hpp>	// Pascal unit
#include <ElFrmPers.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
#include <DsnConst.hpp>	// Pascal unit
#include <DesignWindows.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <ElImgLst.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Frmformpers
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TPersPropsForm;
class PASCALIMPLEMENTATION TPersPropsForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Extctrls::TPanel* BtnPanel;
	Elpopbtn::TElPopupButton* OkBtn;
	Elpopbtn::TElPopupButton* CancelBtn;
	Elimglst::TElImageList* ImageList;
	Eltree::TElTree* Tree;
	Elfrmpers::TElFormPersist* ElFormPersist1;
	Elini::TElIniFile* ElIniFile1;
	void __fastcall TreeItemChange(System::TObject* Sender, Eltree::TElTreeItem* Item, Eltree::TItemChangeMode ItemChangeMode);
	void __fastcall TreeHeaderSectionCollapse(System::TObject* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall TreeHeaderSectionExpand(System::TObject* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall ElFormPersist1Restore(System::TObject* Sender);
	void __fastcall ElFormPersist1Save(System::TObject* Sender);
	
protected:
	int FOldWidth;
	
public:
	Elmtree::TElMTree* Props;
	Elfrmpers::TElFormPersist* FPers;
	void __fastcall GetData(void);
	void __fastcall SetData(void);
	__fastcall virtual TPersPropsForm(Classes::TComponent* AOwner);
public:
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TPersPropsForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TPersPropsForm(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TPersPropsForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TPropListProperty;
class PASCALIMPLEMENTATION TPropListProperty : public Designeditors::TPropertyEditor 
{
	typedef Designeditors::TPropertyEditor inherited;
	
public:
	virtual void __fastcall Edit(void);
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
	virtual AnsiString __fastcall GetValue();
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TPropListProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TPropertyEditor(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TPropListProperty(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElPropListEditor;
class PASCALIMPLEMENTATION TElPropListEditor : public Designeditors::TComponentEditor 
{
	typedef Designeditors::TComponentEditor inherited;
	
public:
	virtual void __fastcall ExecuteVerb(int Index);
	virtual AnsiString __fastcall GetVerb(int Index);
	virtual int __fastcall GetVerbCount(void);
public:
	#pragma option push -w-inl
	/* TComponentEditor.Create */ inline __fastcall virtual TElPropListEditor(Classes::TComponent* AComponent, Designintf::_di_IDesigner ADesigner) : Designeditors::TComponentEditor(AComponent, ADesigner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElPropListEditor(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TPersPropsForm* PersPropsForm;

}	/* namespace Frmformpers */
using namespace Frmformpers;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// frmFormPers
