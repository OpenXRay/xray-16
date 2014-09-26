// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElToolbar.pas' rev: 6.00

#ifndef ElToolbarHPP
#define ElToolbarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElStrToken.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ActnList.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltoolbar
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElBarOrientation { eboHorz, eboVert };
#pragma option pop

#pragma option push -b-
enum TElToolButtonType { ebtButton, ebtSeparator, ebtDivider };
#pragma option pop

class DELPHICLASS TCustomElToolButton;
class PASCALIMPLEMENTATION TCustomElToolButton : public Elpopbtn::TCustomElGraphicButton 
{
	typedef Elpopbtn::TCustomElGraphicButton inherited;
	
private:
	void *FLargeGlyph;
	void *FGlyph;
	TElToolButtonType FButtonType;
	bool FWrap;
	bool ActionVisibleInverted;
	bool FSettingVisible;
	bool FRealVisible;
	bool FFakeBoolProp;
	int FFakeIntProp;
	Classes::TNotifyEvent FFakeNotifyEvent;
	Controls::TBevelKind FFakeBevelKind;
	int FButtonID;
	bool FOwnerSettings;
	void __fastcall SetWrap(bool newValue);
	void __fastcall SetButtonType(TElToolButtonType newValue);
	void __fastcall SetLargeGlyph(Graphics::TBitmap* newValue);
	void __fastcall SetNumLargeGlyphs(int newValue);
	HIDESBASE void __fastcall SetGlyph(Graphics::TBitmap* newValue);
	HIDESBASE void __fastcall SetNumGlyphs(int newValue);
	HIDESBASE int __fastcall GetNumGlyphs(void);
	int __fastcall GetNumLargeGlyphs(void);
	HIDESBASE Graphics::TBitmap* __fastcall GetGlyph(void);
	Graphics::TBitmap* __fastcall GetLargeGlyph(void);
	HIDESBASE void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall LargeGlyphChanged(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	
protected:
	virtual void __fastcall SetUseArrow(bool newValue);
	void __fastcall SwitchGlyphs(bool ToLarge);
	virtual void __fastcall SetFlat(bool Value);
	virtual void __fastcall SetParent(Controls::TWinControl* AParent);
	virtual void __fastcall Paint(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	virtual int __fastcall GetThemePartID(void);
	virtual int __fastcall GetThemeStateID(void);
	virtual WideString __fastcall GetThemedClassName();
	virtual int __fastcall GetArrowThemePartID(void);
	virtual int __fastcall GetArrowThemeStateID(void);
	virtual WideString __fastcall GetArrowThemedClassName();
	virtual void __fastcall DrawThemedBackground(Graphics::TCanvas* Canvas);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	void __fastcall ReadButtonID(Classes::TReader* Reader);
	void __fastcall WriteButtonID(Classes::TWriter* Writer);
	HIDESBASE MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMVisibleChanged(Messages::TMessage &Message);
	virtual void __fastcall ImagesChanged(System::TObject* Sender);
	virtual void __fastcall SetLayout(Buttons::TButtonLayout Value);
	virtual void __fastcall SetMargin(int Value);
	virtual void __fastcall SetShowGlyph(bool newValue);
	virtual void __fastcall SetShowText(bool newValue);
	virtual void __fastcall SetSpacing(int Value);
	DYNAMIC TMetaClass* __fastcall GetActionLinkClass(void);
	virtual void __fastcall Loaded(void);
	void __fastcall SetOwnerSettings(bool Value);
	__property Transparent  = {default=0};
	__property bool Wrap = {read=FWrap, write=SetWrap, nodefault};
	__property TElToolButtonType ButtonType = {read=FButtonType, write=SetButtonType, nodefault};
	__property Graphics::TBitmap* LargeGlyph = {read=GetLargeGlyph, write=SetLargeGlyph};
	__property int NumLargeGlyphs = {read=GetNumLargeGlyphs, write=SetNumLargeGlyphs, nodefault};
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property int NumGlyphs = {read=GetNumGlyphs, write=SetNumGlyphs, nodefault};
	__property bool OwnerSettings = {read=FOwnerSettings, write=SetOwnerSettings, default=1};
	__property bool Default = {read=FFakeBoolProp, write=FFakeBoolProp, stored=false, nodefault};
	__property bool ShowFocus = {read=FFakeBoolProp, write=FFakeBoolProp, stored=false, nodefault};
	__property bool TabStop = {read=FFakeBoolProp, write=FFakeBoolProp, stored=false, nodefault};
	__property int TabOrder = {read=FFakeIntProp, write=FFakeIntProp, stored=false, nodefault};
	__property Classes::TNotifyEvent OnEnter = {read=FFakeNotifyEvent, write=FFakeNotifyEvent, stored=false};
	__property Classes::TNotifyEvent OnExit = {read=FFakeNotifyEvent, write=FFakeNotifyEvent, stored=false};
	
public:
	__fastcall virtual TCustomElToolButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElToolButton(void);
	__property bool RealVisible = {read=FRealVisible, nodefault};
	
__published:
	__property Controls::TBevelKind BevelKind = {read=FFakeBevelKind, write=FFakeBevelKind, stored=false, nodefault};
	__property bool DoubleBuffered = {read=FFakeBoolProp, write=FFakeBoolProp, stored=false, nodefault};
};


class DELPHICLASS TElToolButton;
class PASCALIMPLEMENTATION TElToolButton : public TCustomElToolButton 
{
	typedef TCustomElToolButton inherited;
	
__published:
	__property Wrap ;
	__property ButtonType ;
	__property LargeGlyph ;
	__property NumLargeGlyphs ;
	__property Glyph ;
	__property NumGlyphs ;
	__property OwnerSettings  = {default=1};
	__property AdjustSpaceForGlyph  = {default=1};
	__property PullDownMenu ;
	__property PopupPlace  = {default=0};
	__property DisableAutoPopup  = {default=0};
	__property Cancel  = {default=0};
	__property ModalResult  = {default=0};
	__property AllowAllUp  = {default=0};
	__property GroupIndex  = {default=0};
	__property Down  = {default=0};
	__property Flat  = {default=0};
	__property Layout  = {default=0};
	__property Margin  = {default=-1};
	__property Spacing  = {default=4};
	__property UseArrow  = {default=0};
	__property ShadowFollowsColor ;
	__property ShowGlyph  = {default=1};
	__property ShowText  = {default=1};
	__property OnArrowClick ;
	__property Icon ;
	__property ImageIsAlphaBlended  = {default=0};
	__property IsSwitch  = {default=0};
	__property TextDrawType  = {default=0};
	__property ThinFrame  = {default=0};
	__property DownSound ;
	__property UpSound ;
	__property ClickSound ;
	__property ArrowClickSound ;
	__property SoundMap ;
	__property UseIcon  = {default=0};
	__property ImageIndex  = {default=-1};
	__property UseImageList  = {default=0};
	__property OldStyled  = {default=0};
	__property Background ;
	__property DownBackground ;
	__property BackgroundDrawBorder  = {default=0};
	__property Transparent  = {default=0};
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
	__property Action ;
	__property Constraints ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElToolButton.Create */ inline __fastcall virtual TElToolButton(Classes::TComponent* AOwner) : TCustomElToolButton(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElToolButton.Destroy */ inline __fastcall virtual ~TElToolButton(void) { }
	#pragma option pop
	
};


typedef TMetaClass*TElToolButtonClass;

class DELPHICLASS TElToolBar;
class PASCALIMPLEMENTATION TElToolBar : public Elpanel::TElPanel 
{
	typedef Elpanel::TElPanel inherited;
	
private:
	bool FNoReAlign;
	bool FShowMoreMenu;
	bool FTransparentButtons;
	bool FUseImageList;
	Controls::TImageList* FImages;
	Controls::TImageList* FHotImages;
	Controls::TImageList* FDisabledImages;
	int FUpdateCount;
	int FUpdatingButtons;
	bool FUseLargeGlyphs;
	bool FHidden;
	bool FHideable;
	TElBarOrientation FOrientation;
	Graphics::TColor FButtonColor;
	int FMinSize;
	bool FAutoSize;
	bool FFlat;
	int FLargeBtnWidth;
	int FLargeBtnHeight;
	Buttons::TButtonLayout FGlyphLayout;
	int FSpacing;
	int FMargin;
	bool FShowGlyph;
	bool FShowCaption;
	bool FLargeSize;
	int FBtnWidth;
	int FBtnHeight;
	int FBtnOffsHorz;
	int FBtnOffsVert;
	bool FAutoWrap;
	bool FCreating;
	Controls::TAlign FSaveAlign;
	AnsiString FDummy;
	Elimgfrm::TElImageForm* FButtonImageForm;
	bool FMouseInControl;
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCLButtonDown(Messages::TWMNCHitMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkGnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	
protected:
	bool FThinButtons;
	Elini::TElIniFile* FStorage;
	AnsiString FStoragePath;
	bool FAdjustButtonWidth;
	bool FAdjustButtonHeight;
	Ellist::TElList* FButtons;
	TElToolButton* FFocusedButton;
	bool FTransparent;
	bool FImageIsAlphaBlended;
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	void __fastcall StartMoreMenu(void);
	void __fastcall PutMoreItemsToBar(void);
	void __fastcall OnMoreItemClick(System::TObject* Sender);
	virtual void __fastcall SetButtonImageForm(Elimgfrm::TElImageForm* newValue);
	virtual void __fastcall SetBtnWidth(int newValue);
	virtual void __fastcall SetBtnHeight(int newValue);
	virtual void __fastcall SetFlat(bool newValue);
	virtual void __fastcall SetLargeSize(bool newValue);
	virtual void __fastcall SetLargeBtnWidth(int newValue);
	virtual void __fastcall SetLargeBtnHeight(int newValue);
	virtual void __fastcall SetButtonColor(Graphics::TColor newValue);
	virtual void __fastcall SetAutoSize(bool newValue);
	virtual void __fastcall SetTransparentButtons(bool newValue);
	virtual void __fastcall SetBtnOffsHorz(int newValue);
	virtual void __fastcall SetBtnOffsVert(int newValue);
	void __fastcall SetAutoWrap(bool newValue);
	void __fastcall SetShowGlyph(bool newValue);
	void __fastcall SetShowCaption(bool newValue);
	virtual void __fastcall SetGlyphLayout(Buttons::TButtonLayout newValue);
	virtual void __fastcall SetSpacing(int newValue);
	virtual void __fastcall SetMargin(int newValue);
	TElToolButton* __fastcall GetToolButton(int index);
	void __fastcall SetToolButton(int index, TElToolButton* newValue);
	int __fastcall GetButtonCount(void);
	HIDESBASE MESSAGE void __fastcall CMControlListChange(Messages::TMessage &Msg);
	MESSAGE void __fastcall CMControlChange(Controls::TCMControlChange &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	void __fastcall SetMinSize(int newValue);
	void __fastcall SetOrientation(TElBarOrientation newValue);
	void __fastcall SetUseLargeGlyphs(bool newValue);
	void __fastcall SetImages(Controls::TImageList* newValue);
	void __fastcall SetHotImages(Controls::TImageList* newValue);
	void __fastcall SetDisabledImages(Controls::TImageList* newValue);
	void __fastcall SetUseImageList(bool newValue);
	virtual void __fastcall SetShowMoreMenu(bool newValue);
	virtual void __fastcall SetMoreMenuActive(bool newValue);
	virtual void __fastcall AlignControls(Controls::TControl* AControl, Types::TRect &Rect);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Paint(void);
	void __fastcall RedrawMoreBtn(void);
	int __fastcall GetRealClientWidth(void);
	int __fastcall GetRealClientHeight(void);
	Types::TRect __fastcall GetMoreBtnRect();
	void __fastcall SetThinButtons(bool Value);
	virtual void __fastcall DrawThemedBackground(void);
	virtual WideString __fastcall GetThemedClassName();
	int __fastcall GetFreeButtonID(void);
	TElToolButton* __fastcall GetButtonByID(int ID);
	void __fastcall SetAdjustButtonWidth(bool Value);
	int __fastcall GetEffectiveButtonWidth(TCustomElToolButton* Button, bool IncludeArrow);
	void __fastcall SetAdjustButtonHeight(bool Value);
	int __fastcall GetEffectiveButtonHeight(TCustomElToolButton* Button);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	MESSAGE void __fastcall WMMouseLeave(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	void __fastcall StartLeaveTracking(void);
	virtual TMetaClass* __fastcall GetButtonClass(void);
	void __fastcall SetFocusedButton(TElToolButton* Value);
	virtual void __fastcall SetTransparent(bool newValue);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	DYNAMIC void __fastcall DoEnter(void);
	virtual void __fastcall SetParent(Controls::TWinControl* AParent);
	void __fastcall SetImageIsAlphaBlended(bool Value);
	__property TElToolButton* FocusedButton = {read=FFocusedButton, write=SetFocusedButton};
	
public:
	__fastcall virtual TElToolBar(Classes::TComponent* AOwner);
	__fastcall virtual ~TElToolBar(void);
	virtual TElToolButton* __fastcall AddButton(TElToolButtonType ButtonType);
	void __fastcall OrderedControls(Ellist::TElList* L);
	virtual void __fastcall AlignButtons(void);
	virtual void __fastcall UpdateButtons(void);
	void __fastcall BeginUpdate(void);
	void __fastcall EndUpdate(void);
	void __fastcall Save(void);
	void __fastcall Restore(void);
	bool __fastcall Setup(bool ShowTextOptions, bool ShowIconOptions);
	TCustomElToolButton* __fastcall GetNextButton(TCustomElToolButton* CurrentButton, bool Forward, bool IncludeDisabled);
	__property TElToolButton* ToolButton[int index] = {read=GetToolButton, write=SetToolButton};
	__property AnsiString Caption = {read=FDummy, write=FDummy};
	__property int ButtonCount = {read=GetButtonCount, nodefault};
	
__published:
	__property int BtnWidth = {read=FBtnWidth, write=SetBtnWidth, default=24};
	__property int BtnHeight = {read=FBtnHeight, write=SetBtnHeight, default=24};
	__property int BtnOffsHorz = {read=FBtnOffsHorz, write=SetBtnOffsHorz, default=3};
	__property int BtnOffsVert = {read=FBtnOffsVert, write=SetBtnOffsVert, default=3};
	__property bool AutoWrap = {read=FAutoWrap, write=SetAutoWrap, nodefault};
	__property bool ShowGlyph = {read=FShowGlyph, write=SetShowGlyph, default=1};
	__property bool ShowCaption = {read=FShowCaption, write=SetShowCaption, default=0};
	__property bool LargeSize = {read=FLargeSize, write=SetLargeSize, default=0};
	__property int LargeBtnWidth = {read=FLargeBtnWidth, write=SetLargeBtnWidth, default=48};
	__property int LargeBtnHeight = {read=FLargeBtnHeight, write=SetLargeBtnHeight, default=48};
	__property Buttons::TButtonLayout GlyphLayout = {read=FGlyphLayout, write=SetGlyphLayout, nodefault};
	__property int Spacing = {read=FSpacing, write=SetSpacing, default=4};
	__property int Margin = {read=FMargin, write=SetMargin, default=-1};
	__property bool Flat = {read=FFlat, write=SetFlat, default=1};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=1};
	__property int MinSize = {read=FMinSize, write=SetMinSize, default=8};
	__property Graphics::TColor ButtonColor = {read=FButtonColor, write=SetButtonColor, default=-2147483633};
	__property Elimgfrm::TElImageForm* ButtonImageForm = {read=FButtonImageForm, write=SetButtonImageForm};
	__property TElBarOrientation Orientation = {read=FOrientation, write=SetOrientation, default=0};
	__property bool UseLargeGlyphs = {read=FUseLargeGlyphs, write=SetUseLargeGlyphs, nodefault};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property Controls::TImageList* HotImages = {read=FHotImages, write=SetHotImages};
	__property Controls::TImageList* DisabledImages = {read=FDisabledImages, write=SetDisabledImages};
	__property bool UseImageList = {read=FUseImageList, write=SetUseImageList, nodefault};
	__property bool TransparentButtons = {read=FTransparentButtons, write=SetTransparentButtons, nodefault};
	__property bool ThinButtons = {read=FThinButtons, write=SetThinButtons, nodefault};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=FStorage};
	__property AnsiString StoragePath = {read=FStoragePath, write=FStoragePath};
	__property bool AdjustButtonWidth = {read=FAdjustButtonWidth, write=SetAdjustButtonWidth, default=1};
	__property bool AdjustButtonHeight = {read=FAdjustButtonHeight, write=SetAdjustButtonHeight, default=1};
	__property bool ImageIsAlphaBlended = {read=FImageIsAlphaBlended, write=SetImageIsAlphaBlended, default=0};
	__property bool ShowMoreMenu = {read=FShowMoreMenu, write=SetShowMoreMenu, nodefault};
	__property UseXPThemes  = {default=1};
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
	/* TWinControl.CreateParented */ inline __fastcall TElToolBar(HWND ParentWindow) : Elpanel::TElPanel(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElToolButtonActionLink;
class PASCALIMPLEMENTATION TElToolButtonActionLink : public Elpopbtn::TElGraphicButtonActionLink 
{
	typedef Elpopbtn::TElGraphicButtonActionLink inherited;
	
protected:
	virtual bool __fastcall IsVisibleLinked(void);
	virtual void __fastcall SetVisible(bool Value);
public:
	#pragma option push -w-inl
	/* TBasicActionLink.Create */ inline __fastcall virtual TElToolButtonActionLink(System::TObject* AClient) : Elpopbtn::TElGraphicButtonActionLink(AClient) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TBasicActionLink.Destroy */ inline __fastcall virtual ~TElToolButtonActionLink(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE int DEF_SepSize;

}	/* namespace Eltoolbar */
using namespace Eltoolbar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElToolbar
