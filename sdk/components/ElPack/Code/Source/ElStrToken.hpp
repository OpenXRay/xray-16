// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElStrToken.pas' rev: 6.00

#ifndef ElStrTokenHPP
#define ElStrTokenHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elstrtoken
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS EElStrTokenizerError;
class PASCALIMPLEMENTATION EElStrTokenizerError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElStrTokenizerError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElStrTokenizerError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElStrTokenizerError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElStrTokenizerError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElStrTokenizerError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElStrTokenizerError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElStrTokenizerError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElStrTokenizerError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElStrTokenizerError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElStringTokenizer;
class PASCALIMPLEMENTATION TElStringTokenizer : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	int FPos;
	AnsiString FSourceString;
	bool FReturnTokens;
	AnsiString FDelimiters;
	bool FLastWasToken;
	void __fastcall SetSourceString(AnsiString newValue);
	bool __fastcall IntHasMoreTokens(void);
	bool __fastcall IntNextToken(AnsiString &AResult);
	
public:
	__fastcall TElStringTokenizer(void);
	__fastcall TElStringTokenizer(AnsiString str);
	__fastcall TElStringTokenizer(AnsiString str, AnsiString Delim);
	__fastcall TElStringTokenizer(AnsiString str, AnsiString Delim, bool ReturnTokens);
	bool __fastcall HasMoreTokens(void);
	AnsiString __fastcall NextToken();
	int __fastcall CountTokens(void);
	AnsiString __fastcall NextTokenDelim(AnsiString Delims);
	void __fastcall FindAll(Classes::TStrings* AStrings);
	
__published:
	__property AnsiString SourceString = {read=FSourceString, write=SetSourceString};
	__property bool ReturnTokens = {read=FReturnTokens, write=FReturnTokens, nodefault};
	__property AnsiString Delimiters = {read=FDelimiters, write=FDelimiters};
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElStringTokenizer(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elstrtoken */
using namespace Elstrtoken;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElStrToken
