// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'PgCtlProp.pas' rev: 6.00

#ifndef PgCtlPropHPP
#define PgCtlPropHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <DsnConst.hpp>	// Pascal unit
#include <DesignWindows.hpp>	// Pascal unit
#include <DesignEditors.hpp>	// Pascal unit
#include <DesignMenus.hpp>	// Pascal unit
#include <DesignIntf.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElPgCtl.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Pgctlprop
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElPageControlEditor;
class PASCALIMPLEMENTATION TElPageControlEditor : public Designeditors::TComponentEditor 
{
	typedef Designeditors::TComponentEditor inherited;
	
public:
	virtual void __fastcall ExecuteVerb(int Index);
	virtual AnsiString __fastcall GetVerb(int Index);
	virtual int __fastcall GetVerbCount(void);
	
private:
	Elpgctl::TElPageControl* __fastcall GetPageControl(void);
	
public:
	virtual void __fastcall PrepareItem(int Index, const Designmenus::_di_IMenuItem AItem);
public:
	#pragma option push -w-inl
	/* TComponentEditor.Create */ inline __fastcall virtual TElPageControlEditor(Classes::TComponent* AComponent, Designintf::_di_IDesigner ADesigner) : Designeditors::TComponentEditor(AComponent, ADesigner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElPageControlEditor(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElActivePageProperty;
class PASCALIMPLEMENTATION TElActivePageProperty : public Designeditors::TComponentProperty 
{
	typedef Designeditors::TComponentProperty inherited;
	
public:
	virtual Designintf::TPropertyAttributes __fastcall GetAttributes(void);
	virtual void __fastcall GetValues(Classes::TGetStrProc Proc);
public:
	#pragma option push -w-inl
	/* TPropertyEditor.Create */ inline __fastcall virtual TElActivePageProperty(const Designintf::_di_IDesigner ADesigner, int APropCount) : Designeditors::TComponentProperty(ADesigner, APropCount) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TPropertyEditor.Destroy */ inline __fastcall virtual ~TElActivePageProperty(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Pgctlprop */
using namespace Pgctlprop;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// PgCtlProp
