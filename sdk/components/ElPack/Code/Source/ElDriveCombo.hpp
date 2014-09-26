// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDriveCombo.pas' rev: 6.00

#ifndef ElDriveComboHPP
#define ElDriveComboHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElACtrls.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ShellAPI.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eldrivecombo
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElDriveComboOption { dcoDisplayFloppy, dcoDisplayNetwork, dcoDisplayHard, dcoDisplayCD, dcoDisplayRAM };
#pragma option pop

typedef Set<TElDriveComboOption, dcoDisplayFloppy, dcoDisplayRAM>  TElDriveComboOptions;

class DELPHICLASS TElDriveComboBox;
class PASCALIMPLEMENTATION TElDriveComboBox : public Elactrls::TElAdvancedComboBox 
{
	typedef Elactrls::TElAdvancedComboBox inherited;
	
private:
	int FDummyInt;
	char FDummyChar;
	
protected:
	char FDrive;
	Elvclutils::TElTextCase FTextCase;
	TElDriveComboOptions FOptions;
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	void __fastcall FillItems(void);
	void __fastcall SetDrive(char Value);
	virtual void __fastcall CreateWnd(void);
	void __fastcall SetTextCase(Elvclutils::TElTextCase Value);
	void __fastcall SetOptions(TElDriveComboOptions Value);
	DYNAMIC void __fastcall Change(void);
	
public:
	__fastcall virtual TElDriveComboBox(Classes::TComponent* AOwner);
	__property char Drive = {read=FDrive, write=SetDrive, nodefault};
	__property char ItemIndex = {read=FDummyChar, nodefault};
	
__published:
	__property int Items = {read=FDummyInt, nodefault};
	__property int Style = {read=FDummyInt, nodefault};
	__property int ItemHeight = {read=FDummyInt, nodefault};
	__property Elvclutils::TElTextCase TextCase = {read=FTextCase, write=SetTextCase, nodefault};
	__property TElDriveComboOptions Options = {read=FOptions, write=SetOptions, nodefault};
public:
	#pragma option push -w-inl
	/* TElAdvancedComboBox.Destroy */ inline __fastcall virtual ~TElDriveComboBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDriveComboBox(HWND ParentWindow) : Elactrls::TElAdvancedComboBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldrivecombo */
using namespace Eldrivecombo;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDriveCombo
