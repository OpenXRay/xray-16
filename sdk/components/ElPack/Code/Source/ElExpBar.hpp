// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElExpBar.pas' rev: 6.00

#ifndef ElExpBarHPP
#define ElExpBarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElHTMLPanel.hpp>	// Pascal unit
#include <ElAdvPanel.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ElStrToken.hpp>	// Pascal unit
#include <ElScrollBox.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elexpbar
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS EExplorerBarError;
class PASCALIMPLEMENTATION EExplorerBarError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EExplorerBarError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EExplorerBarError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EExplorerBarError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EExplorerBarError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EExplorerBarError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EExplorerBarError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EExplorerBarError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EExplorerBarError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EExplorerBarError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElExplorerBarGroupButton;
class PASCALIMPLEMENTATION TElExplorerBarGroupButton : public Eladvpanel::TElAdvCaptionButton 
{
	typedef Eladvpanel::TElAdvCaptionButton inherited;
	
protected:
	virtual void __fastcall DrawThemedBackground(Graphics::TCanvas* Canvas);
	virtual WideString __fastcall GetThemedClassName();
	virtual int __fastcall GetThemePartID(void);
	virtual int __fastcall GetThemeStateID(void);
public:
	#pragma option push -w-inl
	/* TElAdvCaptionButton.Create */ inline __fastcall virtual TElExplorerBarGroupButton(Classes::TComponent* AOwner) : Eladvpanel::TElAdvCaptionButton(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TCustomElGraphicButton.Destroy */ inline __fastcall virtual ~TElExplorerBarGroupButton(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElExplorerBarGroup;
class PASCALIMPLEMENTATION TElExplorerBarGroup : public Eladvpanel::TCustomElAdvancedPanel 
{
	typedef Eladvpanel::TCustomElAdvancedPanel inherited;
	
protected:
	Controls::TAlign FAlign;
	virtual void __fastcall TriggerMinimizeEvent(void);
	virtual void __fastcall TriggerRestoreEvent(void);
	DYNAMIC void __fastcall Resize(void);
	HIDESBASE MESSAGE void __fastcall CMVisibleChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMMove(Messages::TMessage &Message);
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall DrawThemedBackground(void);
	virtual Eladvpanel::TElAdvCaptionButton* __fastcall CreateButton(void);
	virtual Eladvpanel::TElAdvCaptionPanel* __fastcall CreatePanel(void);
	HIDESBASE MESSAGE void __fastcall WMEraseBkGnd(Messages::TWMEraseBkgnd &Msg);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual void __fastcall CreateWnd(void);
	
public:
	__fastcall virtual TElExplorerBarGroup(Classes::TComponent* AOwner);
	virtual int __fastcall GetButtonWidth(void);
	virtual int __fastcall GetCaptionHeight(void);
	
__published:
	__property Controls::TAlign Align = {read=FAlign, write=FAlign, stored=false, default=0};
	__property OnImageNeeded ;
	__property OnLinkClick ;
	__property Cursor ;
	__property LinkColor ;
	__property LinkPopupMenu ;
	__property LinkStyle ;
	__property Background ;
	__property BackgroundType  = {default=2};
	__property GradientEndColor  = {default=0};
	__property GradientStartColor  = {default=0};
	__property GradientSteps  = {default=16};
	__property Alignment  = {default=2};
	__property Layout  = {default=1};
	__property ImageForm ;
	__property OnMove ;
	__property BevelInner ;
	__property BevelOuter ;
	__property BevelSpaceColor ;
	__property BevelWidth  = {default=1};
	__property BorderStyle  = {default=0};
	__property BorderWidth  = {default=0};
	__property Canvas ;
	__property Color  = {default=-2147483633};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property Locked  = {default=0};
	__property MouseCapture ;
	__property ParentColor  = {default=0};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property TransparentXPThemes  = {default=1};
	__property UseXPThemes  = {default=1};
	__property Visible  = {default=1};
	__property Caption ;
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property Action ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property Minimized  = {default=0};
	__property CaptionSettings ;
	__property OnMinimize ;
	__property OnRestore ;
	__property OnClose ;
public:
	#pragma option push -w-inl
	/* TCustomElAdvancedPanel.Destroy */ inline __fastcall virtual ~TElExplorerBarGroup(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElExplorerBarGroup(HWND ParentWindow) : Eladvpanel::TCustomElAdvancedPanel(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElExplorerBar;
class PASCALIMPLEMENTATION TElExplorerBar : public Elscrollbox::TElScrollBox 
{
	typedef Elscrollbox::TElScrollBox inherited;
	
private:
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	
protected:
	int FInRealign;
	bool FUpdated;
	int FMargin;
	int FSpacing;
	int FGroupWidth;
	Elini::TElIniFile* FStorage;
	AnsiString FStoragePath;
	virtual void __fastcall RealignGroups(void);
	MESSAGE void __fastcall CMControlChange(Controls::TCMControlChange &Msg);
	HIDESBASE MESSAGE void __fastcall CMControlListChange(Messages::TMessage &Msg);
	DYNAMIC void __fastcall Resize(void);
	void __fastcall SetMargin(int Value);
	void __fastcall SetSpacing(int Value);
	virtual void __fastcall CreateWnd(void);
	void __fastcall SetGroupWidth(int Value);
	virtual WideString __fastcall GetThemedClassName();
	HIDESBASE MESSAGE void __fastcall WMVScroll(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMHScroll(Messages::TMessage &Message);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall DrawThemedBackground(HDC DC);
	
public:
	__fastcall virtual TElExplorerBar(Classes::TComponent* AOwner);
	TElExplorerBarGroup* __fastcall AddPanel(void);
	void __fastcall BeginUpdate(void);
	void __fastcall EndUpdate(void);
	void __fastcall Restore(void);
	void __fastcall Save(void);
	
__published:
	__property int Margin = {read=FMargin, write=SetMargin, default=4};
	__property int Spacing = {read=FSpacing, write=SetSpacing, default=8};
	__property int GroupWidth = {read=FGroupWidth, write=SetGroupWidth, default=88};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=FStorage};
	__property AnsiString StoragePath = {read=FStoragePath, write=FStoragePath};
public:
	#pragma option push -w-inl
	/* TElScrollBox.Destroy */ inline __fastcall virtual ~TElExplorerBar(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElExplorerBar(HWND ParentWindow) : Elscrollbox::TElScrollBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElExplorerBarGroupCaption;
class PASCALIMPLEMENTATION TElExplorerBarGroupCaption : public Eladvpanel::TElAdvCaptionPanel 
{
	typedef Eladvpanel::TElAdvCaptionPanel inherited;
	
protected:
	virtual void __fastcall DrawThemedBackground(void);
	HIDESBASE MESSAGE void __fastcall WMEraseBkGnd(Messages::TWMEraseBkgnd &Msg);
	virtual WideString __fastcall GetThemedClassName();
public:
	#pragma option push -w-inl
	/* TElAdvCaptionPanel.Create */ inline __fastcall virtual TElExplorerBarGroupCaption(Classes::TComponent* AOwner) : Eladvpanel::TElAdvCaptionPanel(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TCustomElHTMLPanel.Destroy */ inline __fastcall virtual ~TElExplorerBarGroupCaption(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElExplorerBarGroupCaption(HWND ParentWindow) : Eladvpanel::TElAdvCaptionPanel(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elexpbar */
using namespace Elexpbar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElExpBar
