// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElStack.pas' rev: 6.00

#ifndef ElStackHPP
#define ElStackHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elstack
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS EElStackError;
class PASCALIMPLEMENTATION EElStackError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElStackError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElStackError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElStackError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElStackError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElStackError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElStackError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElStackError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElStackError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElStackError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElStack;
class PASCALIMPLEMENTATION TElStack : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	void * operator[](int Index) { return Items[Index]; }
	
private:
	void * *FList;
	int FCount;
	int FCapacity;
	void * __fastcall Get(int index);
	void __fastcall Put(int index, void * value);
	void __fastcall Grow(void);
	void __fastcall SetCapacity(int NewCapacity);
	
protected:
	void * __fastcall GetLast(void);
	
public:
	__fastcall virtual ~TElStack(void);
	void __fastcall Clear(void);
	void __fastcall Push(void * value);
	void * __fastcall Pop(void);
	bool __fastcall Empty(void);
	__property int Capacity = {read=FCapacity, write=SetCapacity, nodefault};
	__property void * Items[int Index] = {read=Get, write=Put/*, default*/};
	__property int Count = {read=FCount, nodefault};
	__property void * Last = {read=GetLast};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElStack(void) : System::TObject() { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elstack */
using namespace Elstack;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElStack
