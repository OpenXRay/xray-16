// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElFlatCtl.pas' rev: 6.00

#ifndef ElFlatCtlHPP
#define ElFlatCtlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElHook.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elflatctl
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElFlatController;
class PASCALIMPLEMENTATION TElFlatController : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	Elhook::TElHook* FHook;
	bool FMouseOver;
	bool FFlatFocusedScrollbars;
	Elvclutils::TElBorderSides FBorderSides;
	unsigned FTheme;
	void __fastcall SetFlatFocusedScrollbars(bool newValue);
	void __fastcall HookAfterProcessHandler(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall SetActive(bool newValue);
	bool __fastcall GetActive(void);
	void __fastcall SetControl(Controls::TWinControl* newValue);
	Controls::TWinControl* __fastcall GetControl(void);
	void __fastcall SetDesignActive(bool newValue);
	bool __fastcall GetDesignActive(void);
	void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	void __fastcall HookBeforeProcessHandler(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	
protected:
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	bool FUseXPThemes;
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	bool __fastcall IsControlThemed(Controls::TControl* Control);
	void __fastcall SetUseXPThemes(bool Value);
	void __fastcall CreateThemeHandle(void);
	void __fastcall FreeThemeHandle(void);
	virtual void __fastcall Loaded(void);
	void __fastcall DoNCPaint(void);
	
public:
	void __fastcall DrawFlatBorder(void);
	void __fastcall UpdateFrame(void);
	__fastcall virtual TElFlatController(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFlatController(void);
	bool __fastcall IsThemeApplied(void);
	
__published:
	__property bool FlatFocusedScrollbars = {read=FFlatFocusedScrollbars, write=SetFlatFocusedScrollbars, nodefault};
	__property bool Active = {read=GetActive, write=SetActive, nodefault};
	__property Controls::TWinControl* Control = {read=GetControl, write=SetControl};
	__property bool DesignActive = {read=GetDesignActive, write=SetDesignActive, nodefault};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, nodefault};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, nodefault};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
};


class DELPHICLASS TElFlatEntry;
class PASCALIMPLEMENTATION TElFlatEntry : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	TElFlatController* FController;
	bool FDesignActive;
	Controls::TWinControl* FControl;
	bool FActive;
	bool FFlatFocusedScrollbars;
	bool __fastcall GetActive(void);
	void __fastcall SetActive(bool newValue);
	bool __fastcall GetFlatFocusedScrollbars(void);
	void __fastcall SetFlatFocusedScrollbars(bool newValue);
	bool __fastcall GetDesignActive(void);
	void __fastcall SetDesignActive(bool newValue);
	void __fastcall SetControl(Controls::TWinControl* newValue);
	Elvclutils::TElFlatBorderType __fastcall GetActiveBorderType(void);
	void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	Elvclutils::TElFlatBorderType __fastcall GetInactiveBorderType(void);
	void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	
protected:
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	Elvclutils::TElBorderSides FBorderSides;
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	bool __fastcall GetUseXPThemes(void);
	void __fastcall SetUseXPThemes(bool Value);
	
public:
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	__fastcall virtual TElFlatEntry(Classes::TCollection* Collection);
	__fastcall virtual ~TElFlatEntry(void);
	
__published:
	__property bool Active = {read=GetActive, write=SetActive, nodefault};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property bool FlatFocusedScrollbars = {read=GetFlatFocusedScrollbars, write=SetFlatFocusedScrollbars, nodefault};
	__property bool DesignActive = {read=GetDesignActive, write=SetDesignActive, nodefault};
	__property Controls::TWinControl* Control = {read=FControl, write=SetControl};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=GetActiveBorderType, write=SetActiveBorderType, nodefault};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=GetInactiveBorderType, write=SetInactiveBorderType, nodefault};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property bool UseXPThemes = {read=GetUseXPThemes, write=SetUseXPThemes, nodefault};
};


class DELPHICLASS TElFlatEntries;
class DELPHICLASS TElFlatMultiController;
class PASCALIMPLEMENTATION TElFlatMultiController : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FFlatFocusedScrollbars;
	bool FAutoAddControls;
	Elhook::TElHook* FHook;
	TElFlatEntries* FEntries;
	Elvclutils::TElBorderSides FBorderSides;
	MESSAGE void __fastcall CMControlListChange(Messages::TMessage &Msg);
	void __fastcall SetEntries(TElFlatEntries* newValue);
	void __fastcall SetActive(bool newValue);
	bool __fastcall GetActive(void);
	void __fastcall SetDesignActive(bool newValue);
	bool __fastcall GetDesignActive(void);
	void __fastcall AfterProcessHandler(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall SetAutoAddControls(bool newValue);
	void __fastcall ScanForm(void);
	void __fastcall SetFlatFocusedScrollbars(bool newValue);
	void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	
protected:
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	bool FUseXPThemes;
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	virtual void __fastcall Loaded(void);
	void __fastcall SetUseXPThemes(bool Value);
	
public:
	__fastcall virtual TElFlatMultiController(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFlatMultiController(void);
	
__published:
	__property TElFlatEntries* Entries = {read=FEntries, write=SetEntries};
	__property bool Active = {read=GetActive, write=SetActive, nodefault};
	__property bool DesignActive = {read=GetDesignActive, write=SetDesignActive, nodefault};
	__property bool AutoAddControls = {read=FAutoAddControls, write=SetAutoAddControls, nodefault};
	__property bool FlatFocusedScrollbars = {read=FFlatFocusedScrollbars, write=SetFlatFocusedScrollbars, nodefault};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, nodefault};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, nodefault};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
};


class PASCALIMPLEMENTATION TElFlatEntries : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElFlatEntry* operator[](int index) { return Entries[index]; }
	
private:
	TElFlatMultiController* FController;
	TElFlatEntry* __fastcall GetEntries(int index);
	void __fastcall SetEntries(int index, TElFlatEntry* newValue);
	
public:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	HIDESBASE TElFlatEntry* __fastcall Add(void);
	__property TElFlatEntry* Entries[int index] = {read=GetEntries, write=SetEntries/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Create */ inline __fastcall TElFlatEntries(TMetaClass* ItemClass) : Classes::TCollection(ItemClass) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElFlatEntries(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elflatctl */
using namespace Elflatctl;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElFlatCtl
