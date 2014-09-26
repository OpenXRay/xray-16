// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElShellCtl.pas' rev: 6.00

#ifndef ElShellCtlHPP
#define ElShellCtlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElDragDrop.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElScrollBar.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElShellUtils.hpp>	// Pascal unit
#include <ElTreeStdEditors.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Variants.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ActiveX.hpp>	// Pascal unit
#include <ComObj.hpp>	// Pascal unit
#include <ShlObj.hpp>	// Pascal unit
#include <ShellAPI.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <CommCtrl.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elshellctl
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElShellSortType { sstCustom, sstName, sstExt, sstSize, sstCreationDate, sstModifyDate, sstAccessDate };
#pragma option pop

#pragma option push -b-
enum TElShellSortModifier { ssmFoldersFirst, ssmExecutablesFirst };
#pragma option pop

typedef Set<TElShellSortModifier, ssmFoldersFirst, ssmExecutablesFirst>  TElShellSortModifiers;

#pragma option push -b-
enum TElShellSizeFormat { ssfAsIs, ssfKb, ssfAuto };
#pragma option pop

class DELPHICLASS EElShellError;
class PASCALIMPLEMENTATION EElShellError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElShellError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElShellError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElShellError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElShellError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElShellError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElShellError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElShellError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElShellError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElShellError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElShellTreeItem;
class PASCALIMPLEMENTATION TElShellTreeItem : public Eltree::TElTreeItem 
{
	typedef Eltree::TElTreeItem inherited;
	
private:
	bool FIsValidFile;
	
protected:
	bool FValid;
	AnsiString FAttrAsString;
	AnsiString FComment;
	AnsiString FSizeAsString;
	AnsiString FDisplayName;
	AnsiString FTypeName;
	AnsiString FFileName;
	unsigned FAttr;
	_ITEMIDLIST *FPIDL;
	_WIN32_FIND_DATAA *Win32FindData;
	void __fastcall GetAttributes(_di_IShellFolder iParentFolder);
	AnsiString __fastcall GetDisplayName();
	HIDESBASE AnsiString __fastcall GetFullName();
	bool __fastcall GetHasSubFolders(void);
	bool __fastcall GetIsFolder(void);
	bool __fastcall GetIsRemovable(void);
	_di_IShellFolder __fastcall GetParentFolder();
	Shlobj::PItemIDList __fastcall GetPIDL(void);
	unsigned __fastcall GetSize(void);
	void __fastcall SetDisplayName(const AnsiString Value);
	System::TDateTime __fastcall GetCreationTime(void);
	System::TDateTime __fastcall GetModificationTime(void);
	System::TDateTime __fastcall GetLastAccessTime(void);
	void __fastcall GetWin32Data(_di_IShellFolder ParentFolder);
	void __fastcall CheckWin32FindData(void);
	AnsiString __fastcall GetFileName();
	AnsiString __fastcall GetSizeAsString();
	AnsiString __fastcall GetTypeName();
	bool __fastcall GetIsFileObject(void);
	AnsiString __fastcall GetAttrAsString();
	AnsiString __fastcall GetComment();
	void __fastcall Invalidate(void);
	TElShellTreeItem* __fastcall FindItemByPIDL(Shlobj::PItemIDList APIDL);
	bool __fastcall GetCanRename(void);
	WideString __fastcall GetHintText(_di_IShellFolder ParentFolder);
	int __fastcall GetPicture(_di_IShellFolder ParentFolder);
	
public:
	__fastcall virtual TElShellTreeItem(Eltree::TCustomElTree* AOwner);
	__fastcall virtual ~TElShellTreeItem(void);
	Shlobj::PItemIDList __fastcall BuildFullPIDL(void);
	__property unsigned Attr = {read=FAttr, nodefault};
	__property AnsiString DisplayName = {read=GetDisplayName, write=SetDisplayName};
	__property AnsiString FullName = {read=GetFullName};
	__property bool HasSubFolders = {read=GetHasSubFolders, nodefault};
	__property bool IsFolder = {read=GetIsFolder, nodefault};
	__property bool IsRemovable = {read=GetIsRemovable, nodefault};
	__property _di_IShellFolder ParentFolder = {read=GetParentFolder};
	__property Shlobj::PItemIDList PIDL = {read=GetPIDL};
	__property System::TDateTime ModificationTime = {read=GetModificationTime};
	__property System::TDateTime CreationTime = {read=GetCreationTime};
	__property System::TDateTime LastAccessTime = {read=GetLastAccessTime};
	__property AnsiString FileName = {read=GetFileName};
	__property AnsiString SizeAsString = {read=GetSizeAsString};
	__property bool IsFileObject = {read=GetIsFileObject, nodefault};
	__property AnsiString Comment = {read=GetComment};
	__property bool CanRename = {read=GetCanRename, nodefault};
	
__published:
	__property unsigned Size = {read=GetSize, nodefault};
	__property AnsiString TypeName = {read=GetTypeName};
	__property AnsiString AttrAsString = {read=GetAttrAsString};
};


typedef void __fastcall (__closure *TShellTreeItemAddingEvent)(System::TObject* Sender, AnsiString ItemName, _di_IShellFolder ShellFolder, Shlobj::PItemIDList RelPIDL, bool &Allowed);

typedef void __fastcall (__closure *TShellTreeItemAddedEvent)(System::TObject* Sender, AnsiString ItemName, _di_IShellFolder ShellFolder, Shlobj::PItemIDList RelPIDL, TElShellTreeItem* Item);

class DELPHICLASS TElShellTree;
class PASCALIMPLEMENTATION TElShellTree : public Eltree::TCustomElTree 
{
	typedef Eltree::TCustomElTree inherited;
	
protected:
	Eltreestdeditors::TElTreeInplaceEdit* FEditor;
	Elshellutils::TShellFolders FRootFolder;
	_ITEMIDLIST *FRootPIDL;
	_ITEMIDLIST *FFocusedPIDL;
	_di_IShellFolder FIFolder;
	AnsiString FCustomRootFolder;
	bool FUseSystemMenus;
	bool FClearOnCollapse;
	bool FCheckForChildren;
	bool FShowHidden;
	bool FShowFiles;
	bool FHighlightCompressed;
	Classes::TStringList* FFileFilters;
	TShellTreeItemAddingEvent FOnFilterItem;
	TShellTreeItemAddedEvent FOnItemAdded;
	bool FFileSystemOnly;
	TElShellSortType FSortType;
	TElShellSortModifiers FSortModifiers;
	TElShellSizeFormat FSizeFormat;
	bool FDefaultColumns;
	bool FDefaultEditor;
	int FMaxColumns;
	bool FExpandRoot;
	Shlobj::PItemIDList __fastcall GetFocusedPIDL(void);
	AnsiString __fastcall GetFocusedDisplayName();
	void __fastcall BuildTree(void);
	void __fastcall ReleaseFocusedPIDL(void);
	virtual void __fastcall DoItemFocused(void);
	void __fastcall SetCustomRootFolder(const AnsiString Value);
	virtual Eltree::TElTreeView* __fastcall CreateView(void);
	virtual Eltree::TElTreeItems* __fastcall CreateItems(void);
	bool __fastcall CheckChildren(Eltree::TElTreeItem* Item, _di_IShellFolder AFolder);
	void __fastcall FillItemWithData(TElShellTreeItem* Item, _di_IShellFolder AFolder, int recursive);
	void __fastcall SetShowHidden(bool Value);
	void __fastcall SetShowFiles(bool Value);
	void __fastcall SetHighlightCompressed(bool Value);
	Classes::TStrings* __fastcall GetFileFilters(void);
	void __fastcall SetFileFilters(const Classes::TStrings* Value);
	virtual bool __fastcall NameFiltered(AnsiString S, _di_IShellFolder ShellFolder, Shlobj::PItemIDList RelPIDL);
	virtual void __fastcall CreateHandle(void);
	virtual void __fastcall DoItemCollapse(Eltree::TElTreeItem* Item);
	virtual void __fastcall DoItemExpand(Eltree::TElTreeItem* Item);
	virtual void __fastcall DoItemExpanding(Eltree::TElTreeItem* Item, bool &CanProcess);
	Elshellutils::TShellFolders __fastcall GetRootFolder(void);
	void __fastcall SetRootFolder(Elshellutils::TShellFolders Value);
	virtual void __fastcall DoItemAdded(AnsiString S, _di_IShellFolder ShellFolder, Shlobj::PItemIDList RelPIDL, TElShellTreeItem* Item);
	void __fastcall SetFileSystemOnly(bool Value);
	TElShellTreeItem* __fastcall GetItemFocused(void);
	void __fastcall SetItemFocused(TElShellTreeItem* Value);
	virtual void __fastcall DoCompareItems(Eltree::TElTreeItem* Item1, Eltree::TElTreeItem* Item2, int &res);
	void __fastcall SetSortType(TElShellSortType Value);
	void __fastcall SetSortModifiers(TElShellSortModifiers Value);
	void __fastcall SetSizeFormat(TElShellSizeFormat Value);
	void __fastcall SetDefaultColumns(bool Value);
	void __fastcall AddDefaultColumns(void);
	int __fastcall DeleteDefaultColumns(void);
	virtual void __fastcall TriggerVirtualTextNeeded(Eltree::TElTreeItem* Item, int SectionIndex, WideString &Text);
	virtual void __fastcall TriggerVirtualValueNeeded(Eltree::TElTreeItem* Item, int SectionIndex, int VarType, Variant &Value);
	virtual void __fastcall TriggerSortBegin(void);
	virtual void __fastcall TriggerTryEditEvent(Eltree::TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType &CellType, bool &CanEdit);
	virtual void __fastcall TriggerInplaceEditorNeeded(Eltree::TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType SupposedFieldType, Eltree::TElTreeInplaceEditor* &Editor);
	void __fastcall FileFiltersChange(System::TObject* Sender);
	void __fastcall OnValidateEdit(System::TObject* Sender, bool &InputValid);
	void __fastcall SetExpandRoot(bool Value);
	virtual void __fastcall TriggerVirtualHintNeeded(Eltree::TElTreeItem* Item, WideString &Hint);
	virtual int __fastcall DoGetPicture(Eltree::TElTreeItem* Item);
	
public:
	void __fastcall SetRootPIDL(Shlobj::PItemIDList PIDL);
	__fastcall virtual TElShellTree(Classes::TComponent* AOwner);
	__fastcall virtual ~TElShellTree(void);
	virtual void __fastcall Loaded(void);
	void __fastcall RefreshTree(Eltree::TElTreeItem* Item, int recursive);
	void __fastcall SetSelectionPIDL(Shlobj::PItemIDList PIDL);
	Shlobj::PItemIDList __fastcall BuildRootPIDL(void);
	__property Shlobj::PItemIDList FocusedPIDL = {read=GetFocusedPIDL};
	__property AnsiString FocusedDisplayName = {read=GetFocusedDisplayName};
	__property TElShellTreeItem* ItemFocused = {read=GetItemFocused, write=SetItemFocused};
	__property Images ;
	__property Images2 ;
	
__published:
	__property Elshellutils::TShellFolders RootFolder = {read=GetRootFolder, write=SetRootFolder, nodefault};
	__property AnsiString CustomRootFolder = {read=FCustomRootFolder, write=SetCustomRootFolder};
	__property bool UseSystemMenus = {read=FUseSystemMenus, write=FUseSystemMenus, nodefault};
	__property bool ClearOnCollapse = {read=FClearOnCollapse, write=FClearOnCollapse, nodefault};
	__property bool CheckForChildren = {read=FCheckForChildren, write=FCheckForChildren, nodefault};
	__property bool ShowHidden = {read=FShowHidden, write=SetShowHidden, default=1};
	__property bool ShowFiles = {read=FShowFiles, write=SetShowFiles, default=0};
	__property bool HighlightCompressed = {read=FHighlightCompressed, write=SetHighlightCompressed, nodefault};
	__property Classes::TStrings* FileFilters = {read=GetFileFilters, write=SetFileFilters};
	__property TShellTreeItemAddingEvent OnItemAdding = {read=FOnFilterItem, write=FOnFilterItem};
	__property TShellTreeItemAddedEvent OnItemAdded = {read=FOnItemAdded, write=FOnItemAdded};
	__property bool FileSystemOnly = {read=FFileSystemOnly, write=SetFileSystemOnly, nodefault};
	__property TElShellSortType SortType = {read=FSortType, write=SetSortType, default=1};
	__property TElShellSortModifiers SortModifiers = {read=FSortModifiers, write=SetSortModifiers, nodefault};
	__property TElShellSizeFormat SizeFormat = {read=FSizeFormat, write=SetSizeFormat, nodefault};
	__property bool DefaultColumns = {read=FDefaultColumns, write=SetDefaultColumns, nodefault};
	__property bool DefaultEditor = {read=FDefaultEditor, write=FDefaultEditor, default=1};
	__property bool ExpandRoot = {read=FExpandRoot, write=SetExpandRoot, default=0};
	__property ActiveBorderType  = {default=1};
	__property Align  = {default=0};
	__property AlwaysKeepFocus  = {default=0};
	__property AlwaysKeepSelection  = {default=1};
	__property AutoExpand  = {default=0};
	__property AutoLineHeight  = {default=1};
	__property AutoLookup  = {default=0};
	__property AutoResizeColumns  = {default=1};
	__property DefaultSectionWidth ;
	__property AdjustMultilineHeight  = {default=1};
	__property Background ;
	__property BackgroundType  = {default=2};
	__property BarStyle  = {default=0};
	__property BarStyleVerticalLines  = {default=0};
	__property BorderSides ;
	__property ChangeDelay  = {default=500};
	__property ChangeStateImage  = {default=0};
	__property CheckBoxGlyph ;
	__property CheckBoxSize  = {default=15};
	__property CustomCheckboxes  = {default=0};
	__property CustomPlusMinus  = {default=0};
	__property DeselectChildrenOnCollapse  = {default=0};
	__property DblClickMode  = {default=1};
	__property DoInplaceEdit  = {default=1};
	__property DragAllowed  = {default=0};
	__property DragCursor ;
	__property DragExpandDelay  = {default=500};
	__property DraggableSections  = {default=0};
	__property DrawFocusRect  = {default=1};
	__property DragImageMode  = {default=0};
	__property DragRectAcceptColor  = {default=32768};
	__property DragRectDenyColor  = {default=255};
	__property DragScrollInterval  = {default=100};
	__property DragTrgDrawMode  = {default=2};
	__property DragType  = {default=1};
	__property ExpandOnDblClick  = {default=1};
	__property ExpandOnDragOver  = {default=0};
	__property ExplorerEditMode  = {default=1};
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
	__property HintType  = {default=2};
	__property HorzDivLinesColor  = {default=-2147483633};
	__property HorzScrollBarStyles ;
	__property HeaderImageForm ;
	__property ImageForm ;
	__property InactiveBorderType  = {default=3};
	__property IncrementalSearch ;
	__property InplaceEditorDelay  = {default=500};
	__property ItemIndent  = {default=17};
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
	__property MainTreeColumn  = {default=0};
	__property MinusPicture ;
	__property MoveColumnOnDrag  = {default=0};
	__property MoveFocusOnCollapse  = {default=0};
	__property MouseFrameSelect ;
	__property MultiSelect  = {default=1};
	__property MultiSelectLevel  = {default=-1};
	__property OwnerDrawByColumn  = {default=1};
	__property OwnerDrawMask ;
	__property PathSeparator  = {default=92};
	__property PlusMinusTransparent  = {default=0};
	__property PlusPicture ;
	__property QuickEditMode  = {default=0};
	__property RadioButtonGlyph ;
	__property RightAlignedText  = {default=0};
	__property RightAlignedTree  = {default=0};
	__property RightClickSelect  = {default=1};
	__property RowHotTrack  = {default=0};
	__property RowSelect  = {default=1};
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
	__property SortUseCase  = {default=1};
	__property Storage ;
	__property StoragePath ;
	__property StickyHeaderSections  = {default=0};
	__property StripedOddColor ;
	__property StripedEvenColor ;
	__property StripedItems  = {default=0};
	__property Tracking  = {default=1};
	__property TrackColor  = {default=-2147483635};
	__property UnderlineTracked  = {default=1};
	__property UseCustomScrollBars  = {default=1};
	__property VertDivLinesColor  = {default=-2147483633};
	__property VerticalLines  = {default=0};
	__property VerticalLinesLong  = {default=1};
	__property VertScrollBarStyles ;
	__property UseSystemHintColors  = {default=0};
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
	__property OnOleTargetDrag ;
	__property OnOleTargetDrop ;
	__property OnOleDragStart ;
	__property OnOleDragFinish ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DoubleBuffered  = {default=1};
	__property DragKind  = {default=0};
	__property BorderStyle  = {default=1};
	__property Ctl3D ;
	__property Cursor  = {default=-2};
	__property Enabled  = {default=1};
	__property Hint ;
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property Visible  = {default=1};
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
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
	__property OnResize ;
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElTree.CreateClass */ inline __fastcall TElShellTree(Classes::TComponent* AOwner, TMetaClass* ItemClass) : Eltree::TCustomElTree(AOwner, ItemClass) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElShellTree(HWND ParentWindow) : Eltree::TCustomElTree(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElShellComboBox;
class PASCALIMPLEMENTATION TElShellComboBox : public Elactrls::TElAdvancedComboBox 
{
	typedef Elactrls::TElAdvancedComboBox inherited;
	
protected:
	bool FNoRebuild;
	_ITEMIDLIST *FSelectionPIDL;
	bool FExplorerStyle;
	bool FShowHidden;
	bool FFileSystemOnly;
	int FDummyInt;
	Stdctrls::TCustomEdit* FEditor;
	Controls::TCursor FCursor;
	Stdctrls::TComboBoxStyle FStyle;
	void __fastcall SetExplorerStyle(bool Value);
	void __fastcall FillCombo(_di_IShellFolder BaseFolder, Shlobj::PItemIDList BasePIDL, int Level);
	virtual void __fastcall CreateWnd(void);
	void __fastcall SetShowHidden(bool Value);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual int __fastcall GetItemWidth(int Index);
	void __fastcall SetFileSystemOnly(bool Value);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Msg);
	HIDESBASE MESSAGE void __fastcall CNDrawItem(Messages::TWMDrawItem &Msg);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	Shlobj::PItemIDList __fastcall GetSelection(void);
	void __fastcall ShowEdit(void);
	DYNAMIC void __fastcall KeyPress(char &Key);
	DYNAMIC void __fastcall DropDown(void);
	HIDESBASE MESSAGE void __fastcall WMDeleteItem(Messages::TMessage &Message);
	void __fastcall AcceptEdit(void);
	void __fastcall CancelEdit(void);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE void __fastcall SetCursor(Controls::TCursor Value);
	void __fastcall FillItems(void);
	HIDESBASE void __fastcall SetStyle(Stdctrls::TComboBoxStyle Value);
	virtual void __fastcall DestroyWnd(void);
	
public:
	__fastcall virtual ~TElShellComboBox(void);
	__fastcall virtual TElShellComboBox(Classes::TComponent* AOwner);
	void __fastcall SetSelection(Shlobj::PItemIDList PIDL);
	__property Shlobj::PItemIDList Selection = {read=GetSelection, write=SetSelection};
	
__published:
	__property int Items = {read=FDummyInt, nodefault};
	__property int ItemHeight = {read=FDummyInt, nodefault};
	__property bool ExplorerStyle = {read=FExplorerStyle, write=SetExplorerStyle, default=1};
	__property bool ShowHidden = {read=FShowHidden, write=SetShowHidden, default=1};
	__property bool FileSystemOnly = {read=FFileSystemOnly, write=SetFileSystemOnly, nodefault};
	__property Controls::TCursor Cursor = {read=FCursor, write=SetCursor, nodefault};
	__property Stdctrls::TComboBoxStyle Style = {read=FStyle, write=SetStyle, stored=false, default=4};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElShellComboBox(HWND ParentWindow) : Elactrls::TElAdvancedComboBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElShellListItem;
class PASCALIMPLEMENTATION TElShellListItem : public Eltree::TElTreeItem 
{
	typedef Eltree::TElTreeItem inherited;
	
private:
	bool FIsValidFile;
	bool FValid;
	
protected:
	unsigned FAttr;
	AnsiString FAttrAsString;
	AnsiString FComment;
	AnsiString FDisplayName;
	AnsiString FFileName;
	_ITEMIDLIST *FPIDL;
	AnsiString FSizeAsString;
	AnsiString FTypeName;
	_WIN32_FIND_DATAA *Win32FindData;
	AnsiString __fastcall GetAttrAsString();
	bool __fastcall GetCanRename(void);
	AnsiString __fastcall GetComment();
	System::TDateTime __fastcall GetCreationTime(void);
	AnsiString __fastcall GetDisplayName();
	AnsiString __fastcall GetFileName();
	HIDESBASE AnsiString __fastcall GetFullName();
	bool __fastcall GetIsFileObject(void);
	bool __fastcall GetIsFolder(void);
	bool __fastcall GetIsRemovable(void);
	System::TDateTime __fastcall GetLastAccessTime(void);
	System::TDateTime __fastcall GetModificationTime(void);
	Shlobj::PItemIDList __fastcall GetPIDL(void);
	unsigned __fastcall GetSize(void);
	AnsiString __fastcall GetSizeAsString();
	AnsiString __fastcall GetTypeName();
	void __fastcall SetDisplayName(const AnsiString Value);
	void __fastcall CheckWin32FindData(void);
	void __fastcall GetWin32Data(_di_IShellFolder ParentFolder);
	void __fastcall Invalidate(void);
	void __fastcall GetAttributes(_di_IShellFolder iParentFolder);
	WideString __fastcall GetHintText(_di_IShellFolder ParentFolder);
	int __fastcall GetPicture(_di_IShellFolder ParentFolder);
	
public:
	__fastcall virtual TElShellListItem(Eltree::TCustomElTree* AOwner);
	__fastcall virtual ~TElShellListItem(void);
	Shlobj::PItemIDList __fastcall BuildFullPIDL(void);
	__property unsigned Attr = {read=FAttr, nodefault};
	__property bool CanRename = {read=GetCanRename, nodefault};
	__property AnsiString Comment = {read=GetComment};
	__property System::TDateTime CreationTime = {read=GetCreationTime};
	__property AnsiString DisplayName = {read=GetDisplayName, write=SetDisplayName};
	__property AnsiString FileName = {read=GetFileName};
	__property AnsiString FullName = {read=GetFullName};
	__property bool IsFileObject = {read=GetIsFileObject, nodefault};
	__property bool IsFolder = {read=GetIsFolder, nodefault};
	__property bool IsRemovable = {read=GetIsRemovable, nodefault};
	__property System::TDateTime LastAccessTime = {read=GetLastAccessTime};
	__property System::TDateTime ModificationTime = {read=GetModificationTime};
	__property Shlobj::PItemIDList PIDL = {read=GetPIDL};
	__property AnsiString SizeAsString = {read=GetSizeAsString};
	
__published:
	__property AnsiString AttrAsString = {read=GetAttrAsString};
	__property unsigned Size = {read=GetSize, nodefault};
	__property AnsiString TypeName = {read=GetTypeName};
};


typedef void __fastcall (__closure *TShellListItemAddingEvent)(System::TObject* Sender, AnsiString ItemName, _di_IShellFolder ShellFolder, Shlobj::PItemIDList RelPIDL, bool &Allowed);

typedef void __fastcall (__closure *TShellListItemAddedEvent)(System::TObject* Sender, AnsiString ItemName, _di_IShellFolder ShellFolder, Shlobj::PItemIDList RelPIDL, TElShellListItem* Item);

class DELPHICLASS TElShellList;
class PASCALIMPLEMENTATION TElShellList : public Eltree::TCustomElTree 
{
	typedef Eltree::TCustomElTree inherited;
	
private:
	unsigned FAttr;
	
protected:
	Eltreestdeditors::TElTreeInplaceEdit* FEditor;
	AnsiString FCustomFolder;
	bool FDefaultColumns;
	_ITEMIDLIST *FFocusedPIDL;
	bool FHighlightCompressed;
	Elshellutils::TShellFolders FFolder;
	bool FShowHidden;
	TElShellSizeFormat FSizeFormat;
	TElShellSortModifiers FSortModifiers;
	TElShellSortType FSortType;
	bool FUseSystemMenus;
	_di_IShellFolder FIFolder;
	int FMaxColumns;
	bool FFileSystemOnly;
	Classes::TStringList* FFileFilters;
	TShellListItemAddingEvent FOnFilterItem;
	TShellListItemAddedEvent FOnItemAdded;
	bool FDefaultEditor;
	_ITEMIDLIST *FRootPIDL;
	void __fastcall SetCustomFolder(const AnsiString Value);
	void __fastcall SetDefaultColumns(bool Value);
	AnsiString __fastcall GetFocusedDisplayName();
	Shlobj::PItemIDList __fastcall GetFocusedPIDL(void);
	TElShellListItem* __fastcall GetItemFocused(void);
	void __fastcall SetHighlightCompressed(bool Value);
	void __fastcall SetItemFocused(TElShellListItem* Value);
	void __fastcall SetFolder(Elshellutils::TShellFolders Value);
	void __fastcall SetShowHidden(bool Value);
	void __fastcall SetSizeFormat(TElShellSizeFormat Value);
	void __fastcall SetSortModifiers(TElShellSortModifiers Value);
	void __fastcall SetSortType(TElShellSortType Value);
	void __fastcall SetPIDL(Shlobj::PItemIDList PIDL);
	void __fastcall AddDefaultColumns(void);
	int __fastcall DeleteDefaultColumns(void);
	virtual void __fastcall TriggerSortBegin(void);
	virtual void __fastcall TriggerTryEditEvent(Eltree::TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType &CellType, bool &CanEdit);
	virtual void __fastcall TriggerVirtualTextNeeded(Eltree::TElTreeItem* Item, int SectionIndex, WideString &Text);
	virtual void __fastcall TriggerVirtualValueNeeded(Eltree::TElTreeItem* Item, int SectionIndex, int VarType, Variant &Value);
	void __fastcall SetFileSystemOnly(bool Value);
	TElShellListItem* __fastcall FindItemByPIDL(Shlobj::PItemIDList APIDL);
	virtual bool __fastcall NameFiltered(AnsiString S, _di_IShellFolder ShellFolder, Shlobj::PItemIDList RelPIDL);
	Classes::TStrings* __fastcall GetFileFilters(void);
	void __fastcall SetFileFilters(const Classes::TStrings* Value);
	void __fastcall FileFiltersChange(System::TObject* Sender);
	virtual void __fastcall CreateHandle(void);
	virtual Eltree::TElTreeItems* __fastcall CreateItems(void);
	virtual Eltree::TElTreeView* __fastcall CreateView(void);
	virtual void __fastcall DoCompareItems(Eltree::TElTreeItem* Item1, Eltree::TElTreeItem* Item2, int &res);
	virtual void __fastcall TriggerVirtualHintNeeded(Eltree::TElTreeItem* Item, WideString &Hint);
	virtual int __fastcall DoGetPicture(Eltree::TElTreeItem* Item);
	
public:
	void __fastcall RefreshList(void);
	__fastcall virtual TElShellList(Classes::TComponent* AOwner);
	__fastcall virtual ~TElShellList(void);
	void __fastcall SetRootPIDL(Shlobj::PItemIDList PIDL);
	void __fastcall SetSelectionPIDL(Shlobj::PItemIDList PIDL);
	Shlobj::PItemIDList __fastcall BuildRootPIDL(void);
	__property AnsiString FocusedDisplayName = {read=GetFocusedDisplayName};
	__property Shlobj::PItemIDList FocusedPIDL = {read=GetFocusedPIDL};
	__property TElShellListItem* ItemFocused = {read=GetItemFocused, write=SetItemFocused};
	
__published:
	__property AnsiString CustomFolder = {read=FCustomFolder, write=SetCustomFolder};
	__property bool DefaultColumns = {read=FDefaultColumns, write=SetDefaultColumns, nodefault};
	__property bool HighlightCompressed = {read=FHighlightCompressed, write=SetHighlightCompressed, nodefault};
	__property Elshellutils::TShellFolders Folder = {read=FFolder, write=SetFolder, nodefault};
	__property bool ShowHidden = {read=FShowHidden, write=SetShowHidden, default=1};
	__property TElShellSizeFormat SizeFormat = {read=FSizeFormat, write=SetSizeFormat, nodefault};
	__property TElShellSortModifiers SortModifiers = {read=FSortModifiers, write=SetSortModifiers, nodefault};
	__property TElShellSortType SortType = {read=FSortType, write=SetSortType, default=1};
	__property bool UseSystemMenus = {read=FUseSystemMenus, write=FUseSystemMenus, nodefault};
	__property bool FileSystemOnly = {read=FFileSystemOnly, write=SetFileSystemOnly, nodefault};
	__property Classes::TStrings* FileFilters = {read=GetFileFilters, write=SetFileFilters};
	__property TShellListItemAddedEvent OnItemAdded = {read=FOnItemAdded, write=FOnItemAdded};
	__property TShellListItemAddingEvent OnItemAdding = {read=FOnFilterItem, write=FOnFilterItem};
	__property bool DefaultEditor = {read=FDefaultEditor, write=FDefaultEditor, default=1};
	__property ActiveBorderType  = {default=1};
	__property Align  = {default=0};
	__property AlwaysKeepFocus  = {default=0};
	__property AlwaysKeepSelection  = {default=1};
	__property AutoExpand  = {default=0};
	__property AutoLineHeight  = {default=1};
	__property AutoLookup  = {default=0};
	__property AutoResizeColumns  = {default=1};
	__property DefaultSectionWidth ;
	__property AdjustMultilineHeight  = {default=1};
	__property Background ;
	__property BackgroundType  = {default=2};
	__property BarStyle  = {default=0};
	__property BarStyleVerticalLines  = {default=0};
	__property BorderSides ;
	__property ChangeDelay  = {default=500};
	__property ChangeStateImage  = {default=0};
	__property CheckBoxGlyph ;
	__property CheckBoxSize  = {default=15};
	__property CustomCheckboxes  = {default=0};
	__property CustomPlusMinus  = {default=0};
	__property DeselectChildrenOnCollapse  = {default=0};
	__property DblClickMode  = {default=1};
	__property DoInplaceEdit  = {default=1};
	__property DragAllowed  = {default=0};
	__property DragCursor ;
	__property DragExpandDelay  = {default=500};
	__property DraggableSections  = {default=0};
	__property DrawFocusRect  = {default=1};
	__property DragImageMode  = {default=0};
	__property DragRectAcceptColor  = {default=32768};
	__property DragRectDenyColor  = {default=255};
	__property DragScrollInterval  = {default=100};
	__property DragTrgDrawMode  = {default=2};
	__property DragType  = {default=1};
	__property ExpandOnDblClick  = {default=1};
	__property ExpandOnDragOver  = {default=0};
	__property ExplorerEditMode  = {default=1};
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
	__property HintType  = {default=2};
	__property HorzDivLinesColor  = {default=-2147483633};
	__property HorzScrollBarStyles ;
	__property HeaderImageForm ;
	__property ImageForm ;
	__property InactiveBorderType  = {default=3};
	__property IncrementalSearch ;
	__property InplaceEditorDelay  = {default=500};
	__property ItemIndent  = {default=17};
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
	__property MainTreeColumn  = {default=0};
	__property MinusPicture ;
	__property MoveColumnOnDrag  = {default=0};
	__property MoveFocusOnCollapse  = {default=0};
	__property MouseFrameSelect ;
	__property MultiSelect  = {default=1};
	__property MultiSelectLevel  = {default=-1};
	__property OwnerDrawByColumn  = {default=1};
	__property OwnerDrawMask ;
	__property PathSeparator  = {default=92};
	__property PlusMinusTransparent  = {default=0};
	__property PlusPicture ;
	__property QuickEditMode  = {default=0};
	__property RadioButtonGlyph ;
	__property RightAlignedText  = {default=0};
	__property RightAlignedTree  = {default=0};
	__property RightClickSelect  = {default=1};
	__property RowHotTrack  = {default=0};
	__property RowSelect  = {default=1};
	__property ScrollbarOpposite ;
	__property ScrollTracking  = {default=0};
	__property SelectColumn  = {default=-1};
	__property ShowButtons  = {default=0};
	__property ShowColumns  = {default=0};
	__property ShowCheckboxes  = {default=0};
	__property ShowEmptyImages  = {default=1};
	__property ShowEmptyImages2  = {default=0};
	__property ShowHint ;
	__property ShowImages  = {default=1};
	__property ShowLeafButton ;
	__property ShowLines  = {default=0};
	__property ShowRoot  = {default=0};
	__property ShowRootButtons  = {default=0};
	__property SelectionMode  = {default=1};
	__property SortDir  = {default=0};
	__property SortMode  = {default=0};
	__property Storage ;
	__property StoragePath ;
	__property StickyHeaderSections  = {default=0};
	__property StripedOddColor ;
	__property StripedEvenColor ;
	__property StripedItems  = {default=0};
	__property Tracking  = {default=1};
	__property TrackColor  = {default=-2147483635};
	__property UnderlineTracked  = {default=1};
	__property UseCustomScrollBars  = {default=1};
	__property VertDivLinesColor  = {default=-2147483633};
	__property VerticalLines  = {default=0};
	__property VerticalLinesLong  = {default=1};
	__property VertScrollBarStyles ;
	__property UseSystemHintColors  = {default=0};
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
	__property OnOleTargetDrag ;
	__property OnOleTargetDrop ;
	__property OnOleDragStart ;
	__property OnOleDragFinish ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DoubleBuffered  = {default=1};
	__property DragKind  = {default=0};
	__property BorderStyle  = {default=1};
	__property Ctl3D ;
	__property Cursor  = {default=-2};
	__property Enabled  = {default=1};
	__property Hint ;
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property Visible  = {default=1};
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
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
	__property OnResize ;
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElTree.CreateClass */ inline __fastcall TElShellList(Classes::TComponent* AOwner, TMetaClass* ItemClass) : Eltree::TCustomElTree(AOwner, ItemClass) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElShellList(HWND ParentWindow) : Eltree::TCustomElTree(ParentWindow) { }
	#pragma option pop
	
};


typedef AnsiString ElShellCtl__7[8];

//-- var, const, procedure ---------------------------------------------------
static const Word siBase = 0x5b2;
static const Shortint siMin = 0x0;
static const Shortint siName = 0x0;
static const Shortint siSize = 0x1;
static const Shortint siType = 0x2;
static const Shortint siModified = 0x3;
static const Shortint siAttr = 0x4;
static const Shortint siComment = 0x5;
static const Shortint siCreated = 0x6;
static const Shortint siAccessed = 0x7;
static const Shortint siMax = 0x7;
extern PACKAGE AnsiString DefaultColumnNames[8];
extern PACKAGE int DefaultColumnAlignments[8];
extern PACKAGE TElShellSortType ColumnSortTypes[8];

}	/* namespace Elshellctl */
using namespace Elshellctl;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElShellCtl
