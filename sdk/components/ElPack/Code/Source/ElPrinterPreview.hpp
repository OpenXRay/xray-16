// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElPrinterPreview.pas' rev: 6.00

#ifndef ElPrinterPreviewHPP
#define ElPrinterPreviewHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElEdits.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElCombos.hpp>	// Pascal unit
#include <ElHook.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElSpin.hpp>	// Pascal unit
#include <ElPrinter.hpp>	// Pascal unit
#include <Printers.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElStatBar.hpp>	// Pascal unit
#include <ElToolbar.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elprinterpreview
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElPrinterPreviewDlg;
class PASCALIMPLEMENTATION TElPrinterPreviewDlg : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Eltoolbar::TElToolBar* Toolbar;
	Forms::TScrollBox* ScrollBox;
	Elstatbar::TElStatusBar* StatusBar;
	Eltoolbar::TElToolButton* PrintBtn;
	Eltoolbar::TElToolButton* ElToolButton2;
	Eltoolbar::TElToolButton* OnePageBtn;
	Eltoolbar::TElToolButton* MultipageBtn;
	Eltoolbar::TElToolButton* ElToolButton1;
	Eltoolbar::TElToolButton* SaveBtn;
	Eltoolbar::TElToolButton* ElToolButton3;
	Dialogs::TPrintDialog* PrintDialog;
	Eltoolbar::TElToolButton* PrintSetupBtn;
	Eltoolbar::TElToolButton* PrevPageBtn;
	Eltoolbar::TElToolButton* ElToolButton5;
	Eltoolbar::TElToolButton* NextPageBtn;
	Elspin::TElSpinEdit* PageSpin;
	Dialogs::TPrinterSetupDialog* PrinterSetupDialog;
	Dialogs::TSaveDialog* SaveDialog;
	Elpopbtn::TElGraphicButton* CloseBtn;
	Elpanel::TElPanel* PagesPanel;
	Elpanel::TElPanel* MainPagePanel;
	Elhook::TElHook* ElHook1;
	Elcombos::TElComboBox* ScaleCombo;
	void __fastcall PrintBtnClick(System::TObject* Sender);
	void __fastcall ScrollBoxResize(System::TObject* Sender);
	void __fastcall MainPagePanelPaint(System::TObject* Sender);
	void __fastcall PageSpinChange(System::TObject* Sender);
	void __fastcall FormCreate(System::TObject* Sender);
	void __fastcall FormDestroy(System::TObject* Sender);
	void __fastcall ScaleComboExit(System::TObject* Sender);
	void __fastcall NextPageBtnClick(System::TObject* Sender);
	void __fastcall PrintSetupBtnClick(System::TObject* Sender);
	void __fastcall CloseBtnClick(System::TObject* Sender);
	void __fastcall SaveBtnClick(System::TObject* Sender);
	void __fastcall FormShow(System::TObject* Sender);
	void __fastcall PrevPageBtnClick(System::TObject* Sender);
	void __fastcall ScComboKeyDown(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall ElHook1AfterProcess(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall FormResize(System::TObject* Sender);
	void __fastcall ScaleComboChange(System::TObject* Sender);
	void __fastcall MultipageBtnClick(System::TObject* Sender);
	void __fastcall OnePageBtnClick(System::TObject* Sender);
	
private:
	int FScale;
	int FCurrentPage;
	int FTotalPages;
	Ellist::TElList* Panels;
	int PagePanels;
	int HorzPages;
	int VertPages;
	Elprinter::TElPrinter* FPrinter;
	int FScaleIdx;
	int FRealIdx;
	void __fastcall SetCurrentPage(int Value);
	void __fastcall SetTotalPages(int Value);
	void __fastcall SetScale(int Value);
	
protected:
	void __fastcall UpdatePageNumbers(void);
	void __fastcall UpdatePanels(void);
	void __fastcall UpdateMultiPage(void);
	
public:
	void __fastcall SetData(Elprinter::TElPrinter* Printer);
	__property int CurrentPage = {read=FCurrentPage, write=SetCurrentPage, nodefault};
	__property int TotalPages = {read=FTotalPages, write=SetTotalPages, nodefault};
	__property int Scale = {read=FScale, write=SetScale, nodefault};
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TElPrinterPreviewDlg(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TElPrinterPreviewDlg(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TElPrinterPreviewDlg(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElPrinterPreviewDlg(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TElPrinterPreviewDlg* ElPrinterPreviewDlg;

}	/* namespace Elprinterpreview */
using namespace Elprinterpreview;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElPrinterPreview
