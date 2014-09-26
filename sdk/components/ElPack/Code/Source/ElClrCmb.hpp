// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElClrCmb.pas' rev: 6.00

#ifndef ElClrCmbHPP
#define ElClrCmbHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElACtrls.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elclrcmb
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TColorComboOption { ccoNoColor, cco4BitColors, ccoSystemColors, ccoCustomChoice, ccoShowNames };
#pragma option pop

typedef Set<TColorComboOption, ccoNoColor, ccoShowNames>  TColorComboOptions;

typedef void __fastcall (__closure *TTranslateColorNameEvent)(System::TObject* Sender, Graphics::TColor Color, AnsiString &ColorName);

typedef void __fastcall (__closure *TColorComboAddMoreColorsEvent)(System::TObject* Sender, Classes::TStrings* Items);

class DELPHICLASS TElColorCombo;
class PASCALIMPLEMENTATION TElColorCombo : public Elactrls::TElAdvancedComboBox 
{
	typedef Elactrls::TElAdvancedComboBox inherited;
	
private:
	bool FDown;
	bool FMouseInControl;
	Dialogs::TColorDialogOptions FDialogOptions;
	Graphics::TColor FSelectedColor;
	TColorComboOptions FOptions;
	TTranslateColorNameEvent FOnTranslateColorName;
	TColorComboAddMoreColorsEvent FOnAddMoreColors;
	int FDummyInt;
	bool FDummyBool;
	Stdctrls::TComboBoxStyle FDummyStyle;
	bool FInDialog;
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	void __fastcall SetOptions(TColorComboOptions Value);
	
protected:
	void __fastcall SetSelectedColor(Graphics::TColor aColor);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	void __fastcall FillItems(void);
	DYNAMIC void __fastcall Change(void);
	virtual void __fastcall TriggerTranslateColorName(Graphics::TColor Color, AnsiString &ColorName);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall DoAddMoreColors(Classes::TStrings* Items);
	
public:
	__fastcall virtual TElColorCombo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElColorCombo(void);
	
__published:
	__property int Items = {read=FDummyInt, nodefault};
	__property int ItemHeight = {read=FDummyInt, write=FDummyInt, nodefault};
	__property int ItemIndex = {read=FDummyInt, write=FDummyInt, nodefault};
	__property Stdctrls::TComboBoxStyle Style = {read=FDummyStyle, write=FDummyStyle, nodefault};
	__property bool Transparent = {read=FDummyBool, write=FDummyBool, nodefault};
	__property TColorComboOptions Options = {read=FOptions, write=SetOptions, nodefault};
	__property Dialogs::TColorDialogOptions DialogOptions = {read=FDialogOptions, write=FDialogOptions, default=1};
	__property Graphics::TColor SelectedColor = {read=FSelectedColor, write=SetSelectedColor, nodefault};
	__property TTranslateColorNameEvent OnTranslateColorName = {read=FOnTranslateColorName, write=FOnTranslateColorName};
	__property Enabled  = {default=1};
	__property Width ;
	__property Height ;
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property OnClick ;
	__property OnKeyDown ;
	__property OnKeyUp ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnEnter ;
	__property OnExit ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
	__property TColorComboAddMoreColorsEvent OnAddMoreColors = {read=FOnAddMoreColors, write=FOnAddMoreColors};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElColorCombo(HWND ParentWindow) : Elactrls::TElAdvancedComboBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elclrcmb */
using namespace Elclrcmb;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElClrCmb
