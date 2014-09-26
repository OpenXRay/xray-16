// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreePrinter.pas' rev: 6.00

#ifndef ElTreePrinterHPP
#define ElTreePrinterHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElStack.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElPrinter.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltreeprinter
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TPrintTreeItemEvent)(System::TObject* Sender, Eltree::TElTreeItem* Item, bool &Print);

typedef void __fastcall (__closure *TPrintHeaderSectionEvent)(System::TObject* Sender, Elheader::TElHeaderSection* Section, bool &Print);

class DELPHICLASS EElTreePrinterError;
class PASCALIMPLEMENTATION EElTreePrinterError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElTreePrinterError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElTreePrinterError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElTreePrinterError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElTreePrinterError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElTreePrinterError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElTreePrinterError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElTreePrinterError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElTreePrinterError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElTreePrinterError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreePrinter;
class PASCALIMPLEMENTATION TElTreePrinter : public Elprinter::TElControlPrinter 
{
	typedef Elprinter::TElControlPrinter inherited;
	
private:
	Elprinter::TPageEvent FOnAfterPage;
	Elprinter::TPageEvent FOnBeforePage;
	
protected:
	bool FPrinting;
	Htmlrender::TElHTMLRender* FRender;
	Graphics::TColor FBkColor;
	bool FShowButtons;
	bool FShowCheckboxes;
	bool FShowColumns;
	bool FShowEmptyImages;
	bool FShowEmptyImages2;
	bool FShowHeader;
	bool FshowHeaderExpandMarks;
	bool FShowHeaderImages;
	bool FShowHeaderSortMarks;
	bool FShowHiddenItems;
	bool FShowHiddenSections;
	bool FShowImages;
	bool FShowInvisibleItems;
	bool FShowInvisibleSections;
	bool FShowLines;
	bool FShowRoot;
	bool FShowRootButtons;
	bool FFillBackground;
	Graphics::TFont* FFont;
	bool FHeaderOnEveryPage;
	TPrintTreeItemEvent FOnItemPrinting;
	TPrintHeaderSectionEvent FOnSectionPrinting;
	int FScale;
	Graphics::TColor FStripedEvenColor;
	bool FStripedItems;
	Graphics::TColor FStripedOddColor;
	Graphics::TColor FTextColor;
	Eltree::TCustomElTree* FTree;
	Eltree::TElCellStyle* VirtStyle;
	bool FShowLeafButton;
	bool FVerticalLines;
	bool FHorizontalLines;
	Graphics::TColor FHorzDivLinesColor;
	Graphics::TColor FVertDivLinesColor;
	void __fastcall DoDrawHeader(Graphics::TCanvas* Canvas, const Types::TRect &ARect);
	void __fastcall DoDrawHeaderSection(Graphics::TCanvas* Canvas, Elheader::TElHeaderSection* Section, const Types::TRect &ARect);
	void __fastcall DoDrawItem(Graphics::TCanvas* Canvas, int ItemIndex, Eltree::TElTreeItem* Item, const Types::TRect &ARect);
	void __fastcall DoDrawItemCellContents(Graphics::TCanvas* Canvas, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section, const Types::TRect &ARect, Graphics::TColor TextColor, Graphics::TColor TextBkColor, Graphics::TColor ItemBkColor, Graphics::TFontStyles FontStyle);
	void __fastcall DoDrawItemTree(Graphics::TCanvas* Canvas, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section, Types::TRect &ARect);
	void __fastcall SetBkColor(Graphics::TColor Value);
	void __fastcall SetShowButtons(bool Value);
	void __fastcall SetShowCheckboxes(bool newValue);
	void __fastcall SetShowColumns(bool Value);
	void __fastcall SetShowEmptyImages(bool newValue);
	void __fastcall SetShowEmptyImages2(bool newValue);
	void __fastcall SetShowHeader(bool Value);
	void __fastcall SetshowHeaderExpandMarks(bool Value);
	void __fastcall SetShowHeaderImages(bool Value);
	void __fastcall SetShowHeaderSortMarks(bool Value);
	void __fastcall SetShowHiddenItems(bool Value);
	void __fastcall SetShowHiddenSections(bool Value);
	void __fastcall SetShowImages(bool Value);
	void __fastcall SetShowInvisibleItems(bool Value);
	void __fastcall SetShowInvisibleSections(bool Value);
	void __fastcall SetShowLines(bool Value);
	void __fastcall SetShowRoot(bool Value);
	void __fastcall SetShowRootButtons(bool newValue);
	void __fastcall SetFillBackground(bool Value);
	void __fastcall SetHeaderOnEveryPage(bool Value);
	void __fastcall SetScale(int Value);
	void __fastcall SetTree(Eltree::TCustomElTree* Value);
	virtual void __fastcall TriggerItemPrintingEvent(Eltree::TElTreeItem* Item, bool &Print);
	virtual void __fastcall TriggerSectionPrintingEvent(Elheader::TElHeaderSection* Section, bool &Print);
	void __fastcall DrawButtons(Graphics::TCanvas* ACanvas, Eltree::TElTreeItem* Item, bool IsNode, Types::TRect &R);
	void __fastcall DrawCheckBoxes(Graphics::TCanvas* ACanvas, Eltree::TElTreeItem* Item, Types::TRect &R);
	void __fastcall DrawImages(Graphics::TCanvas* ACanvas, Eltree::TElTreeItem* Item, Types::TRect &R);
	void __fastcall DrawItemLines(Graphics::TCanvas* ACanvas, Eltree::TElTreeItem* Item, Types::TRect &R);
	void __fastcall SetVerticalLines(bool Value);
	void __fastcall SetHorizontalLines(bool Value);
	void __fastcall SetShowLeafButton(bool Value);
	void __fastcall SetHorzDivLinesColor(Graphics::TColor Value);
	void __fastcall SetVertDivLinesColor(Graphics::TColor Value);
	virtual void __fastcall TriggerAfterPage(int PageNumber);
	virtual void __fastcall TriggerBeforePage(int PageNumber);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TElTreePrinter(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreePrinter(void);
	void __fastcall Print(void);
	
__published:
	__property Graphics::TColor BkColor = {read=FBkColor, write=SetBkColor, nodefault};
	__property bool ShowButtons = {read=FShowButtons, write=SetShowButtons, default=0};
	__property bool ShowCheckboxes = {read=FShowCheckboxes, write=SetShowCheckboxes, default=0};
	__property bool ShowColumns = {read=FShowColumns, write=SetShowColumns, nodefault};
	__property bool ShowEmptyImages = {read=FShowEmptyImages, write=SetShowEmptyImages, default=0};
	__property bool ShowEmptyImages2 = {read=FShowEmptyImages2, write=SetShowEmptyImages2, default=0};
	__property bool ShowHeader = {read=FShowHeader, write=SetShowHeader, nodefault};
	__property bool showHeaderExpandMarks = {read=FshowHeaderExpandMarks, write=SetshowHeaderExpandMarks, nodefault};
	__property bool ShowHeaderImages = {read=FShowHeaderImages, write=SetShowHeaderImages, nodefault};
	__property bool ShowHeaderSortMarks = {read=FShowHeaderSortMarks, write=SetShowHeaderSortMarks, nodefault};
	__property bool ShowHiddenItems = {read=FShowHiddenItems, write=SetShowHiddenItems, nodefault};
	__property bool ShowHiddenSections = {read=FShowHiddenSections, write=SetShowHiddenSections, nodefault};
	__property bool ShowImages = {read=FShowImages, write=SetShowImages, default=1};
	__property bool ShowInvisibleItems = {read=FShowInvisibleItems, write=SetShowInvisibleItems, default=1};
	__property bool ShowInvisibleSections = {read=FShowInvisibleSections, write=SetShowInvisibleSections, nodefault};
	__property bool ShowLines = {read=FShowLines, write=SetShowLines, default=1};
	__property bool ShowRoot = {read=FShowRoot, write=SetShowRoot, default=0};
	__property bool ShowRootButtons = {read=FShowRootButtons, write=SetShowRootButtons, default=0};
	__property bool FillBackground = {read=FFillBackground, write=SetFillBackground, nodefault};
	__property Graphics::TFont* Font = {read=FFont, write=FFont};
	__property bool HeaderOnEveryPage = {read=FHeaderOnEveryPage, write=SetHeaderOnEveryPage, nodefault};
	__property TPrintTreeItemEvent OnItemPrinting = {read=FOnItemPrinting, write=FOnItemPrinting};
	__property TPrintHeaderSectionEvent OnSectionPrinting = {read=FOnSectionPrinting, write=FOnSectionPrinting};
	__property int Scale = {read=FScale, write=SetScale, default=100};
	__property Graphics::TColor StripedEvenColor = {read=FStripedEvenColor, write=FStripedEvenColor, nodefault};
	__property bool StripedItems = {read=FStripedItems, write=FStripedItems, default=0};
	__property Graphics::TColor StripedOddColor = {read=FStripedOddColor, write=FStripedOddColor, nodefault};
	__property Graphics::TColor TextColor = {read=FTextColor, write=FTextColor, nodefault};
	__property Eltree::TCustomElTree* Tree = {read=FTree, write=SetTree};
	__property bool ShowLeafButton = {read=FShowLeafButton, write=SetShowLeafButton, default=0};
	__property bool VerticalLines = {read=FVerticalLines, write=SetVerticalLines, default=0};
	__property bool HorizontalLines = {read=FHorizontalLines, write=SetHorizontalLines, nodefault};
	__property Graphics::TColor HorzDivLinesColor = {read=FHorzDivLinesColor, write=SetHorzDivLinesColor, nodefault};
	__property Graphics::TColor VertDivLinesColor = {read=FVertDivLinesColor, write=SetVertDivLinesColor, nodefault};
	__property Elprinter::TPageEvent OnAfterPage = {read=FOnAfterPage, write=FOnAfterPage};
	__property Elprinter::TPageEvent OnBeforePage = {read=FOnBeforePage, write=FOnBeforePage};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreeprinter */
using namespace Eltreeprinter;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreePrinter
