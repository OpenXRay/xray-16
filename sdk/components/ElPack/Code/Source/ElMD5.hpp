// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElMD5.pas' rev: 6.00

#ifndef ElMD5HPP
#define ElMD5HPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Classes.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elmd5
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TSourceType { SourceFile, SourceByteArray, SourceString };
#pragma option pop

#pragma pack(push, 2)
struct ULONG32
{
	Word LoWord16;
	Word HiWord16;
} ;
#pragma pack(pop)

typedef ULONG32 *PULONG32;

typedef int *PLong;

#pragma pack(push, 4)
struct hashDigest
{
	unsigned A;
	unsigned B;
	unsigned C;
	unsigned D;
} ;
#pragma pack(pop)

typedef hashDigest *PTR_Hash;

class DELPHICLASS TCrMD5;
class PASCALIMPLEMENTATION TCrMD5 : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TSourceType FType;
	AnsiString FInputFilePath;
	Byte *FInputArray;
	AnsiString FInputString;
	hashDigest *FOutputDigest;
	int FSourceLength;
	unsigned FActiveBlock[16];
	unsigned FA;
	unsigned FB;
	unsigned FC;
	unsigned FD;
	unsigned FAA;
	unsigned FBB;
	unsigned FCC;
	unsigned FDD;
	void __fastcall FF(unsigned &a, unsigned &b, unsigned &c, unsigned &d, unsigned &x, Byte s, unsigned ac);
	void __fastcall GG(unsigned &a, unsigned &b, unsigned &c, unsigned &d, unsigned &x, Byte s, unsigned ac);
	void __fastcall HH(unsigned &a, unsigned &b, unsigned &c, unsigned &d, unsigned &x, Byte s, unsigned ac);
	void __fastcall II(unsigned &a, unsigned &b, unsigned &c, unsigned &d, unsigned &x, Byte s, unsigned ac);
	
public:
	void __fastcall MD5_Initialize(void);
	void __fastcall MD5_Transform(void);
	void __fastcall MD5_Finish(void);
	void __fastcall MD5_Hash_Bytes(void);
	void __fastcall MD5_Hash_File(void);
	void __fastcall MD5_Hash(void);
	__property System::PByte pInputArray = {read=FInputArray, write=FInputArray};
	__property PTR_Hash pOutputArray = {read=FOutputDigest, write=FOutputDigest};
	
__published:
	__property TSourceType InputType = {read=FType, write=FType, nodefault};
	__property AnsiString InputFilePath = {read=FInputFilePath, write=FInputFilePath};
	__property AnsiString InputString = {read=FInputString, write=FInputString};
	__property int InputLength = {read=FSourceLength, write=FSourceLength, nodefault};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TCrMD5(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TCrMD5(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint S11 = 0x7;
static const Shortint S12 = 0xc;
static const Shortint S13 = 0x11;
static const Shortint S14 = 0x16;
static const Shortint S21 = 0x5;
static const Shortint S22 = 0x9;
static const Shortint S23 = 0xe;
static const Shortint S24 = 0x14;
static const Shortint S31 = 0x4;
static const Shortint S32 = 0xb;
static const Shortint S33 = 0x10;
static const Shortint S34 = 0x17;
static const Shortint S41 = 0x6;
static const Shortint S42 = 0xa;
static const Shortint S43 = 0xf;
static const Shortint S44 = 0x15;

}	/* namespace Elmd5 */
using namespace Elmd5;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElMD5
