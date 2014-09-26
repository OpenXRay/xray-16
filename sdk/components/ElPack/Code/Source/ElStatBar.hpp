// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElStatBar.pas' rev: 6.00

#ifndef ElStatBarHPP
#define ElStatBarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elstatbar
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElStatusPanelBevel { epbLowered, epbNone, epbRaised };
#pragma option pop

#pragma option push -b-
enum TElStatusPanelStyle { epsText, epsControl, epsOwnerDraw };
#pragma option pop

class DELPHICLASS TElStatusPanel;
class DELPHICLASS TElStatusBar;
typedef void __fastcall (__closure *TElPanelEvent)(System::TObject* Sender, TElStatusPanel* Panel);

class DELPHICLASS TElStatusPanels;
class PASCALIMPLEMENTATION TElStatusPanels : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElStatusPanel* operator[](int Index) { return Items[Index]; }
	
private:
	TElStatusBar* FStatusBar;
	TElStatusPanel* __fastcall GetItems(int Index);
	void __fastcall SetItems(int Index, TElStatusPanel* newValue);
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	
public:
	__fastcall TElStatusPanels(TElStatusBar* StatusBar);
	HIDESBASE TElStatusPanel* __fastcall Add(void);
	__property TElStatusPanel* Items[int Index] = {read=GetItems, write=SetItems/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElStatusPanels(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElStatusBar : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	TElStatusPanelBevel FBevel;
	Elini::TElIniFile* FStorage;
	AnsiString FStoragePath;
	bool FResizablePanels;
	bool FPressed;
	TElStatusPanel* FHintPanel;
	TElStatusPanel* FPressedPanel;
	TElStatusPanel* FResizePanel;
	WideString FSimpleText;
	bool FSimplePanel;
	bool FSizeGrip;
	TElPanelEvent FOnPanelResize;
	TElPanelEvent FOnPanelDraw;
	TElPanelEvent FOnPanelClick;
	TElPanelEvent FOnPanelDblClick;
	TElStatusPanels* FPanels;
	Controls::TCursor FOldCursor;
	int FDelta;
	int FLine;
	bool FLineVis;
	bool FIgnoreSize;
	bool FSimpleTextIsHTML;
	Htmlrender::TElHTMLRender* FRender;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	WideString FHint;
	void __fastcall SetPanels(TElStatusPanels* Value);
	void __fastcall SetSimpleText(WideString newValue);
	void __fastcall SetSimplePanel(bool newValue);
	void __fastcall SetSizeGrip(bool newValue);
	HIDESBASE MESSAGE void __fastcall WMERASEBKGND(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TWMNCHitTest &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Msg);
	void __fastcall IntHintShow(Forms::THintInfo &HintInfo);
	void __fastcall DrawDragLine(bool Restore);
	void __fastcall SetBevel(TElStatusPanelBevel newValue);
	void __fastcall SetSimpleTextIsHTML(bool Value);
	
protected:
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall TriggerPanelResizeEvent(TElStatusPanel* Panel);
	virtual void __fastcall TriggerPanelDrawEvent(TElStatusPanel* Panel);
	virtual void __fastcall TriggerPanelDblClickEvent(TElStatusPanel* Panel);
	virtual void __fastcall TriggerPanelClickEvent(TElStatusPanel* Panel);
	virtual void __fastcall Paint(void);
	virtual void __fastcall DrawPanel(TElStatusPanel* Panel);
	virtual void __fastcall UpdatePanels(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	void __fastcall IntLButtonDown(int X, int Y);
	void __fastcall IntLButtonUp(int X, int Y);
	void __fastcall IntMouseMove(int X, int Y);
	void __fastcall InitDragLine(void);
	void __fastcall DeinitDragLine(void);
	DYNAMIC void __fastcall Resize(void);
	void __fastcall SetHint(WideString Value);
	
public:
	__fastcall virtual TElStatusBar(Classes::TComponent* AOwner);
	__fastcall virtual ~TElStatusBar(void);
	virtual TElStatusPanel* __fastcall PanelAtPoint(const Types::TPoint &Pos);
	__property Canvas ;
	void __fastcall Save(void);
	void __fastcall Restore(void);
	
__published:
	__property TElStatusPanels* Panels = {read=FPanels, write=SetPanels};
	__property WideString SimpleText = {read=FSimpleText, write=SetSimpleText};
	__property bool SimplePanel = {read=FSimplePanel, write=SetSimplePanel, default=1};
	__property bool SimpleTextIsHTML = {read=FSimpleTextIsHTML, write=SetSimpleTextIsHTML, nodefault};
	__property bool SizeGrip = {read=FSizeGrip, write=SetSizeGrip, default=1};
	__property bool ResizablePanels = {read=FResizablePanels, write=FResizablePanels, nodefault};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=FStorage};
	__property AnsiString StoragePath = {read=FStoragePath, write=FStoragePath};
	__property TElStatusPanelBevel Bevel = {read=FBevel, write=SetBevel, nodefault};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property UseXPThemes  = {default=1};
	__property Align  = {default=0};
	__property Color ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Visible  = {default=1};
	__property TElPanelEvent OnPanelResize = {read=FOnPanelResize, write=FOnPanelResize};
	__property TElPanelEvent OnPanelDraw = {read=FOnPanelDraw, write=FOnPanelDraw};
	__property TElPanelEvent OnPanelClick = {read=FOnPanelClick, write=FOnPanelClick};
	__property TElPanelEvent OnPanelDblClick = {read=FOnPanelDblClick, write=FOnPanelDblClick};
	__property Htmlrender::TElHTMLImageNeededEvent OnHTMLImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnResize ;
	__property OnContextPopup ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElStatusBar(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElStatusPanel : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	WideString FHint;
	Classes::TAlignment FAlignment;
	TElStatusPanelBevel FBevel;
	TElStatusPanelStyle FStyle;
	WideString FText;
	int FWidth;
	Controls::TControl* FControl;
	Controls::TWinControl* FOldParent;
	#pragma pack(push, 1)
	Types::TRect FOldBounds;
	#pragma pack(pop)
	
	bool FCtlVisible;
	bool FOldVisible;
	bool FVisible;
	TElStatusBar* FOwner;
	int FSavedWidth;
	bool FResizable;
	bool FAutoSize;
	bool FIsHTML;
	void __fastcall SetAutoSize(bool newValue);
	void __fastcall SetVisible(bool newValue);
	void __fastcall SetAlignment(Classes::TAlignment newValue);
	void __fastcall SetBevel(TElStatusPanelBevel newValue);
	void __fastcall SetStyle(TElStatusPanelStyle newValue);
	void __fastcall SetText(WideString newValue);
	void __fastcall SetWidth(int newValue);
	void __fastcall SetControl(Controls::TControl* newValue);
	int __fastcall GetLeft(void);
	int __fastcall GetRight(void);
	Types::TRect __fastcall GetPanelRect();
	void __fastcall SetIsHTML(bool Value);
	
protected:
	void __fastcall SaveWidth(void);
	void __fastcall RestoreWidth(void);
	virtual void __fastcall UpdateControl(void);
	int __fastcall CurWidth(void);
	int __fastcall CalcAutoSize(void);
	
public:
	__fastcall virtual TElStatusPanel(Classes::TCollection* Collection);
	__fastcall virtual ~TElStatusPanel(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	int __fastcall GetRealWidth(void);
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, nodefault};
	__property int Left = {read=GetLeft, nodefault};
	__property int Right = {read=GetRight, nodefault};
	__property Types::TRect PanelRect = {read=GetPanelRect};
	
__published:
	__property bool Resizable = {read=FResizable, write=FResizable, default=1};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, nodefault};
	__property TElStatusPanelBevel Bevel = {read=FBevel, write=SetBevel, default=0};
	__property TElStatusPanelStyle Style = {read=FStyle, write=SetStyle, default=0};
	__property WideString Text = {read=FText, write=SetText};
	__property int Width = {read=FWidth, write=SetWidth, default=100};
	__property Controls::TControl* Control = {read=FControl, write=SetControl};
	__property bool Visible = {read=FVisible, write=SetVisible, default=1};
	__property WideString Hint = {read=FHint, write=FHint};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, nodefault};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE WideString __fastcall ConvertHintTextW(AnsiString Hint);

}	/* namespace Elstatbar */
using namespace Elstatbar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElStatBar
