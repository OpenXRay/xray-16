// Borland C++ Builder 
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTree.pas' rev: 6.00

#ifndef ElTreeHPP                                        
#define ElTreeHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ActiveX.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElExtBkgnd.hpp>	// Pascal unit
#include <ElDragDrop.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElArray.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <Variants.hpp>	// Pascal unit
#include <ElHintWnd.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElScrollBar.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElHook.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltree
{
//-- type declarations -------------------------------------------------------
typedef Set<Shortint, 1, 8>  TSTIStates;

class DELPHICLASS EElTreeError;
class PASCALIMPLEMENTATION EElTreeError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElTreeError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElTreeError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElTreeError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElTreeError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElTreeError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElTreeError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElTreeError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElTreeError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElTreeError(void) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TItemChangeMode { icmText, icmState, icmCheckState };
#pragma option pop

#pragma option push -b-
enum TSTItemPart { ipButton, ipMainText, ipColumn, ipInside, ipPicture, ipPicture2, ipCheckBox, ipOutside };
#pragma option pop

#pragma option push -b-
enum TSTSelModes { smSimple, smUsual };
#pragma option pop

#pragma option push -b-
enum TSortDirs { sdAscend, sdDescend };
#pragma option pop

#pragma option push -b-
enum TSortModes { smNone, smAdd, smClick, smAddClick };
#pragma option pop

#pragma option push -b-
enum TSortTypes { stCustom, stText, stNumber, stFloating, stDateTime, stDate, stTime, stBoolean, stCurrency };
#pragma option pop

#pragma option push -b-
enum THintModes { shmNone, shmLong, shmAll };
#pragma option pop

#pragma option push -b-
enum TLineHintType { lhtMainTextOnly, lhtCellTextOnly, lhtSmart };
#pragma option pop

#pragma option push -b-
enum TElHintType { shtMainText, shtHintOnly, shtHintOrText };
#pragma option pop

#pragma option push -b-
enum TDragImgMode { dimNever, dimOne, dimAll };
#pragma option pop

#pragma option push -b-
enum TNodeAttachMode { naAdd, naAddFirst, naAddChild, naAddChildFirst, naInsert };
#pragma option pop

#pragma option push -b-
enum TElCheckBoxType { ectCheckBox, ect3SCheckBox, ectRadioButton };
#pragma option pop

#pragma option push -b-
enum TVirtualityLevel { vlNone, vlTextAndStyles };
#pragma option pop

#pragma option push -b-
enum TElItemBorderStyle { ibsNone, ibsRaised, ibsFlat, ibsSunken, ibsSpace };
#pragma option pop

#pragma option push -b-
enum TElDragType { dtOLE, dtDelphi, dtBoth };
#pragma option pop

#pragma option push -b-
enum TElDblClickMode { dcmNone, dcmExpand, dcmEdit };
#pragma option pop

#pragma option push -b-
enum TDragTargetDraw { ColorFrame, ColorRect, SelColorRect, dtdNone, dtdUpColorLine, dtdDownColorLine, dtdUpSelColorLine, dtdDownSelColorLine };
#pragma option pop

typedef TElWideStrings TElFStrings;
;

typedef TElWideStringList TElFStringList;
;

class DELPHICLASS TElTreeItem;
class DELPHICLASS TElTreeInplaceEditor;
typedef void __fastcall (__closure *TInplaceEditorNeededEvent)(System::TObject* Sender, TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType SupposedFieldType, TElTreeInplaceEditor* &Editor);

typedef void __fastcall (__closure *TInplaceOperationEvent)(System::TObject* Sender, bool &DefaultConversion);

typedef void __fastcall (__closure *TInplaceAfterOperationEvent)(System::TObject* Sender, bool &Accepted, bool &DefaultConversion);

typedef void __fastcall (__closure *TInplaceValidationEvent)(System::TObject* Sender, bool &InputValid);

class DELPHICLASS TCustomElTree;
class DELPHICLASS TElTreeInplaceManager;
class PASCALIMPLEMENTATION TElTreeInplaceManager : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Ellist::TElList* FEditorsList;
	
protected:
	void __fastcall RegisterEditor(TElTreeInplaceEditor* Editor);
	void __fastcall UnregisterEditor(TElTreeInplaceEditor* Editor);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TElTreeInplaceManager(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceManager(void);
	TElTreeInplaceEditor* __fastcall GetSuitableEditor(Elheader::TElFieldType SupposedFieldType);
};


class DELPHICLASS TElCellStyle;
typedef void __fastcall (__closure *TVirtualStyleNeededEvent)(System::TObject* Sender, TElTreeItem* Item, int SectionIndex, TElCellStyle* Style);

typedef void __fastcall (__closure *TVirtualTextNeededEvent)(System::TObject* Sender, TElTreeItem* Item, int SectionIndex, WideString &Text);

typedef void __fastcall (__closure *TVirtualHintNeededEvent)(System::TObject* Sender, TElTreeItem* Item, WideString &Hint);

typedef void __fastcall (__closure *TVirtualValueNeededEvent)(System::TObject* Sender, TElTreeItem* Item, int SectionIndex, int VarType, Variant &Value);

typedef void __fastcall (__closure *TElTreeChangingEvent)(System::TObject* Sender, TElTreeItem* Item, bool &AllowChange);

typedef void __fastcall (__closure *TOnItemExpandEvent)(System::TObject* Sender, TElTreeItem* Item);

typedef void __fastcall (__closure *TElTreeItemDragTargetEvent)(System::TObject* Sender, TElTreeItem* Item, const Types::TRect &ItemRect, int X, int Y);

typedef void __fastcall (__closure *TApplyVisFilterEvent)(System::TObject* Sender, TElTreeItem* Item, bool &Hidden);

class DELPHICLASS TElTreeItems;
typedef TMetaClass*TElTreeItemClass;

typedef void __fastcall (*TIterateProc)(TElTreeItem* Item, int Index, bool &ContinueIterate, void * IterateData, TCustomElTree* Tree);

typedef bool __fastcall (*TElLookupCompareProc)(TElTreeItem* Item, void * SearchDetails);

class PASCALIMPLEMENTATION TElTreeItems : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
public:
	TElTreeItem* operator[](int Index) { return Item[Index]; }
	
private:
	TCustomElTree* FOwner;
	TElTreeItem* __fastcall GetItem(int index);
	TElTreeItem* __fastcall GetVisItem(int index);
	
protected:
	TElTreeItem* FRoot;
	TMetaClass*FItemClass;
	int __fastcall GetVisCount(void);
	virtual void __fastcall ReadData(Classes::TStream* Stream);
	virtual void __fastcall WriteData(Classes::TStream* Stream);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	virtual TElTreeItem* __fastcall CreateItem(TCustomElTree* FOwner);
	int __fastcall GetCount(void);
	int __fastcall GetRootCount(void);
	TElTreeItem* __fastcall GetRootItem(int Index);
	
public:
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	__fastcall virtual TElTreeItems(TCustomElTree* AOwner);
	__fastcall TElTreeItems(TCustomElTree* AOwner, TMetaClass* ItemClass);
	__fastcall virtual ~TElTreeItems(void);
	void __fastcall LoadFromStream(Classes::TStream* Stream);
	void __fastcall SaveToStream(Classes::TStream* Stream);
	void __fastcall SaveToFile(AnsiString FileName);
	void __fastcall LoadFromFile(AnsiString FileName);
	void __fastcall SaveToStringList(Classes::TStrings* AStrings);
	void __fastcall LoadFromStringList(Classes::TStrings* Strings);
	void __fastcall LoadFromWideStringList(Elunicodestrings::TElWideStrings* Strings);
	void __fastcall SaveToWideStringList(Elunicodestrings::TElWideStrings* AStrings);
	void __fastcall DeleteItem(TElTreeItem* Child);
	int __fastcall GetAbsIndex(TElTreeItem* Child);
	int __fastcall GetVisIndex(TElTreeItem* Child);
	TElTreeItem* __fastcall AddItem(TElTreeItem* Parent);
	TElTreeItem* __fastcall AddLastItem(TElTreeItem* Parent);
	void __fastcall SetItem(int Index, TElTreeItem* Value);
	TElTreeItem* __fastcall InsertItem(int Index, TElTreeItem* Parent);
	void __fastcall AllocateStorage(int MaxItems);
	virtual TElTreeItem* __fastcall Add(TElTreeItem* Item, WideString Text);
	virtual TElTreeItem* __fastcall AddChild(TElTreeItem* Item, WideString Text);
	virtual TElTreeItem* __fastcall AddChildFirst(TElTreeItem* Item, WideString Text);
	virtual TElTreeItem* __fastcall AddChildObject(TElTreeItem* Item, WideString Text, void * Ptr);
	virtual TElTreeItem* __fastcall AddChildObjectFirst(TElTreeItem* Item, WideString Text, void * Ptr);
	virtual TElTreeItem* __fastcall AddFirst(TElTreeItem* Item, WideString Text);
	virtual TElTreeItem* __fastcall AddObject(TElTreeItem* Item, WideString Text, void * Ptr);
	virtual TElTreeItem* __fastcall AddObjectFirst(TElTreeItem* Item, WideString Text, void * Ptr);
	virtual TElTreeItem* __fastcall Insert(TElTreeItem* Item, WideString Text);
	virtual TElTreeItem* __fastcall InsertObject(TElTreeItem* Item, WideString Text, void * Ptr);
	virtual TElTreeItem* __fastcall InsertAfter(TElTreeItem* Item, WideString Text);
	virtual TElTreeItem* __fastcall InsertAfterObject(TElTreeItem* Item, WideString Text, void * Ptr);
	void __fastcall InsertItemFromString(int Index, WideString AString);
	virtual void __fastcall Delete(TElTreeItem* Item);
	TElTreeItem* __fastcall GetFirstNode(void);
	void __fastcall Clear(void);
	void __fastcall IterateBranch(bool VisibleOnly, TIterateProc IterateProc, void * IterateData, TElTreeItem* BranchParent);
	void __fastcall IterateFrom(bool VisibleOnly, bool CheckCollapsed, TIterateProc IterateProc, void * IterateData, TElTreeItem* StartFrom);
	void __fastcall IterateBackFrom(bool VisibleOnly, bool CheckCollapsed, TIterateProc IterateProc, void * IterateData, TElTreeItem* StartFrom);
	void __fastcall Iterate(bool VisibleOnly, bool CheckCollapsed, TIterateProc IterateProc, void * IterateData);
	void __fastcall IterateBack(bool VisibleOnly, bool CheckCollapsed, TIterateProc IterateProc, void * IterateData);
	virtual void __fastcall BeginUpdate(void);
	virtual void __fastcall EndUpdate(void);
	TElTreeItem* __fastcall LookForItem(TElTreeItem* StartItem, WideString TextToFind, void * DataToFind, int ColumnNum, bool LookForData, bool CheckStartItem, bool SubItemsOnly, bool VisibleOnly, bool NoCase);
	TElTreeItem* __fastcall LookForItem2(TElTreeItem* StartItem, WideString TextToFind, bool WholeTextOnly, void * DataToFind, int ColumnNum, bool LookForData, bool CheckStartItem, bool SubItemsOnly, bool VisibleOnly, bool CheckCollapsed, bool NoCase);
	TElTreeItem* __fastcall LookForItemEx(TElTreeItem* StartItem, int ColumnNum, bool CheckStartItem, bool SubItemsOnly, bool VisibleOnly, void * SearchDetails, TElLookupCompareProc CompareProc);
	TElTreeItem* __fastcall LookBackForItemEx2(TElTreeItem* StartItem, int ColumnNum, bool CheckStartItem, bool SubItemsOnly, bool VisibleOnly, bool CheckCollapsed, void * SearchDetails, TElLookupCompareProc CompareProc);
	TElTreeItem* __fastcall LookForItemEx2(TElTreeItem* StartItem, int ColumnNum, bool CheckStartItem, bool SubItemsOnly, bool VisibleOnly, bool CheckCollapsed, void * SearchDetails, TElLookupCompareProc CompareProc);
	__property TMetaClass* ItemClass = {read=FItemClass, write=FItemClass};
	__property TCustomElTree* Owner = {read=FOwner};
	__property TElTreeItem* Item[int Index] = {read=GetItem/*, default*/};
	__property TElTreeItem* ItemAsVis[int Index] = {read=GetVisItem};
	__property int Count = {read=GetCount, nodefault};
	__property int VisCount = {read=GetVisCount, nodefault};
	__property int RootCount = {read=GetRootCount, nodefault};
	__property TElTreeItem* RootItem[int Index] = {read=GetRootItem};
};


typedef void __fastcall (__closure *TColumnNotifyEvent)(System::TObject* Sender, int SectionIndex);

typedef void __fastcall (__closure *TOnItemChangeEvent)(System::TObject* Sender, TElTreeItem* Item, TItemChangeMode ItemChangeMode);

typedef void __fastcall (__closure *TOnItemDrawEvent)(System::TObject* Sender, TElTreeItem* Item, Graphics::TCanvas* Surface, const Types::TRect &R, int SectionIndex);

typedef void __fastcall (__closure *TOnItemCheckedEvent)(System::TObject* Sender, TElTreeItem* Item);

typedef void __fastcall (__closure *TOnItemExpanding)(System::TObject* Sender, TElTreeItem* Item, bool &CanProcess);

typedef void __fastcall (__closure *TElTreeItemPostDrawEvent)(System::TObject* Sender, Graphics::TCanvas* Canvas, TElTreeItem* Item, const Types::TRect &ItemRect, bool &DrawFocusRect);

typedef void __fastcall (__closure *TOnShowHintEvent)(System::TObject* Sender, TElTreeItem* Item, Elheader::TElHeaderSection* Section, WideString &Text, Controls::THintWindow* HintWindow, const Types::TPoint &MousePos, bool &DoShowHint);

typedef void __fastcall (__closure *TOnCompareItems)(System::TObject* Sender, TElTreeItem* Item1, TElTreeItem* Item2, int &res);

typedef void __fastcall (__closure *TOnPicDrawEvent)(System::TObject* Sender, TElTreeItem* Item, int &ImageIndex);

typedef void __fastcall (__closure *THotTrackEvent)(System::TObject* Sender, TElTreeItem* OldItem, TElTreeItem* NewItem);

typedef void __fastcall (__closure *TElScrollEvent)(System::TObject* Sender, Forms::TScrollBarKind ScrollBarKind, int ScrollCode);

typedef void __fastcall (__closure *TItemSaveEvent)(System::TObject* Sender, Classes::TStream* Stream, TElTreeItem* Item);

typedef void __fastcall (__closure *TTryEditEvent)(System::TObject* Sender, TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType &CellType, bool &CanEdit);

typedef void __fastcall (__closure *TElColumnMoveEvent)(TCustomElTree* Sender, Elheader::TElHeaderSection* Section, int OldPos, int NewPos);

typedef void __fastcall (__closure *TCellStyleSaveEvent)(System::TObject* Sender, Classes::TStream* Stream, TElCellStyle* Style);

typedef void __fastcall (__closure *TItemSelChangeEvent)(System::TObject* Sender, TElTreeItem* Item);

typedef void __fastcall (__closure *THeaderSectionEvent)(System::TObject* Sender, Elheader::TElHeaderSection* Section);

typedef void __fastcall (__closure *TMeasureItemPartEvent)(System::TObject* Sender, TElTreeItem* Item, int PartIndex, Types::TPoint &Size);

typedef void __fastcall (__closure *TOleDragFinishEvent)(System::TObject* Sender, Eldragdrop::TDragType dwEffect, HRESULT Result);

typedef void __fastcall (__closure *TOleDragStartEvent)(System::TObject* Sender, _di_IDataObject &dataObj, _di_IDropSource &dropSource, Eldragdrop::TDragTypes &dwOKEffects);

class DELPHICLASS TElTreeView;
class DELPHICLASS TElCellControl;
class PASCALIMPLEMENTATION TElCellControl : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Menus::TPopupMenu* FPopupMenu;
	TElCellStyle* FOwner;
	WideString FCaption;
	bool FVisible;
	bool FEnabled;
	Classes::TNotifyEvent FOnClick;
	Controls::TMouseEvent FOnMouseDown;
	Controls::TMouseEvent FOnMouseUp;
	Classes::TNotifyEvent FOnDblClick;
	Controls::TMouseMoveEvent FOnMouseMove;
	Graphics::TFont* FFont;
	void __fastcall SetPopupMenu(Menus::TPopupMenu* newValue);
	void __fastcall FontChanged(System::TObject* Sender);
	void __fastcall SetFont(Graphics::TFont* newValue);
	
protected:
	int FBorderWidth;
	virtual void __fastcall SetCaption(WideString newValue);
	virtual void __fastcall SetVisible(bool newValue);
	virtual void __fastcall SetEnabled(bool newValue);
	virtual void __fastcall TriggerClickEvent(void);
	virtual void __fastcall TriggerMouseDownEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerMouseUpEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerDblClickEvent(void);
	virtual void __fastcall TriggerMouseMoveEvent(Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall SetBorderWidth(int Value);
	virtual bool __fastcall PassClicks(void);
	
public:
	virtual void __fastcall Update(void);
	HIDESBASE virtual void __fastcall Assign(TElCellControl* Source) = 0 ;
	virtual void __fastcall Paint(Graphics::TCanvas* Canvas, const Types::TRect &Rect) = 0 ;
	__fastcall virtual TElCellControl(void);
	__fastcall virtual ~TElCellControl(void);
	__property int BorderWidth = {read=FBorderWidth, write=SetBorderWidth, nodefault};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	
__published:
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property TElCellStyle* Owner = {read=FOwner};
	__property bool Enabled = {read=FEnabled, write=SetEnabled, default=1};
	__property bool Visible = {read=FVisible, write=SetVisible, default=1};
	__property Classes::TNotifyEvent OnClick = {read=FOnClick, write=FOnClick};
	__property Controls::TMouseEvent OnMouseDown = {read=FOnMouseDown, write=FOnMouseDown};
	__property Controls::TMouseEvent OnMouseUp = {read=FOnMouseUp, write=FOnMouseUp};
	__property Classes::TNotifyEvent OnDblClick = {read=FOnDblClick, write=FOnDblClick};
	__property Controls::TMouseMoveEvent OnMouseMove = {read=FOnMouseMove, write=FOnMouseMove};
	__property Menus::TPopupMenu* PopupMenu = {read=FPopupMenu, write=SetPopupMenu};
};


class PASCALIMPLEMENTATION TElTreeView : public Controls::TCustomControl 
{
	typedef Controls::TCustomControl inherited;
	
protected:
	Elheader::TElHeader* FHeader;
	TCustomElTree* FOwner;
	TElTreeItems* FItems;
	TElCellStyle* VirtStyle;
	Extctrls::TTimer* FHintTimer;
	Elhintwnd::TElHintWindow* FHintWnd;
	#pragma pack(push, 1)
	Types::TPoint FHintCoord;
	#pragma pack(pop)
	
	TElTreeItem* FHintItem;
	TElTreeItem* FHintItemEx;
	bool FPainting;
	bool FClearVis;
	bool FClearAll;
	bool FVisUpdated;
	bool FRangeUpdate;
	int FHRange;
	#pragma pack(push, 1)
	Types::TPoint FPressCoord;
	#pragma pack(pop)
	
	bool FPressed;
	bool FMouseSel;
	#pragma pack(push, 1)
	Types::TPoint FClickCoord;
	#pragma pack(pop)
	
	bool FClicked;
	TElCellControl* FClickControl;
	bool FIgnoreClick;
	bool FIgnoreClick2;
	bool FClickPassed;
	TElTreeItem* FPassedItem;
	Classes::TShiftState FPassedShift;
	int FClickSection;
	TElTreeItem* FClickItem;
	TElTreeItem* FTrackItem;
	TElTreeItem* FEditingItem;
	TElTreeItem* FFocused;
	TElTreeItem* FSelected;
	TElTreeItem* FDropTrg;
	TElTreeItem* FMFSStartItem;
	#pragma pack(push, 1)
	Types::TPoint FMFSStartCoord;
	#pragma pack(pop)
	
	TElTreeItem* FMFSEndItem;
	#pragma pack(push, 1)
	Types::TPoint FMFSendCoord;
	#pragma pack(pop)
	
	Ellist::TElList* FMFSList;
	Ellist::TElList* FVisible;
	bool FOverColors;
	bool FRowOvColors;
	Extctrls::TTimer* FDragScrollTimer;
	Extctrls::TTimer* FDragExpandTimer;
	bool FDropAcc;
	bool FInDragging;
	int FDDY;
	Controls::TImageList* FDragImages;
	TElTreeInplaceEditor* FInpEdit;
	bool FEditing;
	Elheader::TElFieldType FEditType;
	int FEditSect;
	Extctrls::TTimer* FEditTimer;
	TElTreeItem* FItemToEdit;
	bool FOldHide;
	Menus::TPopupMenu* FFakePopup;
	Htmlrender::TElHTMLRender* FRender;
	Graphics::TBitmap* FTmpBmp;
	AnsiString SearchText;
	Classes::TThread* SearchTextTimeoutThread;
	bool FScrollFirstClick;
	bool FHasFocus;
	void __fastcall StartClearSearchTimeoutThread(void);
	void __fastcall StopClearSearchTimeoutThread(void);
	void __fastcall SearchTextTimeout(System::TObject* Sender);
	bool __fastcall ProcessSearch(char Key);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	void __fastcall RedoTmpBmp(void);
	void __fastcall RedrawTree(Graphics::TCanvas* ACanvas, int RealLeftPos, Ellist::TElList* ItemsList);
	void __fastcall DrawImages(Graphics::TCanvas* ACanvas, TElTreeItem* Item, Graphics::TBitmap* HelperBitmap, Types::TRect &R, Types::TRect &ItemRect);
	void __fastcall DrawButtons(Graphics::TCanvas* ACanvas, TElTreeItem* Item, bool IsNode, Graphics::TBitmap* HelperBitmap, Types::TRect &R, Types::TRect &ItemRect);
	void __fastcall DrawCheckBoxes(Graphics::TCanvas* ACanvas, TElTreeItem* Item, Graphics::TBitmap* HelperBitmap, Types::TRect &R, Types::TRect &ItemRect);
	void __fastcall DrawItemLines(Graphics::TCanvas* ACanvas, TElTreeItem* Item, Types::TRect &R, Types::TRect &ItemRect);
	void __fastcall DoRedrawItem(Graphics::TCanvas* ACanvas, TElTreeItem* Item, const Types::TRect &ItemRect, const Types::TRect &SurfRect);
	void __fastcall DoRedrawItemTree(Graphics::TCanvas* ACanvas, TElTreeItem* Item, const Types::TRect &ItemRect, const Types::TRect &SurfRect);
	virtual void __fastcall Paint(void);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	void __fastcall DoPaintBkgnd(HDC DC, const Types::TRect &ClipRect);
	void __fastcall UpdateView(void);
	void __fastcall TryStartHint(int XPos, int YPos);
	void __fastcall OnHintTimer(System::TObject* Sender);
	void __fastcall DoHideLineHint(void);
	void __fastcall DoShowLineHint(TElTreeItem* Item, Elheader::TElHeaderSection* Section);
	WideString __fastcall GetHintText(TElTreeItem* Item, Elheader::TElHeaderSection* &Section);
	int __fastcall CalcPageUpPos(int CurIdx);
	int __fastcall CalcPageDownPos(int CurIdx);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Message);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonDblClk(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMRButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	void __fastcall IntLButtonDown(int X, int Y, Classes::TShiftState Shift);
	bool __fastcall IntLButtonUp(int X, int Y, Classes::TShiftState Shift);
	void __fastcall IntRButtonDown(int X, int Y, Classes::TShiftState Shift);
	bool __fastcall IntRButtonUp(int X, int Y, Classes::TShiftState Shift);
	bool __fastcall IntLButtonDblClick(int X, int Y, Classes::TShiftState Shift);
	bool __fastcall IntRButtonDblClick(int X, int Y, Classes::TShiftState Shift);
	void __fastcall IntMouseMove(int X, int Y, Classes::TShiftState Shift);
	HIDESBASE MESSAGE void __fastcall CMMouseWheel(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Msg);
	void __fastcall SetHPosition(int value);
	void __fastcall SetVPosition(int value);
	void __fastcall DoSetTopIndex(int Value);
	void __fastcall OnHScroll(System::TObject* Sender, Elscrollbar::TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);
	void __fastcall OnVScroll(System::TObject* Sender, Elscrollbar::TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);
	void __fastcall FillVisFwd(int StartIndex);
	void __fastcall DefineHRange(void);
	virtual int __fastcall GetVisCount(void);
	int __fastcall GetVisiblesHeight(void);
	void __fastcall OnEditTimer(System::TObject* Sender);
	virtual void __fastcall DoEditItem(TElTreeItem* Item, int SectionNum);
	virtual void __fastcall DoEndEdit(bool ByCancel);
	virtual void __fastcall EditOperationCancelled(void);
	virtual void __fastcall EditOperationAccepted(void);
	void __fastcall FillDragImage(void);
	DYNAMIC void __fastcall DoStartDrag(Controls::TDragObject* &DragObject);
	virtual void __fastcall DoDragOver(Controls::TDragObject* Source, int X, int Y, bool CanDrop);
	DYNAMIC void __fastcall DoEndDrag(System::TObject* Target, int X, int Y);
	HIDESBASE MESSAGE void __fastcall CMDrag(Controls::TCMDrag &Message);
	virtual bool __fastcall DragScroll(Controls::TDragObject* Source, int X, int Y);
	void __fastcall OnScrollTimer(System::TObject* Sender);
	void __fastcall OnDragExpandTimer(System::TObject* Sender);
	virtual Controls::TDragImageList* __fastcall GetDragImages(void);
	void __fastcall OnDropTargetDrag(System::TObject* Sender, Controls::TDragState State, Eldragdrop::TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, Eldragdrop::TDragType &DragType);
	void __fastcall OnDropTargetDrop(System::TObject* Sender, Eldragdrop::TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, Eldragdrop::TDragType &DragType);
	virtual Types::TRect __fastcall GetItemRect(int ItemIndex);
	virtual TElTreeItem* __fastcall GetItemAtY(int Y);
	virtual TElTreeItem* __fastcall GetItemAt(int X, int Y, TSTItemPart &ItemPart, int &HitColumn);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	void __fastcall ProcessPassedClick(void);
	void __fastcall FitMostChildren(TElTreeItem* Item);
	DYNAMIC void __fastcall DoEnter(void);
	DYNAMIC void __fastcall DoExit(void);
	void __fastcall DoSetSelected(TElTreeItem* value);
	virtual int __fastcall GetVisCount2(void);
	virtual TElTreeItem* __fastcall FindNewFocused(Word Key, System::PInteger PVal1, bool &Sel);
	void __fastcall DrawMouseSelectFrame(void);
	void __fastcall AllocateMouseSelectFrame(void);
	void __fastcall DeallocateMouseSelectFrame(void);
	void __fastcall SelectMouseSelectItems(void);
	HIDESBASE MESSAGE void __fastcall WMCancelMode(Messages::TMessage &Message);
	void __fastcall CancelMouseSel(void);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	MESSAGE void __fastcall CMDeactivate(Messages::TMessage &Message);
	virtual void __fastcall InitiateEditOp(TElTreeItem* Item, int HCol, bool Immediate);
	
public:
	__fastcall virtual TElTreeView(Classes::TComponent* Owner);
	__fastcall virtual ~TElTreeView(void);
	virtual void __fastcall SetFocus(void);
	__property TCustomElTree* Owner = {read=FOwner};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElTreeView(HWND ParentWindow) : Controls::TCustomControl(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TCustomElTree : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
protected:
	TElTreeInplaceManager* FEditorManager;
	Graphics::TColor FStripedOddColor;
	Graphics::TColor FStripedEvenColor;
	bool FStripedItems;
	TVirtualStyleNeededEvent FOnVirtualStyleNeeded;
	Ellist::TElList* FSortSections;
	TVirtualTextNeededEvent FOnVirtualTextNeeded;
	TVirtualityLevel FVirtualityLevel;
	TVirtualHintNeededEvent FOnVirtualHintNeeded;
	TVirtualValueNeededEvent FOnVirtualValueNeeded;
	TLineHintType FLineHintType;
	int FLineHintTimeout;
	int FFireFocusEvents;
	bool FTransButtons;
	bool FTransCheckBoxes;
	Graphics::TColor FTrackColor;
	bool FExpandOnDragOver;
	Stdctrls::TScrollStyle FForcedScrollBars;
	bool FMoveFocusOnCollapse;
	int FHeaderHeight;
	Elscrollbar::TElScrollHintNeededEvent FOnVertScrollHintNeeded;
	Elscrollbar::TElScrollDrawPartEvent FOnHorzScrollDrawPart;
	Elscrollbar::TElScrollHintNeededEvent FOnHorzScrollHintNeeded;
	Elscrollbar::TElScrollDrawPartEvent FOnVertScrollDrawPart;
	Elscrollbar::TElScrollHitTestEvent FOnVertScrollHitTest;
	TElTreeChangingEvent FOnChanging;
	Controls::TBevelKind FBevelKindDummy;
	TElHintType FHintType;
	Classes::TNotifyEvent FOnClick;
	Classes::TNotifyEvent FOnDblClick;
	Controls::TDragDropEvent FOnDrop;
	Controls::TDragOverEvent FOnOver;
	Controls::TEndDragEvent FOnDrag;
	Classes::TNotifyEvent FOnEnter;
	Classes::TNotifyEvent FOnExit;
	Controls::TKeyEvent FOnKeyDown;
	Controls::TKeyPressEvent FOnKeyPress;
	Controls::TKeyEvent FOnKeyUp;
	Controls::TMouseEvent FOnMouseDown;
	Controls::TMouseMoveEvent FOnMouseMove;
	Controls::TMouseEvent FOnMouseUp;
	Controls::TStartDragEvent FOnStartDrag;
	TOnItemExpandEvent FOnItemPreDraw;
	TElTreeItemDragTargetEvent FOnDragTargetChange;
	Graphics::TColor FGradientStartColor;
	Graphics::TColor FGradientEndColor;
	int FGradientSteps;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FRowHotTrack;
	Graphics::TColor FFocusedSelectColor;
	Graphics::TColor FHideSelectColor;
	Graphics::TColor FFocusedSelectTextColor;
	Graphics::TColor FHideSelectTextColor;
	bool FNoBlendSelected;
	bool FScrollBackground;
	Graphics::TBitmap* FBackground;
	Elvclutils::TElBkGndType FBackgroundType;
	bool FAdjustMultilineHeight;
	bool FFlatFocusedScrollbars;
	bool FAutoResizeColumns;
	bool FHideFocusRect;
	bool FShowEmptyImages;
	bool FShowEmptyImages2;
	bool FShowRootButtons;
	bool FUnderlineTracked;
	bool FCustomCheckboxes;
	Graphics::TBitmap* FCheckBoxGlyph;
	Graphics::TBitmap* FRadioButtonGlyph;
	bool FFilteredVisibility;
	TApplyVisFilterEvent FOnApplyVisFilter;
	bool FRightAlignedText;
	bool FFlat;
	bool FRightAlignedTree;
	char FPathSeparator;
	Graphics::TPenStyle FLinesStyle;
	Graphics::TColor FLinesColor;
	bool FDeselectChildrenOnCollapse;
	bool FDrawFocusRect;
	bool FBarStyle;
	bool FAlwaysKeepFocus;
	bool FAlwaysKeepSelection;
	bool FFullRowSelect;
	TElDragType FDragType;
	bool FMouseOver;
	Eldragdrop::TElDropTarget* FDropTarget;
	Controls::TDragObject* FDragObject;
	bool FAutoLookup;
	int FSelectColumn;
	bool FAutoExpand;
	Graphics::TBitmap* FLeafPicture;
	Graphics::TBitmap* FPlusPicture;
	Graphics::TBitmap* FMinusPicture;
	bool FCustomPlusMinus;
	bool FShowHeader;
	bool FShowCheckboxes;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elini::TElIniFile* FStorage;
	AnsiString FStoragePath;
	TDragImgMode FDragImageMode;
	bool FHideHorzScrollBar;
	bool FHideVertScrollBar;
	bool FExpandOnDblClick;
	bool FHideHintOnMove;
	int FSortSection;
	TSortModes FSortMode;
	TSortTypes FSortType;
	bool FDragAllowed;
	Graphics::TColor FBkColor;
	Graphics::TColor FTextColor;
	bool FShowButtons;
	bool FShowLines;
	bool FShowImages;
	bool FShowRoot;
	Graphics::TColor FLineHintColor;
	THintModes FShowHintMode;
	Forms::TFormBorderStyle FBorderStyle;
	bool FCanEdit;
	bool FIgnoreSBChange;
	bool FScrollbarsInitialized;
	bool FSortRequired;
	bool FProcUpdate;
	bool FUpdated;
	int FInSorting;
	bool FBSVLines;
	bool FHLines;
	bool FVLines;
	Ellist::TElList* FAllList;
	Ellist::TElList* FSelectedList;
	bool FScrollTracking;
	bool FTracking;
	bool FHeaderHotTrack;
	bool FODFollowCol;
	WideString FODMask;
	Controls::TImageList* FImages;
	Controls::TImageList* FImages2;
	Imglist::TChangeLink* FImageChangeLink;
	int FTopIndex;
	int FBottomIndex;
	bool FChStateImage;
	WideString FRealHint;
	WideString FHint;
	int FMainTreeCol;
	bool FMultiSelect;
	int FMultiSelectLevel;
	bool FRowSelect;
	bool FHideSelect;
	int FLineHeight;
	bool FAutoLineHeight;
	int ItemExt;
	bool FUseCustomBars;
	bool FTreeIsFocused;
	int FHPos;
	bool FVScrollVisible;
	bool FHScrollVisible;
	TSTSelModes FSelMode;
	TSortDirs FSortDir;
	bool FSelChange;
	bool FColSizeUpdate;
	bool FUpdating;
	int FUpdateCount;
	bool FHintHide;
	bool FUseSystemHintColors;
	bool IgnoreResize;
	Graphics::TColor FCurBkColor;
	Graphics::TColor FCurTextColor;
	bool FDelOnEdit;
	bool FAutoSizingColumns;
	TElTreeItems* FItems;
	TColumnNotifyEvent FOnColumnResize;
	TColumnNotifyEvent FOnColumnClick;
	Elheader::TElSectionRedrawEvent FOnColumnDraw;
	TOnItemChangeEvent FOnItemChange;
	TOnItemDrawEvent FOnItemDraw;
	TOnItemCheckedEvent FOnItemChecked;
	TOnItemExpandEvent FOnItemExpand;
	TOnItemExpandEvent FOnItemCollapse;
	TOnItemExpanding FOnItemExpanding;
	TOnItemExpanding FOnItemCollapsing;
	TOnItemExpandEvent FOnItemDelete;
	Classes::TNotifyEvent FOnItemFocused;
	TElTreeItemPostDrawEvent FOnItemPostDraw;
	TOnShowHintEvent FOnShowHint;
	TOnCompareItems FOnCompareItems;
	TOnPicDrawEvent FOnItemPicDraw;
	TOnPicDrawEvent FOnItemPicDraw2;
	THotTrackEvent FOnHotTrack;
	TElScrollEvent FOnScroll;
	TItemSaveEvent FOnItemSave;
	TItemSaveEvent FOnItemLoad;
	TTryEditEvent FOnTryEdit;
	TElColumnMoveEvent FOnHeaderColumnMove;
	TCellStyleSaveEvent FOnSave;
	TCellStyleSaveEvent FOnLoad;
	TItemSelChangeEvent FOnItemSelectedChange;
	Elheader::TElHeaderLookupEvent FOnHeaderLookup;
	Elheader::TElHeaderLookupDoneEvent FOnHeaderLookupDone;
	Classes::TNotifyEvent FOnHeaderResize;
	THeaderSectionEvent FOnHeaderSectionExpand;
	THeaderSectionEvent FOnHeaderSectionCollapse;
	Elheader::TMeasureSectionEvent FOnHeaderSectionMeasure;
	TColumnNotifyEvent FOnSectionAutoSize;
	TColumnNotifyEvent FOnSectionFilterCall;
	TMeasureItemPartEvent FOnMeasureItemPart;
	Classes::TNotifyEvent FOnSortBegin;
	Classes::TNotifyEvent FOnSortEnd;
	Controls::TKeyEvent FOnEditKeyDown;
	TOleDragFinishEvent FOnOleDragFinish;
	TOleDragStartEvent FOnOleDragStart;
	Eldragdrop::TTargetDragEvent FOnOleTargetDrag;
	Eldragdrop::TTargetDropEvent FOnOleTargetDrop;
	int TotalHiddenCount;
	int TotalVisCount;
	int TotalVarHeightCount;
	TElTreeView* FView;
	Elheader::TElHeader* FHeader;
	Elscrollbar::TElScrollBar* FHScrollBar;
	Elscrollbar::TElScrollBar* FVScrollBar;
	Elscrollbar::TElScrollBarStyles* FHorzScrollBarStyle;
	Elscrollbar::TElScrollBarStyles* FVertScrollBarStyle;
	bool FFakeBool;
	int SavedHH;
	Extctrls::TTimer* FDelayTimer;
	TElTreeItem* FDelayedItem;
	int FDragExpandDelay;
	int FChangeDelay;
	TDragTargetDraw FDragTrgDrawMode;
	Controls::TMouseEvent FOnHeaderMouseDown;
	Classes::TNotifyEvent FOnAfterSelectionChange;
	Graphics::TColor FDragRectAcceptColor;
	Graphics::TColor FDragRectDenyColor;
	bool FIncrementalSearch;
	bool FRightClickSelect;
	bool FScrollbarOpposite;
	bool FVerticalLinesLong;
	Elvclutils::TElBorderSides FBorderSides;
	TInplaceEditorNeededEvent FOnInplaceEditorNeeded;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	bool FQuickEditMode;
	Elheader::TElFieldType FMainTextType;
	Elscrollbar::TElScrollHitTestEvent FOnHorzScrollHitTest;
	bool FMouseFrameSelect;
	Graphics::TColor FVertDivLinesColor;
	Graphics::TColor FHorzDivLinesColor;
	int FDragScrollInterval;
	bool FShowLeafButton;
	bool FExplorerEditMode;
	int FCheckBoxSize;
	bool FIgnoreEnabled;
	int FInplaceEditorDelay;
	Graphics::TFont* FHeaderFont;
	bool FHeaderUseTreeFont;
	bool FKeepSelectionWithinLevel;
	bool FAutoCollapse;
	bool FIgnoreResizes;
	bool FSortUseCase;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	TElDblClickMode FDblClickMode;
	bool FDoubleBuffered;
	bool InSizeMove;
	Elhook::TElHook* FHook;
	void __fastcall SetStripedOddColor(Graphics::TColor Value);
	void __fastcall SetStripedEvenColor(Graphics::TColor Value);
	void __fastcall SetStripedItems(bool Value);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	void __fastcall OnBeforeHook(System::TObject* Sender, Messages::TMessage &Message, bool &Handled);
	virtual void __fastcall SetParent(Controls::TWinControl* AParent);
	void __fastcall SetVirtualityLevel(TVirtualityLevel Value);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	int __fastcall GetDefaultSectionWidth(void);
	void __fastcall SetDefaultSectionWidth(int Value);
	void __fastcall OnHeaderSectionResize(Elheader::TCustomElHeader* Header, Elheader::TElHeaderSection* Section);
	void __fastcall OnHeaderSectionClick(Elheader::TCustomElHeader* Header, Elheader::TElHeaderSection* Section);
	void __fastcall OnHeaderSectionDelete(Elheader::TCustomElHeader* Header, Elheader::TElHeaderSection* Section);
	void __fastcall DoHeaderMouseDown(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall OnHeaderSectionLookup(System::TObject* Sender, Elheader::TElHeaderSection* Section, AnsiString &Text);
	void __fastcall OnHeaderSectionLookupDone(System::TObject* Sender, Elheader::TElHeaderSection* Section, AnsiString Text, bool Accepted);
	void __fastcall OnHeaderExpand(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall OnHeaderCollapse(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall OnHeaderSectionVisChange(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall HeaderSectionAutoSizeHandler(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall SectionAutoSizeTransfer(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall SectionFilterCallTransfer(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall DoHeaderResize(System::TObject* Sender);
	virtual void __fastcall OnFontChange(System::TObject* Sender);
	void __fastcall OnSignChange(System::TObject* Sender);
	void __fastcall ImageListChange(System::TObject* Sender);
	TElTreeItem* __fastcall GetDropTarget(void);
	void __fastcall SetTextColor(Graphics::TColor value);
	void __fastcall SetBkColor(Graphics::TColor value);
	bool __fastcall GetHeaderWrapCaptions(void);
	void __fastcall SetHeaderWrapCaptions(bool Value);
	void __fastcall SetHeaderHotTrack(bool value);
	void __fastcall SetHeaderHeight(int value);
	void __fastcall SetShowEmptyImages(bool newValue);
	void __fastcall SetShowEmptyImages2(bool newValue);
	void __fastcall SetImages(Controls::TImageList* Value);
	void __fastcall SetImages2(Controls::TImageList* newValue);
	void __fastcall SetLineHintTimeout(int Value);
	void __fastcall SetLineStyle(bool Value);
	void __fastcall SetRootStyle(bool Value);
	void __fastcall SetImagesStyle(bool Value);
	void __fastcall SetBorderStyle(Forms::TBorderStyle Value);
	void __fastcall SetButtonStyle(bool Value);
	void __fastcall SetUpdating(bool value);
	bool __fastcall GetUpdating(void);
	void __fastcall SetHLines(bool value);
	void __fastcall SetVLines(bool value);
	void __fastcall SetBSVLines(bool value);
	void __fastcall SetRowSelect(bool value);
	void __fastcall SetMultiSelectLevel(int Value);
	void __fastcall SetMultiSelect(bool value);
	void __fastcall SetFocused(TElTreeItem* value);
	void __fastcall SetHideSelect(bool value);
	void __fastcall SetAutoExpand(bool value);
	void __fastcall SetMoveFocusOnCollapse(bool value);
	Elheader::TElHeaderSections* __fastcall GetHeaderSections(void);
	void __fastcall SetHeaderSections(Elheader::TElHeaderSections* value);
	void __fastcall SetChStateImage(bool value);
	void __fastcall SetUseStdBars(bool value);
	void __fastcall SetItemIndent(int value);
	void __fastcall SetLineHeight(int value);
	void __fastcall SetAutoLineHeight(bool value);
	int __fastcall GetHeaderHeight(void);
	void __fastcall SetMainTreeCol(int value);
	void __fastcall SetItems(TElTreeItems* value);
	int __fastcall GetTotalVisCount(void);
	bool __fastcall GetDraggableSections(void);
	void __fastcall SetDraggableSections(bool newValue);
	void __fastcall SetSortMode(TSortModes newValue);
	void __fastcall SetSortSection(int newValue);
	bool __fastcall GetMoveColumnOnDrag(void);
	void __fastcall SetMoveColumnOnDrag(bool newValue);
	void __fastcall SetHideHorzScrollBar(bool newValue);
	void __fastcall SetHideVertScrollBar(bool newValue);
	Controls::TImageList* __fastcall GetHeaderImages(void);
	void __fastcall SetHeaderImages(Controls::TImageList* newValue);
	bool __fastcall GetFireFocusEvents(void);
	void __fastcall SetFireFocusEvents(bool Value);
	void __fastcall SetScrollbarOpposite(bool Value);
	void __fastcall SetVerticalLinesLong(bool Value);
	int __fastcall GetSelCount(void);
	TElTreeItem* __fastcall GetSelected(void);
	TElTreeItem* __fastcall GetFocused(void);
	void __fastcall SetSelected(TElTreeItem* newValue);
	void __fastcall SetStorage(Elini::TElIniFile* newValue);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetHeaderImageForm(Elimgfrm::TElImageForm* newValue);
	Elimgfrm::TElImageForm* __fastcall GetHeaderImageForm(void);
	void __fastcall SetShowCheckboxes(bool newValue);
	void __fastcall SetPlusPicture(Graphics::TBitmap* newValue);
	void __fastcall SetMinusPicture(Graphics::TBitmap* newValue);
	void __fastcall SetCustomPlusMinus(bool newValue);
	void __fastcall SetSelectColumn(int newValue);
	void __fastcall SetDragType(TElDragType newValue);
	void __fastcall HeaderResizeTransfer(System::TObject* Sender);
	void __fastcall HeaderResizeHandler(System::TObject* Sender);
	bool __fastcall GetStickyHeaderSections(void);
	void __fastcall SetStickyHeaderSections(bool newValue);
	void __fastcall SetBarStyle(bool newValue);
	void __fastcall SetDrawFocusRect(bool newValue);
	void __fastcall SetLinesColor(Graphics::TColor newValue);
	void __fastcall SetHorzDivLinesColor(Graphics::TColor newValue);
	void __fastcall SetLinesStyle(Graphics::TPenStyle newValue);
	void __fastcall SetRightAlignedTree(bool newValue);
	void __fastcall SetFlat(bool newValue);
	void __fastcall SetRightAlignedText(bool newValue);
	void __fastcall SetFilteredVisibility(bool newValue);
	void __fastcall SetUnderlineTracked(bool newValue);
	void __fastcall SetCustomCheckboxes(bool newValue);
	void __fastcall SetCheckBoxGlyph(Graphics::TBitmap* newValue);
	void __fastcall SetRadioButtonGlyph(Graphics::TBitmap* newValue);
	void __fastcall SetShowRootButtons(bool newValue);
	void __fastcall SetHideFocusRect(bool newValue);
	bool __fastcall GetLockHeaderHeight(void);
	void __fastcall SetLockHeaderHeight(bool newValue);
	void __fastcall SetTransButtons(bool newValue);
	void __fastcall UpdateFrame(void);
	void __fastcall SetHeaderActiveFilterColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetHeaderActiveFilterColor(void);
	void __fastcall SetHeaderFilterColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetHeaderFilterColor(void);
	void __fastcall SetHeaderFlat(bool newValue);
	bool __fastcall GetHeaderFlat(void);
	void __fastcall DrawFlatBorder(bool HorzTracking, bool VertTracking);
	void __fastcall DrawFlatBorderEx(HDC DC, bool HorzTracking, bool VertTracking);
	void __fastcall ReRenderAllHTMLItems(void);
	void __fastcall SetFlatFocusedScrollbars(bool newValue);
	void __fastcall SetBackground(Graphics::TBitmap* newValue);
	void __fastcall SetBackgroundType(Elvclutils::TElBkGndType newValue);
	void __fastcall BackgroundChange(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall WMNCHITTEST(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMVScroll(Messages::TWMScroll &Msg);
	HIDESBASE MESSAGE void __fastcall WMHScroll(Messages::TWMScroll &Msg);
	MESSAGE void __fastcall WMEnable(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Message);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Message);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMSysColorChange(Messages::TMessage &Msg);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	MESSAGE void __fastcall IFMCanPaintBkgnd(Messages::TMessage &Message);
	void __fastcall SetHideSelectColor(Graphics::TColor newValue);
	void __fastcall SetFocusedSelectColor(Graphics::TColor newValue);
	void __fastcall SetHideSelectTextColor(Graphics::TColor newValue);
	void __fastcall SetFocusedSelectTextColor(Graphics::TColor newValue);
	void __fastcall SetRowHotTrack(bool newValue);
	void __fastcall SetActiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetInactiveBorderType(Elvclutils::TElFlatBorderType newValue);
	void __fastcall SetGradientStartColor(Graphics::TColor newValue);
	void __fastcall SetGradientEndColor(Graphics::TColor newValue);
	void __fastcall SetGradientSteps(int newValue);
	void __fastcall SetHPosition(int value);
	void __fastcall SetVPosition(int value);
	virtual void __fastcall ClickTransfer(System::TObject* Sender);
	virtual void __fastcall DblClickTransfer(System::TObject* Sender);
	virtual void __fastcall DropTransfer(System::TObject* Sender, System::TObject* Source, int X, int Y);
	virtual void __fastcall OverTransfer(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	virtual void __fastcall DragTransfer(System::TObject* Sender, System::TObject* Target, int X, int Y);
	virtual void __fastcall EnterTransfer(System::TObject* Sender);
	virtual void __fastcall ExitTransfer(System::TObject* Sender);
	virtual void __fastcall KeyDownTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall KeyPressTransfer(System::TObject* Sender, char &Key);
	virtual void __fastcall KeyUpTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall MouseDownTransfer(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall MouseMoveTransfer(System::TObject* Sender, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall MouseUpTransfer(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall StartDragTransfer(System::TObject* Sender, Controls::TDragObject* &DragObject);
	virtual void __fastcall MeasureSectionTransfer(System::TObject* Sender, Elheader::TElHeaderSection* Section, Types::TPoint &Size);
	HIDESBASE void __fastcall SetCursor(Controls::TCursor newValue);
	Controls::TCursor __fastcall GetCursor(void);
	int __fastcall SetScrollInfo(HWND hWnd, int BarFlag, const tagSCROLLINFO &ScrollInfo, BOOL Redraw);
	BOOL __fastcall GetScrollInfo(HWND hWnd, int BarFlag, tagSCROLLINFO &ScrollInfo);
	void __fastcall SetHorzScrollBarStyle(Elscrollbar::TElScrollBarStyles* newValue);
	void __fastcall SetVertScrollBarStyle(Elscrollbar::TElScrollBarStyles* newValue);
	void __fastcall HorzScrollDrawPartTransfer(System::TObject* Sender, Graphics::TCanvas* Canvas, const Types::TRect &R, Elscrollbar::TElScrollBarPart Part, bool Enabled, bool Focused, bool Pressed, bool &DefaultDraw);
	void __fastcall HorzScrollHintNeededTransfer(System::TObject* Sender, int TrackPosition, WideString &Hint);
	void __fastcall VertScrollDrawPartTransfer(System::TObject* Sender, Graphics::TCanvas* Canvas, const Types::TRect &R, Elscrollbar::TElScrollBarPart Part, bool Enabled, bool Focused, bool Pressed, bool &DefaultDraw);
	void __fastcall VertScrollHintNeededHandler(System::TObject* Sender, int TrackPosition, WideString &Hint);
	void __fastcall VertScrollHintNeededTransfer(System::TObject* Sender, int TrackPosition, WideString &Hint);
	bool __fastcall GetHeaderInvertSortArrows(void);
	void __fastcall SetHeaderInvertSortArrows(bool newValue);
	void __fastcall SBChanged(System::TObject* Sender);
	void __fastcall ScrollBarMouseDown(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall SetForcedScrollBars(Stdctrls::TScrollStyle newValue);
	Controls::TCursor __fastcall GetDragCursor(void);
	void __fastcall SetDragCursor(Controls::TCursor Value);
	void __fastcall SetTrackColor(Graphics::TColor value);
	void __fastcall SetNoBlendSelected(bool newValue);
	Elheader::TElHeaderSection* __fastcall GetLockedHeaderSection(void);
	void __fastcall SetLockedHeaderSection(Elheader::TElHeaderSection* newValue);
	virtual void __fastcall SetAdjustMultilineHeight(bool newValue);
	DYNAMIC void __fastcall ActionChange(System::TObject* Sender, bool CheckDefaults);
	virtual void __fastcall AlignControls(Controls::TControl* AControl, Types::TRect &Rect);
	void __fastcall AlignPieces(void);
	virtual TElTreeItem* __fastcall GetRoot(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual int __fastcall CompareItems(TElTreeItem* Item1, TElTreeItem* Item2, Elheader::TElSSortMode SM, TSortTypes ST, int FSortSection);
	virtual void __fastcall SetCanEdit(bool value);
	virtual void __fastcall SetShowHeader(bool value);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMColorChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMCtl3DChanged(Messages::TMessage &Message);
	DYNAMIC void __fastcall Resize(void);
	virtual int __fastcall DoGetPicture(TElTreeItem* Item);
	virtual int __fastcall DoGetPicture2(TElTreeItem* Item);
	virtual int __fastcall DefineLineHeight(void);
	virtual void __fastcall UpdateScrollBars(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual TElTreeItems* __fastcall CreateItems(void);
	virtual TElTreeItems* __fastcall CreateItemsExt(TMetaClass* ItemClass);
	virtual Elheader::TElHeader* __fastcall CreateHeader(void);
	bool __fastcall DoSetFocused(TElTreeItem* value, bool Forced);
	bool __fastcall DoSetFocusedEx(TElTreeItem* value, bool Forced, bool Delayed);
	virtual void __fastcall SetHeaderColor(Graphics::TColor newValue);
	virtual Graphics::TColor __fastcall GetHeaderColor(void);
	WideString __fastcall GetHint();
	void __fastcall SetHint(WideString newValue);
	virtual void __fastcall DoChanging(TElTreeItem* Item, bool &AllowChange);
	virtual void __fastcall DoOnColumnResize(int SectionIndex);
	virtual void __fastcall DoColumnClick(int SectionIndex);
	virtual void __fastcall DoItemFocused(void);
	virtual void __fastcall DoItemDraw(TElTreeItem* Item, Graphics::TCanvas* Surface, const Types::TRect &R, int SectionIndex);
	virtual void __fastcall DoItemChange(TElTreeItem* Item, TItemChangeMode ItemChangeMode);
	virtual void __fastcall DoItemExpanding(TElTreeItem* Item, bool &CanProcess);
	virtual void __fastcall DoItemCollapsing(TElTreeItem* Item, bool &CanProcess);
	virtual void __fastcall DoItemChecked(TElTreeItem* Item);
	virtual void __fastcall DoItemExpand(TElTreeItem* Item);
	virtual void __fastcall DoItemCollapse(TElTreeItem* Item);
	virtual void __fastcall DoItemDelete(TElTreeItem* Item);
	virtual void __fastcall DoCompareItems(TElTreeItem* Item1, TElTreeItem* Item2, int &res);
	virtual void __fastcall DoHeaderDraw(Elheader::TCustomElHeader* Header, Graphics::TCanvas* Canvas, Elheader::TElHeaderSection* Section, const Types::TRect &Rect, bool Pressed);
	virtual void __fastcall OnHeaderSectionChange(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section, Elheader::TSectionChangeMode Change);
	virtual void __fastcall OnHeaderSectionMove(Elheader::TCustomElHeader* Sender, Elheader::TElHeaderSection* Section, int OldPos, int NewPos);
	virtual void __fastcall TriggerHotTrackEvent(TElTreeItem* OldItem, TElTreeItem* NewItem);
	virtual void __fastcall TriggerScrollEvent(Forms::TScrollBarKind ScrollBarKind, int ScrollCode);
	virtual void __fastcall TriggerHeaderColumnMoveEvent(Elheader::TElHeaderSection* Section, int OldPos, int NewPos);
	virtual void __fastcall TriggerItemSaveEvent(Classes::TStream* Stream, TElTreeItem* Item);
	virtual void __fastcall TriggerItemLoadEvent(Classes::TStream* Stream, TElTreeItem* Item);
	virtual void __fastcall TriggerItemSelectedChangeEvent(TElTreeItem* Item);
	virtual void __fastcall DoShowHint(TElTreeItem* Item, Elheader::TElHeaderSection* Section, WideString &Text, Controls::THintWindow* HintWindow, const Types::TPoint &MousePos, bool &DoShowHint);
	virtual void __fastcall Paint(void);
	virtual void __fastcall OnHeaderSectionCreate(Elheader::TCustomElHeader* Header, Elheader::TElHeaderSection* Section);
	virtual void __fastcall TriggerHeaderLookupEvent(Elheader::TElHeaderSection* Section, AnsiString &Text);
	virtual void __fastcall TriggerHeaderLookupDoneEvent(Elheader::TElHeaderSection* Section, AnsiString Text, bool Accepted);
	virtual void __fastcall TriggerHeaderSectionExpandEvent(Elheader::TElHeaderSection* Section);
	virtual void __fastcall TriggerHeaderSectionCollapseEvent(Elheader::TElHeaderSection* Section);
	virtual void __fastcall TriggerMeasureItemPartEvent(TElTreeItem* Item, int PartIndex, Types::TPoint &Size);
	virtual void __fastcall TriggerApplyVisFilterEvent(TElTreeItem* Item, bool &Hidden);
	virtual void __fastcall TriggerItemPostDrawEvent(Graphics::TCanvas* Canvas, TElTreeItem* Item, const Types::TRect &ItemRect, bool &DrawFocusRect);
	virtual void __fastcall TriggerOleTargetDragEvent(Controls::TDragState State, Eldragdrop::TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, Eldragdrop::TDragType &DragType);
	virtual void __fastcall TriggerOleTargetDropEvent(Eldragdrop::TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, Eldragdrop::TDragType &DragType);
	virtual void __fastcall TriggerOleDragStartEvent(_di_IDataObject &dataObj, _di_IDropSource &dropSource, Eldragdrop::TDragTypes &dwOKEffects);
	virtual void __fastcall TriggerOleDragFinishEvent(Eldragdrop::TDragType dwEffect, HRESULT Result);
	virtual Controls::TDragImageList* __fastcall GetDragImages(void);
	void __fastcall AutoSizeAllColumns(void);
	void __fastcall AutoSizeColumn(int SectionIndex);
	virtual TElTreeItem* __fastcall GetTopItem(void);
	virtual void __fastcall SetTopItem(TElTreeItem* Item);
	virtual void __fastcall Loaded(void);
	TSortTypes __fastcall SectionTypeToSortType(Elheader::TElFieldType SectionType);
	virtual void __fastcall TriggerSortBegin(void);
	virtual void __fastcall TriggerSortEnd(void);
	virtual TElTreeView* __fastcall CreateView(void);
	virtual void __fastcall CreateWnd(void);
	void __fastcall StartDelayedFocus(TElTreeItem* FocusItemToReport);
	void __fastcall StopDelayedFocus(void);
	void __fastcall OnDelayTimer(System::TObject* Sender);
	virtual void __fastcall DoAfterSelectionChange(void);
	void __fastcall SetDragRectAcceptColor(const Graphics::TColor Value);
	void __fastcall SetDragRectDenyColor(Graphics::TColor Value);
	void __fastcall SetDragTrgDrawMode(TDragTargetDraw Value);
	int __fastcall GetVisibleRowCount(void);
	void __fastcall DoSetDragTrgDrawMode(TDragTargetDraw Value, bool RedrawItem);
	DYNAMIC void __fastcall DoEndDrag(System::TObject* Target, int X, int Y);
	void __fastcall UpdateDiffItems(void);
	void __fastcall SlowCompareItems(TElTreeItem* Item1, TElTreeItem* Item2, Elheader::TElHeaderSection* Section, int &Result);
	virtual void __fastcall TriggerVirtualTextNeeded(TElTreeItem* Item, int SectionIndex, WideString &Text);
	virtual void __fastcall TriggerVirtualHintNeeded(TElTreeItem* Item, WideString &Hint);
	virtual void __fastcall TriggerVirtualValueNeeded(TElTreeItem* Item, int SectionIndex, int VarType, Variant &Value);
	virtual void __fastcall TriggerVirtualStyleNeeded(TElTreeItem* Item, int SectionIndex, TElCellStyle* Style);
	virtual void __fastcall TriggerTryEditEvent(TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType &CellType, bool &CanEdit);
	virtual void __fastcall TriggerInplaceEditorNeeded(TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType SupposedFieldType, TElTreeInplaceEditor* &Editor);
	virtual void __fastcall VertScrollHitTestTransfer(System::TObject* Sender, int X, int Y, Elscrollbar::TElScrollBarPart &Part, bool &DefaultTest);
	virtual void __fastcall HorzScrollHitTestTransfer(System::TObject* Sender, int X, int Y, Elscrollbar::TElScrollBarPart &Part, bool &DefaultTest);
	void __fastcall SetVertDivLinesColor(Graphics::TColor Value);
	void __fastcall SetCheckBoxSize(int Value);
	TElTreeItem* __fastcall GetTrackItem(void);
	bool __fastcall GetDragging(void);
	void __fastcall SetShowLeafButton(bool Value);
	void __fastcall SetLeafPicture(Graphics::TBitmap* Value);
	void __fastcall MouseWheelTransfer(System::TObject* Sender, Classes::TShiftState Shift, int WheelDelta, const Types::TPoint &MousePos, bool &Handled);
	void __fastcall MouseWheelDownTransfer(System::TObject* Sender, Classes::TShiftState Shift, const Types::TPoint &MousePos, bool &Handled);
	void __fastcall MouseWheelUpTransfer(System::TObject* Sender, Classes::TShiftState Shift, const Types::TPoint &MousePos, bool &Handled);
	void __fastcall FitMostChildren(TElTreeItem* Item);
	virtual WideString __fastcall GetThemedClassName();
	virtual void __fastcall SetUseXPThemes(const bool Value);
	int __fastcall GetCheckBoxSize(void);
	Menus::TPopupMenu* __fastcall GetHeaderPopupMenu(void);
	void __fastcall SetHeaderPopupMenu(Menus::TPopupMenu* Value);
	void __fastcall SetHeaderFont(Graphics::TFont* Value);
	void __fastcall SetHeaderUseTreeFont(bool Value);
	void __fastcall HeaderFontChanged(System::TObject* Sender);
	bool __fastcall IsStripedColorStored(void);
	HIDESBASE MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	DYNAMIC void __fastcall DoEnter(void);
	DYNAMIC void __fastcall DoExit(void);
	void __fastcall SetSortUseCase(bool Value);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetDblClickMode(TElDblClickMode Value);
	void __fastcall SetExpandOnDblClick(bool Value);
	Graphics::TBitmap* __fastcall GetPlusPicture(void);
	Graphics::TBitmap* __fastcall GetLeafPicture(void);
	Graphics::TBitmap* __fastcall GetMinusPicture(void);
	Graphics::TBitmap* __fastcall GetCheckBoxGlyph(void);
	Graphics::TBitmap* __fastcall GetRadioButtonGlyph(void);
	void __fastcall OnCheckSignChange(System::TObject* Sender);
	MESSAGE void __fastcall WMUpdateSBFrame(Messages::TMessage &Message);
	void __fastcall SetDoubleBuffered(bool Value);
	__property Graphics::TColor TextColor = {read=FTextColor, write=SetTextColor, default=-2147483640};
	__property Graphics::TColor BkColor = {read=FBkColor, write=SetBkColor, default=-2147483643};
	__property bool ShowButtons = {read=FShowButtons, write=SetButtonStyle, default=1};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, default=1};
	__property bool ShowLines = {read=FShowLines, write=SetLineStyle, default=1};
	__property bool ShowImages = {read=FShowImages, write=SetImagesStyle, default=1};
	__property bool ShowRoot = {read=FShowRoot, write=SetRootStyle, default=0};
	__property THintModes LineHintMode = {read=FShowHintMode, write=FShowHintMode, default=1};
	__property Graphics::TColor LineHintColor = {read=FLineHintColor, write=FLineHintColor, default=-2147483643};
	__property bool HideSelection = {read=FHideSelect, write=SetHideSelect, default=0};
	__property bool HideHintOnTimer = {read=FHintHide, write=FHintHide, default=0};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property Controls::TImageList* Images2 = {read=FImages2, write=SetImages2};
	__property bool ChangeStateImage = {read=FChStateImage, write=SetChStateImage, default=0};
	__property bool ShowColumns = {read=FShowHeader, write=SetShowHeader, default=0};
	__property TDragTargetDraw DragTrgDrawMode = {read=FDragTrgDrawMode, write=SetDragTrgDrawMode, default=2};
	__property bool DraggableSections = {read=GetDraggableSections, write=SetDraggableSections, default=0};
	__property TSTSelModes SelectionMode = {read=FSelMode, write=FSelMode, default=1};
	__property bool DoInplaceEdit = {read=FCanEdit, write=SetCanEdit, default=1};
	__property bool VerticalLines = {read=FVLines, write=SetVLines, default=0};
	__property bool BarStyleVerticalLines = {read=FBSVLines, write=SetBSVLines, default=0};
	__property bool HorizontalLines = {read=FHLines, write=SetHLines, default=0};
	__property bool ScrollTracking = {read=FScrollTracking, write=FScrollTracking, default=0};
	__property bool HotTrack = {read=FTracking, write=FTracking, default=1};
	__property bool Tracking = {read=FTracking, write=FTracking, default=1};
	__property bool RowSelect = {read=FRowSelect, write=SetRowSelect, default=1};
	__property bool MultiSelect = {read=FMultiSelect, write=SetMultiSelect, default=1};
	__property int MultiSelectLevel = {read=FMultiSelectLevel, write=SetMultiSelectLevel, default=-1};
	__property int LineHeight = {read=FLineHeight, write=SetLineHeight, nodefault};
	__property bool AutoLineHeight = {read=FAutoLineHeight, write=SetAutoLineHeight, default=1};
	__property bool HeaderHotTrack = {read=FHeaderHotTrack, write=SetHeaderHotTrack, default=1};
	__property Elheader::TElHeaderSections* HeaderSections = {read=GetHeaderSections, write=SetHeaderSections};
	__property int HeaderHeight = {read=GetHeaderHeight, write=SetHeaderHeight, nodefault};
	__property int MainTreeColumn = {read=FMainTreeCol, write=SetMainTreeCol, default=0};
	__property bool OwnerDrawByColumn = {read=FODFollowCol, write=FODFollowCol, default=1};
	__property WideString OwnerDrawMask = {read=FODMask, write=FODMask};
	__property bool DragAllowed = {read=FDragAllowed, write=FDragAllowed, default=0};
	__property TSortDirs SortDir = {read=FSortDir, write=FSortDir, default=0};
	__property TSortModes SortMode = {read=FSortMode, write=SetSortMode, default=0};
	__property int SortSection = {read=FSortSection, write=SetSortSection, default=0};
	__property TSortTypes SortType = {read=FSortType, write=FSortType, default=1};
	__property bool HideHintOnMove = {read=FHideHintOnMove, write=FHideHintOnMove, default=1};
	__property bool ExpandOnDblClick = {read=FExpandOnDblClick, write=SetExpandOnDblClick, default=1};
	__property bool MoveColumnOnDrag = {read=GetMoveColumnOnDrag, write=SetMoveColumnOnDrag, default=0};
	__property bool HideHorzScrollBar = {read=FHideHorzScrollBar, write=SetHideHorzScrollBar, default=0};
	__property bool HideVertScrollBar = {read=FHideVertScrollBar, write=SetHideVertScrollBar, default=0};
	__property Elscrollbar::TElScrollBarStyles* HorzScrollBarStyles = {read=FHorzScrollBarStyle, write=SetHorzScrollBarStyle, stored=true};
	__property Elscrollbar::TElScrollBarStyles* VertScrollBarStyles = {read=FVertScrollBarStyle, write=SetVertScrollBarStyle, stored=true};
	__property bool NoBlendSelected = {read=FNoBlendSelected, write=SetNoBlendSelected, default=0};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property Elvclutils::TElBkGndType BackgroundType = {read=FBackgroundType, write=SetBackgroundType, default=2};
	__property bool ScrollBackground = {read=FScrollBackground, write=FScrollBackground, default=0};
	__property Controls::TImageList* HeaderImages = {read=GetHeaderImages, write=SetHeaderImages};
	__property TDragImgMode DragImageMode = {read=FDragImageMode, write=FDragImageMode, default=0};
	__property AnsiString StoragePath = {read=FStoragePath, write=FStoragePath};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=SetStorage};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Elimgfrm::TElImageForm* HeaderImageForm = {read=GetHeaderImageForm, write=SetHeaderImageForm};
	__property bool ShowCheckboxes = {read=FShowCheckboxes, write=SetShowCheckboxes, default=0};
	__property Graphics::TBitmap* PlusPicture = {read=GetPlusPicture, write=SetPlusPicture};
	__property Graphics::TBitmap* MinusPicture = {read=GetMinusPicture, write=SetMinusPicture};
	__property bool CustomPlusMinus = {read=FCustomPlusMinus, write=SetCustomPlusMinus, default=0};
	__property int SelectColumn = {read=FSelectColumn, write=SetSelectColumn, default=-1};
	__property bool AutoExpand = {read=FAutoExpand, write=SetAutoExpand, default=0};
	__property bool AutoLookup = {read=FAutoLookup, write=FAutoLookup, default=0};
	__property TElDragType DragType = {read=FDragType, write=SetDragType, default=1};
	__property bool FullRowSelect = {read=FFullRowSelect, write=FFullRowSelect, default=1};
	__property bool AlwaysKeepSelection = {read=FAlwaysKeepSelection, write=FAlwaysKeepSelection, default=1};
	__property bool AlwaysKeepFocus = {read=FAlwaysKeepFocus, write=FAlwaysKeepFocus, default=0};
	__property bool StickyHeaderSections = {read=GetStickyHeaderSections, write=SetStickyHeaderSections, default=0};
	__property bool BarStyle = {read=FBarStyle, write=SetBarStyle, default=0};
	__property bool DrawFocusRect = {read=FDrawFocusRect, write=SetDrawFocusRect, default=1};
	__property bool DeselectChildrenOnCollapse = {read=FDeselectChildrenOnCollapse, write=FDeselectChildrenOnCollapse, default=0};
	__property Graphics::TColor HorzDivLinesColor = {read=FHorzDivLinesColor, write=SetHorzDivLinesColor, default=-2147483633};
	__property Graphics::TColor LinesColor = {read=FLinesColor, write=SetLinesColor, default=-2147483633};
	__property Graphics::TPenStyle LinesStyle = {read=FLinesStyle, write=SetLinesStyle, default=2};
	__property char PathSeparator = {read=FPathSeparator, write=FPathSeparator, default=92};
	__property bool RightAlignedTree = {read=FRightAlignedTree, write=SetRightAlignedTree, default=0};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property bool RightAlignedText = {read=FRightAlignedText, write=SetRightAlignedText, default=0};
	__property bool FilteredVisibility = {read=FFilteredVisibility, write=SetFilteredVisibility, default=0};
	__property bool UnderlineTracked = {read=FUnderlineTracked, write=SetUnderlineTracked, default=1};
	__property bool CustomCheckboxes = {read=FCustomCheckboxes, write=SetCustomCheckboxes, default=0};
	__property Graphics::TBitmap* CheckBoxGlyph = {read=GetCheckBoxGlyph, write=SetCheckBoxGlyph};
	__property Graphics::TBitmap* RadioButtonGlyph = {read=GetRadioButtonGlyph, write=SetRadioButtonGlyph};
	__property bool ScrollbarOpposite = {read=FScrollbarOpposite, write=SetScrollbarOpposite, nodefault};
	__property bool ShowRootButtons = {read=FShowRootButtons, write=SetShowRootButtons, default=1};
	__property bool ShowEmptyImages = {read=FShowEmptyImages, write=SetShowEmptyImages, default=0};
	__property bool ShowEmptyImages2 = {read=FShowEmptyImages2, write=SetShowEmptyImages2, default=0};
	__property bool HideFocusRect = {read=FHideFocusRect, write=SetHideFocusRect, default=0};
	__property bool LockHeaderHeight = {read=GetLockHeaderHeight, write=SetLockHeaderHeight, default=0};
	__property bool AutoResizeColumns = {read=FAutoResizeColumns, write=FAutoResizeColumns, default=1};
	__property Graphics::TColor HeaderActiveFilterColor = {read=GetHeaderActiveFilterColor, write=SetHeaderActiveFilterColor, default=0};
	__property Graphics::TColor HeaderFilterColor = {read=GetHeaderFilterColor, write=SetHeaderFilterColor, default=-2147483630};
	__property bool HeaderFlat = {read=GetHeaderFlat, write=SetHeaderFlat, default=0};
	__property bool HeaderWrapCaptions = {read=GetHeaderWrapCaptions, write=SetHeaderWrapCaptions, default=0};
	__property bool FlatFocusedScrollbars = {read=FFlatFocusedScrollbars, write=SetFlatFocusedScrollbars, default=1};
	__property Graphics::TColor HideSelectColor = {read=FHideSelectColor, write=SetHideSelectColor, default=-2147483633};
	__property Graphics::TColor FocusedSelectColor = {read=FFocusedSelectColor, write=SetFocusedSelectColor, default=-2147483635};
	__property Graphics::TColor HideSelectTextColor = {read=FHideSelectTextColor, write=SetHideSelectTextColor, default=-2147483632};
	__property Graphics::TColor FocusedSelectTextColor = {read=FFocusedSelectTextColor, write=SetFocusedSelectTextColor, default=-2147483634};
	__property bool UseCustomScrollBars = {read=FUseCustomBars, write=SetUseStdBars, default=1};
	__property bool RowHotTrack = {read=FRowHotTrack, write=SetRowHotTrack, default=0};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
	__property int ItemIndent = {read=ItemExt, write=SetItemIndent, default=17};
	__property Graphics::TColor GradientStartColor = {read=FGradientStartColor, write=SetGradientStartColor, default=0};
	__property Graphics::TColor GradientEndColor = {read=FGradientEndColor, write=SetGradientEndColor, default=0};
	__property int GradientSteps = {read=FGradientSteps, write=SetGradientSteps, default=16};
	__property Controls::TCursor Cursor = {read=GetCursor, write=SetCursor, default=-2};
	__property bool HeaderInvertSortArrows = {read=GetHeaderInvertSortArrows, write=SetHeaderInvertSortArrows, default=0};
	__property bool MoveFocusOnCollapse = {read=FMoveFocusOnCollapse, write=SetMoveFocusOnCollapse, default=0};
	__property Stdctrls::TScrollStyle ForcedScrollBars = {read=FForcedScrollBars, write=SetForcedScrollBars, default=0};
	__property bool PlusMinusTransparent = {read=FTransButtons, write=SetTransButtons, default=0};
	__property WideString Hint = {read=GetHint, write=SetHint};
	__property Graphics::TColor DragRectAcceptColor = {read=FDragRectAcceptColor, write=SetDragRectAcceptColor, default=32768};
	__property Graphics::TColor DragRectDenyColor = {read=FDragRectDenyColor, write=SetDragRectDenyColor, default=255};
	__property int DragExpandDelay = {read=FDragExpandDelay, write=FDragExpandDelay, default=500};
	__property bool IncrementalSearch = {read=FIncrementalSearch, write=FIncrementalSearch, nodefault};
	__property bool AdjustMultilineHeight = {read=FAdjustMultilineHeight, write=SetAdjustMultilineHeight, default=1};
	__property bool ExpandOnDragOver = {read=FExpandOnDragOver, write=FExpandOnDragOver, default=0};
	__property Controls::TCursor DragCursor = {read=GetDragCursor, write=SetDragCursor, nodefault};
	__property Graphics::TColor TrackColor = {read=FTrackColor, write=SetTrackColor, default=-2147483635};
	__property bool UseSystemHintColors = {read=FUseSystemHintColors, write=FUseSystemHintColors, default=0};
	__property Graphics::TColor HeaderColor = {read=GetHeaderColor, write=SetHeaderColor, default=-2147483633};
	__property int ChangeDelay = {read=FChangeDelay, write=FChangeDelay, default=500};
	__property bool RightClickSelect = {read=FRightClickSelect, write=FRightClickSelect, default=1};
	__property Graphics::TColor StripedOddColor = {read=FStripedOddColor, write=SetStripedOddColor, stored=IsStripedColorStored, nodefault};
	__property Graphics::TColor StripedEvenColor = {read=FStripedEvenColor, write=SetStripedEvenColor, stored=IsStripedColorStored, nodefault};
	__property bool StripedItems = {read=FStripedItems, write=SetStripedItems, default=0};
	__property TInplaceEditorNeededEvent OnInplaceEditorNeeded = {read=FOnInplaceEditorNeeded, write=FOnInplaceEditorNeeded};
	__property bool QuickEditMode = {read=FQuickEditMode, write=FQuickEditMode, default=0};
	__property Elheader::TElFieldType MainTextType = {read=FMainTextType, write=FMainTextType, default=1};
	__property TElHintType HintType = {read=FHintType, write=FHintType, default=2};
	__property Elscrollbar::TElScrollHitTestEvent OnVertScrollHitTest = {read=FOnVertScrollHitTest, write=FOnVertScrollHitTest};
	__property Elscrollbar::TElScrollHitTestEvent OnHorzScrollHitTest = {read=FOnHorzScrollHitTest, write=FOnHorzScrollHitTest};
	__property bool MouseFrameSelect = {read=FMouseFrameSelect, write=FMouseFrameSelect, nodefault};
	__property Graphics::TColor VertDivLinesColor = {read=FVertDivLinesColor, write=SetVertDivLinesColor, default=-2147483633};
	__property TOnItemChangeEvent OnItemChange = {read=FOnItemChange, write=FOnItemChange};
	__property TOnItemDrawEvent OnItemDraw = {read=FOnItemDraw, write=FOnItemDraw};
	__property TOnItemCheckedEvent OnItemChecked = {read=FOnItemChecked, write=FOnItemChecked};
	__property TOnItemExpandEvent OnItemExpand = {read=FOnItemExpand, write=FOnItemExpand};
	__property TOnItemExpandEvent OnItemCollapse = {read=FOnItemCollapse, write=FOnItemCollapse};
	__property TOnItemExpanding OnItemExpanding = {read=FOnItemExpanding, write=FOnItemExpanding};
	__property TOnItemExpanding OnItemCollapsing = {read=FOnItemCollapsing, write=FOnItemCollapsing};
	__property TElScrollEvent OnScroll = {read=FOnScroll, write=FOnScroll};
	__property TOnItemExpandEvent OnItemDeletion = {read=FOnItemDelete, write=FOnItemDelete};
	__property TElTreeChangingEvent OnChanging = {read=FOnChanging, write=FOnChanging};
	__property Classes::TNotifyEvent OnItemFocused = {read=FOnItemFocused, write=FOnItemFocused};
	__property TOnShowHintEvent OnShowLineHint = {read=FOnShowHint, write=FOnShowHint};
	__property TOnCompareItems OnCompareItems = {read=FOnCompareItems, write=FOnCompareItems};
	__property TOnPicDrawEvent OnItemPicDraw = {read=FOnItemPicDraw, write=FOnItemPicDraw};
	__property TOnPicDrawEvent OnItemPicDraw2 = {read=FOnItemPicDraw2, write=FOnItemPicDraw2};
	__property THotTrackEvent OnHotTrack = {read=FOnHotTrack, write=FOnHotTrack};
	__property TTryEditEvent OnTryEdit = {read=FOnTryEdit, write=FOnTryEdit};
	__property TItemSaveEvent OnItemSave = {read=FOnItemSave, write=FOnItemSave};
	__property TItemSaveEvent OnItemLoad = {read=FOnItemLoad, write=FOnItemLoad};
	__property TItemSelChangeEvent OnItemSelectedChange = {read=FOnItemSelectedChange, write=FOnItemSelectedChange};
	__property TCellStyleSaveEvent OnCellStyleSave = {read=FOnSave, write=FOnSave};
	__property TCellStyleSaveEvent OnCellStyleLoad = {read=FOnLoad, write=FOnLoad};
	__property Classes::TNotifyEvent OnSortBegin = {read=FOnSortBegin, write=FOnSortBegin};
	__property Classes::TNotifyEvent OnSortEnd = {read=FOnSortEnd, write=FOnSortEnd};
	__property Classes::TNotifyEvent OnHeaderResize = {read=FOnHeaderResize, write=FOnHeaderResize};
	__property Elheader::TElHeaderLookupEvent OnHeaderLookup = {read=FOnHeaderLookup, write=FOnHeaderLookup};
	__property Elheader::TElHeaderLookupDoneEvent OnHeaderLookupDone = {read=FOnHeaderLookupDone, write=FOnHeaderLookupDone};
	__property THeaderSectionEvent OnHeaderSectionExpand = {read=FOnHeaderSectionExpand, write=FOnHeaderSectionExpand};
	__property THeaderSectionEvent OnHeaderSectionCollapse = {read=FOnHeaderSectionCollapse, write=FOnHeaderSectionCollapse};
	__property TColumnNotifyEvent OnHeaderSectionAutoSize = {read=FOnSectionAutoSize, write=FOnSectionAutoSize};
	__property TColumnNotifyEvent OnHeaderColumnResize = {read=FOnColumnResize, write=FOnColumnResize};
	__property TColumnNotifyEvent OnHeaderColumnClick = {read=FOnColumnClick, write=FOnColumnClick};
	__property TElColumnMoveEvent OnHeaderColumnMove = {read=FOnHeaderColumnMove, write=FOnHeaderColumnMove};
	__property Elheader::TElSectionRedrawEvent OnHeaderColumnDraw = {read=FOnColumnDraw, write=FOnColumnDraw};
	__property TColumnNotifyEvent OnHeaderSectionFilterCall = {read=FOnSectionFilterCall, write=FOnSectionFilterCall};
	__property Elheader::TMeasureSectionEvent OnHeaderSectionMeasure = {read=FOnHeaderSectionMeasure, write=FOnHeaderSectionMeasure};
	__property TApplyVisFilterEvent OnApplyVisFilter = {read=FOnApplyVisFilter, write=FOnApplyVisFilter};
	__property TElTreeItemPostDrawEvent OnItemPostDraw = {read=FOnItemPostDraw, write=FOnItemPostDraw};
	__property TMeasureItemPartEvent OnMeasureItemPart = {read=FOnMeasureItemPart, write=FOnMeasureItemPart};
	__property Htmlrender::TElHTMLImageNeededEvent OnHTMLImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Classes::TNotifyEvent OnClick = {read=FOnClick, write=FOnClick};
	__property Classes::TNotifyEvent OnDblClick = {read=FOnDblClick, write=FOnDblClick};
	__property Controls::TDragDropEvent OnDragDrop = {read=FOnDrop, write=FOnDrop};
	__property Controls::TDragOverEvent OnDragOver = {read=FOnOver, write=FOnOver};
	__property Classes::TNotifyEvent OnEnter = {read=FOnEnter, write=FOnEnter};
	__property Classes::TNotifyEvent OnExit = {read=FOnExit, write=FOnExit};
	__property Controls::TKeyEvent OnKeyDown = {read=FOnKeyDown, write=FOnKeyDown};
	__property Controls::TKeyPressEvent OnKeyPress = {read=FOnKeyPress, write=FOnKeyPress};
	__property Controls::TKeyEvent OnKeyUp = {read=FOnKeyUp, write=FOnKeyUp};
	__property Controls::TMouseEvent OnMouseDown = {read=FOnMouseDown, write=FOnMouseDown};
	__property Controls::TMouseMoveEvent OnMouseMove = {read=FOnMouseMove, write=FOnMouseMove};
	__property Controls::TMouseEvent OnMouseUp = {read=FOnMouseUp, write=FOnMouseUp};
	__property Controls::TStartDragEvent OnStartDrag = {read=FOnStartDrag, write=FOnStartDrag};
	__property Eldragdrop::TTargetDragEvent OnOleTargetDrag = {read=FOnOleTargetDrag, write=FOnOleTargetDrag};
	__property Eldragdrop::TTargetDropEvent OnOleTargetDrop = {read=FOnOleTargetDrop, write=FOnOleTargetDrop};
	__property TOleDragStartEvent OnOleDragStart = {read=FOnOleDragStart, write=FOnOleDragStart};
	__property TOleDragFinishEvent OnOleDragFinish = {read=FOnOleDragFinish, write=FOnOleDragFinish};
	__property Elscrollbar::TElScrollDrawPartEvent OnHorzScrollDrawPart = {read=FOnHorzScrollDrawPart, write=FOnHorzScrollDrawPart};
	__property Elscrollbar::TElScrollHintNeededEvent OnHorzScrollHintNeeded = {read=FOnHorzScrollHintNeeded, write=FOnHorzScrollHintNeeded};
	__property Elscrollbar::TElScrollDrawPartEvent OnVertScrollDrawPart = {read=FOnVertScrollDrawPart, write=FOnVertScrollDrawPart};
	__property Elscrollbar::TElScrollHintNeededEvent OnVertScrollHintNeeded = {read=FOnVertScrollHintNeeded, write=FOnVertScrollHintNeeded};
	__property Controls::TMouseEvent OnHeaderMouseDown = {read=FOnHeaderMouseDown, write=FOnHeaderMouseDown};
	__property Classes::TNotifyEvent OnAfterSelectionChange = {read=FOnAfterSelectionChange, write=FOnAfterSelectionChange};
	__property TOnItemExpandEvent OnItemPreDraw = {read=FOnItemPreDraw, write=FOnItemPreDraw};
	__property TElTreeItemDragTargetEvent OnDragTargetChange = {read=FOnDragTargetChange, write=FOnDragTargetChange};
	__property int LineHintTimeout = {read=FLineHintTimeout, write=SetLineHintTimeout, default=3000};
	__property bool VerticalLinesLong = {read=FVerticalLinesLong, write=SetVerticalLinesLong, default=1};
	__property int DefaultSectionWidth = {read=GetDefaultSectionWidth, write=SetDefaultSectionWidth, nodefault};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property TLineHintType LineHintType = {read=FLineHintType, write=FLineHintType, default=2};
	__property TVirtualTextNeededEvent OnVirtualTextNeeded = {read=FOnVirtualTextNeeded, write=FOnVirtualTextNeeded};
	__property TVirtualityLevel VirtualityLevel = {read=FVirtualityLevel, write=SetVirtualityLevel, nodefault};
	__property TVirtualHintNeededEvent OnVirtualHintNeeded = {read=FOnVirtualHintNeeded, write=FOnVirtualHintNeeded};
	__property TVirtualValueNeededEvent OnVirtualValueNeeded = {read=FOnVirtualValueNeeded, write=FOnVirtualValueNeeded};
	__property TVirtualStyleNeededEvent OnVirtualStyleNeeded = {read=FOnVirtualStyleNeeded, write=FOnVirtualStyleNeeded};
	__property int CheckBoxSize = {read=GetCheckBoxSize, write=SetCheckBoxSize, default=15};
	__property int DragScrollInterval = {read=FDragScrollInterval, write=FDragScrollInterval, default=100};
	__property bool ShowLeafButton = {read=FShowLeafButton, write=SetShowLeafButton, nodefault};
	__property Graphics::TBitmap* LeafPicture = {read=GetLeafPicture, write=SetLeafPicture};
	__property bool ExplorerEditMode = {read=FExplorerEditMode, write=FExplorerEditMode, nodefault};
	__property bool IgnoreEnabled = {read=FIgnoreEnabled, write=FIgnoreEnabled, nodefault};
	__property int InplaceEditorDelay = {read=FInplaceEditorDelay, write=FInplaceEditorDelay, default=500};
	__property Graphics::TFont* HeaderFont = {read=FHeaderFont, write=SetHeaderFont};
	__property bool HeaderUseTreeFont = {read=FHeaderUseTreeFont, write=SetHeaderUseTreeFont, default=1};
	__property bool KeepSelectionWithinLevel = {read=FKeepSelectionWithinLevel, write=FKeepSelectionWithinLevel, nodefault};
	__property bool AutoCollapse = {read=FAutoCollapse, write=FAutoCollapse, default=1};
	__property bool SortUseCase = {read=FSortUseCase, write=SetSortUseCase, default=1};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property TElDblClickMode DblClickMode = {read=FDblClickMode, write=SetDblClickMode, default=1};
	
public:
	__fastcall virtual TCustomElTree(Classes::TComponent* AOwner);
	__fastcall TCustomElTree(Classes::TComponent* AOwner, TMetaClass* ItemClass);
	__fastcall virtual ~TCustomElTree(void);
	virtual void __fastcall Update(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	virtual void __fastcall FullCollapse(void);
	virtual void __fastcall FullExpand(void);
	DYNAMIC bool __fastcall CanFocus(void);
	DYNAMIC bool __fastcall Focused(void);
	virtual Types::TRect __fastcall GetItemRect(int ItemIndex);
	virtual TElTreeItem* __fastcall GetItemAtY(int Y);
	virtual TElTreeItem* __fastcall GetItemAt(int X, int Y, TSTItemPart &ItemPart, int &HitColumn);
	virtual void __fastcall MeasureCell(TElTreeItem* Item, int ColumnNum, Types::TPoint &Size);
	virtual TElTreeItem* __fastcall GetNextSelected(TElTreeItem* Prev);
	virtual void __fastcall AllSelected(Ellist::TElList* SelectedItems);
	virtual void __fastcall SelectAll(void);
	virtual void __fastcall InvertSelection(void);
	virtual void __fastcall SelectAllEx(bool IncludeHidden);
	virtual void __fastcall InvertSelectionEx(bool IncludeHidden);
	virtual void __fastcall DeselectAll(void);
	virtual void __fastcall DeselectAllEx(bool IncludeHidden);
	virtual void __fastcall SelectRange(TElTreeItem* FromItem, TElTreeItem* ToItem);
	virtual void __fastcall SelectRange2(TElTreeItem* FromItem, TElTreeItem* ToItem, bool SelectDisabled);
	virtual void __fastcall SelectRangeEx(TElTreeItem* FromItem, TElTreeItem* ToItem, bool IncludeHidden);
	virtual void __fastcall SelectRangeEx2(TElTreeItem* FromItem, TElTreeItem* ToItem, bool IncludeHidden, bool SelectDisabled);
	virtual void __fastcall Sort(bool recursive);
	virtual void __fastcall Save(void);
	virtual void __fastcall Restore(void);
	void __fastcall EnsureVisible(TElTreeItem* Item);
	void __fastcall EnsureVisibleBottom(TElTreeItem* Item);
	bool __fastcall IsEditing(void);
	virtual void __fastcall EditItem(TElTreeItem* Item, int SectionNum);
	void __fastcall EndEdit(bool ByCancel);
	virtual void __fastcall SaveStringsToStream(Classes::TStream* Stream);
	TElTreeItem* __fastcall GetNodeAt(int X, int Y);
	virtual void __fastcall CreateWindowHandle(const Controls::TCreateParams &Params);
	virtual bool __fastcall IsInView(TElTreeItem* Item);
	int __fastcall MeasureColumnWidth(int ColumnNum, bool VisibleOnly);
	int __fastcall IndexInView(TElTreeItem* Item);
	void __fastcall AllSelectedEx(Ellist::TElList* SelectedItems, bool Order);
	void __fastcall AddSortSection(int Index, bool ReSort);
	void __fastcall RemoveSortSection(int Index, bool ReSort);
	void __fastcall ClearSortList(bool ReSort);
	__property int TopIndex = {read=FTopIndex, write=SetVPosition, nodefault};
	__property int BottomIndex = {read=FBottomIndex, nodefault};
	__property bool IsUpdating = {read=GetUpdating, write=SetUpdating, nodefault};
	__property TElTreeItems* Items = {read=FItems, write=SetItems};
	__property TElTreeItem* ItemFocused = {read=GetFocused, write=SetFocused};
	__property int SelectedCount = {read=GetSelCount, nodefault};
	__property bool FireFocusEvents = {read=GetFireFocusEvents, write=SetFireFocusEvents, default=1};
	__property TElTreeItem* Selected = {read=GetSelected, write=SetSelected};
	__property TElTreeItem* TopItem = {read=GetTopItem, write=SetTopItem};
	__property Controls::TDragObject* DragObject = {read=FDragObject};
	__property TElTreeView* View = {read=FView};
	__property bool HorzScrollBarVisible = {read=FHScrollVisible, nodefault};
	__property bool VertScrollBarVisible = {read=FVScrollVisible, nodefault};
	__property Elheader::TElHeaderSection* LockedHeaderSection = {read=GetLockedHeaderSection, write=SetLockedHeaderSection};
	__property int VisibleRowCount = {read=GetVisibleRowCount, nodefault};
	__property TElTreeItem* DropTarget = {read=GetDropTarget};
	__property Elscrollbar::TElScrollBar* HScrollBar = {read=FHScrollBar};
	__property Elscrollbar::TElScrollBar* VScrollBar = {read=FVScrollBar};
	__property TElTreeItem* TrackItem = {read=GetTrackItem};
	
__published:
	__property int LeftPosition = {read=FHPos, write=SetHPosition, nodefault};
	__property Controls::TBevelKind BevelKind = {read=FBevelKindDummy, write=FBevelKindDummy, stored=false, default=0};
	__property Menus::TPopupMenu* HeaderPopupMenu = {read=GetHeaderPopupMenu, write=SetHeaderPopupMenu};
	__property bool DoubleBuffered = {read=FDoubleBuffered, write=SetDoubleBuffered, default=1};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElTree(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElTreeInplaceEditor : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
protected:
	WideString FDefaultValueAsText;
	bool FEditing;
	TCustomElTree* FTree;
	Elheader::TElFieldTypes FTypes;
	TElTreeItem* FItem;
	WideString FValueAsText;
	int FSectionIndex;
	Elheader::TElFieldType FDataType;
	#pragma pack(push, 1)
	Types::TRect FCellRect;
	#pragma pack(pop)
	
	TInplaceOperationEvent FOnBeforeOperation;
	TInplaceAfterOperationEvent FOnAfterOperation;
	TInplaceValidationEvent FOnValidateResult;
	void __fastcall SetTree(TCustomElTree* Value);
	virtual bool __fastcall GetVisible(void) = 0 ;
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall SetEditorParent(void);
	virtual void __fastcall StartOperation(void);
	virtual void __fastcall CompleteOperation(bool Accepted);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerValidateResult(bool &InputValid);
	virtual void __fastcall DoStartOperation(void) = 0 ;
	virtual void __fastcall DoStopOperation(bool Accepted);
	
public:
	__property TElTreeItem* Item = {read=FItem};
	__property WideString ValueAsText = {read=FValueAsText, write=FValueAsText};
	__property int SectionIndex = {read=FSectionIndex, nodefault};
	__property Elheader::TElFieldType DataType = {read=FDataType, nodefault};
	__property Types::TRect CellRect = {read=FCellRect};
	__property bool Visible = {read=GetVisible, nodefault};
	
__published:
	__property TCustomElTree* Tree = {read=FTree, write=SetTree};
	__property Elheader::TElFieldTypes Types = {read=FTypes, write=FTypes, default=0};
	__property WideString DefaultValueAsText = {read=FDefaultValueAsText, write=FDefaultValueAsText};
	__property TInplaceOperationEvent OnBeforeOperation = {read=FOnBeforeOperation, write=FOnBeforeOperation};
	__property TInplaceAfterOperationEvent OnAfterOperation = {read=FOnAfterOperation, write=FOnAfterOperation};
	__property TInplaceValidationEvent OnValidateResult = {read=FOnValidateResult, write=FOnValidateResult};
public:
	#pragma option push -w-inl
	/* TComponent.Create */ inline __fastcall virtual TElTreeInplaceEditor(Classes::TComponent* AOwner) : Classes::TComponent(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TComponent.Destroy */ inline __fastcall virtual ~TElTreeInplaceEditor(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElCellCheckBox;
class PASCALIMPLEMENTATION TElCellCheckBox : public TElCellControl 
{
	typedef TElCellControl inherited;
	
private:
	Classes::TAlignment FAlignment;
	Stdctrls::TCheckBoxState FState;
	bool FAllowGrayed;
	void __fastcall SetState(Stdctrls::TCheckBoxState newValue);
	void __fastcall SetAllowGrayed(bool newValue);
	bool __fastcall GetChecked(void);
	void __fastcall SetChecked(bool newValue);
	void __fastcall SetAlignment(Classes::TAlignment newValue);
	
protected:
	virtual void __fastcall TriggerClickEvent(void);
	virtual void __fastcall TriggerMouseDownEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerMouseUpEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	
public:
	virtual void __fastcall Assign(TElCellControl* Source);
	__fastcall virtual TElCellCheckBox(void);
	__fastcall virtual ~TElCellCheckBox(void);
	virtual void __fastcall Paint(Graphics::TCanvas* Canvas, const Types::TRect &R);
	__property Stdctrls::TCheckBoxState State = {read=FState, write=SetState, nodefault};
	__property bool Checked = {read=GetChecked, write=SetChecked, nodefault};
	__property bool AllowGrayed = {read=FAllowGrayed, write=SetAllowGrayed, nodefault};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, nodefault};
};


class DELPHICLASS TElCellButtonGlyph;
class PASCALIMPLEMENTATION TElCellButtonGlyph : public Elpopbtn::TElButtonGlyph 
{
	typedef Elpopbtn::TElButtonGlyph inherited;
	
public:
	__property ImageList ;
	__property ImageIndex ;
	__property UseImageList ;
public:
	#pragma option push -w-inl
	/* TElButtonGlyph.Create */ inline __fastcall TElCellButtonGlyph(void) : Elpopbtn::TElButtonGlyph() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TElButtonGlyph.Destroy */ inline __fastcall virtual ~TElCellButtonGlyph(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElCellButton;
class PASCALIMPLEMENTATION TElCellButton : public TElCellControl 
{
	typedef TElCellControl inherited;
	
private:
	TElCellButtonGlyph* FGlyph;
	Buttons::TButtonLayout FLayout;
	bool FFixClick;
	bool FDown;
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* newValue);
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall SetDown(bool newValue);
	void __fastcall SetLayout(Buttons::TButtonLayout newValue);
	bool __fastcall GetUseImageList(void);
	void __fastcall SetUseImageList(bool newValue);
	Controls::TImageList* __fastcall GetImageList(void);
	void __fastcall SetImageList(Controls::TImageList* newValue);
	int __fastcall GetImageIndex(void);
	void __fastcall SetImageIndex(int newValue);
	
protected:
	virtual void __fastcall TriggerMouseDownEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerMouseUpEvent(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	
public:
	virtual void __fastcall Assign(TElCellControl* Source);
	__fastcall virtual TElCellButton(void);
	__fastcall virtual ~TElCellButton(void);
	virtual void __fastcall Paint(Graphics::TCanvas* Canvas, const Types::TRect &R);
	__property bool UseImageList = {read=GetUseImageList, write=SetUseImageList, nodefault};
	__property Controls::TImageList* ImageList = {read=GetImageList, write=SetImageList};
	__property int ImageIndex = {read=GetImageIndex, write=SetImageIndex, nodefault};
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property bool FixClick = {read=FFixClick, write=FFixClick, nodefault};
	__property bool Down = {read=FDown, write=SetDown, default=0};
	__property Buttons::TButtonLayout Layout = {read=FLayout, write=SetLayout, nodefault};
};


class DELPHICLASS TElCellProgressBar;
class PASCALIMPLEMENTATION TElCellProgressBar : public TElCellControl 
{
	typedef TElCellControl inherited;
	
protected:
	int FMinValue;
	int FMaxValue;
	int FValue;
	Graphics::TColor FBarColor;
	bool FShowProgressText;
	Classes::TAlignment FTextAlignment;
	Graphics::TColor FFrameColor;
	Graphics::TColor FColor;
	void __fastcall SetMinValue(int Value);
	void __fastcall SetMaxValue(int Value);
	void __fastcall SetValue(int Value);
	void __fastcall SetBarColor(Graphics::TColor Value);
	void __fastcall SetShowProgressText(bool Value);
	void __fastcall SetTextAlignment(Classes::TAlignment Value);
	void __fastcall SetFrameColor(Graphics::TColor Value);
	void __fastcall SetColor(Graphics::TColor Value);
	virtual bool __fastcall PassClicks(void);
	
public:
	__fastcall virtual TElCellProgressBar(void);
	__fastcall virtual ~TElCellProgressBar(void);
	virtual void __fastcall Assign(TElCellControl* Source);
	virtual void __fastcall Paint(Graphics::TCanvas* Canvas, const Types::TRect &R);
	__property int MinValue = {read=FMinValue, write=SetMinValue, nodefault};
	__property int MaxValue = {read=FMaxValue, write=SetMaxValue, nodefault};
	__property int Value = {read=FValue, write=SetValue, nodefault};
	__property Graphics::TColor BarColor = {read=FBarColor, write=SetBarColor, nodefault};
	__property Classes::TAlignment TextAlignment = {read=FTextAlignment, write=SetTextAlignment, nodefault};
	__property bool ShowProgressText = {read=FShowProgressText, write=SetShowProgressText, nodefault};
	__property Graphics::TColor FrameColor = {read=FFrameColor, write=SetFrameColor, nodefault};
	__property Graphics::TColor Color = {read=FColor, write=SetColor, nodefault};
};


class PASCALIMPLEMENTATION TElCellStyle : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	int FTag;
	TElTreeItem* FOwner;
	Graphics::TColor FCellBkColor;
	Graphics::TColor FTextBkColor;
	Graphics::TColor FTextColor;
	unsigned FTextFlags;
	Graphics::TBitmap* FPicture;
	Elheader::TElFieldType FCellType;
	Elheader::TElSectionStyle FStyle;
	bool FOwnerProps;
	int FFontSize;
	Graphics::TFontStyles FFontStyles;
	AnsiString FFontName;
	TElCellControl* FControl;
	bool FUseBkColor;
	void __fastcall SetControl(TElCellControl* newValue);
	void __fastcall SetFontSize(int newValue);
	void __fastcall SetFontStyles(Graphics::TFontStyles newValue);
	void __fastcall SetFontName(AnsiString newValue);
	void __fastcall SetOwnerColors(bool newValue);
	void __fastcall SetStyle(Elheader::TElSectionStyle newValue);
	void __fastcall SetCellBkColor(Graphics::TColor newValue);
	void __fastcall SetTextBkColor(Graphics::TColor newValue);
	void __fastcall SetTextColor(Graphics::TColor newValue);
	void __fastcall SetTextFlags(unsigned newValue);
	void __fastcall SetPicture(Graphics::TBitmap* newValue);
	void __fastcall SetCellType(Elheader::TElFieldType newValue);
	void __fastcall SetUseBkColor(bool Value);
	
public:
	__fastcall TElCellStyle(TElTreeItem* Owner);
	__fastcall virtual ~TElCellStyle(void);
	void __fastcall Assign(TElCellStyle* Source);
	void __fastcall Update(void);
	__property int Tag = {read=FTag, write=FTag, nodefault};
	__property TElCellControl* Control = {read=FControl, write=SetControl};
	__property Graphics::TColor CellBkColor = {read=FCellBkColor, write=SetCellBkColor, nodefault};
	__property Graphics::TColor TextBkColor = {read=FTextBkColor, write=SetTextBkColor, nodefault};
	__property Graphics::TColor TextColor = {read=FTextColor, write=SetTextColor, nodefault};
	__property unsigned TextFlags = {read=FTextFlags, write=SetTextFlags, nodefault};
	__property Graphics::TBitmap* Picture = {read=FPicture, write=SetPicture};
	__property Elheader::TElFieldType CellType = {read=FCellType, write=SetCellType, nodefault};
	__property Elheader::TElSectionStyle Style = {read=FStyle, write=SetStyle, nodefault};
	__property bool OwnerProps = {read=FOwnerProps, write=SetOwnerColors, nodefault};
	__property int FontSize = {read=FFontSize, write=SetFontSize, nodefault};
	__property Graphics::TFontStyles FontStyles = {read=FFontStyles, write=SetFontStyles, nodefault};
	__property AnsiString FontName = {read=FFontName, write=SetFontName};
	__property TElTreeItem* Owner = {read=FOwner};
	__property bool UseBkColor = {read=FUseBkColor, write=SetUseBkColor, nodefault};
};


#pragma pack(push, 4)
struct TElTreeItemStaticData
{
	WideString FText;
	Elunicodestrings::TElWideStringList* FColText;
	WideString FHint;
	TElCellStyle* FMainStyle;
	Ellist::TElList* FStyles;
} ;
#pragma pack(pop)

typedef TElTreeItemStaticData *PElTreeItemStaticData;

class PASCALIMPLEMENTATION TElTreeItem : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
protected:
	Htmlrender::TElHTMLData* FHTMLData;
	Elarray::TElArray* FHTMLDataArray;
	int FTag;
	System::TObject* FObject;
	System::_di_IInterface FDataInterface;
	void *FSortData;
	int FSortType;
	int FSortRef;
	TElTreeItemStaticData *FStaticData;
	TElItemBorderStyle FBorderStyle;
	unsigned FComplexHeight;
	Stdctrls::TCheckBoxState FCheckBoxState;
	TElCheckBoxType FCheckBoxType;
	TSTIStates FState;
	int FIState;
	Ellist::TElList* FChildren;
	TCustomElTree* FOwner;
	TElTreeItems* FList;
	void *FData;
	Graphics::TColor FRowBkColor;
	Graphics::TColor FColor;
	Graphics::TColor FBkColor;
	Graphics::TColor FStrikedLineColor;
	int FBoolData1;
	int FTextLeft;
	int FTextRight;
	int FImageIndex;
	int FStImageIndex;
	int FImageIndex2;
	int FStImageIndex2;
	TElTreeItem* FParent;
	TElTreeItem* FRoot;
	int FIndentAdjust;
	Graphics::TColor FBorderSpaceColor;
	Shortint FOverlayIndex;
	Shortint FOverlayIndex2;
	WideString __fastcall GetText();
	virtual WideString __fastcall GetHint();
	void __fastcall SetHint(WideString Value);
	void __fastcall SetBorderStyle(TElItemBorderStyle Value);
	TElTreeItem* __fastcall GetParent(void);
	int __fastcall GetLevel(void);
	void __fastcall SetColor(int index, Graphics::TColor value);
	void __fastcall SetUseBkColor(bool newValue);
	bool __fastcall GetHasChildren(void);
	bool __fastcall GetHasVisibleChildren(void);
	void __fastcall SetExpanded(bool value);
	void __fastcall SetParentColors(bool value);
	void __fastcall SetParentStyle(bool value);
	int __fastcall GetIndex(void);
	int __fastcall GetAbsIndex(void);
	int __fastcall GetVisIndex(void);
	int __fastcall GetChildIndex(TElTreeItem* Child);
	bool __fastcall IsExpanded(void);
	bool __fastcall GetFullExpand(void);
	void __fastcall MakeFullyExpanded(bool value);
	void __fastcall OnColTextChange(System::TObject* Sender);
	void __fastcall SetImageIndex(int value);
	void __fastcall SetStImageIndex(int value);
	void __fastcall SetImageIndex2(int value);
	void __fastcall SetStImageIndex2(int value);
	void __fastcall SetForceButtons(bool newValue);
	int __fastcall GetChildrenCount(void);
	int __fastcall GetCount(void);
	TElTreeItem* __fastcall GetItems(int Index);
	void __fastcall SetUseStyles(bool newValue);
	void __fastcall OnStyleDelete(System::TObject* Sender, void * Item);
	TElCellStyle* __fastcall GetStyles(int index);
	void __fastcall SetStyles(int index, TElCellStyle* newValue);
	int __fastcall GetStylesCount(void);
	void __fastcall SetCheckBoxState(Stdctrls::TCheckBoxState newValue);
	void __fastcall SetChecked(bool newValue);
	bool __fastcall GetChecked(void);
	void __fastcall SetShowCheckBox(bool newValue);
	void __fastcall SetCheckBoxType(TElCheckBoxType newValue);
	void __fastcall SetCheckBoxEnabled(bool newValue);
	void __fastcall SetSuppressButtons(bool newValue);
	void __fastcall SetEnabled(bool newValue);
	void __fastcall SetHidden(bool newValue);
	bool __fastcall GetFullyVisible(void);
	void __fastcall SetFullyVisible(bool newValue);
	bool __fastcall GetSelected(void);
	void __fastcall SetSelected(bool newValue);
	void __fastcall CreateStyles(void);
	void __fastcall SetOwnerHeight(bool newValue);
	void __fastcall SetHeight(int newValue);
	int __fastcall GetHeight(void);
	void __fastcall SetSuppressLines(bool newValue);
	void __fastcall UpdateItem(void);
	virtual void __fastcall SetText(WideString Value);
	bool __fastcall GetState(int index);
	void __fastcall SetState(int index, bool value);
	void __fastcall RemoveChild(TElTreeItem* Child);
	void __fastcall DeleteChild(TElTreeItem* Child);
	int __fastcall AddChild(TElTreeItem* Child);
	int __fastcall AddLastChild(TElTreeItem* Child);
	int __fastcall InsertChild(int index, TElTreeItem* Child);
	virtual void __fastcall ReadData(Classes::TStream* Stream);
	virtual void __fastcall WriteData(Classes::TStream* Stream);
	void __fastcall ExchangeItems(int I, int J);
	void __fastcall QuickSort(bool recursive, int L, int R, Elheader::TElSSortMode SM, TSortTypes SortType, int FSortSection);
	void __fastcall AddSortData(TSortTypes SortType, int FSortSection);
	void __fastcall ReleaseSortData(void);
	void __fastcall NormalizeSorts(int StartIdx);
	virtual void __fastcall SetRowBkColor(Graphics::TColor newValue);
	bool __fastcall GetOwnerHeight(void);
	virtual void __fastcall SetMultiline(bool newValue);
	void __fastcall SetIsHTML(bool newValue);
	void __fastcall OnHTMLDataDestroy(System::TObject* Sender, void * Item);
	void __fastcall ReRenderMainText(void);
	void __fastcall ReRenderAllTexts(void);
	TElTreeItem* __fastcall GetAncestor(void);
	void __fastcall SetStrikedOutLine(const bool Value);
	void __fastcall SetStrikedLineColor(const Graphics::TColor Value);
	bool __fastcall GetStrikedOutLine(void);
	void __fastcall SetDrawHLine(const bool Value);
	void __fastcall SetAllowEdit(const bool Value);
	int __fastcall CalcSubItemsHeight(void);
	void __fastcall NewStaticData(void);
	void __fastcall DisposeStaticData(void);
	void __fastcall FillStaticData(void);
	Elunicodestrings::TElWideStrings* __fastcall GetColText(void);
	bool __fastcall GetParentStyle(void);
	TElCellStyle* __fastcall GetMainStyle(void);
	bool __fastcall GetUseStyles(void);
	bool __fastcall GetUseBkColor(void);
	bool __fastcall GetParentColors(void);
	bool __fastcall GetDrawHLine(void);
	bool __fastcall GetAllowEdit(void);
	bool __fastcall GetForceButtons(void);
	bool __fastcall GetSuppressButtons(void);
	bool __fastcall GetSuppressLines(void);
	bool __fastcall GetIsHTML(void);
	bool __fastcall GetMultiline(void);
	bool __fastcall GetShowCheckBox(void);
	bool __fastcall GetCheckBoxEnabled(void);
	bool __fastcall GetEnabled(void);
	bool __fastcall GetHidden(void);
	void __fastcall SetIndentAdjust(int Value);
	bool __fastcall GetDropTarget(void);
	bool __fastcall GetHintIsHTML(void);
	void __fastcall SetHintIsHTML(bool Value);
	void __fastcall SetBorderSpaceColor(Graphics::TColor Value);
	void __fastcall SetOverlayIndex(Shortint value);
	void __fastcall SetOverlayIndex2(Shortint value);
	
public:
	__fastcall virtual TElTreeItem(TCustomElTree* AOwner);
	__fastcall virtual ~TElTreeItem(void);
	virtual int __fastcall GetWidth(void);
	bool __fastcall IsUnder(TElTreeItem* Item);
	WideString __fastcall GetFullName(WideString separator);
	WideString __fastcall GetFullNameEx(WideString separator, bool AddRoot);
	void __fastcall Expand(bool recursive);
	void __fastcall Collapse(bool recursive);
	void __fastcall Sort(bool recursive);
	void __fastcall MoveTo(TElTreeItem* NewParent);
	void __fastcall MoveToIns(TElTreeItem* NewParent, int AnIndex);
	void __fastcall Clear(void);
	TElTreeItem* __fastcall GetFirstVisibleChild(void);
	TElTreeItem* __fastcall GetFirstChild(void);
	TElTreeItem* __fastcall GetLastChild(void);
	TElTreeItem* __fastcall GetNextChild(TElTreeItem* Child);
	TElTreeItem* __fastcall GetPrevChild(TElTreeItem* Child);
	TElTreeItem* __fastcall GetFirstSibling(void);
	TElTreeItem* __fastcall GetLastSibling(void);
	TElTreeItem* __fastcall GetNextSibling(void);
	TElTreeItem* __fastcall GetPrevSibling(void);
	TElTreeItem* __fastcall GetLastSubItem(void);
	TElTreeItem* __fastcall GetChildByIndex(int index);
	void __fastcall EditText(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	virtual void __fastcall Delete(void);
	__property TCustomElTree* TreeView = {read=FOwner};
	bool __fastcall IsVisible(void);
	TElTreeItem* __fastcall GetNextVisible(void);
	TElTreeItem* __fastcall GetPrevVisible(void);
	virtual TElTreeItem* __fastcall GetPrev(void);
	virtual TElTreeItem* __fastcall GetNext(void);
	virtual void __fastcall MoveToItem(TElTreeItem* Item, TNodeAttachMode Mode);
	virtual TElCellStyle* __fastcall AddStyle(void);
	virtual void __fastcall RemoveStyle(TElCellStyle* Style);
	void __fastcall RedrawItem(bool DoCheck);
	void __fastcall RedrawItemPart(bool DoCheck, int Left, int Right);
	Types::TRect __fastcall DisplayRect(bool TextOnly);
	void __fastcall EndEdit(bool ByCancel);
	bool __fastcall HasAsParent(TElTreeItem* Item);
	int __fastcall IndexOf(TElTreeItem* Item);
	void __fastcall MakeVisible(void);
	__property int TextLeft = {read=FTextLeft, nodefault};
	__property int TextRight = {read=FTextRight, nodefault};
	__property void * Data = {read=FData, write=FData};
	__property System::TObject* AnObject = {read=FObject, write=FObject};
	__property System::_di_IInterface DataInterface = {read=FDataInterface, write=FDataInterface};
	__property TCustomElTree* Owner = {read=FOwner};
	__property TElTreeItem* Parent = {read=GetParent, write=MoveTo};
	__property bool HasVisibleChildren = {read=GetHasVisibleChildren, nodefault};
	__property bool HasChildren = {read=GetHasChildren, nodefault};
	__property int Index = {read=GetIndex, nodefault};
	__property int AbsoluteIndex = {read=GetAbsIndex, nodefault};
	__property int VisIndex = {read=GetVisIndex, nodefault};
	__property int Count = {read=GetCount, nodefault};
	__property int ChildrenCount = {read=GetChildrenCount, nodefault};
	__property TElTreeItem* Children[int Index] = {read=GetItems};
	__property TElTreeItem* Item[int Index] = {read=GetItems};
	__property int Level = {read=GetLevel, nodefault};
	__property int Tag = {read=FTag, write=FTag, nodefault};
	__property TElCellStyle* Styles[int index] = {read=GetStyles, write=SetStyles};
	__property TElTreeItem* Ancestor = {read=GetAncestor};
	__property bool StrikedOutLine = {read=GetStrikedOutLine, write=SetStrikedOutLine, nodefault};
	__property Graphics::TColor StrikedLineColor = {read=FStrikedLineColor, write=SetStrikedLineColor, nodefault};
	__property bool DrawHLine = {read=GetDrawHLine, write=SetDrawHLine, nodefault};
	__property bool AllowEdit = {read=GetAllowEdit, write=SetAllowEdit, nodefault};
	__property bool Focused = {read=GetState, write=SetState, index=1, nodefault};
	__property bool Selected = {read=GetSelected, write=SetSelected, nodefault};
	__property bool Cut = {read=GetState, write=SetState, index=3, nodefault};
	__property bool Underlined = {read=GetState, write=SetState, index=4, nodefault};
	__property bool Bold = {read=GetState, write=SetState, index=5, nodefault};
	__property bool Italic = {read=GetState, write=SetState, index=6, nodefault};
	__property bool StrikeOut = {read=GetState, write=SetState, index=8, nodefault};
	__property bool ParentStyle = {read=GetParentStyle, write=SetParentStyle, nodefault};
	__property WideString Text = {read=GetText, write=SetText};
	__property Elunicodestrings::TElWideStrings* ColumnText = {read=GetColText};
	__property Elunicodestrings::TElWideStrings* SubItems = {read=GetColText};
	__property bool Expanded = {read=IsExpanded, write=SetExpanded, nodefault};
	__property bool FullyExpanded = {read=GetFullExpand, write=MakeFullyExpanded, nodefault};
	__property Graphics::TColor Color = {read=FColor, write=SetColor, index=1, nodefault};
	__property Graphics::TColor BkColor = {read=FBkColor, write=SetColor, index=2, nodefault};
	__property bool UseBkColor = {read=GetUseBkColor, write=SetUseBkColor, nodefault};
	__property bool ParentColors = {read=GetParentColors, write=SetParentColors, nodefault};
	__property int ImageIndex = {read=FImageIndex, write=SetImageIndex, nodefault};
	__property int StateImageIndex = {read=FStImageIndex, write=SetStImageIndex, nodefault};
	__property int ImageIndex2 = {read=FImageIndex2, write=SetImageIndex2, nodefault};
	__property int StateImageIndex2 = {read=FStImageIndex2, write=SetStImageIndex2, nodefault};
	__property bool ForceButtons = {read=GetForceButtons, write=SetForceButtons, default=0};
	__property bool SuppressButtons = {read=GetSuppressButtons, write=SetSuppressButtons, default=0};
	__property bool SuppressLines = {read=GetSuppressLines, write=SetSuppressLines, nodefault};
	__property WideString Hint = {read=GetHint, write=SetHint};
	__property bool UseStyles = {read=GetUseStyles, write=SetUseStyles, nodefault};
	__property TElCellStyle* MainStyle = {read=GetMainStyle};
	__property int StylesCount = {read=GetStylesCount, nodefault};
	__property Stdctrls::TCheckBoxState CheckBoxState = {read=FCheckBoxState, write=SetCheckBoxState, nodefault};
	__property bool Checked = {read=GetChecked, write=SetChecked, default=0};
	__property bool ShowCheckBox = {read=GetShowCheckBox, write=SetShowCheckBox, default=1};
	__property TElCheckBoxType CheckBoxType = {read=FCheckBoxType, write=SetCheckBoxType, nodefault};
	__property bool CheckBoxEnabled = {read=GetCheckBoxEnabled, write=SetCheckBoxEnabled, nodefault};
	__property bool Enabled = {read=GetEnabled, write=SetEnabled, default=1};
	__property bool Hidden = {read=GetHidden, write=SetHidden, nodefault};
	__property bool FullyVisible = {read=GetFullyVisible, write=SetFullyVisible, nodefault};
	__property int Height = {read=GetHeight, write=SetHeight, nodefault};
	__property bool OwnerHeight = {read=GetOwnerHeight, write=SetOwnerHeight, nodefault};
	__property bool Multiline = {read=GetMultiline, write=SetMultiline, nodefault};
	__property Graphics::TColor RowBkColor = {read=FRowBkColor, write=SetRowBkColor, nodefault};
	__property bool IsHTML = {read=GetIsHTML, write=SetIsHTML, nodefault};
	__property TElItemBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, nodefault};
	__property int IndentAdjust = {read=FIndentAdjust, write=SetIndentAdjust, nodefault};
	__property bool DropTarget = {read=GetDropTarget, nodefault};
	__property bool HintIsHTML = {read=GetHintIsHTML, write=SetHintIsHTML, nodefault};
	__property Graphics::TColor BorderSpaceColor = {read=FBorderSpaceColor, write=SetBorderSpaceColor, nodefault};
	__property Shortint OverlayIndex = {read=FOverlayIndex, write=SetOverlayIndex, nodefault};
	__property Shortint OverlayIndex2 = {read=FOverlayIndex2, write=SetOverlayIndex2, nodefault};
};


typedef void __fastcall (__closure *TTuneUpInplaceEditEvent)(System::TObject* Sender, TElTreeItem* Item, int SectionIndex, Stdctrls::TCustomEdit* Editor);

typedef void __fastcall (__closure *TOnValidateEvent)(System::TObject* Sender, TElTreeItem* Item, Elheader::TElHeaderSection* Section, AnsiString &Text, bool &Accept);

typedef void __fastcall (__closure *TEditRequestEvent)(System::TObject* Sender, TElTreeItem* Item, Elheader::TElHeaderSection* Section);

typedef void __fastcall (__closure *TComboEditShowEvent)(System::TObject* Sender, TElTreeItem* Item, Elheader::TElHeaderSection* Section, Stdctrls::TComboBox* Combobox);

typedef void __fastcall (__closure *TValidateComboEvent)(System::TObject* Sender, TElTreeItem* Item, Elheader::TElHeaderSection* Section, Stdctrls::TComboBox* Combo, bool &Accept);

class DELPHICLASS TElTree;
class PASCALIMPLEMENTATION TElTree : public TCustomElTree 
{
	typedef TCustomElTree inherited;
	
__published:
	__property ActiveBorderType  = {default=1};
	__property DragCursor ;
	__property Align  = {default=0};
	__property AlwaysKeepFocus  = {default=0};
	__property AlwaysKeepSelection  = {default=1};
	__property AutoCollapse  = {default=1};
	__property AutoExpand  = {default=0};
	__property AutoLineHeight  = {default=1};
	__property AutoLookup  = {default=0};
	__property AutoResizeColumns  = {default=1};
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered  = {default=1};
	__property DragKind  = {default=0};
	__property DefaultSectionWidth ;
	__property AdjustMultilineHeight  = {default=1};
	__property Background ;
	__property BackgroundType  = {default=2};
	__property BarStyle  = {default=0};
	__property BarStyleVerticalLines  = {default=0};
	__property BorderStyle  = {default=1};
	__property BorderSides ;
	__property ChangeDelay  = {default=500};
	__property ChangeStateImage  = {default=0};
	__property CheckBoxGlyph ;
	__property CheckBoxSize  = {default=15};
	__property Ctl3D ;
	__property Color  = {default=-2147483643};
	__property Cursor  = {default=-2};
	__property CustomCheckboxes  = {default=0};
	__property CustomPlusMinus  = {default=0};
	__property DeselectChildrenOnCollapse  = {default=0};
	__property DblClickMode  = {default=1};
	__property DoInplaceEdit  = {default=1};
	__property DragAllowed  = {default=0};
	__property DragExpandDelay  = {default=500};
	__property DraggableSections  = {default=0};
	__property DrawFocusRect  = {default=1};
	__property DragImageMode  = {default=0};
	__property DragRectAcceptColor  = {default=32768};
	__property DragRectDenyColor  = {default=255};
	__property DragScrollInterval  = {default=100};
	__property DragTrgDrawMode  = {default=2};
	__property DragType  = {default=1};
	__property Enabled  = {default=1};
	__property ExpandOnDblClick  = {default=1};
	__property ExpandOnDragOver  = {default=0};
	__property ExplorerEditMode ;
	__property FilteredVisibility  = {default=0};
	__property Flat  = {default=0};
	__property FlatFocusedScrollbars  = {default=1};
	__property FocusedSelectColor  = {default=-2147483635};
	__property FocusedSelectTextColor  = {default=-2147483634};
	__property ForcedScrollBars  = {default=0};
	__property Font  = {stored=true};
	__property FullRowSelect  = {default=1};
	__property GradientStartColor  = {default=0};
	__property GradientEndColor  = {default=0};
	__property GradientSteps  = {default=16};
	__property HeaderActiveFilterColor  = {default=0};
	__property HeaderColor  = {default=-2147483633};
	__property HeaderHeight ;
	__property HeaderHotTrack  = {default=1};
	__property HeaderInvertSortArrows  = {default=0};
	__property HeaderSections ;
	__property HeaderFilterColor  = {default=-2147483630};
	__property HeaderFlat  = {default=0};
	__property HeaderFont ;
	__property HeaderUseTreeFont  = {default=1};
	__property HeaderImages ;
	__property HeaderWrapCaptions  = {default=0};
	__property HideFocusRect  = {default=0};
	__property HideHintOnTimer  = {default=0};
	__property HideHintOnMove  = {default=1};
	__property HideSelectColor  = {default=-2147483633};
	__property HideSelectTextColor  = {default=-2147483632};
	__property HideSelection  = {default=0};
	__property HorizontalLines  = {default=0};
	__property HideHorzScrollBar  = {default=0};
	__property HideVertScrollBar  = {default=0};
	__property Hint ;
	__property HintType  = {default=2};
	__property HorzDivLinesColor  = {default=-2147483633};
	__property HorzScrollBarStyles ;
	__property IgnoreEnabled ;
	__property HeaderImageForm ;
	__property ImageForm ;
	__property Images ;
	__property Images2 ;
	__property InactiveBorderType  = {default=3};
	__property IncrementalSearch ;
	__property InplaceEditorDelay  = {default=500};
	__property ItemIndent  = {default=17};
	__property Items ;
	__property KeepSelectionWithinLevel ;
	__property LeafPicture ;
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property LineHeight ;
	__property LinesColor  = {default=-2147483633};
	__property LinesStyle  = {default=2};
	__property LineHintColor  = {default=-2147483643};
	__property LineHintMode  = {default=1};
	__property LineHintTimeout  = {default=3000};
	__property LineHintType  = {default=2};
	__property LockHeaderHeight  = {default=0};
	__property MainTextType  = {default=1};
	__property MainTreeColumn  = {default=0};
	__property MinusPicture ;
	__property MoveColumnOnDrag  = {default=0};
	__property MoveFocusOnCollapse  = {default=0};
	__property MouseFrameSelect ;
	__property MultiSelect  = {default=1};
	__property MultiSelectLevel  = {default=-1};
	__property OwnerDrawByColumn  = {default=1};
	__property OwnerDrawMask ;
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PathSeparator  = {default=92};
	__property PlusMinusTransparent  = {default=0};
	__property PlusPicture ;
	__property PopupMenu ;
	__property QuickEditMode  = {default=0};
	__property RadioButtonGlyph ;
	__property RightAlignedText  = {default=0};
	__property RightAlignedTree  = {default=0};
	__property RightClickSelect  = {default=1};
	__property RowHotTrack  = {default=0};
	__property RowSelect  = {default=1};
	__property NoBlendSelected  = {default=0};
	__property ScrollBackground  = {default=0};
	__property ScrollbarOpposite ;
	__property ScrollTracking  = {default=0};
	__property SelectColumn  = {default=-1};
	__property ShowButtons  = {default=1};
	__property ShowColumns  = {default=0};
	__property ShowCheckboxes  = {default=0};
	__property ShowEmptyImages  = {default=0};
	__property ShowEmptyImages2  = {default=0};
	__property ShowHint ;
	__property ShowImages  = {default=1};
	__property ShowLeafButton ;
	__property ShowLines  = {default=1};
	__property ShowRoot  = {default=0};
	__property ShowRootButtons  = {default=1};
	__property SelectionMode  = {default=1};
	__property SortDir  = {default=0};
	__property SortMode  = {default=0};
	__property SortSection  = {default=0};
	__property SortType  = {default=1};
	__property Storage ;
	__property StoragePath ;
	__property SortUseCase  = {default=1};
	__property StickyHeaderSections  = {default=0};
	__property StripedOddColor ;
	__property StripedEvenColor ;
	__property StripedItems  = {default=0};
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
	__property Tracking  = {default=1};
	__property TrackColor  = {default=-2147483635};
	__property UnderlineTracked  = {default=1};
	__property UseCustomScrollBars  = {default=1};
	__property VertDivLinesColor  = {default=-2147483633};
	__property VerticalLines  = {default=0};
	__property VerticalLinesLong  = {default=1};
	__property VertScrollBarStyles ;
	__property VirtualityLevel ;
	__property Visible  = {default=1};
	__property UseSystemHintColors  = {default=0};
	__property UseXPThemes  = {default=1};
	__property TextColor  = {default=-2147483640};
	__property BkColor  = {default=-2147483643};
	__property OnScroll ;
	__property OnHeaderColumnClick ;
	__property OnHeaderColumnDraw ;
	__property OnHeaderColumnResize ;
	__property OnHeaderColumnMove ;
	__property OnHeaderLookup ;
	__property OnHeaderLookupDone ;
	__property OnHeaderResize ;
	__property OnHeaderSectionExpand ;
	__property OnHeaderSectionCollapse ;
	__property OnHeaderSectionFilterCall ;
	__property OnHeaderSectionAutoSize ;
	__property OnHeaderSectionMeasure ;
	__property OnHorzScrollDrawPart ;
	__property OnHorzScrollHintNeeded ;
	__property OnAfterSelectionChange ;
	__property OnChanging ;
	__property OnDragTargetChange ;
	__property OnItemChange ;
	__property OnItemPreDraw ;
	__property OnItemDraw ;
	__property OnResize ;
	__property OnTryEdit ;
	__property OnInplaceEditorNeeded ;
	__property OnItemChecked ;
	__property OnItemExpand ;
	__property OnItemCollapse ;
	__property OnItemExpanding ;
	__property OnItemCollapsing ;
	__property OnItemDeletion ;
	__property OnItemFocused ;
	__property OnShowLineHint ;
	__property OnCompareItems ;
	__property OnItemPicDraw ;
	__property OnItemPicDraw2 ;
	__property OnItemPostDraw ;
	__property OnHotTrack ;
	__property OnMeasureItemPart ;
	__property OnSortBegin ;
	__property OnSortEnd ;
	__property OnItemSave ;
	__property OnItemLoad ;
	__property OnItemSelectedChange ;
	__property OnCellStyleSave ;
	__property OnCellStyleLoad ;
	__property OnVertScrollDrawPart ;
	__property OnVertScrollHintNeeded ;
	__property OnHTMLImageNeeded ;
	__property OnVirtualTextNeeded ;
	__property OnVirtualHintNeeded ;
	__property OnVirtualValueNeeded ;
	__property OnVirtualStyleNeeded ;
	__property OnHeaderMouseDown ;
	__property OnClick ;
	__property OnEnter ;
	__property OnExit ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnStartDrag ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnDblClick ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMouseWheel ;
	__property OnMouseWheelDown ;
	__property OnMouseWheelUp ;
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
	__property OnOleTargetDrag ;
	__property OnOleTargetDrop ;
	__property OnOleDragStart ;
	__property OnOleDragFinish ;
public:
	#pragma option push -w-inl
	/* TCustomElTree.Create */ inline __fastcall virtual TElTree(Classes::TComponent* AOwner) : TCustomElTree(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElTree.CreateClass */ inline __fastcall TElTree(Classes::TComponent* AOwner, TMetaClass* ItemClass) : TCustomElTree(AOwner, ItemClass) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElTree.Destroy */ inline __fastcall virtual ~TElTree(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElTree(HWND ParentWindow) : TCustomElTree(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreeDragObject;
class PASCALIMPLEMENTATION TElTreeDragObject : public Controls::TDragControlObject 
{
	typedef Controls::TDragControlObject inherited;
	
public:
	virtual void __fastcall Finished(System::TObject* Target, int X, int Y, bool Accepted);
	virtual Controls::TCursor __fastcall GetDragCursor(bool Accepted, int X, int Y);
	__fastcall virtual ~TElTreeDragObject(void);
public:
	#pragma option push -w-inl
	/* TBaseDragControlObject.Create */ inline __fastcall virtual TElTreeDragObject(Controls::TControl* AControl) : Controls::TDragControlObject(AControl) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint stsFocused = 0x1;
static const Shortint stsSelected = 0x2;
static const Shortint stsCut = 0x3;
static const Shortint stsUnderlined = 0x4;
static const Shortint stsBold = 0x5;
static const Shortint stsItalic = 0x6;
static const Shortint stsExpanded = 0x7;
static const Shortint stsStrikeOut = 0x8;
static const Shortint stiMaxState = 0x8;
static const Shortint tisFocused = 0x1;
static const Shortint tisSelected = 0x2;
static const Shortint tisCut = 0x4;
static const Shortint tisExpanded = 0x8;
static const Shortint tisBold = 0x10;
static const Shortint tisItalic = 0x20;
static const Shortint tisUnderlined = 0x40;
static const Byte tisStrikeout = 0x80;
static const Shortint ibfParentColors = 0x1;
static const Shortint ibfParentStyle = 0x2;
static const Shortint ibfSuppressLines = 0x4;
static const Shortint ibfImageDrawn = 0x8;
static const Shortint ibfImageDrawn2 = 0x10;
static const Shortint ibfForceButtons = 0x20;
static const Shortint ibfStrikedOutLine = 0x40;
static const Byte ibfDrawHLine = 0x80;
static const Word ibfAllowSelection = 0x100;
static const Word ibfAllowEdit = 0x200;
static const Word ibfUseBkColor = 0x400;
static const Word ibfDeleting = 0x800;
static const Word ibfUseStyles = 0x1000;
static const Word ibfMultiline = 0x2000;
static const Word ibfHidden = 0x4000;
static const Word ibfEnabled = 0x8000;
static const int ibfSuppressButtons = 0x10000;
static const int ibfCheckBoxEnabled = 0x20000;
static const int ibfShowCheckBox = 0x40000;
static const int ibfIsHTML = 0x80000;
static const int ibfOwnerHeight = 0x100000;
static const int ibfRec = 0x200000;
static const int ibfHintIsHTML = 0x400000;
static const Word CM_MOUSEWHEEL = 0xb20a;
static const Word WM_UPDATESBFRAME = 0x912;
extern PACKAGE System::ResourceString _STExOutOfBounds;
#define Eltree_STExOutOfBounds System::LoadResourceString(&Eltree::_STExOutOfBounds)
extern PACKAGE System::ResourceString _STexInvItem;
#define Eltree_STexInvItem System::LoadResourceString(&Eltree::_STexInvItem)
extern PACKAGE System::ResourceString _STexRecursiveMove;
#define Eltree_STexRecursiveMove System::LoadResourceString(&Eltree::_STexRecursiveMove)
static const Word TM_CLOSEINPLACEEDITOR = 0xf13;
static const Shortint FDivLineWidth = 0x1;
static const Shortint CheckMargin = 0x2;
static const Shortint CheckBoxSize = 0xf;
static const Word crDragSingleNo = 0x4e21;
static const Word crDragSingleMove = 0x4e22;
static const Word crDragSingleCopy = 0x4e23;
extern PACKAGE int MultiLineFlags[2];
extern PACKAGE int MultiLineEllipseFlags[2];
extern PACKAGE Graphics::TBitmap* LeafBmp;
extern PACKAGE Graphics::TBitmap* PlusBmp;
extern PACKAGE Graphics::TBitmap* MinusBmp;

}	/* namespace Eltree */
using namespace Eltree;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTree
