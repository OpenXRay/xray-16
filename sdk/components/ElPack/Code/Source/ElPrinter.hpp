// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElPrinter.pas' rev: 6.00

#ifndef ElPrinterHPP
#define ElPrinterHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Printers.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elprinter
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS EPrinterError;
class PASCALIMPLEMENTATION EPrinterError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EPrinterError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EPrinterError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EPrinterError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EPrinterError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EPrinterError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EPrinterError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EPrinterError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EPrinterError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EPrinterError(void) { }
	#pragma option pop
	
};


typedef void __fastcall (__closure *TPageEvent)(System::TObject* Sender, int PageNumber);

class DELPHICLASS TElPrinter;
class PASCALIMPLEMENTATION TElPrinter : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FActive;
	Ellist::TElList* FCanvas;
	HDC FDC;
	TPageEvent FOnAfterPage;
	TPageEvent FOnBeforePage;
	int FPageIndex;
	Ellist::TElList* FPages;
	int FPageWidth;
	int FPageHeight;
	int FPrintOffsetX;
	int FPrintOffsetY;
	int FRightMargin;
	int FBottomMargin;
	int FLeftMargin;
	int FTopMargin;
	AnsiString FTitle;
	Graphics::TCanvas* __fastcall GetCanvas(int Index);
	Graphics::TMetafile* __fastcall GetPage(int Index);
	int __fastcall GetPageCount(void);
	int __fastcall GetPageHeight(void);
	int __fastcall GetPageWidth(void);
	int __fastcall GetPrintOffsetX(void);
	int __fastcall GetPrintOffsetY(void);
	void __fastcall SetBottomMargin(int Value);
	void __fastcall SetCanvas(int Index, Graphics::TCanvas* Value);
	void __fastcall SetLeftMargin(int Value);
	void __fastcall SetPage(int Index, Graphics::TMetafile* Value);
	void __fastcall SetPageIndex(int Value);
	void __fastcall SetRightMargin(int Value);
	void __fastcall SetTopMargin(int Value);
	
protected:
	virtual void __fastcall TriggerAfterPage(int PageNumber);
	virtual void __fastcall TriggerBeforePage(int PageNumber);
	
public:
	__fastcall virtual TElPrinter(Classes::TComponent* AOwner);
	__fastcall virtual ~TElPrinter(void);
	void __fastcall Abort(void);
	int __fastcall AddPage(void);
	void __fastcall BeginDoc(void);
	void __fastcall Clear(void);
	void __fastcall DeletePage(int Index);
	void __fastcall EndDoc(void);
	void __fastcall InsertPage(int Index);
	virtual void __fastcall Loaded(void);
	void __fastcall NewPage(void);
	void __fastcall Preview(void);
	void __fastcall PrintPages(int StartIndex, int EndIndex);
	void __fastcall SavePage(AnsiString FileName, int Index);
	int __fastcall HorzMMToPixel(int MM100s);
	int __fastcall VertMMToPixel(int MM100s);
	__property bool Active = {read=FActive, nodefault};
	__property Graphics::TCanvas* Canvas[int Index] = {read=GetCanvas, write=SetCanvas};
	__property Graphics::TMetafile* Page[int Index] = {read=GetPage, write=SetPage};
	__property int PageCount = {read=GetPageCount, nodefault};
	__property int PageHeight = {read=FPageHeight, nodefault};
	__property int PageIndex = {read=FPageIndex, write=SetPageIndex, nodefault};
	__property int PageWidth = {read=FPageWidth, nodefault};
	__property int PrintOffsetX = {read=FPrintOffsetX, nodefault};
	__property int PrintOffsetY = {read=FPrintOffsetY, nodefault};
	__property HDC PrinterDC = {read=FDC, nodefault};
	
__published:
	__property int BottomMargin = {read=FBottomMargin, write=SetBottomMargin, nodefault};
	__property int LeftMargin = {read=FLeftMargin, write=SetLeftMargin, nodefault};
	__property int RightMargin = {read=FRightMargin, write=SetRightMargin, nodefault};
	__property int TopMargin = {read=FTopMargin, write=SetTopMargin, nodefault};
	__property AnsiString Title = {read=FTitle, write=FTitle};
	__property TPageEvent OnAfterPage = {read=FOnAfterPage, write=FOnAfterPage};
	__property TPageEvent OnBeforePage = {read=FOnBeforePage, write=FOnBeforePage};
};


class DELPHICLASS TElControlPrinter;
class PASCALIMPLEMENTATION TElControlPrinter : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	TElPrinter* FPrinter;
	void __fastcall SetPrinter(TElPrinter* Value);
	
protected:
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
__published:
	__property TElPrinter* Printer = {read=FPrinter, write=SetPrinter};
public:
	#pragma option push -w-inl
	/* TComponent.Create */ inline __fastcall virtual TElControlPrinter(Classes::TComponent* AOwner) : Classes::TComponent(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TComponent.Destroy */ inline __fastcall virtual ~TElControlPrinter(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elprinter */
using namespace Elprinter;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElPrinter
