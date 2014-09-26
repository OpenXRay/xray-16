// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElPopBtn.pas' rev: 6.00

#ifndef ElPopBtnHPP
#define ElPopBtnHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ActnList.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <CommCtrl.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elpopbtn
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TPullDownEvent)(System::TObject* Sender);

#pragma option push -b-
enum TElButtonState { ebsUp, ebsDisabled, ebsDown, ebsExclusive, ebsArrDown };
#pragma option pop

class DELPHICLASS TElSpeedButton;
class PASCALIMPLEMENTATION TElSpeedButton : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	Graphics::TColor FTransparentColor;
	bool FAutoSize;
	Graphics::TBitmap* FNormalImage;
	Graphics::TBitmap* FDisabledImage;
	Graphics::TBitmap* FMouseInImage;
	Graphics::TBitmap* FPressedImage;
	bool FFlat;
	bool FDrawEdge;
	bool FPressed;
	bool FOver;
	Extctrls::TTimer* FPullTimer;
	Controls::TMouseButton FPullDownBtn;
	int FPullDownInterval;
	bool FPullDownEnabled;
	Menus::TPopupMenu* FPullDownMenu;
	bool FTransparent;
	TPullDownEvent FOnPullDown;
	void __fastcall SetPullDownMenu(Menus::TPopupMenu* newValue);
	void __fastcall SetTransparent(bool newValue);
	void __fastcall SetDrawEdge(bool newValue);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	void __fastcall SetFlat(bool newValue);
	void __fastcall SetNormalImage(Graphics::TBitmap* newValue);
	void __fastcall SetDisabledImage(Graphics::TBitmap* newValue);
	void __fastcall SetMouseInImage(Graphics::TBitmap* newValue);
	void __fastcall SetPressedImage(Graphics::TBitmap* newValue);
	void __fastcall SetTransparentColor(Graphics::TColor newValue);
	MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	
protected:
	virtual void __fastcall SetAutoSize(bool newValue);
	virtual void __fastcall TriggerPullDownEvent(void);
	virtual void __fastcall Paint(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall OnTimer(System::TObject* Sender);
	void __fastcall StartTimer(void);
	void __fastcall PullMenu(void);
	
public:
	__fastcall virtual TElSpeedButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TElSpeedButton(void);
	virtual bool __fastcall InCircle(int X, int Y);
	
__published:
	__property Controls::TMouseButton PullDownBtn = {read=FPullDownBtn, write=FPullDownBtn, nodefault};
	__property int PullDownInterval = {read=FPullDownInterval, write=FPullDownInterval, default=1000};
	__property bool PullDownEnabled = {read=FPullDownEnabled, write=FPullDownEnabled, default=0};
	__property Menus::TPopupMenu* PullDownMenu = {read=FPullDownMenu, write=SetPullDownMenu};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=1};
	__property TPullDownEvent OnPullDown = {read=FOnPullDown, write=FOnPullDown};
	__property bool DrawEdge = {read=FDrawEdge, write=SetDrawEdge, default=0};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property Graphics::TBitmap* NormalImage = {read=FNormalImage, write=SetNormalImage};
	__property Graphics::TBitmap* DisabledImage = {read=FDisabledImage, write=SetDisabledImage};
	__property Graphics::TBitmap* MouseInImage = {read=FMouseInImage, write=SetMouseInImage};
	__property Graphics::TBitmap* PressedImage = {read=FPressedImage, write=SetPressedImage};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=1};
	__property Graphics::TColor TransparentColor = {read=FTransparentColor, write=SetTransparentColor, nodefault};
	__property Align  = {default=0};
	__property Color ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property ParentColor  = {default=1};
	__property Enabled  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDrag ;
	__property OnDragOver ;
	__property OnDragDrop ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
};


#pragma option push -b-
enum TPopupPlace { ppDown, ppRight, ppTop };
#pragma option pop

class DELPHICLASS TCustomElPopupButton;
class DELPHICLASS TElButtonGlyph;
class DELPHICLASS TElGlyphList;
class PASCALIMPLEMENTATION TElGlyphList : public Controls::TImageList 
{
	typedef Controls::TImageList inherited;
	
private:
	Classes::TBits* Used;
	int FCount;
	int __fastcall AllocateIndex(void);
	
public:
	__fastcall TElGlyphList(int AWidth, int AHeight);
	__fastcall virtual ~TElGlyphList(void);
	HIDESBASE int __fastcall AddMasked(Graphics::TBitmap* Image, Graphics::TColor MaskColor);
	HIDESBASE void __fastcall Delete(int Index);
	__property int Count = {read=FCount, nodefault};
public:
	#pragma option push -w-inl
	/* TCustomImageList.Create */ inline __fastcall virtual TElGlyphList(Classes::TComponent* AOwner) : Controls::TImageList(AOwner) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElButtonGlyph : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	Controls::TImageList* FImageList;
	int FImageIndex;
	bool FUseImageList;
	Graphics::TIcon* FIcon;
	bool FUseIcon;
	Graphics::TBitmap* FOriginal;
	TElGlyphList* FGlyphList;
	int FIndexs[5];
	Graphics::TColor FTransparentColor;
	Buttons::TNumGlyphs FNumGlyphs;
	Classes::TNotifyEvent FOnChange;
	bool FStretched;
	int FStrW;
	int FStrH;
	void __fastcall SetImageList(Controls::TImageList* NewValue);
	void __fastcall SetImageIndex(int NewValue);
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall IconChanged(System::TObject* Sender);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	void __fastcall SetNumGlyphs(Buttons::TNumGlyphs Value);
	void __fastcall Repaint(void);
	int __fastcall CreateButtonGlyph(TElButtonState State);
	void __fastcall DrawButtonGlyph(Graphics::TCanvas* Canvas, const Types::TPoint &GlyphPos, TElButtonState State, bool Transparent, Graphics::TColor Color, bool AlphaBlended);
	void __fastcall DrawButtonText(Graphics::TCanvas* Canvas, const WideString Caption, const Types::TRect &TextBounds, TElButtonState State, bool Multiline, bool Active, bool Transparent, Elvclutils::TElTextDrawType TextDrawType, bool UseThemesForText, unsigned Theme, int ThemePart, int ThemeState, bool ShowAccelChar);
	void __fastcall CalcButtonLayout(Graphics::TCanvas* Canvas, const Types::TRect &Client, const Types::TPoint &Offset, const WideString Caption, Buttons::TButtonLayout Layout, int Margin, int Spacing, Types::TPoint &GlyphPos, Types::TRect &TextBounds, bool ShowGlyph, bool ShowText, bool MultiLine, int ArrowWidth, bool UseThemesForText, unsigned Theme, int ThemePart, int ThemeState);
	Types::TRect __fastcall GetGlyphSize();
	void __fastcall SetUseIcon(bool NewValue);
	
protected:
	__property Controls::TImageList* ImageList = {read=FImageList, write=SetImageList};
	__property int ImageIndex = {read=FImageIndex, write=SetImageIndex, nodefault};
	__property bool UseImageList = {read=FUseImageList, write=FUseImageList, nodefault};
	
public:
	__fastcall TElButtonGlyph(void);
	__fastcall virtual ~TElButtonGlyph(void);
	void __fastcall ResetNumGlyphs(void);
	Types::TRect __fastcall Draw(Graphics::TCanvas* Canvas, const Types::TRect &Client, const Types::TPoint &Offset, const WideString Caption, Buttons::TButtonLayout Layout, int Margin, int Spacing, TElButtonState State, TElButtonState GlyphState, bool Transparent, bool Multiline, bool Active, bool ShowGlyph, bool ShowText, int ArrowWidth, Elvclutils::TElTextDrawType TextDrawType, Graphics::TColor Color, bool UseThemesForText, unsigned Theme, int ThemePart, int ThemeState, bool ShowAccelChar, bool ImageIsAlphaBlended);
	void __fastcall GetPaintGlyphSize(const Types::TRect &R, Types::TPoint &Size);
	int __fastcall CalcButtonWidth(Graphics::TCanvas* Canvas, int &MaxHeight, const Types::TPoint &Offset, const WideString Caption, Buttons::TButtonLayout Layout, int Margin, int Spacing, bool ShowGlyph, bool ShowText, bool MultiLine, int ArrowWidth, bool UseThemesForText, unsigned Theme, int ThemePart, int ThemeState);
	__property bool UseIcon = {read=FUseIcon, write=SetUseIcon, nodefault};
	__property Graphics::TIcon* Icon = {read=FIcon};
	__property Graphics::TBitmap* Glyph = {read=FOriginal, write=SetGlyph};
	__property Buttons::TNumGlyphs NumGlyphs = {read=FNumGlyphs, write=SetNumGlyphs, nodefault};
	__property Types::TRect GlyphSize = {read=GetGlyphSize};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
};


class PASCALIMPLEMENTATION TCustomElPopupButton : public Elbtnctl::TElButtonControl 
{
	typedef Elbtnctl::TElButtonControl inherited;
	
private:
	int FNumGlyphs;
	
protected:
	bool FShadowsUseCustom;
	Graphics::TColor FShadowBtnHighlight;
	Graphics::TColor FShadowBtnShadow;
	Graphics::TColor FShadowBtnDkShadow;
	Graphics::TBitmap* FBackground;
	bool FShadowFollowsColor;
	Graphics::TBitmap* FDownBackground;
	bool FBackgroundDrawBorder;
	bool FThinFrame;
	Controls::TImageList* FHotImages;
	Controls::TImageList* FDisabledImages;
	Controls::TImageList* FDownImages;
	Controls::TImageList* FImageList;
	bool FOldStyled;
	bool FUseImageList;
	bool FUseIcon;
	Elsndmap::TElSoundMap* FSoundMap;
	AnsiString FDownSound;
	AnsiString FUpSound;
	AnsiString FClickSound;
	AnsiString FArrowClickSound;
	bool FIsSwitch;
	bool FShowGlyph;
	bool FShowText;
	bool FUseArrow;
	bool FShowFocus;
	bool FMultiLine;
	int FGroupIndex;
	TElButtonGlyph* FGlyph;
	bool FDown;
	bool FArrDown;
	bool FInMenu;
	bool FIgnoreClick;
	bool FDragging;
	bool FAllowAllUp;
	Buttons::TButtonLayout FLayout;
	int FSpacing;
	int FMargin;
	bool FFlat;
	bool FMouseInArrow;
	bool FMouseInControl;
	bool FDisableAp;
	TPopupPlace FPopupPlace;
	bool FDefault;
	bool FCancel;
	bool FActive;
	Controls::TModalResult FModalResult;
	bool FClicksDisabled;
	Imglist::TChangeLink* FChLink;
	Imglist::TChangeLink* FNChLink;
	Imglist::TChangeLink* FDChLink;
	Imglist::TChangeLink* FHChLink;
	Menus::TPopupMenu* FPullDownMenu;
	Classes::TNotifyEvent FOnArrowClick;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	bool FShowBorder;
	bool FAdjustSpaceForGlyph;
	unsigned FArrTheme;
	void __fastcall SetShowBorder(bool newValue);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall SetPullDownMenu(Menus::TPopupMenu* newValue);
	void __fastcall SetDefault(bool Value);
	void __fastcall SetPopupPlace(TPopupPlace Value);
	void __fastcall SetDisableAp(bool Value);
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall UpdateExclusive(void);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	Buttons::TNumGlyphs __fastcall GetNumGlyphs(void);
	void __fastcall SetNumGlyphs(Buttons::TNumGlyphs Value);
	void __fastcall SetDown(bool Value);
	void __fastcall SetAllowAllUp(bool Value);
	void __fastcall SetGroupIndex(int Value);
	void __fastcall SetLayout(Buttons::TButtonLayout Value);
	void __fastcall SetSpacing(int Value);
	void __fastcall SetMargin(int Value);
	void __fastcall UpdateTracking(void);
	void __fastcall IntMouseEnter(void);
	void __fastcall IntMouseLeave(void);
	void __fastcall IntEnabledChanged(void);
	bool __fastcall IntKeyDown(Word &Key, Classes::TShiftState Shift);
	void __fastcall IntKeyUp(Word &Key, Classes::TShiftState Shift);
	void __fastcall IntTextChanged(void);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyUp(Messages::TWMKey &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMButtonPressed(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMDialogKey(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMColorChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Message);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMFocusChanged(Controls::TCMFocusChanged &Message);
	MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Message);
	void __fastcall SetShowFocus(bool newValue);
	void __fastcall SetShowGlyph(bool newValue);
	void __fastcall SetShowText(bool newValue);
	Graphics::TIcon* __fastcall GetIcon(void);
	void __fastcall SetIcon(Graphics::TIcon* newValue);
	void __fastcall SetIsSwitch(bool newValue);
	void __fastcall SetSoundMap(Elsndmap::TElSoundMap* newValue);
	void __fastcall SetImageIndex(int newValue);
	int __fastcall GetImageIndex(void);
	void __fastcall SetUseIcon(bool newValue);
	void __fastcall SetImageList(Controls::TImageList* newValue);
	void __fastcall SetUseImageList(bool newValue);
	bool __fastcall GetUseImageList(void);
	void __fastcall SetOldStyled(bool newValue);
	void __fastcall SetHotImages(Controls::TImageList* newValue);
	void __fastcall SetDownImages(Controls::TImageList* newValue);
	void __fastcall SetDisabledImages(Controls::TImageList* newValue);
	void __fastcall ImagesChanged(System::TObject* Sender);
	void __fastcall SetThinFrame(bool newValue);
	void __fastcall SetBackground(Graphics::TBitmap* newValue);
	void __fastcall SetDownBackground(Graphics::TBitmap* newValue);
	void __fastcall SetBackgroundDrawBorder(bool Value);
	void __fastcall SetShadowFollowsColor(bool Value);
	void __fastcall SetShadowsUseCustom(bool Value);
	void __fastcall SetShadowBtnHighlight(Graphics::TColor Value);
	void __fastcall SetShadowBtnShadow(Graphics::TColor Value);
	void __fastcall SetShadowBtnDkShadow(Graphics::TColor Value);
	void __fastcall SetAdjustSpaceForGlyph(bool Value);
	TElButtonState FOrigState;
	TElButtonState FState;
	bool FDrawDefaultFrame;
	bool FImageIsAlphaBlended;
	virtual void __fastcall SetUseArrow(bool newValue);
	virtual void __fastcall CreateThemeHandle(void);
	virtual void __fastcall FreeThemeHandle(void);
	DYNAMIC HPALETTE __fastcall GetPalette(void);
	virtual void __fastcall Loaded(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Paint(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall SetButtonStyle(bool ADefault);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	virtual void __fastcall SetFlat(bool Value);
	virtual bool __fastcall GetChecked(void);
	virtual void __fastcall SetChecked(bool newValue);
	DYNAMIC TMetaClass* __fastcall GetActionLinkClass(void);
	DYNAMIC void __fastcall ActionChange(System::TObject* Sender, bool CheckDefaults);
	__property bool ClicksDisabled = {read=FClicksDisabled, write=FClicksDisabled, nodefault};
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	void __fastcall SetDrawDefaultFrame(bool Value);
	virtual int __fastcall GetArrowSize(void);
	bool __fastcall DoSaveShadows(void);
	void __fastcall SetImageIsAlphaBlended(bool Value);
	__property Menus::TPopupMenu* PullDownMenu = {read=FPullDownMenu, write=SetPullDownMenu};
	__property TPopupPlace PopupPlace = {read=FPopupPlace, write=SetPopupPlace, default=0};
	__property bool DisableAutoPopup = {read=FDisableAp, write=SetDisableAp, default=0};
	__property bool Cancel = {read=FCancel, write=FCancel, default=0};
	__property bool Default = {read=FDefault, write=SetDefault, default=0};
	__property Controls::TModalResult ModalResult = {read=FModalResult, write=FModalResult, default=0};
	__property bool AllowAllUp = {read=FAllowAllUp, write=SetAllowAllUp, default=0};
	__property int GroupIndex = {read=FGroupIndex, write=SetGroupIndex, default=0};
	__property bool Down = {read=FDown, write=SetDown, default=0};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property Buttons::TButtonLayout Layout = {read=FLayout, write=SetLayout, default=0};
	__property int Margin = {read=FMargin, write=SetMargin, default=-1};
	__property Buttons::TNumGlyphs NumGlyphs = {read=GetNumGlyphs, write=SetNumGlyphs, nodefault};
	__property int Spacing = {read=FSpacing, write=SetSpacing, default=4};
	__property bool ShowFocus = {read=FShowFocus, write=SetShowFocus, default=1};
	__property bool UseArrow = {read=FUseArrow, write=SetUseArrow, default=0};
	__property bool ShadowFollowsColor = {read=FShadowFollowsColor, write=SetShadowFollowsColor, default=1};
	__property bool ShowGlyph = {read=FShowGlyph, write=SetShowGlyph, default=1};
	__property bool ShowText = {read=FShowText, write=SetShowText, default=1};
	__property Classes::TNotifyEvent OnArrowClick = {read=FOnArrowClick, write=FOnArrowClick};
	__property Graphics::TIcon* Icon = {read=GetIcon, write=SetIcon};
	__property bool UseIcon = {read=FUseIcon, write=SetUseIcon, default=0};
	__property bool IsSwitch = {read=FIsSwitch, write=SetIsSwitch, default=0};
	__property AnsiString DownSound = {read=FDownSound, write=FDownSound};
	__property AnsiString UpSound = {read=FUpSound, write=FUpSound};
	__property AnsiString ClickSound = {read=FClickSound, write=FClickSound};
	__property AnsiString ArrowClickSound = {read=FArrowClickSound, write=FArrowClickSound};
	__property Elsndmap::TElSoundMap* SoundMap = {read=FSoundMap, write=SetSoundMap};
	__property int ImageIndex = {read=GetImageIndex, write=SetImageIndex, default=-1};
	__property Controls::TImageList* Images = {read=FImageList, write=SetImageList};
	__property Controls::TImageList* DownImages = {read=FDownImages, write=SetDownImages};
	__property Controls::TImageList* HotImages = {read=FHotImages, write=SetHotImages};
	__property Controls::TImageList* DisabledImages = {read=FDisabledImages, write=SetDisabledImages};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property bool ShowBorder = {read=FShowBorder, write=SetShowBorder, default=1};
	__property bool ShadowsUseCustom = {read=FShadowsUseCustom, write=SetShadowsUseCustom, default=0};
	__property Graphics::TColor ShadowBtnHighlight = {read=FShadowBtnHighlight, write=SetShadowBtnHighlight, stored=DoSaveShadows, nodefault};
	__property Graphics::TColor ShadowBtnShadow = {read=FShadowBtnShadow, write=SetShadowBtnShadow, stored=DoSaveShadows, nodefault};
	__property Graphics::TColor ShadowBtnDkShadow = {read=FShadowBtnDkShadow, write=SetShadowBtnDkShadow, stored=DoSaveShadows, nodefault};
	__property bool UseImageList = {read=GetUseImageList, write=SetUseImageList, default=0};
	__property bool OldStyled = {read=FOldStyled, write=SetOldStyled, default=0};
	__property bool ThinFrame = {read=FThinFrame, write=SetThinFrame, default=0};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property Graphics::TBitmap* DownBackground = {read=FDownBackground, write=SetDownBackground};
	__property bool BackgroundDrawBorder = {read=FBackgroundDrawBorder, write=SetBackgroundDrawBorder, default=0};
	__property bool AdjustSpaceForGlyph = {read=FAdjustSpaceForGlyph, write=SetAdjustSpaceForGlyph, default=1};
	__property bool DrawDefaultFrame = {read=FDrawDefaultFrame, write=SetDrawDefaultFrame, nodefault};
	__property bool ImageIsAlphaBlended = {read=FImageIsAlphaBlended, write=SetImageIsAlphaBlended, default=0};
	
public:
	__fastcall virtual TCustomElPopupButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElPopupButton(void);
	virtual void __fastcall AClick(bool Arrow);
	DYNAMIC void __fastcall Click(void);
	__property bool MouseInControl = {read=FMouseInControl, nodefault};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElPopupButton(HWND ParentWindow) : Elbtnctl::TElButtonControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElPopupButton;
class PASCALIMPLEMENTATION TElPopupButton : public TCustomElPopupButton 
{
	typedef TCustomElPopupButton inherited;
	
protected:
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	
__published:
	__property Background ;
	__property BackgroundDrawBorder  = {default=0};
	__property DownBackground ;
	__property ImageIndex  = {default=-1};
	__property UseImageList  = {default=0};
	__property ImageIsAlphaBlended  = {default=0};
	__property Images ;
	__property HotImages ;
	__property DisabledImages ;
	__property DrawDefaultFrame ;
	__property PullDownMenu ;
	__property PopupPlace  = {default=0};
	__property DisableAutoPopup  = {default=0};
	__property Cancel  = {default=0};
	__property Default  = {default=0};
	__property ModalResult  = {default=0};
	__property MoneyFlat  = {default=0};
	__property MoneyFlatActiveColor ;
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatDownColor ;
	__property AdjustSpaceForGlyph  = {default=1};
	__property AllowAllUp  = {default=0};
	__property GroupIndex  = {default=0};
	__property Down  = {default=0};
	__property Flat  = {default=0};
	__property Glyph ;
	__property ImageForm ;
	__property Layout  = {default=0};
	__property Margin  = {default=-1};
	__property NumGlyphs ;
	__property ShadowFollowsColor  = {default=1};
	__property ShadowsUseCustom  = {default=0};
	__property ShadowBtnHighlight ;
	__property ShadowBtnShadow ;
	__property ShadowBtnDkShadow ;
	__property ShowFocus  = {default=1};
	__property ShowGlyph  = {default=1};
	__property ShowText  = {default=1};
	__property Spacing  = {default=4};
	__property UseArrow  = {default=0};
	__property IsSwitch  = {default=0};
	__property OnArrowClick ;
	__property Icon ;
	__property UseIcon  = {default=0};
	__property ThinFrame  = {default=0};
	__property TextDrawType  = {default=0};
	__property Transparent  = {default=0};
	__property DownSound ;
	__property UpSound ;
	__property ClickSound ;
	__property ArrowClickSound ;
	__property SoundMap ;
	__property DownImages ;
	__property ShowBorder  = {default=1};
	__property OldStyled  = {default=0};
	__property UseXPThemes  = {default=1};
	__property Caption ;
	__property Enabled  = {default=1};
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property PopupMenu ;
	__property Color ;
	__property ParentColor  = {default=1};
	__property Align  = {default=0};
	__property Font ;
	__property HelpContext  = {default=0};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnKeyDown ;
	__property OnKeyUp ;
	__property OnKeyPress ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnStartDrag ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElPopupButton.Create */ inline __fastcall virtual TElPopupButton(Classes::TComponent* AOwner) : TCustomElPopupButton(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElPopupButton.Destroy */ inline __fastcall virtual ~TElPopupButton(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElPopupButton(HWND ParentWindow) : TCustomElPopupButton(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElPopupButtonActionLink;
class PASCALIMPLEMENTATION TElPopupButtonActionLink : public Elbtnctl::TElButtonActionLink 
{
	typedef Elbtnctl::TElButtonActionLink inherited;
	
protected:
	virtual void __fastcall SetImageIndex(int Value);
	virtual void __fastcall SetChecked(bool Value);
public:
	#pragma option push -w-inl
	/* TBasicActionLink.Create */ inline __fastcall virtual TElPopupButtonActionLink(System::TObject* AClient) : Elbtnctl::TElButtonActionLink(AClient) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TBasicActionLink.Destroy */ inline __fastcall virtual ~TElPopupButtonActionLink(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElGlyphCache;
class PASCALIMPLEMENTATION TElGlyphCache : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	Ellist::TElList* GlyphLists;
	
public:
	__fastcall TElGlyphCache(void);
	__fastcall virtual ~TElGlyphCache(void);
	TElGlyphList* __fastcall GetList(int AWidth, int AHeight);
	void __fastcall ReturnList(TElGlyphList* List);
	bool __fastcall Empty(void);
};


class DELPHICLASS TCustomElGraphicButton;
class PASCALIMPLEMENTATION TCustomElGraphicButton : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	int FNumGlyphs;
	Classes::TWndMethod FMenuWindowProc;
	
protected:
	bool FShadowsUseCustom;
	Graphics::TColor FShadowBtnHighlight;
	Graphics::TColor FShadowBtnShadow;
	Graphics::TColor FShadowBtnDkShadow;
	Graphics::TBitmap* FBackground;
	bool FShadowFollowsColor;
	Graphics::TBitmap* FDownBackground;
	bool FBackgroundDrawBorder;
	bool FThinFrame;
	Controls::TImageList* FHotImages;
	Controls::TImageList* FDownImages;
	Controls::TImageList* FDisabledImages;
	Controls::TImageList* FImageList;
	bool FOldStyled;
	bool FUseImageList;
	bool FUseIcon;
	Elsndmap::TElSoundMap* FSoundMap;
	AnsiString FDownSound;
	AnsiString FUpSound;
	AnsiString FClickSound;
	AnsiString FArrowClickSound;
	bool FIsSwitch;
	bool FShowGlyph;
	bool FShowText;
	bool FUseArrow;
	bool FMultiLine;
	int FGroupIndex;
	TElButtonGlyph* FGlyph;
	bool FDown;
	bool FArrDown;
	bool FInMenu;
	bool FIgnoreClick;
	bool FDragging;
	bool FAllowAllUp;
	Buttons::TButtonLayout FLayout;
	int FSpacing;
	int FMargin;
	bool FFlat;
	bool FMouseInArrow;
	bool FMouseInControl;
	bool FDisableAp;
	TPopupPlace FPopupPlace;
	bool FDefault;
	bool FCancel;
	Controls::TModalResult FModalResult;
	bool FClicksDisabled;
	Imglist::TChangeLink* FChLink;
	Imglist::TChangeLink* FNChLink;
	Imglist::TChangeLink* FDChLink;
	Imglist::TChangeLink* FHChLink;
	Menus::TPopupMenu* FPullDownMenu;
	Classes::TNotifyEvent FOnArrowClick;
	bool FTransparent;
	Elvclutils::TElTextDrawType FTextDrawType;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	bool FShowBorder;
	bool FAdjustSpaceForGlyph;
	bool FUseXPThemes;
	unsigned FTheme;
	unsigned FArrTheme;
	HWND FWnd;
	HIDESBASE bool __fastcall IsColorStored(void);
	void __fastcall SetShowBorder(bool newValue);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	virtual void __fastcall SetTransparent(bool newValue);
	void __fastcall SetTextDrawType(Elvclutils::TElTextDrawType newValue);
	void __fastcall SetPullDownMenu(Menus::TPopupMenu* newValue);
	void __fastcall SetPopupPlace(TPopupPlace Value);
	void __fastcall SetDisableAp(bool Value);
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall UpdateExclusive(void);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	Buttons::TNumGlyphs __fastcall GetNumGlyphs(void);
	void __fastcall SetNumGlyphs(Buttons::TNumGlyphs Value);
	void __fastcall SetDown(bool Value);
	void __fastcall SetAllowAllUp(bool Value);
	void __fastcall SetGroupIndex(int Value);
	virtual void __fastcall SetLayout(Buttons::TButtonLayout Value);
	virtual void __fastcall SetSpacing(int Value);
	virtual void __fastcall SetMargin(int Value);
	void __fastcall UpdateTracking(void);
	void __fastcall IntMouseEnter(void);
	void __fastcall IntMouseLeave(void);
	void __fastcall IntEnabledChanged(void);
	void __fastcall IntTextChanged(void);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMButtonPressed(Messages::TMessage &Message);
	MESSAGE void __fastcall CMDialogKey(Messages::TWMKey &Message);
	MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMColorChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	virtual void __fastcall SetShowGlyph(bool newValue);
	virtual void __fastcall SetShowText(bool newValue);
	Graphics::TIcon* __fastcall GetIcon(void);
	void __fastcall SetIcon(Graphics::TIcon* newValue);
	void __fastcall SetIsSwitch(bool newValue);
	void __fastcall SetSoundMap(Elsndmap::TElSoundMap* newValue);
	virtual void __fastcall SetImageIndex(int newValue);
	int __fastcall GetImageIndex(void);
	virtual void __fastcall SetUseIcon(bool newValue);
	virtual void __fastcall SetImageList(Controls::TImageList* newValue);
	virtual void __fastcall SetUseImageList(bool newValue);
	bool __fastcall GetUseImageList(void);
	void __fastcall SetOldStyled(bool newValue);
	void __fastcall SetDownImages(Controls::TImageList* newValue);
	void __fastcall SetHotImages(Controls::TImageList* newValue);
	void __fastcall SetDisabledImages(Controls::TImageList* newValue);
	virtual void __fastcall ImagesChanged(System::TObject* Sender);
	void __fastcall SetThinFrame(bool newValue);
	void __fastcall SetBackground(Graphics::TBitmap* newValue);
	void __fastcall SetDownBackground(Graphics::TBitmap* newValue);
	void __fastcall SetBackgroundDrawBorder(bool Value);
	void __fastcall SetShadowFollowsColor(bool Value);
	void __fastcall SetShadowsUseCustom(bool Value);
	void __fastcall SetShadowBtnHighlight(Graphics::TColor Value);
	void __fastcall SetShadowBtnShadow(Graphics::TColor Value);
	void __fastcall SetShadowBtnDkShadow(Graphics::TColor Value);
	void __fastcall SetAdjustSpaceForGlyph(bool Value);
	void __fastcall SetUseXPThemes(const bool Value);
	void __fastcall CreateThemeHandle(void);
	void __fastcall FreeThemeHandle(void);
	TElButtonState FOrigState;
	TElButtonState FState;
	WideString FCaption;
	WideString FHint;
	bool FMoneyFlat;
	Graphics::TColor FMoneyFlatDownColor;
	Graphics::TColor FMoneyFlatActiveColor;
	Graphics::TColor FMoneyFlatInactiveColor;
	bool FShortcutsEnabled;
	bool FImageIsAlphaBlended;
	virtual void __fastcall SetUseArrow(bool newValue);
	DYNAMIC HPALETTE __fastcall GetPalette(void);
	virtual void __fastcall Loaded(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Paint(void);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	virtual void __fastcall SetFlat(bool Value);
	virtual bool __fastcall GetChecked(void);
	virtual void __fastcall SetChecked(bool newValue);
	DYNAMIC TMetaClass* __fastcall GetActionLinkClass(void);
	DYNAMIC void __fastcall ActionChange(System::TObject* Sender, bool CheckDefaults);
	bool __fastcall DoSaveShadows(void);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	void __fastcall SetCaption(WideString Value);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	virtual int __fastcall GetThemePartID(void);
	virtual int __fastcall GetThemeStateID(void);
	virtual WideString __fastcall GetThemedClassName();
	virtual int __fastcall GetArrowThemePartID(void);
	virtual int __fastcall GetArrowThemeStateID(void);
	virtual WideString __fastcall GetArrowThemedClassName();
	virtual int __fastcall GetArrowSize(void);
	virtual void __fastcall DrawThemedBackground(Graphics::TCanvas* Canvas);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TMessage &Message);
	Types::TPoint __fastcall MeasureButton(bool LockHeight);
	void __fastcall SetMoneyFlat(bool Value);
	void __fastcall SetMoneyFlatDownColor(Graphics::TColor Value);
	void __fastcall SetMoneyFlatActiveColor(Graphics::TColor Value);
	void __fastcall SetMoneyFlatInactiveColor(Graphics::TColor Value);
	bool __fastcall GetMoneyFlat(void);
	void __fastcall IntWndProc(Messages::TMessage &Message);
	virtual void __fastcall DoPullMenu(void);
	void __fastcall SetShortcutsEnabled(bool Value);
	virtual bool __fastcall Focused(void);
	void __fastcall SetImageIsAlphaBlended(bool Value);
	void __fastcall SetHint(WideString Value);
	__property bool ClicksDisabled = {read=FClicksDisabled, write=FClicksDisabled, nodefault};
	__property Menus::TPopupMenu* PullDownMenu = {read=FPullDownMenu, write=SetPullDownMenu};
	__property TPopupPlace PopupPlace = {read=FPopupPlace, write=SetPopupPlace, default=0};
	__property bool DisableAutoPopup = {read=FDisableAp, write=SetDisableAp, default=0};
	__property bool Cancel = {read=FCancel, write=FCancel, default=0};
	__property Controls::TModalResult ModalResult = {read=FModalResult, write=FModalResult, default=0};
	__property bool AllowAllUp = {read=FAllowAllUp, write=SetAllowAllUp, default=0};
	__property int GroupIndex = {read=FGroupIndex, write=SetGroupIndex, default=0};
	__property bool Down = {read=FDown, write=SetDown, default=0};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property Buttons::TButtonLayout Layout = {read=FLayout, write=SetLayout, default=0};
	__property int Margin = {read=FMargin, write=SetMargin, default=-1};
	__property Buttons::TNumGlyphs NumGlyphs = {read=GetNumGlyphs, write=SetNumGlyphs, nodefault};
	__property int Spacing = {read=FSpacing, write=SetSpacing, default=4};
	__property bool UseArrow = {read=FUseArrow, write=SetUseArrow, default=0};
	__property bool ShadowFollowsColor = {read=FShadowFollowsColor, write=SetShadowFollowsColor, nodefault};
	__property bool ShowGlyph = {read=FShowGlyph, write=SetShowGlyph, default=1};
	__property bool ShowText = {read=FShowText, write=SetShowText, default=1};
	__property Classes::TNotifyEvent OnArrowClick = {read=FOnArrowClick, write=FOnArrowClick};
	__property Graphics::TIcon* Icon = {read=GetIcon, write=SetIcon};
	__property bool UseIcon = {read=FUseIcon, write=SetUseIcon, default=0};
	__property bool IsSwitch = {read=FIsSwitch, write=SetIsSwitch, default=0};
	__property AnsiString DownSound = {read=FDownSound, write=FDownSound};
	__property AnsiString UpSound = {read=FUpSound, write=FUpSound};
	__property AnsiString ClickSound = {read=FClickSound, write=FClickSound};
	__property AnsiString ArrowClickSound = {read=FArrowClickSound, write=FArrowClickSound};
	__property Elsndmap::TElSoundMap* SoundMap = {read=FSoundMap, write=SetSoundMap};
	__property int ImageIndex = {read=GetImageIndex, write=SetImageIndex, default=-1};
	__property Controls::TImageList* Images = {read=FImageList, write=SetImageList};
	__property Controls::TImageList* HotImages = {read=FHotImages, write=SetHotImages};
	__property Controls::TImageList* DisabledImages = {read=FDisabledImages, write=SetDisabledImages};
	__property Controls::TImageList* DownImages = {read=FDownImages, write=SetDownImages};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property bool ShowBorder = {read=FShowBorder, write=SetShowBorder, default=1};
	__property bool ShadowsUseCustom = {read=FShadowsUseCustom, write=SetShadowsUseCustom, default=0};
	__property Graphics::TColor ShadowBtnHighlight = {read=FShadowBtnHighlight, write=SetShadowBtnHighlight, stored=DoSaveShadows, default=16250869};
	__property Graphics::TColor ShadowBtnShadow = {read=FShadowBtnShadow, write=SetShadowBtnShadow, stored=DoSaveShadows, default=7764576};
	__property Graphics::TColor ShadowBtnDkShadow = {read=FShadowBtnDkShadow, write=SetShadowBtnDkShadow, stored=DoSaveShadows, default=5856328};
	__property bool UseImageList = {read=GetUseImageList, write=SetUseImageList, default=0};
	__property bool OldStyled = {read=FOldStyled, write=SetOldStyled, default=0};
	__property bool ThinFrame = {read=FThinFrame, write=SetThinFrame, default=0};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property Graphics::TBitmap* DownBackground = {read=FDownBackground, write=SetDownBackground};
	__property bool BackgroundDrawBorder = {read=FBackgroundDrawBorder, write=SetBackgroundDrawBorder, default=0};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property Elvclutils::TElTextDrawType TextDrawType = {read=FTextDrawType, write=SetTextDrawType, default=0};
	__property bool Checked = {read=GetChecked, write=SetChecked, default=0};
	__property Color  = {stored=IsColorStored};
	__property bool AdjustSpaceForGlyph = {read=FAdjustSpaceForGlyph, write=SetAdjustSpaceForGlyph, default=1};
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property bool MoneyFlat = {read=GetMoneyFlat, write=SetMoneyFlat, default=0};
	__property Graphics::TColor MoneyFlatDownColor = {read=FMoneyFlatDownColor, write=SetMoneyFlatDownColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatActiveColor = {read=FMoneyFlatActiveColor, write=SetMoneyFlatActiveColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatInactiveColor = {read=FMoneyFlatInactiveColor, write=SetMoneyFlatInactiveColor, stored=GetMoneyFlat, nodefault};
	__property bool ImageIsAlphaBlended = {read=FImageIsAlphaBlended, write=SetImageIsAlphaBlended, default=0};
	
public:
	__fastcall virtual TCustomElGraphicButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElGraphicButton(void);
	virtual void __fastcall AClick(bool Arrow);
	bool __fastcall IsThemeApplied(void);
	DYNAMIC void __fastcall Click(void);
	__property bool MouseInControl = {read=FMouseInControl, nodefault};
	__property unsigned Theme = {read=FTheme, nodefault};
	__property Classes::TWndMethod MenuWindowProc = {read=FMenuWindowProc, write=FMenuWindowProc};
	__property bool ShortcutsEnabled = {read=FShortcutsEnabled, write=SetShortcutsEnabled, default=0};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
};


typedef TMetaClass*TCustomElGraphicButtonClass;

class DELPHICLASS TElGraphicButton;
class PASCALIMPLEMENTATION TElGraphicButton : public TCustomElGraphicButton 
{
	typedef TCustomElGraphicButton inherited;
	
__published:
	__property Background ;
	__property BackgroundDrawBorder  = {default=0};
	__property DownBackground ;
	__property ImageIsAlphaBlended  = {default=0};
	__property ImageIndex  = {default=-1};
	__property UseImageList  = {default=0};
	__property Images ;
	__property HotImages ;
	__property DisabledImages ;
	__property PullDownMenu ;
	__property PopupPlace  = {default=0};
	__property DisableAutoPopup  = {default=0};
	__property Cancel  = {default=0};
	__property ModalResult  = {default=0};
	__property MoneyFlat  = {default=0};
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatActiveColor ;
	__property MoneyFlatDownColor ;
	__property AdjustSpaceForGlyph  = {default=1};
	__property AllowAllUp  = {default=0};
	__property GroupIndex  = {default=0};
	__property Down  = {default=0};
	__property Flat  = {default=0};
	__property Glyph ;
	__property ImageForm ;
	__property Layout  = {default=0};
	__property Margin  = {default=-1};
	__property NumGlyphs ;
	__property ShadowFollowsColor ;
	__property ShadowsUseCustom  = {default=0};
	__property ShadowBtnHighlight  = {default=16250869};
	__property ShadowBtnShadow  = {default=7764576};
	__property ShadowBtnDkShadow  = {default=5856328};
	__property ShowGlyph  = {default=1};
	__property ShowText  = {default=1};
	__property Spacing  = {default=4};
	__property UseArrow  = {default=0};
	__property IsSwitch  = {default=0};
	__property OnArrowClick ;
	__property Icon ;
	__property UseIcon  = {default=0};
	__property ThinFrame  = {default=0};
	__property TextDrawType  = {default=0};
	__property Transparent  = {default=0};
	__property DownSound ;
	__property UpSound ;
	__property ClickSound ;
	__property ArrowClickSound ;
	__property SoundMap ;
	__property DownImages ;
	__property ShowBorder  = {default=1};
	__property ShortcutsEnabled  = {default=0};
	__property OldStyled  = {default=0};
	__property UseXPThemes  = {default=1};
	__property Caption ;
	__property Enabled  = {default=1};
	__property PopupMenu ;
	__property Color ;
	__property ParentColor  = {default=1};
	__property Align  = {default=0};
	__property Font ;
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnStartDrag ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElGraphicButton.Create */ inline __fastcall virtual TElGraphicButton(Classes::TComponent* AOwner) : TCustomElGraphicButton(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElGraphicButton.Destroy */ inline __fastcall virtual ~TElGraphicButton(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElGraphicButtonActionLink;
class PASCALIMPLEMENTATION TElGraphicButtonActionLink : public Controls::TControlActionLink 
{
	typedef Controls::TControlActionLink inherited;
	
protected:
	TCustomElGraphicButton* FClient;
	virtual void __fastcall AssignClient(System::TObject* AClient);
	virtual bool __fastcall IsCheckedLinked(void);
	virtual bool __fastcall IsImageIndexLinked(void);
	virtual void __fastcall SetImageIndex(int Value);
	virtual void __fastcall SetChecked(bool Value);
	virtual void __fastcall SetCaption(const AnsiString Value);
	virtual void __fastcall SetHint(const AnsiString Value);
public:
	#pragma option push -w-inl
	/* TBasicActionLink.Create */ inline __fastcall virtual TElGraphicButtonActionLink(System::TObject* AClient) : Controls::TControlActionLink(AClient) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TBasicActionLink.Destroy */ inline __fastcall virtual ~TElGraphicButtonActionLink(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TElGlyphCache* GlyphCache;
extern PACKAGE Graphics::TBitmap* Pattern;
extern PACKAGE int ButtonCount;
extern PACKAGE int MenuCancelMsg;
extern PACKAGE HMENU __fastcall GetMenuHandle(Menus::TMenu* AMenu);
extern PACKAGE void __fastcall CreateBrushPattern(void);

}	/* namespace Elpopbtn */
using namespace Elpopbtn;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElPopBtn
