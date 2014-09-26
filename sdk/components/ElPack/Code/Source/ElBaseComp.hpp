// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElBaseComp.pas' rev: 6.00

#ifndef ElBaseCompHPP
#define ElBaseCompHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elbasecomp
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS EBaseEnabledFailed;
class PASCALIMPLEMENTATION EBaseEnabledFailed : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EBaseEnabledFailed(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EBaseEnabledFailed(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EBaseEnabledFailed(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EBaseEnabledFailed(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EBaseEnabledFailed(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EBaseEnabledFailed(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EBaseEnabledFailed(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EBaseEnabledFailed(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EBaseEnabledFailed(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElBaseComponent;
class PASCALIMPLEMENTATION TElBaseComponent : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
protected:
	unsigned FHandle;
	bool FEnabled;
	bool FDesignActive;
	MESSAGE void __fastcall WMQueryEndSession(Messages::TMessage &Message);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	virtual void __fastcall SetEnabled(bool AEnabled);
	virtual void __fastcall DoSetEnabled(bool AEnabled);
	virtual void __fastcall Loaded(void);
	__property unsigned Handle = {read=FHandle, nodefault};
	
public:
	__property bool Enabled = {read=FEnabled, write=SetEnabled, default=0};
	__fastcall virtual TElBaseComponent(Classes::TComponent* AOwner);
	__fastcall virtual ~TElBaseComponent(void);
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elbasecomp */
using namespace Elbasecomp;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElBaseComp
