// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElStrUtils.pas' rev: 6.00

#ifndef ElStrUtilsHPP
#define ElStrUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------
typedef WideString TElFString;

namespace Elstrutils
{
//-- type declarations -------------------------------------------------------
typedef WideString TElFString;

typedef wchar_t TElFChar;

typedef wchar_t *PElFChar;

#pragma option push -b-
enum ElStrUtils__1 { wrfReplaceAll, wrfIgnoreCase };
#pragma option pop

typedef Set<ElStrUtils__1, wrfReplaceAll, wrfIgnoreCase>  TWideReplaceFlags;

//-- var, const, procedure ---------------------------------------------------
#define oleaut "oleaut32.dll"
extern "C" wchar_t * __stdcall SysAllocStringLen(wchar_t * P, int Len);
extern "C" void __stdcall SysFreeString(wchar_t * S);
extern "C" int __stdcall SysStringLen(wchar_t * S);
extern PACKAGE bool doti;
#define SPathDelimiters "/\\"
#define SWidePathDelimiters L"/\\"
extern PACKAGE AnsiString __fastcall IntToStrFmt(int value);
extern PACKAGE AnsiString __fastcall FloatToStrFmt(Extended value, int decims);
extern PACKAGE AnsiString __fastcall IntToStrPad(int value, int MinSize);
extern PACKAGE AnsiString __fastcall CenterStr(AnsiString Str, int len);
extern PACKAGE AnsiString __fastcall OEMToStr(AnsiString S);
extern PACKAGE AnsiString __fastcall StrToOEM(AnsiString S);
extern PACKAGE int __fastcall MessageRes(int Txt, char * Title, Word TextType, bool Sounds);
extern PACKAGE bool __fastcall replace(AnsiString &Str, AnsiString SourceString, AnsiString DestString);
extern PACKAGE AnsiString __fastcall ExtractWord(AnsiString str, int n);
extern PACKAGE int __fastcall FstNonSpace(AnsiString str);
extern PACKAGE int __fastcall NextWordBegin(AnsiString str, int CurrentPos);
extern PACKAGE int __fastcall LastPos(AnsiString SubStr, AnsiString Strn);
extern PACKAGE bool __fastcall LineIsEmpty(AnsiString str);
extern PACKAGE AnsiString __fastcall CompleteLine(AnsiString Str, int FLen, char symb);
extern PACKAGE AnsiString __fastcall PrefixLine(AnsiString Str, int FLen, char symb);
extern PACKAGE AnsiString __fastcall MakeString(int FLen, AnsiString Seq);
extern PACKAGE int __fastcall H2D(AnsiString S);
extern PACKAGE int __fastcall H2DDef(const AnsiString S, int Def);
extern PACKAGE int __fastcall Bin2Int(AnsiString S);
extern PACKAGE int __fastcall Bin2IntDef(AnsiString S, int Default);
extern PACKAGE AnsiString __fastcall Data2Str(void * Buffer, int BufLen);
extern PACKAGE bool __fastcall Str2Data(AnsiString S, void * &Buffer, int &BufLen);
extern PACKAGE bool __fastcall IsDigit(char ch);
extern PACKAGE bool __fastcall IsDigitStr(const AnsiString S);
extern PACKAGE bool __fastcall IsAlpha(char ch);
extern PACKAGE bool __fastcall IsAlphaOrDigit(char ch);
extern PACKAGE bool __fastcall IsAlphaStr(const AnsiString S);
extern PACKAGE bool __fastcall IsIdentStr(const AnsiString S);
extern PACKAGE AnsiString __fastcall ExtractStr(AnsiString &S, int SPos, int SLen);
extern PACKAGE int __fastcall LeftBreak(AnsiString S, int Pos);
extern PACKAGE AnsiString __fastcall EscapeString(AnsiString aString, AnsiString UnsafeChars, char EscapeChar);
extern PACKAGE AnsiString __fastcall UnEscapeString(AnsiString aString, char EscapeChar);
extern PACKAGE bool __fastcall StrStartsWith(char * Source, char * Seq);
extern PACKAGE bool __fastcall ContainsAt(AnsiString Source, int Index, AnsiString Seq);
extern PACKAGE bool __fastcall FileNameLike(AnsiString FileName, AnsiString Mask);
extern PACKAGE bool __fastcall AnsiSameText(const AnsiString S1, const AnsiString S2);
extern PACKAGE AnsiString __fastcall CurrToPrettyStr(const System::Currency Value);
extern PACKAGE System::Currency __fastcall PrettyStrToCurr(const AnsiString Value);
extern PACKAGE int __fastcall CurrSign(const System::Currency Value);
extern PACKAGE char * __fastcall StringDup(AnsiString S);
extern PACKAGE WideString __fastcall uni2uppers(WideString s);
extern PACKAGE WideString __fastcall uni2lowers(WideString s);
extern PACKAGE WideString __fastcall uni2upperf(WideString s);
extern PACKAGE WideString __fastcall uni2lowerf(WideString s);
extern PACKAGE wchar_t * __fastcall WideStringDup(WideString S);
extern PACKAGE int __fastcall WidePos(const WideString Substr, const WideString S);
extern PACKAGE wchar_t * __fastcall WideStrScan(const wchar_t * Str, wchar_t Chr);
extern PACKAGE wchar_t * __fastcall WideStrRScan(const wchar_t * Str, wchar_t Chr);
extern PACKAGE WideString __fastcall WideQuotedStr(const WideString S, wchar_t Quote);
extern PACKAGE WideString __fastcall WideExtractQuotedStr(wchar_t * &Src, wchar_t Quote);
extern PACKAGE wchar_t * __fastcall WideStrEnd(const wchar_t * Str);
extern PACKAGE bool __fastcall WideSameText(const WideString S1, const WideString S2);
extern PACKAGE int __fastcall WideCompareText(const WideString S1, const WideString S2);
extern PACKAGE WideString __fastcall WideExtractStr(WideString &S, int SPos, int SLen);
extern PACKAGE wchar_t * __fastcall WideStrCopy(wchar_t * Target, wchar_t * Source);
extern PACKAGE wchar_t * __fastcall WideStrPCopy(wchar_t * Target, const WideString Source);
extern PACKAGE int __fastcall WideStrComp(const wchar_t * S1, const wchar_t * S2);
extern PACKAGE int __fastcall WideStrLComp(const wchar_t * Str1, const wchar_t * Str2, unsigned MaxLen);
extern PACKAGE unsigned __fastcall WideStrLen(const wchar_t * Str);
extern PACKAGE WideString __fastcall WideStrPas(const wchar_t * Source);
extern PACKAGE void __fastcall WideMove(const void *Source, void *Dest, int Count);
extern PACKAGE void __fastcall FillWord(void *X, int Count, Word Value);
extern PACKAGE void __fastcall FillWideChar(void *X, int Count, wchar_t Value);
extern PACKAGE wchar_t * __fastcall WideStrMove(wchar_t * Dest, const wchar_t * Source, unsigned Count);
extern PACKAGE wchar_t * __fastcall WideStrECopy(wchar_t * Dest, const wchar_t * Source);
extern PACKAGE wchar_t * __fastcall WideStrLCopy(wchar_t * Dest, const wchar_t * Source, unsigned MaxLen);
extern PACKAGE wchar_t * __fastcall WideStrLCat(wchar_t * Dest, const wchar_t * Source, unsigned MaxLen);
extern PACKAGE wchar_t * __fastcall WideStrCat(wchar_t * Dest, const wchar_t * Source);
extern PACKAGE void __fastcall SetWideString(WideString &S, wchar_t * Buffer, int Len);
extern PACKAGE int __fastcall CompareWideStr(const WideString S1, const WideString S2);
extern PACKAGE bool __fastcall SameWideStr(const WideString S1, const WideString S2);
extern PACKAGE wchar_t * __fastcall WideLastChar(const WideString S);
extern PACKAGE wchar_t * __fastcall WideStrAlloc(unsigned Size);
extern PACKAGE unsigned __fastcall WideStrBufSize(const wchar_t * Str);
extern PACKAGE wchar_t * __fastcall WideStrNew(const wchar_t * Str);
extern PACKAGE void __fastcall WideStrDispose(wchar_t * Str);
extern PACKAGE WideString __fastcall WideUpperCase(const WideString S);
extern PACKAGE WideString __fastcall WideLowerCase(const WideString S);
extern PACKAGE bool __fastcall IsWideDelimiter(const WideString Delimiters, const WideString S, int Index);
extern PACKAGE bool __fastcall IsWidePathDelimiter(const WideString S, int Index);
extern PACKAGE WideString __fastcall IncludeWideTrailingDelimiter(const WideString S);
extern PACKAGE WideString __fastcall ExcludeWideTrailingDelimiter(const WideString S);
extern PACKAGE WideString __fastcall GetWideCharRangeString(wchar_t FirstChar, wchar_t LastChar);
extern PACKAGE WideString __fastcall GetWideStringOf(wchar_t Char, unsigned Len);
extern PACKAGE WideString __fastcall WideStringReplace(const WideString S, const WideString OldPattern, const WideString NewPattern, TWideReplaceFlags Flags);
extern PACKAGE bool __fastcall WideReplace(WideString &Str, WideString SourceString, WideString DestString);
extern PACKAGE wchar_t * __fastcall WideStrPos(const wchar_t * Str1, const wchar_t * Str2);
extern PACKAGE WideString __fastcall WideCopy(WideString S, int SPos, int SLen);
extern PACKAGE void __fastcall WideInsert(WideString Text, WideString &S, int SPos);
extern PACKAGE void __fastcall WideDelete(WideString &S, int SPos, int SLen);
extern PACKAGE WideString __fastcall WideMakeString(int FLen, WideString Seq);
extern PACKAGE int __fastcall WideLastPos(WideString SubStr, WideString Strn);
extern PACKAGE void __fastcall TStrDelete(WideString &S, int SPos, int SLen);
extern PACKAGE WideString __fastcall TStrExtractStr(WideString &S, int SPos, int SLen);
extern PACKAGE void __fastcall SetTStr(WideString &S, PElFChar Buffer, int Len);
extern PACKAGE AnsiString __fastcall GetCharRangeString(char FirstChar, char LastChar);

}	/* namespace Elstrutils */
using namespace Elstrutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElStrUtils
