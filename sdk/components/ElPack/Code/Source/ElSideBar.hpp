// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElSideBar.pas' rev: 6.00

#ifndef ElSideBarHPP
#define ElSideBarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElIni.hpp>	// Pascal unit
#include <ElExtBkgnd.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elsidebar
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElSideBarPart { sbpSection, sbpItemImage, sbpItemText, sbpUpScroll, sbpDownScroll, sbpInside };
#pragma option pop

#pragma option push -b-
enum TElSideBarIconLocation { ilLeft, ilTop };
#pragma option pop

class DELPHICLASS TElSideBarContainerPanel;
class PASCALIMPLEMENTATION TElSideBarContainerPanel : public Elpanel::TElPanel 
{
	typedef Elpanel::TElPanel inherited;
	
protected:
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TWMNCHitTest &Message);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	
public:
	__fastcall virtual TElSideBarContainerPanel(Classes::TComponent* AOwner);
public:
	#pragma option push -w-inl
	/* TCustomElPanel.Destroy */ inline __fastcall virtual ~TElSideBarContainerPanel(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElSideBarContainerPanel(HWND ParentWindow) : Elpanel::TElPanel(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElSideBarCItem;
class DELPHICLASS TElSideBar;
class DELPHICLASS TElSideBarSections;
class DELPHICLASS TElSideBarSection;
class PASCALIMPLEMENTATION TElSideBarSections : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElSideBarSection* operator[](int index) { return Items[index]; }
	
private:
	TElSideBar* FBar;
	TElSideBarSection* __fastcall GetItems(int index);
	void __fastcall SetItems(int index, TElSideBarSection* newValue);
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	
public:
	__fastcall TElSideBarSections(TElSideBar* Bar);
	HIDESBASE TElSideBarSection* __fastcall Add(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	__property TElSideBarSection* Items[int index] = {read=GetItems, write=SetItems/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElSideBarSections(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElSideBarItem;
class PASCALIMPLEMENTATION TElSideBarCItem : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	TElSideBar* FBar;
	bool FVisible;
	bool FEnabled;
	int FImageIndex;
	WideString FCaption;
	int FTag;
	WideString FHint;
	#pragma pack(push, 1)
	Types::TRect FTextRect;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	Types::TRect FBoundRect;
	#pragma pack(pop)
	
	bool FPartial;
	void __fastcall SetVisible(bool newValue);
	void __fastcall SetEnabled(bool newValue);
	void __fastcall SetImageIndex(int newValue);
	void __fastcall SetCaption(WideString newValue);
	
protected:
	TElSideBar* __fastcall GetBar(void);
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual AnsiString __fastcall GetDisplayName();
	
public:
	bool Active;
	bool Hidden;
	bool Disabled;
	void *Data;
	__fastcall virtual TElSideBarCItem(Classes::TCollection* Collection);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	
__published:
	__property Index ;
	__property int Tag = {read=FTag, write=FTag, nodefault};
	__property WideString Hint = {read=FHint, write=FHint};
	__property bool Visible = {read=FVisible, write=SetVisible, nodefault};
	__property bool Enabled = {read=FEnabled, write=SetEnabled, nodefault};
	__property int ImageIndex = {read=FImageIndex, write=SetImageIndex, nodefault};
	__property WideString Caption = {read=FCaption, write=SetCaption};
public:
	#pragma option push -w-inl
	/* TCollectionItem.Destroy */ inline __fastcall virtual ~TElSideBarCItem(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElSideBarItem : public TElSideBarCItem 
{
	typedef TElSideBarCItem inherited;
	
public:
	__fastcall virtual TElSideBarItem(Classes::TCollection* Collection);
	__fastcall virtual ~TElSideBarItem(void);
};


class PASCALIMPLEMENTATION TElSideBar : public Elpanel::TCustomElPanel 
{
	typedef Elpanel::TCustomElPanel inherited;
	
private:
	bool FVisibleSections;
	Graphics::TFont* FItemsFont;
	bool FFlatSections;
	bool FFlatItems;
	bool FFlatActiveItem;
	bool FFlat;
	int FUpdateCount;
	Elsndmap::TElSoundMap* FSoundMap;
	Controls::TImageList* FSectionImages;
	Controls::TImageList* FSectionHotImages;
	Controls::TImageList* FSectionDisabledImages;
	Controls::TImageList* FItemDisabledImages;
	Controls::TImageList* FItemImages;
	Controls::TImageList* FItemHotImages;
	bool FSectionTracking;
	bool FItemTracking;
	bool FUnderlineTracked;
	bool FMouseOver;
	TElSideBarSections* FSections;
	bool FVisUpBtn;
	bool FVisDownBtn;
	bool FUpBtnPressed;
	bool FDownBtnPressed;
	bool FSaveUpBtnPressed;
	bool FSaveDownBtnPressed;
	Graphics::TFontStyles FActiveSectionStyle;
	Graphics::TFontStyles FActiveItemStyle;
	Graphics::TColor FTrackSectionFontColor;
	Graphics::TColor FTrackSectionBkColor;
	Graphics::TColor FTrackItemFontColor;
	Graphics::TColor FTrackItemBkColor;
	HWND FSaveCapture;
	bool FPressed;
	bool FAutoScroll;
	bool Registered;
	TElSideBarItem* FTopItem;
	TElSideBarSection* FSection;
	TElSideBarItem* FItem;
	TElSideBarSection* FTrackSection;
	TElSideBarItem* FTrackItem;
	TElSideBarSection* FDownSection;
	TElSideBarItem* FDownItem;
	TElSideBarSection* FSaveDownSection;
	TElSideBarItem* FSaveDownItem;
	Imglist::TChangeLink* FSImagesLink;
	Imglist::TChangeLink* FIImagesLink;
	Imglist::TChangeLink* FSDImagesLink;
	Imglist::TChangeLink* FIDImagesLink;
	Imglist::TChangeLink* FSHImagesLink;
	Imglist::TChangeLink* FIHImagesLink;
	Menus::TPopupMenu* FItemsPopup;
	Menus::TPopupMenu* FSectionsPopup;
	int FSectionHeight;
	Graphics::TColor FSectionsColor;
	bool FWordWrap;
	int FSpacing;
	int FTopSpacing;
	int FScrollDelay;
	Extctrls::TTimer* FScrollTimer;
	AnsiString FItemChangeSound;
	AnsiString FSectionChangeSound;
	int FItemSize;
	bool FRightAlignedBar;
	Classes::TNotifyEvent FOnItemChange;
	Classes::TNotifyEvent FOnSectionChange;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FAutoSelectItem;
	TElSideBarItem* FDelayItem;
	TElSideBarCItem* FHintItem;
	int FChangeDelay;
	Extctrls::TTimer* FDelayTimer;
	bool FKeepSelection;
	Elvclutils::TElBorderSides FBorderSides;
	TElSideBarIconLocation FIconLocation;
	void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetSectionHeight(int newValue);
	void __fastcall SetSections(TElSideBarSections* newValue);
	int __fastcall GetSectionIndex(void);
	void __fastcall SetSectionIndex(int newValue);
	int __fastcall GetItemIndex(void);
	void __fastcall SetItemIndex(int newValue);
	void __fastcall SetSectionsColor(Graphics::TColor newValue);
	void __fastcall SetWordWrap(bool newValue);
	void __fastcall SetSpacing(int newValue);
	void __fastcall SetTopSpacing(int newValue);
	void __fastcall SetScrollDelay(int newValue);
	void __fastcall SetSectionTracking(bool newValue);
	void __fastcall SetItemTracking(bool newValue);
	void __fastcall SetUnderlineTracked(bool newValue);
	void __fastcall SetSoundMap(Elsndmap::TElSoundMap* newValue);
	void __fastcall SetSectionImages(Controls::TImageList* newValue);
	void __fastcall SetSectionHotImages(Controls::TImageList* newValue);
	void __fastcall SetSectionDisabledImages(Controls::TImageList* newValue);
	void __fastcall SetItemDisabledImages(Controls::TImageList* newValue);
	void __fastcall SetItemImages(Controls::TImageList* newValue);
	void __fastcall SetItemHotImages(Controls::TImageList* newValue);
	MESSAGE void __fastcall CMControlChange(Controls::TCMControlChange &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TWMMouseWheel &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	MESSAGE void __fastcall WMGetMinMaxInfo(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Msg);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	void __fastcall SetFlat(bool newValue);
	void __fastcall DrawFlatBorder(void);
	void __fastcall ItemRemoved(System::TObject* Sender, Classes::TCollectionItem* Item);
	void __fastcall ItemAdded(System::TObject* Sender, Classes::TCollectionItem* Item);
	void __fastcall SectionRemoved(System::TObject* Sender, Classes::TCollectionItem* Item);
	void __fastcall SectionAdded(System::TObject* Sender, Classes::TCollectionItem* Item);
	void __fastcall SetFlatSections(bool newValue);
	void __fastcall SetFlatItems(bool newValue);
	void __fastcall SetItemsFont(Graphics::TFont* newValue);
	Graphics::TFont* __fastcall GetSectionsFont(void);
	void __fastcall SetSectionsFont(Graphics::TFont* newValue);
	void __fastcall SectionsFontChanged(System::TObject* Sender);
	void __fastcall ItemsFontChanged(System::TObject* Sender);
	void __fastcall SetVisibleSections(bool newValue);
	void __fastcall ImagesChanged(System::TObject* Sender);
	void __fastcall SetItemSize(int newValue);
	void __fastcall SetRightAlignedBar(bool newValue);
	void __fastcall OnScrollTimer(System::TObject* Sender);
	void __fastcall SetSectionsPopupMenu(Menus::TPopupMenu* newValue);
	void __fastcall SetItemsPopupMenu(Menus::TPopupMenu* newValue);
	int __fastcall GetTopIndex(void);
	void __fastcall SetTopIndex(int newValue);
	void __fastcall UpdateTracks(void);
	void __fastcall SetActiveSectionStyle(Graphics::TFontStyles newValue);
	void __fastcall SetActiveItemStyle(Graphics::TFontStyles newValue);
	void __fastcall SetTrackSectionFontColor(Graphics::TColor Value);
	void __fastcall SetTrackSectionBkColor(Graphics::TColor Value);
	void __fastcall SetTrackItemFontColor(Graphics::TColor Value);
	void __fastcall SetTrackItemBkColor(Graphics::TColor Value);
	void __fastcall SetFlatActiveItem(bool Value);
	void __fastcall SetKeepSelection(bool Value);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	void __fastcall SetIconLocation(TElSideBarIconLocation Value);
	
protected:
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	void __fastcall IntKeyDown(Word &Key, Classes::TShiftState ShiftState);
	void __fastcall IntMouseDown(Controls::TMouseButton Button, short XPos, short YPos);
	void __fastcall IntMouseUp(Controls::TMouseButton Button, short XPos, short YPos);
	void __fastcall IntMouseMove(short XPos, short YPos);
	void __fastcall IntMouseWheel(int WheelDelta, const Types::TPoint &MousePos);
	bool __fastcall IntHintShow(Forms::THintInfo &HintInfo);
	virtual void __fastcall ConstrainedResize(int &MinWidth, int &MinHeight, int &MaxWidth, int &MaxHeight);
	void __fastcall UpdateSectionPanels(void);
	virtual void __fastcall TriggerItemChangeEvent(void);
	virtual void __fastcall TriggerSectionChangeEvent(void);
	DYNAMIC Menus::TPopupMenu* __fastcall GetPopupMenu(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	int __fastcall GetSectionHeight(void);
	int __fastcall GetItemHeight(void);
	Types::TRect __fastcall GetSectionRect(TElSideBarSection* Section);
	Types::TRect __fastcall GetItemsRect();
	Types::TRect __fastcall GetTopSectionsRect();
	Types::TRect __fastcall GetBottomSectionsRect();
	void __fastcall UpdateSection(TElSideBarSection* Section);
	void __fastcall UpdateTopSections(void);
	void __fastcall UpdateBottomSections(void);
	void __fastcall UpdateItems(void);
	void __fastcall UpdateAllSections(void);
	void __fastcall UpdateBar(void);
	void __fastcall UpdateFrame(void);
	virtual int __fastcall GetMinHeight(void);
	virtual void __fastcall CreateWnd(void);
	DYNAMIC void __fastcall GetChildren(Classes::TGetChildProc Proc, Classes::TComponent* Root);
	DYNAMIC Classes::TComponent* __fastcall GetChildOwner(void);
	virtual void __fastcall ShowControl(Controls::TControl* AControl);
	virtual void __fastcall AlignControls(Controls::TControl* AControl, Types::TRect &Rect);
	void __fastcall OnDelayTimer(System::TObject* Sender);
	void __fastcall StartDelayTimer(TElSideBarItem* Item);
	void __fastcall StopDelayTimer(void);
	virtual void __fastcall DestroyWnd(void);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TMessage &Message);
	virtual WideString __fastcall GetThemedClassName();
	virtual Types::TRect __fastcall GetBackgroundClientRect();
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	
public:
	__fastcall virtual TElSideBar(Classes::TComponent* AOwner);
	__fastcall virtual ~TElSideBar(void);
	void __fastcall UpdateChildControl(void);
	void __fastcall ScrollUp(void);
	void __fastcall ScrollDown(void);
	virtual void __fastcall Paint(void);
	void __fastcall BeginUpdate(void);
	void __fastcall EndUpdate(void);
	virtual void __fastcall Loaded(void);
	void __fastcall GetHitTest(int X, int Y, TElSideBarPart &BarPart, TElSideBarSection* &Section, TElSideBarItem* &Item);
	
__published:
	__property bool AutoSelectItem = {read=FAutoSelectItem, write=FAutoSelectItem, nodefault};
	__property TElSideBarSections* Sections = {read=FSections, write=SetSections};
	__property int SectionIndex = {read=GetSectionIndex, write=SetSectionIndex, nodefault};
	__property int ItemIndex = {read=GetItemIndex, write=SetItemIndex, nodefault};
	__property Graphics::TColor SectionsColor = {read=FSectionsColor, write=SetSectionsColor, nodefault};
	__property bool WordWrap = {read=FWordWrap, write=SetWordWrap, nodefault};
	__property int Spacing = {read=FSpacing, write=SetSpacing, nodefault};
	__property int TopSpacing = {read=FTopSpacing, write=SetTopSpacing, nodefault};
	__property int ScrollDelay = {read=FScrollDelay, write=SetScrollDelay, nodefault};
	__property AnsiString ItemChangeSound = {read=FItemChangeSound, write=FItemChangeSound};
	__property AnsiString SectionChangeSound = {read=FSectionChangeSound, write=FSectionChangeSound};
	__property bool SectionTracking = {read=FSectionTracking, write=SetSectionTracking, nodefault};
	__property bool ItemTracking = {read=FItemTracking, write=SetItemTracking, nodefault};
	__property bool UnderlineTracked = {read=FUnderlineTracked, write=SetUnderlineTracked, nodefault};
	__property Elsndmap::TElSoundMap* SoundMap = {read=FSoundMap, write=SetSoundMap};
	__property int ChangeDelay = {read=FChangeDelay, write=FChangeDelay, default=500};
	__property Controls::TImageList* SectionImages = {read=FSectionImages, write=SetSectionImages};
	__property Controls::TImageList* SectionHotImages = {read=FSectionHotImages, write=SetSectionHotImages};
	__property Controls::TImageList* SectionDisabledImages = {read=FSectionDisabledImages, write=SetSectionDisabledImages};
	__property Controls::TImageList* ItemDisabledImages = {read=FItemDisabledImages, write=SetItemDisabledImages};
	__property Controls::TImageList* ItemImages = {read=FItemImages, write=SetItemImages};
	__property Controls::TImageList* ItemHotImages = {read=FItemHotImages, write=SetItemHotImages};
	__property bool FlatSections = {read=FFlatSections, write=SetFlatSections, nodefault};
	__property bool FlatItems = {read=FFlatItems, write=SetFlatItems, nodefault};
	__property bool FlatActiveItem = {read=FFlatActiveItem, write=SetFlatActiveItem, nodefault};
	__property int ItemSize = {read=FItemSize, write=SetItemSize, nodefault};
	__property bool RightAlignedBar = {read=FRightAlignedBar, write=SetRightAlignedBar, nodefault};
	__property int SectionHeight = {read=FSectionHeight, write=SetSectionHeight, nodefault};
	__property Menus::TPopupMenu* ItemsPopupMenu = {read=FItemsPopup, write=SetItemsPopupMenu};
	__property Menus::TPopupMenu* SectionsPopupMenu = {read=FSectionsPopup, write=SetSectionsPopupMenu};
	__property int TopIndex = {read=GetTopIndex, write=SetTopIndex, nodefault};
	__property Graphics::TFontStyles ActiveSectionStyle = {read=FActiveSectionStyle, write=SetActiveSectionStyle, nodefault};
	__property Graphics::TFontStyles ActiveItemStyle = {read=FActiveItemStyle, write=SetActiveItemStyle, nodefault};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, nodefault};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, nodefault};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property Graphics::TFont* ItemsFont = {read=FItemsFont, write=SetItemsFont};
	__property Graphics::TFont* SectionsFont = {read=GetSectionsFont, write=SetSectionsFont};
	__property bool VisibleSections = {read=FVisibleSections, write=SetVisibleSections, nodefault};
	__property Graphics::TColor TrackSectionFontColor = {read=FTrackSectionFontColor, write=SetTrackSectionFontColor, nodefault};
	__property Graphics::TColor TrackSectionBkColor = {read=FTrackSectionBkColor, write=SetTrackSectionBkColor, nodefault};
	__property Graphics::TColor TrackItemFontColor = {read=FTrackItemFontColor, write=SetTrackItemFontColor, nodefault};
	__property Graphics::TColor TrackItemBkColor = {read=FTrackItemBkColor, write=SetTrackItemBkColor, nodefault};
	__property bool KeepSelection = {read=FKeepSelection, write=SetKeepSelection, default=1};
	__property TElSideBarIconLocation IconLocation = {read=FIconLocation, write=SetIconLocation, default=1};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property Classes::TNotifyEvent OnItemChange = {read=FOnItemChange, write=FOnItemChange};
	__property Classes::TNotifyEvent OnSectionChange = {read=FOnSectionChange, write=FOnSectionChange};
	__property Background ;
	__property BackgroundType  = {default=2};
	__property GradientStartColor  = {default=0};
	__property GradientEndColor  = {default=0};
	__property GradientSteps  = {default=16};
	__property Align ;
	__property BorderStyle ;
	__property Color  = {default=-2147483633};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
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
	__property OnResize ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElSideBar(HWND ParentWindow) : Elpanel::TCustomElPanel(ParentWindow) { }
	#pragma option pop
	
};



class DELPHICLASS TElSideBarItems;
class PASCALIMPLEMENTATION TElSideBarItems : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElSideBarItem* operator[](int index) { return Items[index]; }
	
private:
	TElSideBarSection* FSection;
	TElSideBarItem* __fastcall GetItems(int index);
	void __fastcall SetItems(int index, TElSideBarItem* newValue);
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	
public:
	HIDESBASE TElSideBarItem* __fastcall Add(void);
	__fastcall TElSideBarItems(TElSideBarSection* Section);
	__property TElSideBarItem* Items[int index] = {read=GetItems, write=SetItems/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElSideBarItems(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElSideBarSection : public TElSideBarCItem 
{
	typedef TElSideBarCItem inherited;
	
private:
	bool FContainsControls;
	TElSideBarItems* FItems;
	TElSideBarSections* FSections;
	TElSideBarContainerPanel* FPanel;
	bool FInactive;
	void __fastcall SetItems(TElSideBarItems* newValue);
	void __fastcall SetContainsControls(bool newValue);
	void __fastcall AllocatePanel(void);
	
public:
	__fastcall virtual TElSideBarSection(Classes::TCollection* Collection);
	__fastcall virtual ~TElSideBarSection(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	__property TElSideBarContainerPanel* Panel = {read=FPanel};
	
__published:
	__property bool Inactive = {read=FInactive, write=FInactive, nodefault};
	__property TElSideBarItems* Items = {read=FItems, write=SetItems};
	__property bool ContainsControls = {read=FContainsControls, write=SetContainsControls, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elsidebar */
using namespace Elsidebar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElSideBar
