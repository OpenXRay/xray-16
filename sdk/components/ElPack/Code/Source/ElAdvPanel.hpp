// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElAdvPanel.pas' rev: 6.00

#ifndef ElAdvPanelHPP
#define ElAdvPanelHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <HTMLRender.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElHTMLPanel.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eladvpanel
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElPanelCaptionSettings;
class DELPHICLASS TCustomElAdvancedPanel;
class DELPHICLASS TElAdvCaptionButton;
class DELPHICLASS TElAdvCaptionPanel;
class PASCALIMPLEMENTATION TCustomElAdvancedPanel : public Elhtmlpanel::TCustomElHTMLPanel 
{
	typedef Elhtmlpanel::TCustomElHTMLPanel inherited;
	
private:
	void __fastcall SetCaptionSettings(TElPanelCaptionSettings* Value);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TMessage &Msg);
	
protected:
	Graphics::TBitmap* FMinButtonGlyph;
	Controls::TImageList* FImages;
	int FSaveHeight;
	bool FMinimized;
	Elhtmlpanel::TElHTMLPanel* FCaptionPanel;
	TElPanelCaptionSettings* FCaptionSettings;
	Elpopbtn::TElGraphicButton* FMinButton;
	Elpopbtn::TElGraphicButton* FCloseButton;
	Classes::TNotifyEvent FOnMinimize;
	Classes::TNotifyEvent FOnRestore;
	Classes::TNotifyEvent FOnClose;
	virtual void __fastcall SetLinkPopupMenu(Menus::TPopupMenu* newValue);
	virtual void __fastcall SetLinkColor(Graphics::TColor newValue);
	virtual void __fastcall SetLinkStyle(Graphics::TFontStyles newValue);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TMessage &Message);
	virtual void __fastcall TriggerMinimizeEvent(void);
	virtual void __fastcall TriggerRestoreEvent(void);
	void __fastcall AdjustButtonSize(void);
	void __fastcall OnCaptionSize(System::TObject* Sender);
	void __fastcall OnMinButtonClick(System::TObject* Sender);
	void __fastcall OnCloseButtonClick(System::TObject* Sender);
	virtual void __fastcall TriggerCloseEvent(void);
	virtual void __fastcall AdjustClientRect(Types::TRect &Rect);
	virtual void __fastcall CreateWnd(void);
	void __fastcall LinkClickEventTransfer(System::TObject* Sender, WideString HRef);
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual void __fastcall AdjustInnerSize(Types::TRect &R);
	int __fastcall GetBevelAdjustment(void);
	void __fastcall UpdateMinButtonImages(void);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	void __fastcall ReadExpHeight(Classes::TReader* Reader);
	void __fastcall WriteExpHeight(Classes::TWriter* Writer);
	virtual void __fastcall SetTransparentXPThemes(bool Value);
	virtual TElAdvCaptionButton* __fastcall CreateButton(void);
	virtual void __fastcall SetMinimized(bool Value);
	virtual TElAdvCaptionPanel* __fastcall CreatePanel(void);
	virtual int __fastcall GetThemePartID(void);
	virtual void __fastcall UpdateInterior(void);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Message);
	__property bool Minimized = {read=FMinimized, write=SetMinimized, default=0};
	__property TElPanelCaptionSettings* CaptionSettings = {read=FCaptionSettings, write=SetCaptionSettings};
	__property Classes::TNotifyEvent OnMinimize = {read=FOnMinimize, write=FOnMinimize};
	__property Classes::TNotifyEvent OnRestore = {read=FOnRestore, write=FOnRestore};
	__property Classes::TNotifyEvent OnClose = {read=FOnClose, write=FOnClose};
	
public:
	__fastcall virtual TCustomElAdvancedPanel(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElAdvancedPanel(void);
	virtual int __fastcall GetCaptionHeight(void);
	virtual int __fastcall GetButtonWidth(void);
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElAdvancedPanel(HWND ParentWindow) : Elhtmlpanel::TCustomElHTMLPanel(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElPanelCaptionSettings : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
private:
	bool FVisible;
	int FHeight;
	bool FShowCloseButton;
	TCustomElAdvancedPanel* FOwner;
	bool FShowMinButton;
	bool FFlatButtons;
	int FButtonWidth;
	Classes::TAlignment FAlignment;
	Stdctrls::TTextLayout FLayout;
	Graphics::TColor FButtonColor;
	void __fastcall SetText(WideString Value);
	void __fastcall SetVisible(bool Value);
	void __fastcall SetHeight(int Value);
	void __fastcall SetShowCloseButton(bool Value);
	void __fastcall SetShowMinButton(bool Value);
	void __fastcall SetFlatButtons(bool Value);
	WideString __fastcall GetText();
	void __fastcall SetButtonWidth(int Value);
	void __fastcall SetAlignment(Classes::TAlignment Value);
	void __fastcall SetLayout(Stdctrls::TTextLayout newValue);
	Graphics::TFont* __fastcall GetFont(void);
	void __fastcall SetFont(Graphics::TFont* Value);
	bool __fastcall GetParentFont(void);
	void __fastcall SetParentFont(bool Value);
	Graphics::TColor __fastcall GetColor(void);
	void __fastcall SetColor(Graphics::TColor Value);
	void __fastcall SetButtonColor(Graphics::TColor Value);
	Elimgfrm::TElImageForm* __fastcall GetImageForm(void);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* Value);
	
protected:
	bool FInvertMinButtonArrows;
	bool FAutoSize;
	Graphics::TBitmap* __fastcall GetMinButtonGlyph(void);
	void __fastcall SetMinButtonGlyph(Graphics::TBitmap* Value);
	Graphics::TBitmap* __fastcall GetCloseButtonGlyph(void);
	void __fastcall SetCloseButtonGlyph(Graphics::TBitmap* Value);
	void __fastcall SetInvertMinButtonArrows(bool Value);
	void __fastcall SetAutoSize(bool Value);
	void __fastcall FontChanged(void);
	void __fastcall AdjustHeight(void);
	
public:
	__fastcall TElPanelCaptionSettings(TCustomElAdvancedPanel* Owner);
	
__published:
	__property WideString Text = {read=GetText, write=SetText};
	__property bool Visible = {read=FVisible, write=SetVisible, default=1};
	__property int Height = {read=FHeight, write=SetHeight, default=19};
	__property bool ShowCloseButton = {read=FShowCloseButton, write=SetShowCloseButton, nodefault};
	__property bool ShowMinButton = {read=FShowMinButton, write=SetShowMinButton, default=1};
	__property bool FlatButtons = {read=FFlatButtons, write=SetFlatButtons, default=1};
	__property int ButtonWidth = {read=FButtonWidth, write=SetButtonWidth, default=15};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, default=2};
	__property Stdctrls::TTextLayout Layout = {read=FLayout, write=SetLayout, default=1};
	__property Graphics::TFont* Font = {read=GetFont, write=SetFont};
	__property bool ParentFont = {read=GetParentFont, write=SetParentFont, nodefault};
	__property Graphics::TColor Color = {read=GetColor, write=SetColor, nodefault};
	__property Graphics::TColor ButtonColor = {read=FButtonColor, write=SetButtonColor, default=-2147483633};
	__property Elimgfrm::TElImageForm* ImageForm = {read=GetImageForm, write=SetImageForm};
	__property Graphics::TBitmap* MinButtonGlyph = {read=GetMinButtonGlyph, write=SetMinButtonGlyph};
	__property Graphics::TBitmap* CloseButtonGlyph = {read=GetCloseButtonGlyph, write=SetCloseButtonGlyph};
	__property bool InvertMinButtonArrows = {read=FInvertMinButtonArrows, write=SetInvertMinButtonArrows, default=0};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, nodefault};
public:
	#pragma option push -w-inl
	/* TPersistent.Destroy */ inline __fastcall virtual ~TElPanelCaptionSettings(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElAdvancedPanel;
class PASCALIMPLEMENTATION TElAdvancedPanel : public TCustomElAdvancedPanel 
{
	typedef TCustomElAdvancedPanel inherited;
	
__published:
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
	__property TopGrabHandle ;
	__property RightGrabHandle ;
	__property LeftGrabHandle ;
	__property BottomGrabHandle ;
	__property Resizable  = {default=0};
	__property Movable  = {default=0};
	__property OnMove ;
	__property Align ;
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
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property TransparentXPThemes  = {default=1};
	__property UseXPThemes  = {default=1};
	__property Visible  = {default=1};
	__property SizeGrip  = {default=0};
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
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
	__property Minimized  = {default=0};
	__property CaptionSettings ;
	__property OnMinimize ;
	__property OnRestore ;
	__property OnClose ;
public:
	#pragma option push -w-inl
	/* TCustomElAdvancedPanel.Create */ inline __fastcall virtual TElAdvancedPanel(Classes::TComponent* AOwner) : TCustomElAdvancedPanel(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElAdvancedPanel.Destroy */ inline __fastcall virtual ~TElAdvancedPanel(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElAdvancedPanel(HWND ParentWindow) : TCustomElAdvancedPanel(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElAdvCaptionPanel : public Elhtmlpanel::TElHTMLPanel 
{
	typedef Elhtmlpanel::TElHTMLPanel inherited;
	
private:
	int FDummyInt;
	bool FDummyBool;
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TMessage &Msg);
	
protected:
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall AdjustClientRect(Types::TRect &Rect);
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Message);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	
public:
	__fastcall virtual TElAdvCaptionPanel(Classes::TComponent* AOwner);
	
__published:
	__property bool TabStop = {write=FDummyBool, nodefault};
	__property int TabOrder = {write=FDummyInt, nodefault};
public:
	#pragma option push -w-inl
	/* TCustomElHTMLPanel.Destroy */ inline __fastcall virtual ~TElAdvCaptionPanel(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElAdvCaptionPanel(HWND ParentWindow) : Elhtmlpanel::TElHTMLPanel(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElAdvCaptionButton : public Elpopbtn::TElGraphicButton 
{
	typedef Elpopbtn::TElGraphicButton inherited;
	
protected:
	virtual void __fastcall DrawThemedBackground(Graphics::TCanvas* Canvas);
	
public:
	__fastcall virtual TElAdvCaptionButton(Classes::TComponent* AOwner);
public:
	#pragma option push -w-inl
	/* TCustomElGraphicButton.Destroy */ inline __fastcall virtual ~TElAdvCaptionButton(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eladvpanel */
using namespace Eladvpanel;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElAdvPanel
