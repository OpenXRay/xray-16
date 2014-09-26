// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElFontCombo.pas' rev: 6.00

#ifndef ElFontComboHPP
#define ElFontComboHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Controls.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Printers.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elfontcombo
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElFontSampleMode { fsmFontName, fsmFontSample, fsmBoth, fsmNoSample };
#pragma option pop

#pragma option push -b-
enum TElFontDevice { efdScreen, efdPrinter, efdBoth };
#pragma option pop

#pragma option push -b-
enum TElFontComboOption { foAnsiOnly, foTrueTypeOnly, foIncludeOEMFonts, foIncludeSymbolFonts, foOEMFontsOnly, foScalableOnly };
#pragma option pop

typedef Set<TElFontComboOption, foAnsiOnly, foScalableOnly>  TElFontComboOptions;

class DELPHICLASS TElFontComboBox;
class PASCALIMPLEMENTATION TElFontComboBox : public Elactrls::TElAdvancedComboBox 
{
	typedef Elactrls::TElAdvancedComboBox inherited;
	
protected:
	TElFontComboOptions FOptions;
	WideString FSampleText;
	Graphics::TFontPitch FFontPitch;
	TElFontSampleMode FSampleMode;
	TElFontDevice FFontDevice;
	AnsiString FFontName;
	int FFakeInt;
	void __fastcall SetFontName(AnsiString Value);
	void __fastcall SetOptions(TElFontComboOptions Value);
	void __fastcall SetSampleText(WideString Value);
	void __fastcall SetFontPitch(Graphics::TFontPitch Value);
	void __fastcall SetSampleMode(TElFontSampleMode Value);
	virtual void __fastcall CreateWnd(void);
	WideString __fastcall GetItemText(int index);
	virtual int __fastcall GetItemWidth(int Index);
	virtual void __fastcall MeasureItem(int Index, int &Height);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	void __fastcall SetFontDevice(TElFontDevice Value);
	void __fastcall AddFont(AnsiString Font, int FontType);
	AnsiString __fastcall GetFontName();
	virtual void __fastcall Loaded(void);
	
public:
	void __fastcall RebuildFontList(void);
	__fastcall virtual TElFontComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFontComboBox(void);
	
__published:
	__property int Items = {read=FFakeInt, nodefault};
	__property int Style = {read=FFakeInt, nodefault};
	__property int Text = {read=FFakeInt, nodefault};
	__property AnsiString FontName = {read=GetFontName, write=SetFontName};
	__property TElFontComboOptions Options = {read=FOptions, write=SetOptions, nodefault};
	__property WideString SampleText = {read=FSampleText, write=SetSampleText};
	__property Graphics::TFontPitch FontPitch = {read=FFontPitch, write=SetFontPitch, nodefault};
	__property TElFontSampleMode SampleMode = {read=FSampleMode, write=SetSampleMode, nodefault};
	__property TElFontDevice FontDevice = {read=FFontDevice, write=SetFontDevice, nodefault};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElFontComboBox(HWND ParentWindow) : Elactrls::TElAdvancedComboBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elfontcombo */
using namespace Elfontcombo;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElFontCombo
