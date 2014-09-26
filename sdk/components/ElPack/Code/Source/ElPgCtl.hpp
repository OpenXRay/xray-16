// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElPgCtl.pas' rev: 6.00

#ifndef ElPgCtlHPP
#define ElPgCtlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ElTimers.hpp>	// Pascal unit
#include <ElHintWnd.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ComObj.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elpgctl
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElTabStyle { etsTabs, etsButtons, etsFlatButtons, etsNetTabs, etsFlatTabs, etsAngledTabs };
#pragma option pop

#pragma option push -b-
enum TElTabPosition { etpTop, etpBottom, etpRight, etpLeft };
#pragma option pop

class DELPHICLASS TElTabSheet;
typedef void __fastcall (__closure *TElMeasureTabEvent)(System::TObject* Sender, TElTabSheet* Page, tagSIZE &Size);

typedef void __fastcall (__closure *TElTabGetImageEvent)(System::TObject* Sender, int PageIndex, int &ImageIndex);

typedef void __fastcall (__closure *TElTabChangingEvent)(System::TObject* Sender, TElTabSheet* NewPage, bool &AllowChange);

class DELPHICLASS TElPageControl;
class DELPHICLASS TElTabs;
class DELPHICLASS TElTab;
class PASCALIMPLEMENTATION TElTab : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TElTabs* FOwner;
	void __fastcall Draw(Graphics::TCanvas* ACanvas, const Types::TRect &R, TElTabSheet* TabSheet);
	unsigned __fastcall GetBtnTheme(void);
	unsigned __fastcall GetScrollTheme(void);
	unsigned __fastcall GetTabTheme(void);
	TElTabPosition __fastcall GetTabPosition(void);
	
protected:
	virtual int __fastcall GetOuterMargin(void);
	virtual int __fastcall GetInnerMargin(void);
	virtual void __fastcall DrawTabContents(Graphics::TCanvas* Canvas, const Types::TRect &R, TElTabSheet* TabSheet);
	virtual void __fastcall DrawSpace(Graphics::TCanvas* Canvas, int &cx, int &cy, int mx, int my);
	virtual bool __fastcall CanDrawTab(bool ActiveDraw);
	virtual int __fastcall GetAscend(void);
	virtual void __fastcall AdjustDrawingSize(bool Active, Types::TRect &R);
	virtual void __fastcall AdjustTabSize(tagSIZE &Size);
	virtual void __fastcall AdjustFillSize(bool After, Types::TRect &R, TElTabSheet* TabSheet);
	virtual void __fastcall DrawTabLine(Graphics::TCanvas* Canvas, const Types::TRect &R);
	virtual void __fastcall FillSpace(Graphics::TCanvas* Canvas, const Types::TRect &Rect);
	virtual void __fastcall FillTab(Graphics::TCanvas* Canvas, const Types::TRect &Rect, TElTabSheet* TabSheet);
	virtual void __fastcall DrawButtons(Graphics::TCanvas* Canvas, const Types::TRect &LeftRect, const Types::TRect &RightRect, bool CSL, bool CSR);
	virtual int __fastcall GetRowMargin(void);
	bool __fastcall IsThemeApplied(void);
	virtual void __fastcall DrawTabEdges(Graphics::TCanvas* Canvas, Types::TRect &R, TElTabSheet* TabSheet) = 0 ;
	virtual int __fastcall GetContentMargin(void);
	virtual void __fastcall FixupTab(Graphics::TCanvas* Canvas, const Types::TRect &R, TElTabSheet* TabSheet);
	__property unsigned TabTheme = {read=GetTabTheme, nodefault};
	__property unsigned BtnTheme = {read=GetBtnTheme, nodefault};
	__property unsigned ScrollTheme = {read=GetScrollTheme, nodefault};
	__property TElTabPosition TabPosition = {read=GetTabPosition, nodefault};
	
public:
	__fastcall TElTab(TElTabs* Owner);
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElTab(void) { }
	#pragma option pop
	
};


typedef TMetaClass*TElTabClass;

class PASCALIMPLEMENTATION TElTabs : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TElPageControl* FPageControl;
	TElTab* FTab;
	tagSIZE __fastcall MeasureSheet(Graphics::TCanvas* ACanvas, TElTabSheet* Sheet);
	void __fastcall DoDrawTabs(Graphics::TCanvas* ACanvas, const Types::TRect &Rect, bool DoDraw, tagSIZE &Size);
	unsigned __fastcall GetBtnTheme(void);
	unsigned __fastcall GetScrollTheme(void);
	unsigned __fastcall GetTabTheme(void);
	
protected:
	TMetaClass*FTabClass;
	virtual tagSIZE __fastcall CalcTabAreaSize();
	void __fastcall DrawTabs(Graphics::TCanvas* ACanvas, const Types::TRect &R, bool DoDraw, tagSIZE &Size);
	void __fastcall SetTabClass(TMetaClass* Value);
	HFONT __fastcall GetRotatedFont(Graphics::TCanvas* Canvas, int RotationAngle);
	void __fastcall ReorderPages(int MaxRows);
	bool __fastcall IsThemeApplied(void);
	__property unsigned TabTheme = {read=GetTabTheme, nodefault};
	__property unsigned BtnTheme = {read=GetBtnTheme, nodefault};
	__property unsigned ScrollTheme = {read=GetScrollTheme, nodefault};
	
public:
	__fastcall TElTabs(TElPageControl* PageControl);
	__fastcall virtual ~TElTabs(void);
	__property TMetaClass* TabClass = {read=FTabClass, write=SetTabClass};
};


#pragma option push -b-
enum TElPageScrollBtnState { pbsNone, pbsLeftBtnOver, pbsLeftBtnDown, pbsLeftBtnHeld, pbsRightBtnOver, pbsRightBtnDown, pbsRightBtnHeld };
#pragma option pop

class PASCALIMPLEMENTATION TElPageControl : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	int ALines;
	TElTabs* FTabs;
	#pragma pack(push, 1)
	Types::TRect ScrollLeftRect;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	Types::TRect ScrollRightRect;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	Types::TPoint TabsPos;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	tagSIZE TabsSize;
	#pragma pack(pop)
	
	HDC MemDC;
	TElPageScrollBtnState FScrollBtnState;
	HWND FSaveCapture;
	Extctrls::TTimer* FScrollTimer;
	int FTabIndex;
	unsigned FBtnTheme;
	unsigned FScrollTheme;
	#pragma pack(push, 1)
	Types::TPoint FHintCoords;
	#pragma pack(pop)
	
	Eltimers::TElTimer* FHintTimer;
	Controls::THintWindow* FHintWnd;
	bool FNoDTAlert;
	
protected:
	AnsiString FActivateSound;
	TElTabSheet* FActivePage;
	Graphics::TColor FActiveTabColor;
	Graphics::TBitmap* FBackground;
	Elvclutils::TElBkGndType FBackgroundType;
	int FBorderWidth;
	TElTabSheet* FDownTab;
	bool FDrawFocus;
	AnsiString FDummyCaption;
	TElTabSheet* FFirstTab;
	bool FFlat;
	Graphics::TColor FGradientEndColor;
	Graphics::TColor FGradientStartColor;
	int FGradientSteps;
	bool FHotTrack;
	Imglist::TChangeLink* FImageChangeLink;
	Controls::TImageList* FImages;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Graphics::TColor FInactiveTabColor;
	int FMinTabHeight;
	int FMinTabWidth;
	bool FMultiLine;
	TElTabSheet* FNewDockSheet;
	Classes::TNotifyEvent FOnChange;
	TElTabGetImageEvent FOnGetImageIndex;
	TElMeasureTabEvent FOnMeasureTab;
	TElTabChangingEvent FOnChanging;
	Ellist::TElList* FPages;
	bool FRaggedRight;
	bool FScrollOpposite;
	bool FShowBorder;
	bool FShowImages;
	bool FShowTabs;
	Elsndmap::TElSoundMap* FSoundMap;
	TElTabStyle FStyle;
	unsigned FTabHeight;
	TElTabPosition FTabPosition;
	unsigned FTabWidth;
	Graphics::TBitmap* FTmpBmp;
	TElTabSheet* FTrackTab;
	TElTabSheet* FUndockingPage;
	Graphics::TColor FTabBkColor;
	Graphics::TFont* FHotTrackFont;
	bool FShowTabHints;
	bool FSavvyMode;
	Graphics::TColor FFlatTabBorderColor;
	Graphics::TColor FTabBkColorNetStyle;
	bool FVerticalSideCaptions;
	WideString FHint;
	DYNAMIC void __fastcall Resize(void);
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall FreeThemeHandle(void);
	virtual void __fastcall CreateThemeHandle(void);
	virtual bool __fastcall CanChange(TElTabSheet* NewPage);
	bool __fastcall CanScrollLeft(void);
	virtual void __fastcall Change(void);
	void __fastcall ChangeActivePage(TElTabSheet* Page);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	HIDESBASE MESSAGE void __fastcall WMEraseBkGnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TWMNCPaint &Message);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMNCLButtonDown(Messages::TWMNCHitMessage &Message);
	MESSAGE void __fastcall WMNCLButtonUp(Messages::TWMNCHitMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TWMNCHitTest &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Message);
	HIDESBASE MESSAGE void __fastcall CMShowingChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMControlListChange(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMDesignHitTest(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall CMDialogKey(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall CMDockClient(Controls::TCMDockClient &Message);
	MESSAGE void __fastcall CMDockNotification(Controls::TCMDockNotification &Message);
	HIDESBASE MESSAGE void __fastcall CMFocusChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMColorChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMUnDockClient(Controls::TCMUnDockClient &Message);
	virtual void __fastcall CreateHandle(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	DYNAMIC void __fastcall DockOver(Controls::TDragDockObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	DYNAMIC void __fastcall DoAddDockClient(Controls::TControl* Client, const Types::TRect &ARect);
	DYNAMIC void __fastcall DoRemoveDockClient(Controls::TControl* Client);
	int __fastcall GetActivePageIndex(void);
	DYNAMIC void __fastcall GetChildren(Classes::TGetChildProc Proc, Classes::TComponent* Root);
	Controls::TControl* __fastcall GetDockClientFromMousePos(const Types::TPoint &MousePos);
	int __fastcall GetPageCount(void);
	TElTabSheet* __fastcall GetPageFromDockClient(Controls::TControl* Client);
	TElTabSheet* __fastcall GetPages(int index);
	DYNAMIC void __fastcall GetSiteInfo(Controls::TControl* Client, Types::TRect &InfluenceRect, const Types::TPoint &MousePos, bool &CanDock);
	int __fastcall GetTabIndex(void);
	void __fastcall ImageChange(System::TObject* Sender);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall ImageListChange(System::TObject* Sender);
	virtual void __fastcall InsertPage(TElTabSheet* TabSheet);
	void __fastcall MakeTabVisible(TElTabSheet* ATabSheet);
	void __fastcall RebuildTabs(bool ResetFirstItem);
	void __fastcall RedoTmpBmp(void);
	virtual void __fastcall RemovePage(TElTabSheet* TabSheet);
	void __fastcall SetActivePage(TElTabSheet* Value);
	void __fastcall SetActivePageIndex(const int Value);
	void __fastcall SetActiveTabColor(Graphics::TColor Value);
	void __fastcall SetBackground(Graphics::TBitmap* newValue);
	void __fastcall SetBackgroundType(Elvclutils::TElBkGndType newValue);
	HIDESBASE void __fastcall SetBorderWidth(int Value);
	DYNAMIC void __fastcall SetChildOrder(Classes::TComponent* Child, int Order);
	void __fastcall SetDrawFocus(bool Value);
	void __fastcall SetFirstTab(TElTabSheet* Value);
	void __fastcall SetFlat(bool newValue);
	void __fastcall SetGradientEndColor(Graphics::TColor newValue);
	void __fastcall SetGradientStartColor(Graphics::TColor newValue);
	void __fastcall SetGradientSteps(int newValue);
	void __fastcall SetHotTrack(bool newValue);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall SetImages(Controls::TImageList* newValue);
	void __fastcall SetInactiveTabColor(Graphics::TColor Value);
	void __fastcall SetMinTabHeight(int Value);
	void __fastcall SetMinTabWidth(int Value);
	void __fastcall SetMultiLine(bool newValue);
	void __fastcall SetRaggedRight(const bool Value);
	void __fastcall SetScrollOpposite(const bool Value);
	void __fastcall SetShowBorder(bool Value);
	void __fastcall SetShowImages(bool newValue);
	void __fastcall SetShowTabs(bool Value);
	void __fastcall SetStyle(TElTabStyle newValue);
	void __fastcall SetTabHeight(unsigned newValue);
	void __fastcall SetTabIndex(const int Value);
	void __fastcall SetTabPosition(TElTabPosition newValue);
	void __fastcall SetTabWidth(unsigned newValue);
	virtual void __fastcall ShowControl(Controls::TControl* AControl);
	virtual void __fastcall UpdateActivePage(void);
	void __fastcall UpdateTab(TElTabSheet* TabSheet);
	bool __fastcall CanScrollRight(void);
	void __fastcall SetHotTrackFont(Graphics::TFont* Value);
	void __fastcall HotTrackFontChange(System::TObject* Sender);
	virtual void __fastcall Paint(void);
	void __fastcall SetScrollBtnState(TElPageScrollBtnState Value);
	void __fastcall SetTrackTab(TElTabSheet* Value);
	void __fastcall OnScrollTimer(System::TObject* Sender);
	void __fastcall SetTabBkColor(Graphics::TColor Value);
	bool __fastcall HasVisibleTabs(void);
	bool __fastcall DoHitTest(int X, int Y, int &Res);
	void __fastcall UpdateMultilineOrder(void);
	virtual void __fastcall TriggerGetImageEvent(int PageIndex, int &ImageIndex);
	virtual void __fastcall TriggerMeasureTabEvent(TElTabSheet* Page, tagSIZE &Size);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	MESSAGE void __fastcall IFMEffectiveSize(Messages::TMessage &Message);
	MESSAGE void __fastcall IFMCanPaintBkgnd(Messages::TMessage &Message);
	MESSAGE void __fastcall PMRefreshActivePage(Messages::TMessage &Message);
	MESSAGE void __fastcall WMNCRButtonUp(Messages::TWMNCHitMessage &Message);
	MESSAGE void __fastcall WMNCCreate(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMNCDestroy(Messages::TMessage &Message);
	MESSAGE void __fastcall WMNCMouseMove(Messages::TMessage &Message);
	virtual void __fastcall OnHintTimer(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	void __fastcall SetSavvyMode(bool Value);
	void __fastcall SetFlatTabBorderColor(Graphics::TColor Value);
	void __fastcall SetTabBkColorNetStyle(Graphics::TColor Value);
	void __fastcall SetVerticalSideCaptions(bool Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	__property TElPageScrollBtnState ScrollBtnState = {read=FScrollBtnState, write=SetScrollBtnState, nodefault};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property Elvclutils::TElBkGndType BackgroundType = {read=FBackgroundType, write=SetBackgroundType, nodefault};
	__property Graphics::TColor GradientEndColor = {read=FGradientEndColor, write=SetGradientEndColor, nodefault};
	__property Graphics::TColor GradientStartColor = {read=FGradientStartColor, write=SetGradientStartColor, nodefault};
	__property int GradientSteps = {read=FGradientSteps, write=SetGradientSteps, nodefault};
	
public:
	__fastcall virtual TElPageControl(Classes::TComponent* AOwner);
	__fastcall virtual ~TElPageControl(void);
	TElTabSheet* __fastcall FindNextPage(TElTabSheet* CurPage, bool GoForward, bool CheckTabVisible, bool CheckTabEnabled);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall SelectNextPage(bool GoForward);
	virtual void __fastcall SetBounds(int ALeft, int ATop, int AWidth, int AHeight);
	TElTabSheet* __fastcall TabFromPoint(const Types::TPoint &Point);
	void __fastcall UpdateTabs(bool Immediate);
	TElTabSheet* __fastcall NewPage(void);
	virtual void __fastcall Loaded(void);
	__property int ActivePageIndex = {read=GetActivePageIndex, write=SetActivePageIndex, nodefault};
	__property TElTabSheet* FirstTab = {read=FFirstTab, write=SetFirstTab};
	__property int PageCount = {read=GetPageCount, nodefault};
	__property TElTabSheet* Pages[int index] = {read=GetPages};
	__property TElTabSheet* TrackTab = {read=FTrackTab, write=SetTrackTab};
	__property unsigned BtnTheme = {read=FBtnTheme, nodefault};
	__property unsigned ScrollTheme = {read=FScrollTheme, nodefault};
	__property Graphics::TColor TabBkColorNetStyle = {read=FTabBkColorNetStyle, write=SetTabBkColorNetStyle, nodefault};
	
__published:
	__property AnsiString ActivateSound = {read=FActivateSound, write=FActivateSound};
	__property Graphics::TColor ActiveTabColor = {read=FActiveTabColor, write=SetActiveTabColor, default=-2147483633};
	__property int BorderWidth = {read=FBorderWidth, write=SetBorderWidth, nodefault};
	__property AnsiString Caption = {read=FDummyCaption};
	__property Color  = {default=-2147483633};
	__property bool DrawFocus = {read=FDrawFocus, write=SetDrawFocus, nodefault};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property bool HotTrack = {read=FHotTrack, write=SetHotTrack, nodefault};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property Graphics::TColor InactiveTabColor = {read=FInactiveTabColor, write=SetInactiveTabColor, default=-2147483633};
	__property int MinTabHeight = {read=FMinTabHeight, write=SetMinTabHeight, default=40};
	__property int MinTabWidth = {read=FMinTabWidth, write=SetMinTabWidth, default=40};
	__property bool Multiline = {read=FMultiLine, write=SetMultiLine, nodefault};
	__property TElTabChangingEvent OnChanging = {read=FOnChanging, write=FOnChanging};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property TElTabGetImageEvent OnGetImageIndex = {read=FOnGetImageIndex, write=FOnGetImageIndex};
	__property TElMeasureTabEvent OnMeasureTab = {read=FOnMeasureTab, write=FOnMeasureTab};
	__property bool RaggedRight = {read=FRaggedRight, write=SetRaggedRight, nodefault};
	__property bool ScrollOpposite = {read=FScrollOpposite, write=SetScrollOpposite, nodefault};
	__property bool ShowBorder = {read=FShowBorder, write=SetShowBorder, default=1};
	__property bool ShowImages = {read=FShowImages, write=SetShowImages, default=1};
	__property bool ShowTabs = {read=FShowTabs, write=SetShowTabs, default=1};
	__property Elsndmap::TElSoundMap* SoundMap = {read=FSoundMap, write=FSoundMap};
	__property TElTabStyle Style = {read=FStyle, write=SetStyle, nodefault};
	__property unsigned TabHeight = {read=FTabHeight, write=SetTabHeight, default=0};
	__property int TabIndex = {read=GetTabIndex, write=SetTabIndex, default=-1};
	__property TElTabPosition TabPosition = {read=FTabPosition, write=SetTabPosition, nodefault};
	__property unsigned TabWidth = {read=FTabWidth, write=SetTabWidth, default=0};
	__property Graphics::TFont* HotTrackFont = {read=FHotTrackFont, write=SetHotTrackFont};
	__property Graphics::TColor TabBkColor = {read=FTabBkColor, write=SetTabBkColor, default=-2147483633};
	__property TElTabSheet* ActivePage = {read=FActivePage, write=SetActivePage};
	__property bool ShowTabHints = {read=FShowTabHints, write=FShowTabHints, default=1};
	__property bool SavvyMode = {read=FSavvyMode, write=SetSavvyMode, default=0};
	__property Graphics::TColor FlatTabBorderColor = {read=FFlatTabBorderColor, write=SetFlatTabBorderColor, nodefault};
	__property bool VerticalSideCaptions = {read=FVerticalSideCaptions, write=SetVerticalSideCaptions, default=1};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property Align  = {default=0};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property OnDblClick ;
	__property OnStartDrag ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnEnter ;
	__property OnExit ;
	__property OnStartDock ;
	__property OnUnDock ;
	__property OnDockDrop ;
	__property OnDockOver ;
	__property OnEndDock ;
	__property OnResize ;
	__property OnContextPopup ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DockSite  = {default=0};
	__property Floating ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElPageControl(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElTabSheet : public Controls::TCustomControl 
{
	typedef Controls::TCustomControl inherited;
	
private:
	bool FTabShowing;
	#pragma pack(push, 1)
	Types::TRect ARect;
	#pragma pack(pop)
	
	bool AComplete;
	int ALine;
	bool AShown;
	HIDESBASE MESSAGE void __fastcall CMShowingChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	void __fastcall SetTabShowing(bool Value);
	void __fastcall UpdateTabShowing(void);
	int __fastcall GetPageIndex(void);
	void __fastcall SetPageIndex(int Value);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	unsigned __fastcall GetBtnTheme(void);
	unsigned __fastcall GetScrollTheme(void);
	unsigned __fastcall GetTabTheme(void);
	bool __fastcall GetUseXPThemes(void);
	HIDESBASE MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Message);
	
protected:
	Classes::TNotifyEvent FOnShow;
	Classes::TNotifyEvent FOnHide;
	Graphics::TColor FTabColor;
	TElPageControl* FPageControl;
	int FImageIndex;
	bool FTabVisible;
	WideString FCaption;
	bool FTabEnabled;
	Menus::TPopupMenu* FTabMenu;
	WideString FHint;
	virtual void __fastcall TriggerShowEvent(void);
	virtual void __fastcall TriggerHideEvent(void);
	void __fastcall SetTabColor(Graphics::TColor Value);
	void __fastcall SetPageControl(TElPageControl* Value);
	void __fastcall SetImageIndex(int Value);
	void __fastcall SetTabVisible(bool Value);
	void __fastcall SetCaption(WideString Value);
	virtual void __fastcall Paint(void);
	int __fastcall GetTabIndex(void);
	HIDESBASE MESSAGE void __fastcall WMEraseBkGnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Message);
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	void __fastcall SetTabEnabled(bool Value);
	void __fastcall SetTabMenu(Menus::TPopupMenu* Value);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall ReadState(Classes::TReader* Reader);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	__property bool TabShowing = {read=FTabShowing, nodefault};
	
public:
	__fastcall virtual TElTabSheet(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTabSheet(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	bool __fastcall IsThemeApplied(void);
	__property int TabIndex = {read=GetTabIndex, nodefault};
	__property unsigned TabTheme = {read=GetTabTheme, nodefault};
	__property unsigned BtnTheme = {read=GetBtnTheme, nodefault};
	__property unsigned ScrollTheme = {read=GetScrollTheme, nodefault};
	__property bool UseXPThemes = {read=GetUseXPThemes, nodefault};
	
__published:
	__property Graphics::TColor TabColor = {read=FTabColor, write=SetTabColor, default=-2147483633};
	__property Classes::TNotifyEvent OnShow = {read=FOnShow, write=FOnShow};
	__property Classes::TNotifyEvent OnHide = {read=FOnHide, write=FOnHide};
	__property TElPageControl* PageControl = {read=FPageControl, write=SetPageControl};
	__property int ImageIndex = {read=FImageIndex, write=SetImageIndex, nodefault};
	__property bool TabVisible = {read=FTabVisible, write=SetTabVisible, nodefault};
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property int PageIndex = {read=GetPageIndex, write=SetPageIndex, stored=false, nodefault};
	__property bool TabEnabled = {read=FTabEnabled, write=SetTabEnabled, default=1};
	__property Menus::TPopupMenu* TabMenu = {read=FTabMenu, write=SetTabMenu};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property Color  = {default=-2147483633};
	__property ParentColor  = {default=0};
	__property Visible  = {default=1};
	__property BorderWidth  = {default=0};
	__property Constraints ;
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentFont  = {default=1};
	__property Height  = {stored=false};
	__property Left  = {stored=false};
	__property OnContextPopup ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnResize ;
	__property OnStartDrag ;
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Top  = {stored=false};
	__property Width  = {stored=false};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElTabSheet(HWND ParentWindow) : Controls::TCustomControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElStdTab;
class PASCALIMPLEMENTATION TElStdTab : public TElTab 
{
	typedef TElTab inherited;
	
protected:
	virtual int __fastcall GetOuterMargin(void);
	virtual bool __fastcall CanDrawTab(bool ActiveDraw);
	virtual int __fastcall GetAscend(void);
	virtual void __fastcall AdjustDrawingSize(bool Active, Types::TRect &R);
	virtual void __fastcall AdjustTabSize(tagSIZE &Size);
	virtual void __fastcall DrawTabLine(Graphics::TCanvas* Canvas, const Types::TRect &R);
	virtual void __fastcall AdjustFillSize(bool After, Types::TRect &R, TElTabSheet* TabSheet);
	virtual void __fastcall FillTab(Graphics::TCanvas* Canvas, const Types::TRect &Rect, TElTabSheet* TabSheet);
	virtual void __fastcall DrawTabEdges(Graphics::TCanvas* Canvas, Types::TRect &R, TElTabSheet* TabSheet);
public:
	#pragma option push -w-inl
	/* TElTab.Create */ inline __fastcall TElStdTab(TElTabs* Owner) : TElTab(Owner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElStdTab(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElBtnTab;
class PASCALIMPLEMENTATION TElBtnTab : public TElTab 
{
	typedef TElTab inherited;
	
protected:
	virtual int __fastcall GetInnerMargin(void);
	virtual void __fastcall DrawSpace(Graphics::TCanvas* Canvas, int &cx, int &cy, int mx, int my);
	virtual void __fastcall AdjustTabSize(tagSIZE &Size);
	virtual int __fastcall GetRowMargin(void);
	virtual void __fastcall FillTab(Graphics::TCanvas* Canvas, const Types::TRect &Rect, TElTabSheet* TabSheet);
	virtual void __fastcall DrawTabEdges(Graphics::TCanvas* Canvas, Types::TRect &R, TElTabSheet* TabSheet);
public:
	#pragma option push -w-inl
	/* TElTab.Create */ inline __fastcall TElBtnTab(TElTabs* Owner) : TElTab(Owner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElBtnTab(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElFlatBtnTab;
class PASCALIMPLEMENTATION TElFlatBtnTab : public TElTab 
{
	typedef TElTab inherited;
	
protected:
	virtual int __fastcall GetInnerMargin(void);
	virtual void __fastcall DrawSpace(Graphics::TCanvas* Canvas, int &cx, int &cy, int mx, int my);
	virtual void __fastcall AdjustTabSize(tagSIZE &Size);
	virtual void __fastcall AdjustFillSize(bool After, Types::TRect &R, TElTabSheet* TabSheet);
	virtual int __fastcall GetRowMargin(void);
	virtual void __fastcall FillTab(Graphics::TCanvas* Canvas, const Types::TRect &Rect, TElTabSheet* TabSheet);
	virtual void __fastcall DrawTabEdges(Graphics::TCanvas* Canvas, Types::TRect &R, TElTabSheet* TabSheet);
public:
	#pragma option push -w-inl
	/* TElTab.Create */ inline __fastcall TElFlatBtnTab(TElTabs* Owner) : TElTab(Owner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElFlatBtnTab(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElNetTab;
class PASCALIMPLEMENTATION TElNetTab : public TElTab 
{
	typedef TElTab inherited;
	
protected:
	virtual void __fastcall AdjustTabSize(tagSIZE &Size);
	virtual int __fastcall GetInnerMargin(void);
	virtual void __fastcall DrawSpace(Graphics::TCanvas* Canvas, int &cx, int &cy, int mx, int my);
	virtual bool __fastcall CanDrawTab(bool ActiveDraw);
	virtual void __fastcall AdjustDrawingSize(bool Active, Types::TRect &R);
	virtual int __fastcall GetOuterMargin(void);
	virtual void __fastcall DrawTabLine(Graphics::TCanvas* Canvas, const Types::TRect &R);
	virtual int __fastcall GetAscend(void);
	virtual void __fastcall FillSpace(Graphics::TCanvas* Canvas, const Types::TRect &Rect);
	virtual void __fastcall AdjustFillSize(bool After, Types::TRect &R, TElTabSheet* TabSheet);
	virtual void __fastcall FillTab(Graphics::TCanvas* Canvas, const Types::TRect &Rect, TElTabSheet* TabSheet);
	virtual void __fastcall DrawButtons(Graphics::TCanvas* Canvas, const Types::TRect &LeftRect, const Types::TRect &RightRect, bool CSL, bool CSR);
	virtual void __fastcall DrawTabEdges(Graphics::TCanvas* Canvas, Types::TRect &R, TElTabSheet* TabSheet);
public:
	#pragma option push -w-inl
	/* TElTab.Create */ inline __fastcall TElNetTab(TElTabs* Owner) : TElTab(Owner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElNetTab(void) { }
	#pragma option pop
	
};


class DELPHICLASS TEl2DFlatTab;
class PASCALIMPLEMENTATION TEl2DFlatTab : public TElTab 
{
	typedef TElTab inherited;
	
protected:
	virtual void __fastcall AdjustDrawingSize(bool Active, Types::TRect &R);
	virtual void __fastcall AdjustFillSize(bool After, Types::TRect &R, TElTabSheet* TabSheet);
	virtual void __fastcall AdjustTabSize(tagSIZE &Size);
	virtual bool __fastcall CanDrawTab(bool ActiveDraw);
	virtual void __fastcall DrawTabLine(Graphics::TCanvas* Canvas, const Types::TRect &R);
	virtual int __fastcall GetAscend(void);
	virtual int __fastcall GetInnerMargin(void);
	virtual int __fastcall GetOuterMargin(void);
	virtual void __fastcall FillTab(Graphics::TCanvas* Canvas, const Types::TRect &Rect, TElTabSheet* TabSheet);
	virtual void __fastcall DrawTabEdges(Graphics::TCanvas* Canvas, Types::TRect &R, TElTabSheet* TabSheet);
public:
	#pragma option push -w-inl
	/* TElTab.Create */ inline __fastcall TEl2DFlatTab(TElTabs* Owner) : TElTab(Owner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TEl2DFlatTab(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElAngledTab;
class PASCALIMPLEMENTATION TElAngledTab : public TElTab 
{
	typedef TElTab inherited;
	
private:
	int SaveDCState;
	
protected:
	virtual bool __fastcall CanDrawTab(bool ActiveDraw);
	virtual void __fastcall DrawTabEdges(Graphics::TCanvas* Canvas, Types::TRect &R, TElTabSheet* TabSheet);
	virtual void __fastcall DrawTabLine(Graphics::TCanvas* Canvas, const Types::TRect &R);
	virtual void __fastcall AdjustFillSize(bool After, Types::TRect &R, TElTabSheet* TabSheet);
	virtual void __fastcall AdjustDrawingSize(bool Active, Types::TRect &R);
	virtual void __fastcall AdjustTabSize(tagSIZE &Size);
	virtual void __fastcall FillTab(Graphics::TCanvas* Canvas, const Types::TRect &Rect, TElTabSheet* TabSheet);
	virtual int __fastcall GetAscend(void);
	virtual int __fastcall GetInnerMargin(void);
	virtual int __fastcall GetOuterMargin(void);
	virtual int __fastcall GetContentMargin(void);
	void __fastcall CreateTabPoints(const Types::TRect &R, Types::PPoint Points);
	virtual void __fastcall FixupTab(Graphics::TCanvas* Canvas, const Types::TRect &R, TElTabSheet* TabSheet);
public:
	#pragma option push -w-inl
	/* TElTab.Create */ inline __fastcall TElAngledTab(TElTabs* Owner) : TElTab(Owner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElAngledTab(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Word PM_REFRESHACTIVEPAGE = 0x210a;

}	/* namespace Elpgctl */
using namespace Elpgctl;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElPgCtl
