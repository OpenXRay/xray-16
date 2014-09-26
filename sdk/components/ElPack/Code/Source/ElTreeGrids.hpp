// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeGrids.pas' rev: 6.00

#ifndef ElTreeGridsHPP
#define ElTreeGridsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElDragDrop.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElScrollBar.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTreeBtnEdit.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <ElStrArray.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltreegrids
{
//-- type declarations -------------------------------------------------------
typedef TElWideStringArray TElFStringArray;
;

class DELPHICLASS TCustomElTreeGrid;
class PASCALIMPLEMENTATION TCustomElTreeGrid : public Eltree::TCustomElTree 
{
	typedef Eltree::TCustomElTree inherited;
	
private:
	bool FgoAlwaysShowEditor;
	bool FgoRowSelect;
	bool FgoColMoving;
	bool FgoTabs;
	void __fastcall SetgoRowSelect(bool Value);
	void __fastcall SetgoColMoving(bool Value);
	int __fastcall GetCol(void);
	void __fastcall SetCol(int Value);
	int __fastcall GetRow(void);
	void __fastcall SetRow(int Value);
	int __fastcall GetLeftCol(void);
	void __fastcall SetLeftCol(int Value);
	int __fastcall GetColCount(void);
	void __fastcall SetColCount(int Value);
	int __fastcall GetColWidths(int Index);
	void __fastcall SetColWidths(int Index, int Value);
	int __fastcall GetDefaultColWidth(void);
	void __fastcall SetDefaultColWidth(int Value);
	int __fastcall GetDefaultRowHeight(void);
	void __fastcall SetDefaultRowHeight(int Value);
	bool __fastcall GetEditorMode(void);
	void __fastcall SetEditorMode(bool Value);
	int __fastcall GetRowCount(void);
	void __fastcall SetRowCount(int Value);
	int __fastcall GetTopRow(void);
	void __fastcall SetTopRow(int Value);
	bool __fastcall GetgoEditing(void);
	void __fastcall SetgoEditing(bool Value);
	
protected:
	bool FgoTabSkipNonEditable;
	virtual void __fastcall KeyDownTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TMessage &Message);
	virtual Eltree::TElTreeView* __fastcall CreateView(void);
	Eltree::TElTreeItem* __fastcall GetAsCell(int ACol, int ARow);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Message);
	virtual void __fastcall MouseDownTransfer(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall EnsureColumnVisible(int SectionNumber);
	__property bool goAlwaysShowEditor = {read=FgoAlwaysShowEditor, write=FgoAlwaysShowEditor, nodefault};
	__property bool goRowSelect = {read=FgoRowSelect, write=SetgoRowSelect, nodefault};
	__property bool goColMoving = {read=FgoColMoving, write=SetgoColMoving, nodefault};
	__property bool goTabs = {read=FgoTabs, write=FgoTabs, nodefault};
	__property int ColCount = {read=GetColCount, write=SetColCount, nodefault};
	__property int DefaultColWidth = {read=GetDefaultColWidth, write=SetDefaultColWidth, nodefault};
	__property int DefaultRowHeight = {read=GetDefaultRowHeight, write=SetDefaultRowHeight, nodefault};
	__property bool EditorMode = {read=GetEditorMode, write=SetEditorMode, nodefault};
	__property int RowCount = {read=GetRowCount, write=SetRowCount, nodefault};
	__property bool goEditing = {read=GetgoEditing, write=SetgoEditing, nodefault};
	__property bool goTabSkipNonEditable = {read=FgoTabSkipNonEditable, write=FgoTabSkipNonEditable, nodefault};
	
public:
	Types::TRect __fastcall CellRect(int ACol, int ARow);
	void __fastcall MouseToCell(int X, int Y, int &ACol, int &ARow);
	__fastcall virtual TCustomElTreeGrid(Classes::TComponent* Owner);
	Elheader::TElHeaderSection* __fastcall GetNextEditableSection(Elheader::TElHeaderSection* Section, bool GoForward);
	__property int Col = {read=GetCol, write=SetCol, nodefault};
	__property int ColWidths[int Index] = {read=GetColWidths, write=SetColWidths};
	__property int LeftCol = {read=GetLeftCol, write=SetLeftCol, nodefault};
	__property int Row = {read=GetRow, write=SetRow, nodefault};
	__property int TopRow = {read=GetTopRow, write=SetTopRow, nodefault};
	
__published:
	__property VerticalLines  = {default=1};
	__property HorizontalLines  = {default=1};
public:
	#pragma option push -w-inl
	/* TCustomElTree.CreateClass */ inline __fastcall TCustomElTreeGrid(Classes::TComponent* AOwner, TMetaClass* ItemClass) : Eltree::TCustomElTree(AOwner, ItemClass) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElTree.Destroy */ inline __fastcall virtual ~TCustomElTreeGrid(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElTreeGrid(HWND ParentWindow) : Eltree::TCustomElTree(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreeTrickyInplaceEdit;
class PASCALIMPLEMENTATION TElTreeTrickyInplaceEdit : public Eltreebtnedit::TElTreeInplaceButtonEdit 
{
	typedef Eltreebtnedit::TElTreeInplaceButtonEdit inherited;
	
private:
	AnsiString FDummyStr;
	
__published:
	__property AnsiString Name = {read=FDummyStr};
	__property AnsiString Tree = {read=FDummyStr};
public:
	#pragma option push -w-inl
	/* TElTreeInplaceButtonEdit.Create */ inline __fastcall virtual TElTreeTrickyInplaceEdit(Classes::TComponent* AOwner) : Eltreebtnedit::TElTreeInplaceButtonEdit(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TElTreeInplaceButtonEdit.Destroy */ inline __fastcall virtual ~TElTreeTrickyInplaceEdit(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreeStringGrid;
class PASCALIMPLEMENTATION TElTreeStringGrid : public TCustomElTreeGrid 
{
	typedef TCustomElTreeGrid inherited;
	
private:
	Elunicodestrings::TElWideStringArray* FCols;
	Elunicodestrings::TElWideStringArray* FRows;
	Eltreebtnedit::TElTreeInplaceButtonEdit* FEditor;
	Elunicodestrings::TElWideStrings* __fastcall GetCols(int Index);
	void __fastcall SetCols(int Index, Elunicodestrings::TElWideStrings* Value);
	Elunicodestrings::TElWideStrings* __fastcall GetRows(int Index);
	void __fastcall SetRows(int Index, Elunicodestrings::TElWideStrings* Value);
	WideString __fastcall GetCells(int ACol, int ARow);
	void __fastcall SetCells(int ACol, int ARow, WideString Value);
	System::TObject* __fastcall GetObjects(int ACol, int ARow);
	void __fastcall SetObjects(int ACol, int ARow, System::TObject* Value);
	void __fastcall SetEditor(Eltreebtnedit::TElTreeInplaceButtonEdit* Value);
	
protected:
	bool FUseDefaultEditor;
	virtual void __fastcall TriggerInplaceEditorNeeded(Eltree::TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType SupposedFieldType, Eltree::TElTreeInplaceEditor* &Editor);
	virtual void __fastcall OnFontChange(System::TObject* Sender);
	virtual void __fastcall KeyDownTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall EditorKeyDown(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	
public:
	__fastcall virtual TElTreeStringGrid(Classes::TComponent* Owner);
	__fastcall virtual ~TElTreeStringGrid(void);
	__property Elunicodestrings::TElWideStrings* Cols[int Index] = {read=GetCols, write=SetCols};
	__property Elunicodestrings::TElWideStrings* Rows[int Index] = {read=GetRows, write=SetRows};
	__property WideString Cells[int ACol][int ARow] = {read=GetCells, write=SetCells/*, default*/};
	__property System::TObject* Objects[int ACol][int ARow] = {read=GetObjects, write=SetObjects};
	__property Eltreebtnedit::TElTreeInplaceButtonEdit* Editor = {read=FEditor, write=SetEditor};
	
__published:
	__property bool UseDefaultEditor = {read=FUseDefaultEditor, write=FUseDefaultEditor, default=0};
	__property ColCount  = {default=5};
	__property RowCount  = {default=5};
	__property goAlwaysShowEditor  = {default=0};
	__property goRowSelect  = {default=0};
	__property goColMoving  = {default=1};
	__property goEditing  = {default=1};
	__property goTabs  = {default=0};
	__property goTabSkipNonEditable  = {default=0};
	__property DefaultColWidth  = {default=64};
	__property DefaultRowHeight  = {default=24};
	__property ActiveBorderType  = {default=1};
	__property Align  = {default=0};
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
	__property AdjustMultilineHeight  = {default=1};
	__property Background ;
	__property BackgroundType  = {default=2};
	__property BorderStyle  = {default=1};
	__property ChangeDelay  = {default=500};
	__property ChangeStateImage  = {default=0};
	__property CheckBoxGlyph ;
	__property Ctl3D ;
	__property Color  = {default=-2147483643};
	__property Cursor  = {default=-2};
	__property CustomCheckboxes  = {default=0};
	__property CustomPlusMinus  = {default=0};
	__property VertDivLinesColor  = {default=-2147483633};
	__property HorzDivLinesColor  = {default=-2147483633};
	__property DragCursor ;
	__property DragAllowed  = {default=0};
	__property DragTrgDrawMode  = {default=2};
	__property DragType  = {default=1};
	__property DragExpandDelay  = {default=500};
	__property DragImageMode  = {default=0};
	__property DrawFocusRect  = {default=1};
	__property DragRectAcceptColor  = {default=32768};
	__property DragRectDenyColor  = {default=255};
	__property Enabled  = {default=1};
	__property ExpandOnDragOver  = {default=0};
	__property ExplorerEditMode ;
	__property FilteredVisibility  = {default=0};
	__property Flat  = {default=0};
	__property FlatFocusedScrollbars  = {default=1};
	__property FocusedSelectColor  = {default=-2147483635};
	__property FocusedSelectTextColor  = {default=-2147483634};
	__property ForcedScrollBars  = {default=0};
	__property Font  = {stored=true};
	__property GradientStartColor  = {default=0};
	__property GradientEndColor  = {default=0};
	__property GradientSteps  = {default=16};
	__property HeaderActiveFilterColor  = {default=0};
	__property HeaderColor  = {default=-2147483633};
	__property HeaderHeight  = {default=0};
	__property HeaderHotTrack  = {default=1};
	__property HeaderInvertSortArrows  = {default=0};
	__property HeaderSections ;
	__property HeaderFilterColor  = {default=-2147483630};
	__property HeaderFlat  = {default=0};
	__property HeaderImages ;
	__property HeaderWrapCaptions  = {default=0};
	__property HideFocusRect  = {default=0};
	__property HideHintOnTimer  = {default=0};
	__property HideHintOnMove  = {default=1};
	__property HideSelectColor  = {default=-2147483633};
	__property HideSelectTextColor  = {default=-2147483632};
	__property HideSelection  = {default=0};
	__property HorizontalLines  = {default=1};
	__property HideHorzScrollBar  = {default=0};
	__property HideVertScrollBar  = {default=0};
	__property Hint ;
	__property HorzScrollBarStyles ;
	__property HeaderImageForm ;
	__property ImageForm ;
	__property Images ;
	__property Images2 ;
	__property InactiveBorderType  = {default=3};
	__property InplaceEditorDelay  = {default=500};
	__property ItemIndent  = {default=17};
	__property Items ;
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property LinesColor  = {default=-2147483633};
	__property LinesStyle  = {default=2};
	__property LineHintColor  = {default=-2147483643};
	__property LineHintMode  = {default=1};
	__property LineHintTimeout  = {default=3000};
	__property LockHeaderHeight  = {default=1};
	__property MainTreeColumn  = {default=0};
	__property MinusPicture ;
	__property MoveFocusOnCollapse  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PlusMinusTransparent  = {default=0};
	__property PlusPicture ;
	__property PopupMenu ;
	__property RadioButtonGlyph ;
	__property RightAlignedText  = {default=0};
	__property RightAlignedTree  = {default=0};
	__property RightClickSelect  = {default=1};
	__property ScrollbarOpposite ;
	__property ScrollTracking  = {default=0};
	__property ShowButtons  = {default=0};
	__property ShowCheckboxes  = {default=0};
	__property ShowEmptyImages  = {default=0};
	__property ShowEmptyImages2  = {default=0};
	__property ShowHint ;
	__property ShowImages  = {default=0};
	__property ShowLines  = {default=0};
	__property ShowRoot  = {default=0};
	__property ShowRootButtons  = {default=1};
	__property SortDir  = {default=0};
	__property SortMode  = {default=0};
	__property SortSection  = {default=0};
	__property SortType  = {default=1};
	__property Storage ;
	__property StoragePath ;
	__property StickyHeaderSections  = {default=0};
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
	__property UseCustomScrollBars  = {default=1};
	__property VerticalLines  = {default=1};
	__property VerticalLinesLong  = {default=0};
	__property VertScrollBarStyles ;
	__property Visible  = {default=1};
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
	__property OnResize ;
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
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
	__property OnOleTargetDrag ;
	__property OnOleTargetDrop ;
	__property OnOleDragStart ;
	__property OnOleDragFinish ;
public:
	#pragma option push -w-inl
	/* TCustomElTree.CreateClass */ inline __fastcall TElTreeStringGrid(Classes::TComponent* AOwner, TMetaClass* ItemClass) : TCustomElTreeGrid(AOwner, ItemClass) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElTreeStringGrid(HWND ParentWindow) : TCustomElTreeGrid(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreeGridView;
class PASCALIMPLEMENTATION TElTreeGridView : public Eltree::TElTreeView 
{
	typedef Eltree::TElTreeView inherited;
	
protected:
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	MESSAGE void __fastcall WMLButtonDblClick(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Message);
public:
	#pragma option push -w-inl
	/* TElTreeView.Create */ inline __fastcall virtual TElTreeGridView(Classes::TComponent* Owner) : Eltree::TElTreeView(Owner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TElTreeView.Destroy */ inline __fastcall virtual ~TElTreeGridView(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElTreeGridView(HWND ParentWindow) : Eltree::TElTreeView(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS EElTreeGridError;
class PASCALIMPLEMENTATION EElTreeGridError : public Eltree::EElTreeError 
{
	typedef Eltree::EElTreeError inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElTreeGridError(const AnsiString Msg) : Eltree::EElTreeError(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElTreeGridError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Eltree::EElTreeError(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElTreeGridError(int Ident)/* overload */ : Eltree::EElTreeError(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElTreeGridError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Eltree::EElTreeError(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElTreeGridError(const AnsiString Msg, int AHelpContext) : Eltree::EElTreeError(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElTreeGridError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Eltree::EElTreeError(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElTreeGridError(int Ident, int AHelpContext)/* overload */ : Eltree::EElTreeError(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElTreeGridError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Eltree::EElTreeError(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElTreeGridError(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreegrids */
using namespace Eltreegrids;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeGrids
