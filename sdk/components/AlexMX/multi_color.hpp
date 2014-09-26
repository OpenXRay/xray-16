// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'multi_color.pas' rev: 6.00

#ifndef multi_colorHPP
#define multi_colorHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <CommCtrl.hpp>	// Pascal unit
#include <ComCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Multi_color
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TMultiObjColor;
class PASCALIMPLEMENTATION TMultiObjColor : public Extctrls::TShape 
{
	typedef Extctrls::TShape inherited;
	
private:
	int m_BeforeDialog;
	int m_AfterDialog;
	bool m_Diffs;
	bool m_Changed;
	
public:
	__fastcall virtual TMultiObjColor(Classes::TComponent* AOwner);
	__fastcall virtual ~TMultiObjColor(void);
	void __fastcall ObjFirstInit(int value);
	void __fastcall ObjNextInit(int value);
	bool __fastcall ObjApply(int &_to);
	int __fastcall Get(void);
	void __fastcall _Set(int value);
	bool __fastcall diffs(void);
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Multi_color */
using namespace Multi_color;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// multi_color
