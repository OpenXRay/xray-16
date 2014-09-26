// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElImgLst.pas' rev: 6.00

#ifndef ElImgLstHPP
#define ElImgLstHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTools.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <CommCtrl.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elimglst
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElImageList;
class PASCALIMPLEMENTATION TElImageList : public Controls::TImageList 
{
	typedef Controls::TImageList inherited;
	
public:
	void __fastcall ReadImg(Classes::TStream* Stream);
	void __fastcall WriteImg(Classes::TStream* Stream);
	HIDESBASE bool __fastcall Equal(TElImageList* IL);
	__fastcall virtual TElImageList(Classes::TComponent* AOwner);
	
protected:
	void __fastcall GetFullImages(Graphics::TBitmap* Image, Graphics::TBitmap* Mask);
	HIDESBASE void __fastcall ReadLeft(Classes::TReader* Reader);
	HIDESBASE void __fastcall ReadTop(Classes::TReader* Reader);
	HIDESBASE void __fastcall WriteLeft(Classes::TWriter* Writer);
	HIDESBASE void __fastcall WriteTop(Classes::TWriter* Writer);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
public:
	#pragma option push -w-inl
	/* TCustomImageList.CreateSize */ inline __fastcall TElImageList(int AWidth, int AHeight) : Controls::TImageList(AWidth, AHeight) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomImageList.Destroy */ inline __fastcall virtual ~TElImageList(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE int __fastcall DecodeRLE(const void * Source, const void * Target, unsigned Count, unsigned ColorDepth);
extern PACKAGE int __fastcall EncodeRLE(const System::PByte Source, const System::PByte Target, int Count, int BPP);

}	/* namespace Elimglst */
using namespace Elimglst;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElImgLst
