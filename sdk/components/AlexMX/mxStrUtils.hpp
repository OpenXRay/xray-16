// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxStrUtils.pas' rev: 6.00

#ifndef mxStrUtilsHPP
#define mxStrUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxstrutils
{
//-- type declarations -------------------------------------------------------
typedef TSysCharSet TCharSet;
;

//-- var, const, procedure ---------------------------------------------------
#define CRLF "\r\n"
#define DigitChars (System::Set<char, 0, 255> () << '\x30' << '\x31' << '\x32' << '\x33' << '\x34' << '\x35' << '\x36' << '\x37' << '\x38' << '\x39' )
extern PACKAGE AnsiString __fastcall StrToOem(const AnsiString AnsiStr);
extern PACKAGE AnsiString __fastcall OemToAnsiStr(const AnsiString OemStr);
extern PACKAGE bool __fastcall IsEmptyStr(const AnsiString S, const Sysutils::TSysCharSet &EmptyChars);
extern PACKAGE AnsiString __fastcall ReplaceStr(const AnsiString S, const AnsiString Srch, const AnsiString Replace);
extern PACKAGE AnsiString __fastcall DelSpace(const AnsiString S);
extern PACKAGE AnsiString __fastcall DelChars(const AnsiString S, char Chr);
extern PACKAGE AnsiString __fastcall DelBSpace(const AnsiString S);
extern PACKAGE AnsiString __fastcall DelESpace(const AnsiString S);
extern PACKAGE AnsiString __fastcall DelRSpace(const AnsiString S);
extern PACKAGE AnsiString __fastcall DelSpace1(const AnsiString S);
extern PACKAGE AnsiString __fastcall Tab2Space(const AnsiString S, Byte Numb);
extern PACKAGE AnsiString __fastcall MakeStr(char C, int N);
extern PACKAGE AnsiString __fastcall MS(char C, int N);
extern PACKAGE int __fastcall NPos(const AnsiString C, AnsiString S, int N);
extern PACKAGE AnsiString __fastcall AddChar(char C, const AnsiString S, int N);
extern PACKAGE AnsiString __fastcall AddCharR(char C, const AnsiString S, int N);
extern PACKAGE AnsiString __fastcall LeftStr(const AnsiString S, int N);
extern PACKAGE AnsiString __fastcall RightStr(const AnsiString S, int N);
extern PACKAGE int __fastcall CompStr(const AnsiString S1, const AnsiString S2);
extern PACKAGE int __fastcall CompText(const AnsiString S1, const AnsiString S2);
extern PACKAGE AnsiString __fastcall Copy2Symb(const AnsiString S, char Symb);
extern PACKAGE AnsiString __fastcall Copy2SymbDel(AnsiString &S, char Symb);
extern PACKAGE AnsiString __fastcall Copy2Space(const AnsiString S);
extern PACKAGE AnsiString __fastcall Copy2SpaceDel(AnsiString &S);
extern PACKAGE AnsiString __fastcall AnsiProperCase(const AnsiString S, const Sysutils::TSysCharSet &WordDelims);
extern PACKAGE int __fastcall WordCount(const AnsiString S, const Sysutils::TSysCharSet &WordDelims);
extern PACKAGE int __fastcall WordPosition(const int N, const AnsiString S, const Sysutils::TSysCharSet &WordDelims);
extern PACKAGE AnsiString __fastcall ExtractWord(int N, const AnsiString S, const Sysutils::TSysCharSet &WordDelims);
extern PACKAGE AnsiString __fastcall ExtractWordPos(int N, const AnsiString S, const Sysutils::TSysCharSet &WordDelims, int &Pos);
extern PACKAGE AnsiString __fastcall ExtractDelimited(int N, const AnsiString S, const Sysutils::TSysCharSet &Delims);
extern PACKAGE AnsiString __fastcall ExtractSubstr(const AnsiString S, int &Pos, const Sysutils::TSysCharSet &Delims);
extern PACKAGE bool __fastcall IsWordPresent(const AnsiString W, const AnsiString S, const Sysutils::TSysCharSet &WordDelims);
extern PACKAGE AnsiString __fastcall QuotedString(const AnsiString S, char Quote);
extern PACKAGE AnsiString __fastcall ExtractQuotedString(const AnsiString S, char Quote);
extern PACKAGE AnsiString __fastcall Numb2USA(const AnsiString S);
extern PACKAGE AnsiString __fastcall CenterStr(const AnsiString S, int Len);
extern PACKAGE AnsiString __fastcall Dec2Hex(int N, Byte A);
extern PACKAGE AnsiString __fastcall D2H(int N, Byte A);
extern PACKAGE int __fastcall Hex2Dec(const AnsiString S);
extern PACKAGE int __fastcall H2D(const AnsiString S);
extern PACKAGE AnsiString __fastcall Dec2Numb(int N, Byte A, Byte B);
extern PACKAGE int __fastcall Numb2Dec(AnsiString S, Byte B);
extern PACKAGE int __fastcall RomanToInt(const AnsiString S);
extern PACKAGE AnsiString __fastcall IntToRoman(int Value);
extern PACKAGE AnsiString __fastcall IntToBin(int Value, int Digits, int Spaces);
extern PACKAGE int __fastcall FindPart(const AnsiString HelpWilds, const AnsiString InputStr);
extern PACKAGE bool __fastcall IsWild(AnsiString InputStr, AnsiString Wilds, bool IgnoreCase);
extern PACKAGE System::ShortString __fastcall XorString(const System::ShortString &Key, const System::ShortString &Src);
extern PACKAGE AnsiString __fastcall XorEncode(const AnsiString Key, const AnsiString Source);
extern PACKAGE AnsiString __fastcall XorDecode(const AnsiString Key, const AnsiString Source);
extern PACKAGE AnsiString __fastcall GetCmdLineArg(const AnsiString Switch, const Sysutils::TSysCharSet &SwitchChars);

}	/* namespace Mxstrutils */
using namespace Mxstrutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxStrUtils
