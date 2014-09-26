// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHeader.pas' rev: 6.00

#ifndef ElHeaderHPP
#define ElHeaderHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElVCLUtils.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elheader
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElSectionStyle { ElhsText, ElhsOwnerDraw, ElhsPictureOnly };
#pragma option pop

#pragma option push -b-
enum TElSSortMode { hsmNone, hsmAscend, hsmDescend };
#pragma option pop

#pragma option push -b-
enum TElSAlignment { hsaLeft, hsaCenter, hsaRight };
#pragma option pop

#pragma option push -b-
enum TElHResizingStates { trsBegin, trsMove, trsEnd };
#pragma option pop

#pragma option push -b-
enum TSectionChangeMode { scmCaption, scmFieldName, scmFieldType, scmAlign, scmStyle, scmEditable, scmPassword };
#pragma option pop

#pragma option push -b-
enum TElSectionPart { espResizeArea, espText, espExpandSign, espLookupSign, espFilterSign };
#pragma option pop

#pragma option push -b-
enum TAdjustCondition { acAll, acAutoSizedOnly };
#pragma option pop

#pragma option push -b-
enum TElFieldType { sftCustom, sftText, sftNumber, sftFloating, sftDateTime, sftDate, sftTime, sftPicture, sftEnum, sftBLOB, sftUndef, sftBool, sftCurrency, sftMemo };
#pragma option pop

typedef Set<TElFieldType, sftCustom, sftMemo>  TElFieldTypes;

class DELPHICLASS EElHeaderError;
class PASCALIMPLEMENTATION EElHeaderError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElHeaderError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElHeaderError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElHeaderError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElHeaderError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElHeaderError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElHeaderError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElHeaderError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElHeaderError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElHeaderError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TCustomElHeader;
class DELPHICLASS TElHeaderSection;
typedef void __fastcall (__closure *TElHeaderSectionEvent)(TCustomElHeader* Sender, TElHeaderSection* Section);

typedef void __fastcall (__closure *TElHeaderLookupEvent)(System::TObject* Sender, TElHeaderSection* Section, AnsiString &Text);

typedef void __fastcall (__closure *TElHeaderLookupDoneEvent)(System::TObject* Sender, TElHeaderSection* Section, AnsiString Text, bool Accepted);

class PASCALIMPLEMENTATION TElHeaderSection : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
protected:
	bool FFilterIsActive;
	bool FFilterEnabled;
	WideString FHint;
	Stdctrls::TTextLayout FTextLayout;
	bool FUseMainStyle;
	Graphics::TColor FFontColor;
	Graphics::TColor FColor;
	bool FParentColor;
	bool FLookupEnabled;
	bool FExpandable;
	bool FExpanded;
	TElHeaderSection* FParentSection;
	int FParentIdx;
	Menus::TPopupMenu* FPopupMenu;
	AnsiString FPopupName;
	bool FResizable;
	bool FClickSelect;
	bool FProtected;
	AnsiString FFieldName;
	TElFieldType FFieldType;
	bool FEditable;
	int FImageIndex;
	TElSAlignment FPicAlign;
	int FIntTag;
	int FIndex;
	int FTag;
	bool FVisible;
	TElSectionStyle FStyle;
	int FWidth;
	int FMinWidth;
	int FMaxWidth;
	TElSSortMode FSortMode;
	bool FAllowClick;
	TElSAlignment FAlignment;
	WideString FText;
	void *FData;
	TCustomElHeader* FOwner;
	Classes::TStringList* FLookupHist;
	bool FAutoSize;
	float FStickySize;
	int ASaveSize;
	Classes::TNotifyEvent FOnResize;
	bool FShowSortMark;
	void __fastcall SetWidth(int value);
	int __fastcall GetWidth(void);
	int __fastcall GetLeft(void);
	int __fastcall GetRight(void);
	void __fastcall SetMaxWidth(int value);
	void __fastcall SetMinWidth(int value);
	void __fastcall SetText(WideString value);
	void __fastcall SetStyle(TElSectionStyle value);
	void __fastcall SetSortMode(TElSSortMode value);
	void __fastcall SetAlignment(TElSAlignment value);
	void __fastcall SetVisible(bool value);
	int __fastcall GetIndex(void);
	int __fastcall GetPosition(void);
	void __fastcall SetImageIndex(int newValue);
	void __fastcall SetProtected(bool newValue);
	void __fastcall SetExpandable(bool newValue);
	void __fastcall SetExpanded(bool newValue);
	void __fastcall SetParentSection(TElHeaderSection* newValue);
	void __fastcall SetPopupMenu(Menus::TPopupMenu* newValue);
	bool __fastcall GetVisible(void);
	void __fastcall SetLookupEnabled(bool newValue);
	void __fastcall SetParentColor(bool newValue);
	void __fastcall SetColor(Graphics::TColor newValue);
	void __fastcall SetFontColor(Graphics::TColor newValue);
	void __fastcall SetUseMainStyle(bool newValue);
	void __fastcall SetTextLayout(Stdctrls::TTextLayout newValue);
	void __fastcall SetFilterEnabled(bool newValue);
	void __fastcall SetFilterIsActive(bool newValue);
	void __fastcall SetLookupList(Classes::TStringList* newValue);
	void __fastcall SetAutoSize(bool newValue);
	bool __fastcall GetLocked(void);
	void __fastcall SetShowSortMark(bool Value);
	virtual void __fastcall SetFieldName(AnsiString newValue);
	virtual void __fastcall SetFieldType(TElFieldType newValue);
	virtual void __fastcall SetEditable(bool newValue);
	virtual void __fastcall SetResizable(bool newValue);
	void __fastcall SetSaveSize(int newValue);
	__property int FSaveSize = {read=ASaveSize, write=SetSaveSize, nodefault};
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	__property Classes::TNotifyEvent OnResize = {read=FOnResize, write=FOnResize};
	
public:
	__fastcall TElHeaderSection(TCustomElHeader* AOwner);
	__fastcall virtual ~TElHeaderSection(void);
	void __fastcall UpdateSection(void);
	virtual void __fastcall Assign(Classes::TPersistent* source);
	__property int Index = {read=GetIndex, nodefault};
	__property int Left = {read=GetLeft, nodefault};
	__property int Right = {read=GetRight, nodefault};
	__property int Position = {read=GetPosition, nodefault};
	__property void * Data = {read=FData, write=FData};
	__property bool Locked = {read=GetLocked, nodefault};
	__property TCustomElHeader* Owner = {read=FOwner};
	
__published:
	__property WideString Text = {read=FText, write=SetText};
	__property TElSectionStyle Style = {read=FStyle, write=SetStyle, nodefault};
	__property int Width = {read=GetWidth, write=SetWidth, nodefault};
	__property int MaxWidth = {read=FMaxWidth, write=SetMaxWidth, nodefault};
	__property int MinWidth = {read=FMinWidth, write=SetMinWidth, nodefault};
	__property TElSSortMode SortMode = {read=FSortMode, write=SetSortMode, nodefault};
	__property bool AllowClick = {read=FAllowClick, write=FAllowClick, nodefault};
	__property TElSAlignment Alignment = {read=FAlignment, write=SetAlignment, nodefault};
	__property TElSAlignment PictureAlign = {read=FPicAlign, write=FPicAlign, nodefault};
	__property bool Visible = {read=GetVisible, write=SetVisible, nodefault};
	__property int ImageIndex = {read=FImageIndex, write=SetImageIndex, nodefault};
	__property AnsiString FieldName = {read=FFieldName, write=SetFieldName};
	__property TElFieldType FieldType = {read=FFieldType, write=SetFieldType, nodefault};
	__property bool Editable = {read=FEditable, write=SetEditable, nodefault};
	__property bool Password = {read=FProtected, write=SetProtected, default=0};
	__property bool Resizable = {read=FResizable, write=SetResizable, nodefault};
	__property bool ClickSelect = {read=FClickSelect, write=FClickSelect, nodefault};
	__property bool Expandable = {read=FExpandable, write=SetExpandable, nodefault};
	__property bool Expanded = {read=FExpanded, write=SetExpanded, nodefault};
	__property TElHeaderSection* ParentSection = {read=FParentSection, write=SetParentSection};
	__property Menus::TPopupMenu* PopupMenu = {read=FPopupMenu, write=SetPopupMenu};
	__property bool LookupEnabled = {read=FLookupEnabled, write=SetLookupEnabled, nodefault};
	__property Classes::TStringList* LookupHistory = {read=FLookupHist, write=SetLookupList};
	__property bool ParentColor = {read=FParentColor, write=SetParentColor, nodefault};
	__property Graphics::TColor Color = {read=FColor, write=SetColor, nodefault};
	__property Graphics::TColor FontColor = {read=FFontColor, write=SetFontColor, nodefault};
	__property bool UseMainStyle = {read=FUseMainStyle, write=SetUseMainStyle, nodefault};
	__property Stdctrls::TTextLayout TextLayout = {read=FTextLayout, write=SetTextLayout, nodefault};
	__property bool FilterEnabled = {read=FFilterEnabled, write=SetFilterEnabled, nodefault};
	__property bool FilterIsActive = {read=FFilterIsActive, write=SetFilterIsActive, nodefault};
	__property WideString Hint = {read=FHint, write=FHint};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, nodefault};
	__property bool ShowSortMark = {read=FShowSortMark, write=SetShowSortMark, nodefault};
	__property int Tag = {read=FTag, write=FTag, nodefault};
};


class DELPHICLASS TElHeaderSections;
class PASCALIMPLEMENTATION TElHeaderSections : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
public:
	TElHeaderSection* operator[](int Index) { return Item[Index]; }
	
private:
	Ellist::TElList* FList;
	int __fastcall GetCount(void);
	AnsiString __fastcall GetSectionsOrder();
	void __fastcall SetSectionsOrder(AnsiString newValue);
	
protected:
	TCustomElHeader* FOwner;
	virtual TElHeaderSection* __fastcall GetSectionByIntTag(int IntTag);
	virtual TElHeaderSection* __fastcall GetSection(int index);
	virtual void __fastcall SetSection(int index, TElHeaderSection* Section);
	virtual TElHeaderSection* __fastcall GetSectionByPos(int index);
	virtual TElHeaderSection* __fastcall CreateSection(void);
	virtual void __fastcall WriteData(Classes::TStream* Stream);
	virtual void __fastcall IntReadData(Classes::TStream* Stream, bool ClearCurrent);
	virtual void __fastcall ReadData(Classes::TStream* Stream);
	virtual void __fastcall FrameReadData(Classes::TStream* Stream);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	TElHeaderSection* __fastcall LastVisibleSection(void);
	TElHeaderSection* __fastcall GetPrevVisibleSection(TElHeaderSection* Section);
	virtual TElHeaderSection* __fastcall FindSection(int tag);
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	
public:
	__fastcall TElHeaderSections(TCustomElHeader* AOwner);
	__fastcall virtual ~TElHeaderSections(void);
	void __fastcall Clear(void);
	virtual void __fastcall Assign(Classes::TPersistent* source);
	TElHeaderSection* __fastcall AddSection(void);
	TElHeaderSection* __fastcall InsertSection(int index);
	void __fastcall DeleteSection(TElHeaderSection* Section);
	void __fastcall MoveSection(TElHeaderSection* Section, int NewPos);
	void __fastcall LoadFromStream(Classes::TStream* Stream);
	void __fastcall SaveToStream(Classes::TStream* Stream);
	void __fastcall SaveToFile(AnsiString FileName);
	void __fastcall LoadFromFile(AnsiString FileName);
	__property TCustomElHeader* Owner = {read=FOwner};
	__property int Count = {read=GetCount, nodefault};
	__property TElHeaderSection* ItemByPos[int Index] = {read=GetSectionByPos};
	__property TElHeaderSection* Item[int Index] = {read=GetSection, write=SetSection/*, default*/};
	
__published:
	__property AnsiString SectionsOrder = {read=GetSectionsOrder, write=SetSectionsOrder, stored=false};
};


typedef void __fastcall (__closure *TMeasureSectionEvent)(System::TObject* Sender, TElHeaderSection* Section, Types::TPoint &Size);

typedef void __fastcall (__closure *TElSectionRedrawEvent)(TCustomElHeader* Sender, Graphics::TCanvas* Canvas, TElHeaderSection* Section, const Types::TRect &R, bool Pressed);

typedef void __fastcall (__closure *TElSectionResizingEvent)(TCustomElHeader* Sender, TElHeaderSection* Section, TElHResizingStates State, int Width);

typedef void __fastcall (__closure *TElSectionMoveEvent)(TCustomElHeader* Sender, TElHeaderSection* Section, int OldPos, int NewPos);

typedef void __fastcall (__closure *TPictureNeededEvent)(TCustomElHeader* Sender, TElHeaderSection* Section, int &ImageIndex);

typedef void __fastcall (__closure *TSectionChangeEvent)(TCustomElHeader* Sender, TElHeaderSection* Section, TSectionChangeMode Change);

class PASCALIMPLEMENTATION TCustomElHeader : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	bool FMouseInControl;
	
protected:
	bool FWrapCaptions;
	TElHeaderSection* FLockedSection;
	int FHPos;
	bool FInvertSortArrows;
	bool FFlat;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elimgfrm::TElImageForm* FImgForm;
	Graphics::TColor FActiveFilterColor;
	TElHeaderSectionEvent FOnFilterCall;
	TElHeaderSectionEvent FOnSectionAutoSize;
	Graphics::TColor FFilterColor;
	bool FLockHeight;
	bool FRightAlignedText;
	bool FRightAlignedOrder;
	bool FStickySections;
	bool FMoveOnDrag;
	Elini::TElIniFile* FStorage;
	AnsiString FStoragePath;
	Controls::TImageList* FImages;
	Imglist::TChangeLink* FImageChangeLink;
	TElHeaderSections* FSections;
	bool FTracking;
	bool FAllowDrag;
	bool FPainting;
	Graphics::TBitmap* DragBmp;
	Graphics::TBitmap* SaveBmp;
	Graphics::TBitmap* DragBmpMask;
	#pragma pack(push, 1)
	Types::TRect DragRect;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	Types::TPoint FDragCoord;
	#pragma pack(pop)
	
	Stdctrls::TComboBox* FLookup;
	#pragma pack(push, 1)
	Types::TPoint FPressCoord;
	#pragma pack(pop)
	
	bool FPressed;
	TElHeaderSection* FPressedItem;
	TElHeaderSection* FHintSection;
	TElHeaderSection* FLookupSection;
	TElHeaderSection* FTrackSection;
	bool FResizing;
	TElHeaderSection* FResizeSection;
	TElHeaderSection* FDropSrc;
	TElHeaderSection* FDropTrg;
	int FHeaderLine;
	int FLineTab;
	bool FResizeOnDrag;
	bool FHeaderLineVis;
	bool FIgnoreLookupChange;
	bool FDoingLookup;
	HDC FLineDC;
	unsigned FFocusedCtl;
	int LoadingCount;
	bool DeletionHappened;
	bool AdditionHappened;
	bool FInStick;
	int FOldWidth;
	int FUpdateCount;
	Htmlrender::TElHTMLRender* FRender;
	TElHeaderSectionEvent FOnSectionClick;
	TElHeaderSectionEvent FOnSectionResize;
	TElSectionRedrawEvent FOnSectionDraw;
	TElSectionResizingEvent FOnSectionResizing;
	TElHeaderSectionEvent FOnSectionDelete;
	TElSectionMoveEvent FOnSectionMove;
	TElHeaderSectionEvent FOnVisibleChange;
	TPictureNeededEvent FOnPictureNeeded;
	TSectionChangeEvent FOnSectionChange;
	TElHeaderSectionEvent FOnSectionCreate;
	TElHeaderLookupEvent FOnHeaderLookup;
	TElHeaderLookupDoneEvent FOnHeaderLookupDone;
	TElHeaderSectionEvent FOnSectionExpand;
	TElHeaderSectionEvent FOnSectionCollapse;
	TMeasureSectionEvent FOnMeasureSection;
	int FDefaultWidth;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	WideString FHint;
	bool FMultiSort;
	void __fastcall DrawLine(bool Restore);
	void __fastcall AllocateLineDC(void);
	void __fastcall ReleaseLineDC(void);
	int __fastcall GetColumnsWidth(void);
	void __fastcall InvalidateRight(int value);
	void __fastcall SetTracking(bool newValue);
	void __fastcall IntMouseEnter(void);
	void __fastcall IntMouseLeave(void);
	void __fastcall IntSize(void);
	void __fastcall IntExit(void);
	void __fastcall IntLButtonDown(short XPos, short YPos);
	void __fastcall IntLButtonUp(short XPos, short YPos);
	bool __fastcall IntRButtonDown(short XPos, short YPos);
	bool __fastcall IntRButtonUp(short XPos, short YPos);
	void __fastcall IntMouseMove(short XPos, short YPos);
	void __fastcall IntLButtonDblClick(short XPos, short YPos);
	bool __fastcall IntHintShow(Forms::THintInfo &HintInfo);
	HIDESBASE MESSAGE void __fastcall CMDrag(Controls::TCMDrag &Message);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMRButtonDown(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMCancelMode(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	void __fastcall SetSections(TElHeaderSections* value);
	void __fastcall SetImages(Controls::TImageList* newValue);
	void __fastcall OnImageListChange(System::TObject* Sender);
	void __fastcall GetDragImage(int XPos);
	void __fastcall SetStorage(Elini::TElIniFile* newValue);
	void __fastcall EditExit(System::TObject* Sender);
	void __fastcall EditKeyDown(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall EditKeyUp(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall EditChange(System::TObject* Sender);
	void __fastcall SetStickySections(bool newValue);
	void __fastcall AdjustHeaderHeight(void);
	void __fastcall AdjustStickySize(TElHeaderSection* Caller);
	bool __fastcall IsLoading(void);
	void __fastcall SetRightAlignedText(bool newValue);
	void __fastcall SetRightAlignedOrder(bool newValue);
	void __fastcall SetLockHeight(bool newValue);
	void __fastcall SetFilterColor(Graphics::TColor newValue);
	void __fastcall SetActiveFilterColor(Graphics::TColor newValue);
	void __fastcall SetFlat(bool newValue);
	bool __fastcall GetIsDesigning(void);
	void __fastcall SetIsDesigning(bool newValue);
	void __fastcall SetInvertSortArrows(bool newValue);
	void __fastcall SetLeftPos(int newValue);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall SetLockedSection(TElHeaderSection* newValue);
	virtual void __fastcall SetWrapCaptions(bool newValue);
	void __fastcall RedrawSection(Graphics::TCanvas* Canvas, TElHeaderSection* Section, const Types::TRect &R, bool Dithered);
	void __fastcall RedrawSections(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual TElHeaderSections* __fastcall CreateSections(void);
	virtual bool __fastcall InResizeArea(int X, TElHeaderSection* &HitSection);
	virtual void __fastcall Paint(void);
	DYNAMIC void __fastcall Resize(void);
	virtual int __fastcall DoGetPicture(TElHeaderSection* Section);
	virtual void __fastcall DoVisChanged(TElHeaderSection* Section);
	virtual void __fastcall DoSectionDelete(TElHeaderSection* Section);
	virtual void __fastcall DoSectionMove(TElHeaderSection* Section, int OldPos, int NewPos);
	virtual void __fastcall DoSectionResizing(TElHeaderSection* Section, TElHResizingStates State, int NewWidth);
	virtual void __fastcall DoSectionResize(TElHeaderSection* Section);
	virtual void __fastcall DoSectionClick(TElHeaderSection* Section);
	virtual void __fastcall DoSectionDraw(Graphics::TCanvas* Canvas, TElHeaderSection* Section, const Types::TRect &R, bool Pressed);
	virtual void __fastcall DoNotifySectionChange(TElHeaderSection* Section, TSectionChangeMode Change);
	virtual void __fastcall DoSectionExpandEvent(TElHeaderSection* Section);
	virtual void __fastcall DoSectionCollapseEvent(TElHeaderSection* Section);
	virtual void __fastcall DoSectionCreate(TElHeaderSection* Section);
	virtual void __fastcall DoSectionLookupEvent(TElHeaderSection* Section, AnsiString &Text);
	virtual void __fastcall DoSectionLookupDoneEvent(TElHeaderSection* Section, AnsiString Text, bool Accepted);
	virtual void __fastcall TriggerSectionAutoSizeEvent(TElHeaderSection* Section);
	virtual void __fastcall TriggerFilterCallEvent(TElHeaderSection* Section);
	virtual void __fastcall TriggerMeasureSectionEvent(TElHeaderSection* Section, Types::TPoint &Size);
	void __fastcall OnFontChange(System::TObject* Sender);
	int __fastcall GetResizableWidth(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	virtual void __fastcall CreateHandle(void);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	virtual WideString __fastcall GetThemedClassName();
	void __fastcall SetHint(WideString Value);
	__property int SectionsWidth = {read=GetColumnsWidth, nodefault};
	__property TElHeaderSections* Sections = {read=FSections, write=SetSections};
	__property bool ResizeOnDrag = {read=FResizeOnDrag, write=FResizeOnDrag, nodefault};
	__property bool Tracking = {read=FTracking, write=SetTracking, nodefault};
	__property bool AllowDrag = {read=FAllowDrag, write=FAllowDrag, nodefault};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property bool MoveOnDrag = {read=FMoveOnDrag, write=FMoveOnDrag, nodefault};
	__property AnsiString StoragePath = {read=FStoragePath, write=FStoragePath};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=SetStorage};
	__property bool StickySections = {read=FStickySections, write=SetStickySections, nodefault};
	__property bool RightAlignedText = {read=FRightAlignedText, write=SetRightAlignedText, nodefault};
	__property bool RightAlignedOrder = {read=FRightAlignedOrder, write=SetRightAlignedOrder, nodefault};
	__property bool LockHeight = {read=FLockHeight, write=SetLockHeight, nodefault};
	__property Graphics::TColor FilterColor = {read=FFilterColor, write=SetFilterColor, nodefault};
	__property Graphics::TColor ActiveFilterColor = {read=FActiveFilterColor, write=SetActiveFilterColor, nodefault};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property TSectionChangeEvent OnSectionChange = {read=FOnSectionChange, write=FOnSectionChange};
	__property TElHeaderSectionEvent OnSectionShowHide = {read=FOnVisibleChange, write=FOnVisibleChange};
	__property TElHeaderSectionEvent OnSectionClick = {read=FOnSectionClick, write=FOnSectionClick};
	__property TElHeaderSectionEvent OnSectionResize = {read=FOnSectionResize, write=FOnSectionResize};
	__property TElSectionRedrawEvent OnSectionDraw = {read=FOnSectionDraw, write=FOnSectionDraw};
	__property TElHeaderSectionEvent OnSectionDelete = {read=FOnSectionDelete, write=FOnSectionDelete};
	__property TElSectionResizingEvent OnSectionResizing = {read=FOnSectionResizing, write=FOnSectionResizing};
	__property TElSectionMoveEvent OnSectionMove = {read=FOnSectionMove, write=FOnSectionMove};
	__property TPictureNeededEvent OnPictureNeeded = {read=FOnPictureNeeded, write=FOnPictureNeeded};
	__property TElHeaderSectionEvent OnSectionCreate = {read=FOnSectionCreate, write=FOnSectionCreate};
	__property TElHeaderSectionEvent OnSectionExpand = {read=FOnSectionExpand, write=FOnSectionExpand};
	__property TElHeaderSectionEvent OnSectionCollapse = {read=FOnSectionCollapse, write=FOnSectionCollapse};
	__property TElHeaderLookupEvent OnHeaderLookup = {read=FOnHeaderLookup, write=FOnHeaderLookup};
	__property TElHeaderLookupDoneEvent OnHeaderLookupDone = {read=FOnHeaderLookupDone, write=FOnHeaderLookupDone};
	__property TMeasureSectionEvent OnMeasureSection = {read=FOnMeasureSection, write=FOnMeasureSection};
	__property TElHeaderSectionEvent OnSectionAutoSize = {read=FOnSectionAutoSize, write=FOnSectionAutoSize};
	__property TElHeaderSectionEvent OnFilterCall = {read=FOnFilterCall, write=FOnFilterCall};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property bool IsDesigning = {read=GetIsDesigning, write=SetIsDesigning, nodefault};
	__property bool InvertSortArrows = {read=FInvertSortArrows, write=SetInvertSortArrows, default=0};
	__property bool WrapCaptions = {read=FWrapCaptions, write=SetWrapCaptions, nodefault};
	__property int DefaultWidth = {read=FDefaultWidth, write=FDefaultWidth, default=120};
	__property Htmlrender::TElHTMLImageNeededEvent OnHTMLImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	
public:
	__fastcall virtual TCustomElHeader(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElHeader(void);
	TElHeaderSection* __fastcall GetSectionAt(int X, int Y);
	TElHeaderSection* __fastcall GetSectionAtEx(int X, int Y, TElSectionPart &SectionPart);
	Types::TRect __fastcall GetSectionRect(int SectionNum);
	int __fastcall MeasureSectionWidth(TElHeaderSection* Section, System::PInteger TextWidth, System::PInteger SectionHeight);
	int __fastcall CalcHeaderHeight(void);
	__property Canvas ;
	virtual void __fastcall Loaded(void);
	bool __fastcall Setup(void);
	void __fastcall Save(void);
	void __fastcall Restore(void);
	virtual void __fastcall Update(void);
	void __fastcall BeginUpdate(void);
	void __fastcall EndUpdate(void);
	void __fastcall MarkStickySections(void);
	__property int LeftPos = {read=FHPos, write=SetLeftPos, nodefault};
	__property TElHeaderSection* LockedSection = {read=FLockedSection, write=SetLockedSection};
	__property bool MultiSort = {read=FMultiSort, write=FMultiSort, nodefault};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElHeader(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElHeader;
class PASCALIMPLEMENTATION TElHeader : public TCustomElHeader 
{
	typedef TCustomElHeader inherited;
	
__published:
	__property ActiveFilterColor ;
	__property AllowDrag ;
	__property Align  = {default=0};
	__property Color  = {default=-2147483643};
	__property DefaultWidth  = {default=120};
	__property Enabled  = {default=1};
	__property Flat ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
	__property MoveOnDrag ;
	__property Font ;
	__property FilterColor ;
	__property Images ;
	__property ImageForm ;
	__property InvertSortArrows  = {default=0};
	__property LockHeight ;
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ResizeOnDrag ;
	__property RightAlignedText ;
	__property RightAlignedOrder ;
	__property SectionsWidth ;
	__property Sections ;
	__property ShowHint ;
	__property StickySections ;
	__property Tracking ;
	__property Storage ;
	__property StoragePath ;
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property WrapCaptions ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnResize ;
	__property OnSectionClick ;
	__property OnSectionResize ;
	__property OnSectionDraw ;
	__property OnSectionResizing ;
	__property OnSectionDelete ;
	__property OnSectionMove ;
	__property OnSectionShowHide ;
	__property OnPictureNeeded ;
	__property OnSectionChange ;
	__property OnSectionCreate ;
	__property OnSectionExpand ;
	__property OnSectionCollapse ;
	__property OnHeaderLookup ;
	__property OnHeaderLookupDone ;
	__property OnHTMLImageNeeded ;
	__property OnSectionAutoSize ;
	__property OnFilterCall ;
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElHeader.Create */ inline __fastcall virtual TElHeader(Classes::TComponent* AOwner) : TCustomElHeader(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElHeader.Destroy */ inline __fastcall virtual ~TElHeader(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHeader(HWND ParentWindow) : TCustomElHeader(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint ResizeWidth = 0x5;
extern PACKAGE Graphics::TBitmap* ElHeaderAscBmp;
extern PACKAGE Graphics::TBitmap* ElHeaderDescBmp;
extern PACKAGE Graphics::TBitmap* ElHeaderLeftBmp;
extern PACKAGE Graphics::TBitmap* ElHeaderRightBmp;
extern PACKAGE Graphics::TBitmap* ElHeaderPointBmp;

}	/* namespace Elheader */
using namespace Elheader;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHeader
