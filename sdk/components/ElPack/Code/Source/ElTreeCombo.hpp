// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeCombo.pas' rev: 6.00

#ifndef ElTreeComboHPP
#define ElTreeComboHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ElScrollBar.hpp>	// Pascal unit
#include <ElBtnEdit.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElImgLst.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ActiveX.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElDragDrop.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltreecombo
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElComboDropEvent)(System::TObject* Sender, bool Dropped);

class DELPHICLASS TDropdownElTree;
class DELPHICLASS TElTreeCombo;
class PASCALIMPLEMENTATION TElTreeCombo : public Elbtnedit::TCustomElButtonEdit 
{
	typedef Elbtnedit::TCustomElButtonEdit inherited;
	
private:
	Eltree::TEditRequestEvent FOnEditRequest;
	Controls::TKeyEvent FOnTreeKeyDown;
	Controls::TKeyPressEvent FOnTreeKeyPress;
	Controls::TKeyEvent FOnTreeKeyUp;
	Controls::TMouseEvent FOnTreeMouseDown;
	Controls::TMouseMoveEvent FOnTreeMouseMove;
	Controls::TMouseEvent FOnTreeMouseUp;
	bool FAutoProcessSelection;
	Classes::TNotifyEvent FOnNewTextSelection;
	int FDropWidth;
	int FDropHeight;
	Eltree::TValidateComboEvent FOnValidateCombo_FTree;
	Eltree::TCellStyleSaveEvent FOnCellStyleLoad_FTree;
	Eltree::TCellStyleSaveEvent FOnCellStyleSave_FTree;
	Eltree::TComboEditShowEvent FOnComboEditShow_FTree;
	Elheader::TElHeaderLookupEvent FOnHeaderLookup_FTree;
	Elheader::TElHeaderLookupDoneEvent FOnHeaderLookupDone_FTree;
	Classes::TNotifyEvent FOnHeaderResize_FTree;
	Eltree::TColumnNotifyEvent FOnHeaderSectionAutoSize_FTree;
	Eltree::THeaderSectionEvent FOnHeaderSectionCollapse_FTree;
	Eltree::THeaderSectionEvent FOnHeaderSectionExpand_FTree;
	Eltree::TColumnNotifyEvent FOnHeaderSectionFilterCall_FTree;
	Elscrollbar::TElScrollDrawPartEvent FOnHorzScrollDrawPart_FTree;
	Elscrollbar::TElScrollHintNeededEvent FOnHorzScrollHintNeeded_FTree;
	Eltree::THotTrackEvent FOnHotTrack_FTree;
	Eltree::TItemSaveEvent FOnItemLoad_FTree;
	Eltree::TOnPicDrawEvent FOnItemPicDraw2_FTree;
	Eltree::TItemSaveEvent FOnItemSave_FTree;
	Eltree::TItemSelChangeEvent FOnItemSelectedChange_FTree;
	Eltree::TOleDragFinishEvent FOnOleDragFinish_FTree;
	Eltree::TOleDragStartEvent FOnOleDragStart_FTree;
	Eldragdrop::TTargetDragEvent FOnOleTargetDrag_FTree;
	Eldragdrop::TTargetDropEvent FOnOleTargetDrop_FTree;
	Classes::TNotifyEvent FOnResize_FTree;
	Eltree::TElScrollEvent FOnScroll_FTree;
	Eltree::TTryEditEvent FOnTryEdit_FTree;
	Eltree::TOnValidateEvent FOnValidateInplaceEdit_FTree;
	Elscrollbar::TElScrollDrawPartEvent FOnVertScrollDrawPart_FTree;
	Elscrollbar::TElScrollHintNeededEvent FOnVertScrollHintNeeded_FTree;
	Controls::TCursor FSaveCursor;
	Controls::TCursor FSaveCursor1;
	bool FReadOnly;
	bool FDropped;
	TElComboDropEvent FOnDrop;
	TDropdownElTree* FTree;
	Eltree::TElTreeItem* FSelection;
	Ellist::TElList* FSelectionList;
	bool IgnoreFocus;
	bool IgnoreDrop;
	bool FMouseDown;
	HWND FSaveCapture;
	Eltree::TOnCompareItems FOnCompareItems;
	Eltree::TColumnNotifyEvent FOnHeaderColumnClick;
	Elheader::TElSectionRedrawEvent FOnHeaderColumnDraw;
	Eltree::TElColumnMoveEvent FOnHeaderColumnMove;
	Eltree::TColumnNotifyEvent FOnHeaderColumnResize;
	Eltree::TOnItemChangeEvent FOnItemChange;
	Eltree::TOnItemExpandEvent FOnItemCollapse;
	Eltree::TOnItemExpanding FOnItemCollapsing;
	Eltree::TOnItemExpandEvent FOnItemDeletion;
	Eltree::TOnItemDrawEvent FOnItemDraw;
	Eltree::TOnItemExpandEvent FOnItemExpand;
	Eltree::TOnItemExpanding FOnItemExpanding;
	Classes::TNotifyEvent FOnItemFocused;
	Eltree::TOnPicDrawEvent FOnItemPicDraw;
	Eltree::TOnShowHintEvent FOnShowLineHint;
	Controls::TStartDragEvent FOnStartDrag_FTree;
	void __fastcall ProcessSelect(void);
	Eltree::TElTreeItem* __fastcall GetSelection(void);
	void __fastcall SetSelection(Eltree::TElTreeItem* newValue);
	Ellist::TElList* __fastcall GetSelectionList(void);
	void __fastcall SetAutoLineHeight_FTree(bool newValue);
	bool __fastcall GetAutoLineHeight_FTree(void);
	void __fastcall SetBkColor_FTree(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetBkColor_FTree(void);
	void __fastcall SetChangeStateImage_FTree(bool newValue);
	bool __fastcall GetChangeStateImage_FTree(void);
	void __fastcall SetCtl3D_FTree(bool newValue);
	bool __fastcall GetCtl3D_FTree(void);
	HIDESBASE void __fastcall SetCursor(Controls::TCursor newValue);
	Controls::TCursor __fastcall GetCursor(void);
	void __fastcall SetDraggableSections(bool newValue);
	bool __fastcall GetDraggableSections(void);
	void __fastcall SetFont_FTree(Graphics::TFont* newValue);
	Graphics::TFont* __fastcall GetFont_FTree(void);
	void __fastcall SetHeaderHotTrack_FTree(bool newValue);
	bool __fastcall GetHeaderHotTrack_FTree(void);
	void __fastcall SetHeaderImages_FTree(Controls::TImageList* newValue);
	Controls::TImageList* __fastcall GetHeaderImages_FTree(void);
	void __fastcall SetHeaderSections_FTree(Elheader::TElHeaderSections* newValue);
	Elheader::TElHeaderSections* __fastcall GetHeaderSections_FTree(void);
	void __fastcall SetHideHintOnMove_FTree(bool newValue);
	bool __fastcall GetHideHintOnMove_FTree(void);
	void __fastcall SetHideHintOnTimer_FTree(bool newValue);
	bool __fastcall GetHideHintOnTimer_FTree(void);
	void __fastcall SetHideHorzScrollBar_FTree(bool newValue);
	bool __fastcall GetHideHorzScrollBar_FTree(void);
	void __fastcall SetHideVertScrollBar_FTree(bool newValue);
	bool __fastcall GetHideVertScrollBar_FTree(void);
	void __fastcall SetHorizontalLines_FTree(bool newValue);
	bool __fastcall GetHorizontalLines_FTree(void);
	void __fastcall SetImages_FTree(Controls::TImageList* newValue);
	Controls::TImageList* __fastcall GetImages_FTree(void);
	void __fastcall SetItems_FTree(Eltree::TElTreeItems* newValue);
	Eltree::TElTreeItems* __fastcall GetItems_FTree(void);
	void __fastcall SetLineHintMode(Eltree::THintModes newValue);
	Eltree::THintModes __fastcall GetLineHintMode(void);
	void __fastcall SetMainTreeColumn(int newValue);
	int __fastcall GetMainTreeColumn(void);
	void __fastcall SetMultiSelect(bool newValue);
	bool __fastcall GetMultiSelect(void);
	void __fastcall SetOwnerDrawByColumn(bool newValue);
	bool __fastcall GetOwnerDrawByColumn(void);
	void __fastcall SetOwnerDrawMask(AnsiString newValue);
	AnsiString __fastcall GetOwnerDrawMask();
	HIDESBASE void __fastcall SetParentCtl3D(bool newValue);
	bool __fastcall GetParentCtl3D(void);
	HIDESBASE void __fastcall SetParentFont(bool newValue);
	bool __fastcall GetParentFont(void);
	HIDESBASE void __fastcall SetParentShowHint(bool newValue);
	bool __fastcall GetParentShowHint(void);
	void __fastcall SetRowSelect(bool newValue);
	bool __fastcall GetRowSelect(void);
	void __fastcall SetScrollTracking(bool newValue);
	bool __fastcall GetScrollTracking(void);
	void __fastcall SetSelectionMode(Eltree::TSTSelModes newValue);
	Eltree::TSTSelModes __fastcall GetSelectionMode(void);
	void __fastcall SetShowButtons(bool newValue);
	bool __fastcall GetShowButtons(void);
	void __fastcall SetShowColumns(bool newValue);
	bool __fastcall GetShowColumns(void);
	HIDESBASE void __fastcall SetShowHint(bool newValue);
	bool __fastcall GetShowHint(void);
	void __fastcall SetShowImages(bool newValue);
	bool __fastcall GetShowImages(void);
	void __fastcall SetShowLines(bool newValue);
	bool __fastcall GetShowLines(void);
	void __fastcall SetShowRoot(bool newValue);
	bool __fastcall GetShowRoot(void);
	void __fastcall SetSortDir(Eltree::TSortDirs newValue);
	Eltree::TSortDirs __fastcall GetSortDir(void);
	void __fastcall SetSortMode(Eltree::TSortModes newValue);
	Eltree::TSortModes __fastcall GetSortMode(void);
	void __fastcall SetSortSection(int newValue);
	int __fastcall GetSortSection(void);
	void __fastcall SetSortType(Eltree::TSortTypes newValue);
	Eltree::TSortTypes __fastcall GetSortType(void);
	void __fastcall SetTextColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetTextColor(void);
	void __fastcall SetTracking(bool newValue);
	bool __fastcall GetTracking(void);
	void __fastcall SetVerticalLines(bool newValue);
	bool __fastcall GetVerticalLines(void);
	void __fastcall ButtonClick(System::TObject* Sender);
	void __fastcall CompareItemsTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item1, Eltree::TElTreeItem* Item2, int &res);
	void __fastcall ClickHandler(System::TObject* Sender);
	void __fastcall DblClickHandler(System::TObject* Sender);
	void __fastcall HeaderColumnClickTransfer(System::TObject* Sender, int SectionIndex);
	void __fastcall HeaderColumnDrawTransfer(Elheader::TCustomElHeader* Sender, Graphics::TCanvas* Canvas, Elheader::TElHeaderSection* Section, const Types::TRect &R, bool Pressed);
	void __fastcall HeaderColumnMoveTransfer(Eltree::TCustomElTree* Sender, Elheader::TElHeaderSection* Section, int OldPos, int NewPos);
	void __fastcall HeaderColumnResizeTransfer(System::TObject* Sender, int SectionIndex);
	void __fastcall ItemChangeTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, Eltree::TItemChangeMode ItemChangeMode);
	void __fastcall ItemCollapseTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item);
	void __fastcall ItemCollapsingTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, bool &CanProcess);
	void __fastcall ItemDeletionTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item);
	void __fastcall ItemDrawTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, Graphics::TCanvas* Surface, const Types::TRect &R, int SectionIndex);
	void __fastcall ItemExpandTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item);
	void __fastcall ItemExpandingTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, bool &CanProcess);
	void __fastcall ItemFocusedTransfer(System::TObject* Sender);
	void __fastcall ItemPicDrawTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, int &ImageIndex);
	void __fastcall KeyDownHandler(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall ShowLineHintTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section, WideString &Text, Controls::THintWindow* HintWindow, const Types::TPoint &MousePos, bool &DoShowHint);
	void __fastcall TreeResizeTransfer(System::TObject* Sender);
	void __fastcall ScrollTransfer(System::TObject* Sender, Forms::TScrollBarKind ScrollBarKind, int ScrollCode);
	void __fastcall SetDropped(bool newValue);
	void __fastcall SetLineHeight_FTree(int newValue);
	int __fastcall GetLineHeight_FTree(void);
	bool __fastcall GetReadOnly(void);
	HIDESBASE void __fastcall SetReadOnly(bool newValue);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	void __fastcall SetAlwaysKeepSelection_FTree(bool newValue);
	bool __fastcall GetAlwaysKeepSelection_FTree(void);
	void __fastcall SetAutoExpand_FTree(bool newValue);
	bool __fastcall GetAutoExpand_FTree(void);
	void __fastcall SetAutoLookup_FTree(bool newValue);
	bool __fastcall GetAutoLookup_FTree(void);
	void __fastcall SetBarStyle_FTree(bool newValue);
	bool __fastcall GetBarStyle_FTree(void);
	void __fastcall SetCustomPlusMinus_FTree(bool newValue);
	bool __fastcall GetCustomPlusMinus_FTree(void);
	void __fastcall SetDeselectChildrenOnCollapse_FTree(bool newValue);
	bool __fastcall GetDeselectChildrenOnCollapse_FTree(void);
	void __fastcall SetTreeDragImageMode(Eltree::TDragImgMode newValue);
	Eltree::TDragImgMode __fastcall GetTreeDragImageMode(void);
	void __fastcall SetTreeDrawFocusRect(bool newValue);
	bool __fastcall GetTreeDrawFocusRect(void);
	void __fastcall SetTreeFullRowSelect(bool newValue);
	bool __fastcall GetTreeFullRowSelect(void);
	void __fastcall SetTreeImages2(Controls::TImageList* newValue);
	Controls::TImageList* __fastcall GetTreeImages2(void);
	void __fastcall SetMinusPicture_FTree(Graphics::TBitmap* newValue);
	Graphics::TBitmap* __fastcall GetMinusPicture_FTree(void);
	void __fastcall SetPlusPicture_FTree(Graphics::TBitmap* newValue);
	Graphics::TBitmap* __fastcall GetPlusPicture_FTree(void);
	void __fastcall SetShowCheckboxes_FTree(bool newValue);
	bool __fastcall GetShowCheckboxes_FTree(void);
	void __fastcall SetStickyHeaderSections_FTree(bool newValue);
	bool __fastcall GetStickyHeaderSections_FTree(void);
	void __fastcall SetStoragePath_FTree(AnsiString newValue);
	AnsiString __fastcall GetStoragePath_FTree();
	void __fastcall CellStyleLoad_FTreeTransfer(System::TObject* Sender, Classes::TStream* Stream, Eltree::TElCellStyle* Style);
	void __fastcall CellStyleSave_FTreeTransfer(System::TObject* Sender, Classes::TStream* Stream, Eltree::TElCellStyle* Style);
	void __fastcall ComboEditShow_FTreeTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section, Stdctrls::TComboBox* Combobox);
	void __fastcall HeaderLookup_FTreeTransfer(System::TObject* Sender, Elheader::TElHeaderSection* Section, AnsiString &Text);
	void __fastcall HeaderLookupDone_FTreeTransfer(System::TObject* Sender, Elheader::TElHeaderSection* Section, AnsiString Text, bool Accepted);
	void __fastcall HeaderResize_FTreeTransfer(System::TObject* Sender);
	void __fastcall HeaderSectionAutoSize_FTreeTransfer(System::TObject* Sender, int SectionIndex);
	void __fastcall HeaderSectionCollapse_FTreeTransfer(System::TObject* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall HeaderSectionExpand_FTreeTransfer(System::TObject* Sender, Elheader::TElHeaderSection* Section);
	void __fastcall HeaderSectionFilterCall_FTreeTransfer(System::TObject* Sender, int SectionIndex);
	void __fastcall HorzScrollDrawPart_FTreeTransfer(System::TObject* Sender, Graphics::TCanvas* Canvas, const Types::TRect &R, Elscrollbar::TElScrollBarPart Part, bool Enabled, bool Focused, bool Pressed, bool &DefaultDraw);
	void __fastcall HorzScrollHintNeeded_FTreeTransfer(System::TObject* Sender, int TrackPosition, WideString &Hint);
	void __fastcall HotTrack_FTreeTransfer(System::TObject* Sender, Eltree::TElTreeItem* OldItem, Eltree::TElTreeItem* NewItem);
	void __fastcall ItemLoad_FTreeTransfer(System::TObject* Sender, Classes::TStream* Stream, Eltree::TElTreeItem* Item);
	void __fastcall ItemPicDraw2_FTreeTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, int &ImageIndex);
	void __fastcall ItemSave_FTreeTransfer(System::TObject* Sender, Classes::TStream* Stream, Eltree::TElTreeItem* Item);
	void __fastcall ItemSelectedChange_FTreeTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item);
	void __fastcall OleDragFinish_FTreeTransfer(System::TObject* Sender, Eldragdrop::TDragType dwEffect, HRESULT Result);
	void __fastcall OleDragStart_FTreeTransfer(System::TObject* Sender, _di_IDataObject &dataObj, _di_IDropSource &dropSource, Eldragdrop::TDragTypes &dwOKEffects);
	void __fastcall OleTargetDrag_FTreeTransfer(System::TObject* Sender, Controls::TDragState State, Eldragdrop::TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, Eldragdrop::TDragType &DragType);
	void __fastcall OleTargetDrop_FTreeTransfer(System::TObject* Sender, Eldragdrop::TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, Eldragdrop::TDragType &DragType);
	void __fastcall TryEdit_FTreeTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, int SectionIndex, Elheader::TElFieldType &CellType, bool &CanEdit);
	void __fastcall ValidateCombo_FTreeTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section, Stdctrls::TComboBox* Combo, bool &Accept);
	void __fastcall ValidateInplaceEdit_FTreeTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section, AnsiString &Text, bool &Accept);
	void __fastcall VertScrollDrawPart_FTreeTransfer(System::TObject* Sender, Graphics::TCanvas* Canvas, const Types::TRect &R, Elscrollbar::TElScrollBarPart Part, bool Enabled, bool Focused, bool Pressed, bool &DefaultDraw);
	void __fastcall VertScrollHintNeeded_FTreeTransfer(System::TObject* Sender, int TrackPosition, WideString &Hint);
	void __fastcall PrepareSelection(void);
	void __fastcall SetDropWidth(int newValue);
	void __fastcall SetDropHeight(int newValue);
	void __fastcall SetPathSeparator(char newValue);
	char __fastcall GetPathSeparator(void);
	bool __fastcall GetDrawFocusRect_FTree(void);
	void __fastcall SetFilteredVisibility_FTree(bool newValue);
	bool __fastcall GetFilteredVisibility_FTree(void);
	void __fastcall SetRightAlignedText_FTree(bool newValue);
	bool __fastcall GetRightAlignedText_FTree(void);
	void __fastcall SetRightAlignedTree_FTree(bool newValue);
	bool __fastcall GetRightAlignedTree_FTree(void);
	void __fastcall SetCheckBoxGlyph_FTree(Graphics::TBitmap* newValue);
	Graphics::TBitmap* __fastcall GetCheckBoxGlyph_FTree(void);
	void __fastcall SetCustomCheckboxes_FTree(bool newValue);
	bool __fastcall GetCustomCheckboxes_FTree(void);
	void __fastcall SetRadioButtonGlyph_FTree(Graphics::TBitmap* newValue);
	Graphics::TBitmap* __fastcall GetRadioButtonGlyph_FTree(void);
	void __fastcall SetUnderlineTracked_FTree(bool newValue);
	bool __fastcall GetUnderlineTracked_FTree(void);
	void __fastcall SetDoInplaceEdit(bool newValue);
	bool __fastcall GetDoInplaceEdit(void);
	void __fastcall TreeKeyDownTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall TreeKeyPressTransfer(System::TObject* Sender, char &Key);
	void __fastcall TreeKeyUpTransfer(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall TreeMouseDownTransfer(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall TreeMouseMoveTransfer(System::TObject* Sender, Classes::TShiftState Shift, int X, int Y);
	void __fastcall TreeMouseUpTransfer(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall SetShowRootButtons(bool newValue);
	bool __fastcall GetShowRootButtons(void);
	void __fastcall EditRequestTransfer(System::TObject* Sender, Eltree::TElTreeItem* Item, Elheader::TElHeaderSection* Section);
	void __fastcall SetTreeFlat(bool newValue);
	bool __fastcall GetTreeFlat(void);
	void __fastcall SetTreeHeaderActiveFilterColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetTreeHeaderActiveFilterColor(void);
	void __fastcall SetTreeHeaderFilterColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetTreeHeaderFilterColor(void);
	void __fastcall SetTreeHeaderFlat(bool newValue);
	bool __fastcall GetTreeHeaderFlat(void);
	void __fastcall SetAutoResizeColumns(bool newValue);
	bool __fastcall GetAutoResizeColumns(void);
	void __fastcall SetTreeBackground(Graphics::TBitmap* newValue);
	Graphics::TBitmap* __fastcall GetTreeBackground(void);
	void __fastcall SetTreeBackgroundType(Elvclutils::TElBkGndType newValue);
	Elvclutils::TElBkGndType __fastcall GetTreeBackgroundType(void);
	HIDESBASE void __fastcall SetFlatFocusedScrollbars(bool newValue);
	bool __fastcall GetFlatFocusedScrollbars(void);
	void __fastcall SetGradientEndColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetGradientEndColor(void);
	void __fastcall SetGradientStartColor(Graphics::TColor newValue);
	Graphics::TColor __fastcall GetGradientStartColor(void);
	void __fastcall SetGradientSteps(int newValue);
	int __fastcall GetGradientSteps(void);
	HIDESBASE void __fastcall SetHorzScrollBarStyles(Elscrollbar::TElScrollBarStyles* newValue);
	Elscrollbar::TElScrollBarStyles* __fastcall GetHorzScrollBarStyles(void);
	HIDESBASE void __fastcall SetVertScrollBarStyles(Elscrollbar::TElScrollBarStyles* newValue);
	Elscrollbar::TElScrollBarStyles* __fastcall GetVertScrollBarStyles(void);
	Stdctrls::TScrollStyle __fastcall GetForcedScrollBars(void);
	void __fastcall SetForcedScrollBars(const Stdctrls::TScrollStyle Value);
	Graphics::TColor __fastcall GetFocusedSelectColor_Tree(void);
	void __fastcall SetFocusedSelectColor_Tree(const Graphics::TColor Value);
	int __fastcall GetTreeHeaderHeight(void);
	void __fastcall SetTreeHeaderHeight(const int Value);
	bool __fastcall GetHeaderInvertSortArrows_FTree(void);
	void __fastcall SetHeaderInvertSortArrows_FTree(const bool Value);
	int __fastcall GetLeftPosition_FTree(void);
	void __fastcall SetLeftPosition_FTree(const int Value);
	Graphics::TColor __fastcall GetLinesColor_FTree(void);
	Graphics::TPenStyle __fastcall GetLinesStyle_FTree(void);
	void __fastcall SetLinesColor_FTree(const Graphics::TColor Value);
	void __fastcall SetLinesStyle_FTree(const Graphics::TPenStyle Value);
	bool __fastcall GetLockHeaderHeight_FTree(void);
	void __fastcall SetLockHeaderHeight_FTree(const bool Value);
	bool __fastcall GetMoveColumnOnDrag_FTree(void);
	bool __fastcall GetMoveFocusOnCollapse_FTree(void);
	void __fastcall SetMoveColumnOnDrag_FTree(const bool Value);
	void __fastcall SetMoveFocusOnCollapse_FTree(const bool Value);
	bool __fastcall GetNoBlendSelected(void);
	void __fastcall SetNoBlendSelected(const bool Value);
	bool __fastcall GetScrollBackground(void);
	void __fastcall SetScrollBackground(const bool Value);
	int __fastcall GetSelectColumn(void);
	void __fastcall SetSelectColumn(const int Value);
	void __fastcall StartDrag_FTreeTransfer(System::TObject* Sender, Controls::TDragObject* &DragObject);
	Elvclutils::TElFlatBorderType __fastcall GetTreeActiveBorderType(void);
	Elvclutils::TElFlatBorderType __fastcall GetTreeInactiveBorderType(void);
	void __fastcall SetTreeActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetTreeInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetStorage_FTree(Elini::TElIniFile* newValue);
	Elini::TElIniFile* __fastcall GetStorage_FTree(void);
	bool __fastcall GetAlwaysKeepFocus(void);
	void __fastcall SetAlwaysKeepFocus(bool Value);
	bool __fastcall GetAdjustMultilineHeight(void);
	void __fastcall SetAdjustMultilineHeight(bool Value);
	void __fastcall SetBarStyleVerticalLines(bool Value);
	bool __fastcall GetBarStyleVerticalLines(void);
	int __fastcall GetChangeDelay(void);
	void __fastcall SetChangeDelay(int Value);
	Graphics::TColor __fastcall GetHorzDivLinesColor(void);
	void __fastcall SetHorzDivLinesColor(Graphics::TColor Value);
	Eltree::TDragTargetDraw __fastcall GetDragTrgDrawMode(void);
	void __fastcall SetDragTrgDrawMode(Eltree::TDragTargetDraw Value);
	int __fastcall GetDragExpandDelay(void);
	void __fastcall SetDragExpandDelay(int Value);
	Graphics::TColor __fastcall GetDragRectAcceptColor(void);
	void __fastcall SetDragRectAcceptColor(Graphics::TColor Value);
	Graphics::TColor __fastcall GetDragRectDenyColor(void);
	void __fastcall SetDragRectDenyColor(Graphics::TColor Value);
	bool __fastcall GetExpandOnDragOver(void);
	void __fastcall SetExpandOnDragOver(bool Value);
	Graphics::TColor __fastcall GetFocusedSelectTextColor(void);
	void __fastcall SetFocusedSelectTextColor(Graphics::TColor Value);
	Graphics::TColor __fastcall GetHeaderColor(void);
	void __fastcall SetHeaderColor(Graphics::TColor Value);
	bool __fastcall GetHeaderWrapCaptions(void);
	void __fastcall SetHeaderWrapCaptions(bool Value);
	bool __fastcall GetHideFocusRect(void);
	void __fastcall SetHideFocusRect(bool Value);
	Graphics::TColor __fastcall GetHideSelectColor(void);
	void __fastcall SetHideSelectColor(Graphics::TColor Value);
	Graphics::TColor __fastcall GetHideSelectTextColor(void);
	void __fastcall SetHideSelectTextColor(Graphics::TColor Value);
	bool __fastcall GetHideSelection(void);
	HIDESBASE void __fastcall SetHideSelection(bool Value);
	bool __fastcall GetIncrementalSearch(void);
	void __fastcall SetIncrementalSearch(bool Value);
	int __fastcall GetItemIndent(void);
	void __fastcall SetItemIndent(int Value);
	Graphics::TColor __fastcall GetLineHintColor(void);
	void __fastcall SetLineHintColor(Graphics::TColor Value);
	int __fastcall GetLineHintTimeout(void);
	void __fastcall SetLineHintTimeout(int Value);
	Eltree::TLineHintType __fastcall GetLineHintType(void);
	void __fastcall SetLineHintType(Eltree::TLineHintType Value);
	bool __fastcall GetPlusMinusTransparent(void);
	void __fastcall SetPlusMinusTransparent(bool Value);
	bool __fastcall GetRightClickSelect(void);
	void __fastcall SetRightClickSelect(bool Value);
	bool __fastcall GetRowHotTrack(void);
	void __fastcall SetRowHotTrack(bool Value);
	bool __fastcall GetScrollbarOpposite(void);
	void __fastcall SetScrollbarOpposite(bool Value);
	Graphics::TColor __fastcall GetTrackColor(void);
	void __fastcall SetTrackColor(Graphics::TColor Value);
	bool __fastcall GetUseCustomScrollBars(void);
	HIDESBASE void __fastcall SetUseCustomScrollBars(bool Value);
	bool __fastcall GetVerticalLinesLong(void);
	void __fastcall SetVerticalLinesLong(bool Value);
	bool __fastcall GetUseSystemHintColors(void);
	void __fastcall SetUseSystemHintColors(bool Value);
	Elheader::TMeasureSectionEvent __fastcall GetOnHeaderSectionMeasure();
	void __fastcall SetOnHeaderSectionMeasure(Elheader::TMeasureSectionEvent Value);
	Eltree::TApplyVisFilterEvent __fastcall GetOnApplyVisFilter();
	void __fastcall SetOnApplyVisFilter(Eltree::TApplyVisFilterEvent Value);
	Classes::TNotifyEvent __fastcall GetOnAfterSelectionChange();
	void __fastcall SetOnAfterSelectionChange(Classes::TNotifyEvent Value);
	Eltree::TOnItemCheckedEvent __fastcall GetOnItemChecked();
	void __fastcall SetOnItemChecked(Eltree::TOnItemCheckedEvent Value);
	Classes::TNotifyEvent __fastcall GetOnSortBegin();
	void __fastcall SetOnSortBegin(Classes::TNotifyEvent Value);
	Classes::TNotifyEvent __fastcall GetOnSortEnd();
	void __fastcall SetOnSortEnd(Classes::TNotifyEvent Value);
	Htmlrender::TElHTMLImageNeededEvent __fastcall GetOnHTMLImageNeeded();
	void __fastcall SetOnHTMLImageNeeded(Htmlrender::TElHTMLImageNeededEvent Value);
	Graphics::TColor __fastcall GetStripedOddColor(void);
	void __fastcall SetStripedOddColor(Graphics::TColor Value);
	Graphics::TColor __fastcall GetStripedEvenColor(void);
	void __fastcall SetStripedEvenColor(Graphics::TColor Value);
	bool __fastcall GetStripedItems(void);
	void __fastcall SetStripedItems(bool Value);
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyUp(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMSysKeyDown(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMSysKeyUp(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Message);
	
protected:
	bool FCloseOnClick;
	bool FSizeableTree;
	bool FAdjustDropDownPos;
	virtual void __fastcall TriggerDropEvent(bool Dropped);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall TriggerNewTextSelectionEvent(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	Eltree::TElHintType __fastcall GetHintType(void);
	void __fastcall SetHintType(Eltree::TElHintType Value);
	MESSAGE void __fastcall WMActivateApp(Messages::TWMActivateApp &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMMButtonDblClk(Messages::TWMMouse &Message);
	MESSAGE void __fastcall EMSetReadOnly(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMCancelMode(Messages::TMessage &Message);
	Graphics::TColor __fastcall GetVertDivLinesColor(void);
	void __fastcall SetVertDivLinesColor(Graphics::TColor Value);
	MESSAGE void __fastcall CMCancelMode(Controls::TCMCancelMode &Msg);
	Elheader::TElHeaderSections* __fastcall GetHeaderSections(void);
	void __fastcall SetHeaderSections(Elheader::TElHeaderSections* Value);
	Controls::TImageList* __fastcall GetHeaderImages(void);
	void __fastcall SetHeaderImages(Controls::TImageList* Value);
	int __fastcall GetMultiSelectLevel(void);
	void __fastcall SetMultiSelectLevel(int Value);
	int __fastcall GetDragScrollInterval(void);
	void __fastcall SetDragScrollInterval(int Value);
	bool __fastcall GetMouseFrameSelect(void);
	void __fastcall SetMouseFrameSelect(bool Value);
	virtual Eltree::TCustomElTree* __fastcall CreateTree(void);
	bool __fastcall GetShowLeafButton(void);
	void __fastcall SetShowLeafButton(bool Value);
	Graphics::TBitmap* __fastcall GetLeafPicture(void);
	void __fastcall SetLeafPicture(Graphics::TBitmap* Value);
	Graphics::TFont* __fastcall GetHeaderFont(void);
	void __fastcall SetHeaderFont(Graphics::TFont* Value);
	bool __fastcall GetHeaderUseTreeFont(void);
	void __fastcall SetHeaderUseTreeFont(bool Value);
	void __fastcall SetSizeableTree(bool Value);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual void __fastcall Loaded(void);
	bool __fastcall IsButtonWidthStored(void);
	__property Eltree::TApplyVisFilterEvent OnApplyVisFilter = {read=GetOnApplyVisFilter, write=SetOnApplyVisFilter};
	
public:
	__fastcall virtual TElTreeCombo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeCombo(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	Eltree::TElTree* __fastcall GetTree(void);
	__property Ellist::TElList* SelectionList = {read=GetSelectionList};
	__property bool Dropped = {read=FDropped, write=SetDropped, nodefault};
	__property Eltree::TElTreeItem* Selection = {read=GetSelection, write=SetSelection};
	
__published:
	__property ButtonWidth  = {stored=IsButtonWidthStored};
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property AutoSize  = {default=1};
	__property RTLContent ;
	__property BorderSides ;
	__property PasswordChar ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property WantTabs  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property ImageForm ;
	__property WordWrap  = {default=1};
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property ActiveBorderType  = {default=1};
	__property ButtonFlat ;
	__property ButtonColor ;
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property Align  = {default=0};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property Cursor  = {default=0};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Text ;
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property Visible  = {default=1};
	__property bool AutoLineHeight = {read=GetAutoLineHeight_FTree, write=SetAutoLineHeight_FTree, nodefault};
	__property Graphics::TColor TreeBkColor = {read=GetBkColor_FTree, write=SetBkColor_FTree, nodefault};
	__property bool TreeChangeStateImage = {read=GetChangeStateImage_FTree, write=SetChangeStateImage_FTree, nodefault};
	__property bool TreeCtl3D = {read=GetCtl3D_FTree, write=SetCtl3D_FTree, nodefault};
	__property Controls::TCursor TreeCursor = {read=GetCursor, write=SetCursor, nodefault};
	__property bool DraggableSections = {read=GetDraggableSections, write=SetDraggableSections, nodefault};
	__property Graphics::TColor TreeFocusedSelectColor = {read=GetFocusedSelectColor_Tree, write=SetFocusedSelectColor_Tree, nodefault};
	__property Graphics::TFont* TreeFont = {read=GetFont_FTree, write=SetFont_FTree};
	__property bool TreeHeaderHotTrack = {read=GetHeaderHotTrack_FTree, write=SetHeaderHotTrack_FTree, nodefault};
	__property Controls::TImageList* TreeHeaderImages = {read=GetHeaderImages_FTree, write=SetHeaderImages_FTree};
	__property bool TreeHeaderInvertSortArrows = {read=GetHeaderInvertSortArrows_FTree, write=SetHeaderInvertSortArrows_FTree, nodefault};
	__property Elheader::TElHeaderSections* TreeHeaderSections = {read=GetHeaderSections_FTree, write=SetHeaderSections_FTree};
	__property bool TreeHideHintOnMove = {read=GetHideHintOnMove_FTree, write=SetHideHintOnMove_FTree, nodefault};
	__property bool TreeHideHintOnTimer = {read=GetHideHintOnTimer_FTree, write=SetHideHintOnTimer_FTree, nodefault};
	__property bool TreeHideHorzScrollBar = {read=GetHideHorzScrollBar_FTree, write=SetHideHorzScrollBar_FTree, nodefault};
	__property bool TreeHideVertScrollBar = {read=GetHideVertScrollBar_FTree, write=SetHideVertScrollBar_FTree, nodefault};
	__property bool TreeHorizontalLines = {read=GetHorizontalLines_FTree, write=SetHorizontalLines_FTree, nodefault};
	__property int TreeLeftPosition = {read=GetLeftPosition_FTree, write=SetLeftPosition_FTree, nodefault};
	__property Controls::TImageList* Images = {read=GetImages_FTree, write=SetImages_FTree};
	__property Controls::TImageList* Images2 = {read=GetTreeImages2, write=SetTreeImages2};
	__property Eltree::TElTreeItems* Items = {read=GetItems_FTree, write=SetItems_FTree};
	__property Eltree::THintModes LineHintMode = {read=GetLineHintMode, write=SetLineHintMode, nodefault};
	__property int MainTreeColumn = {read=GetMainTreeColumn, write=SetMainTreeColumn, nodefault};
	__property bool MultiSelect = {read=GetMultiSelect, write=SetMultiSelect, nodefault};
	__property bool NoBlendSelected = {read=GetNoBlendSelected, write=SetNoBlendSelected, nodefault};
	__property bool OwnerDrawByColumn = {read=GetOwnerDrawByColumn, write=SetOwnerDrawByColumn, nodefault};
	__property AnsiString OwnerDrawMask = {read=GetOwnerDrawMask, write=SetOwnerDrawMask};
	__property bool TreeParentCtl3D = {read=GetParentCtl3D, write=SetParentCtl3D, nodefault};
	__property bool TreeParentFont = {read=GetParentFont, write=SetParentFont, nodefault};
	__property bool TreeParentShowHint = {read=GetParentShowHint, write=SetParentShowHint, nodefault};
	__property bool TreeScrollBackground = {read=GetScrollBackground, write=SetScrollBackground, nodefault};
	__property bool RowSelect = {read=GetRowSelect, write=SetRowSelect, nodefault};
	__property bool ScrollTracking = {read=GetScrollTracking, write=SetScrollTracking, nodefault};
	__property int SelectColumn = {read=GetSelectColumn, write=SetSelectColumn, nodefault};
	__property Eltree::TSTSelModes SelectionMode = {read=GetSelectionMode, write=SetSelectionMode, nodefault};
	__property bool ShowButtons = {read=GetShowButtons, write=SetShowButtons, nodefault};
	__property bool ShowColumns = {read=GetShowColumns, write=SetShowColumns, nodefault};
	__property bool TreeShowHint = {read=GetShowHint, write=SetShowHint, nodefault};
	__property bool ShowImages = {read=GetShowImages, write=SetShowImages, nodefault};
	__property bool ShowLines = {read=GetShowLines, write=SetShowLines, nodefault};
	__property bool ShowRoot = {read=GetShowRoot, write=SetShowRoot, nodefault};
	__property Eltree::TSortDirs SortDir = {read=GetSortDir, write=SetSortDir, nodefault};
	__property Eltree::TSortModes SortMode = {read=GetSortMode, write=SetSortMode, nodefault};
	__property int SortSection = {read=GetSortSection, write=SetSortSection, nodefault};
	__property Eltree::TSortTypes SortType = {read=GetSortType, write=SetSortType, nodefault};
	__property Graphics::TColor TextColor = {read=GetTextColor, write=SetTextColor, nodefault};
	__property bool Tracking = {read=GetTracking, write=SetTracking, nodefault};
	__property bool VerticalLines = {read=GetVerticalLines, write=SetVerticalLines, nodefault};
	__property bool AlwaysKeepSelection = {read=GetAlwaysKeepSelection_FTree, write=SetAlwaysKeepSelection_FTree, nodefault};
	__property bool AutoExpand = {read=GetAutoExpand_FTree, write=SetAutoExpand_FTree, nodefault};
	__property bool AutoLookup = {read=GetAutoLookup_FTree, write=SetAutoLookup_FTree, nodefault};
	__property bool BarStyle = {read=GetBarStyle_FTree, write=SetBarStyle_FTree, nodefault};
	__property bool CustomPlusMinus = {read=GetCustomPlusMinus_FTree, write=SetCustomPlusMinus_FTree, nodefault};
	__property bool DeselectChildrenOnCollapse = {read=GetDeselectChildrenOnCollapse_FTree, write=SetDeselectChildrenOnCollapse_FTree, nodefault};
	__property Eltree::TDragImgMode DragImageMode = {read=GetTreeDragImageMode, write=SetTreeDragImageMode, nodefault};
	__property bool DrawFocusRect = {read=GetTreeDrawFocusRect, write=SetTreeDrawFocusRect, nodefault};
	__property Stdctrls::TScrollStyle ForcedScrollBars = {read=GetForcedScrollBars, write=SetForcedScrollBars, nodefault};
	__property bool FullRowSelect = {read=GetTreeFullRowSelect, write=SetTreeFullRowSelect, nodefault};
	__property Graphics::TBitmap* MinusPicture = {read=GetMinusPicture_FTree, write=SetMinusPicture_FTree};
	__property bool MoveColumnOnDrag = {read=GetMoveColumnOnDrag_FTree, write=SetMoveColumnOnDrag_FTree, nodefault};
	__property bool MoveFocusOnCollapse = {read=GetMoveFocusOnCollapse_FTree, write=SetMoveFocusOnCollapse_FTree, nodefault};
	__property Graphics::TBitmap* PlusPicture = {read=GetPlusPicture_FTree, write=SetPlusPicture_FTree};
	__property bool ShowCheckboxes = {read=GetShowCheckboxes_FTree, write=SetShowCheckboxes_FTree, nodefault};
	__property bool StickyHeaderSections = {read=GetStickyHeaderSections_FTree, write=SetStickyHeaderSections_FTree, nodefault};
	__property Elini::TElIniFile* Storage = {read=GetStorage_FTree, write=SetStorage_FTree};
	__property AnsiString StoragePath = {read=GetStoragePath_FTree, write=SetStoragePath_FTree};
	__property int DropWidth = {read=FDropWidth, write=SetDropWidth, nodefault};
	__property int DropHeight = {read=FDropHeight, write=SetDropHeight, nodefault};
	__property int LineHeight = {read=GetLineHeight_FTree, write=SetLineHeight_FTree, nodefault};
	__property Graphics::TColor LinesColor = {read=GetLinesColor_FTree, write=SetLinesColor_FTree, nodefault};
	__property Graphics::TPenStyle LinesStyle = {read=GetLinesStyle_FTree, write=SetLinesStyle_FTree, nodefault};
	__property bool LockHeaderHeight = {read=GetLockHeaderHeight_FTree, write=SetLockHeaderHeight_FTree, nodefault};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property char PathSeparator = {read=GetPathSeparator, write=SetPathSeparator, nodefault};
	__property bool FilteredVisibility = {read=GetFilteredVisibility_FTree, write=SetFilteredVisibility_FTree, nodefault};
	__property bool RightAlignedText = {read=GetRightAlignedText_FTree, write=SetRightAlignedText_FTree, nodefault};
	__property bool RightAlignedTree = {read=GetRightAlignedTree_FTree, write=SetRightAlignedTree_FTree, nodefault};
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
	__property Eltree::TOnCompareItems OnCompareItems = {read=FOnCompareItems, write=FOnCompareItems};
	__property Eltree::TColumnNotifyEvent OnHeaderColumnClick = {read=FOnHeaderColumnClick, write=FOnHeaderColumnClick};
	__property Elheader::TElSectionRedrawEvent OnHeaderColumnDraw = {read=FOnHeaderColumnDraw, write=FOnHeaderColumnDraw};
	__property Eltree::TElColumnMoveEvent OnHeaderColumnMove = {read=FOnHeaderColumnMove, write=FOnHeaderColumnMove};
	__property Eltree::TColumnNotifyEvent OnHeaderColumnResize = {read=FOnHeaderColumnResize, write=FOnHeaderColumnResize};
	__property Eltree::TOnItemChangeEvent OnItemChange = {read=FOnItemChange, write=FOnItemChange};
	__property Eltree::TOnItemExpandEvent OnItemCollapse = {read=FOnItemCollapse, write=FOnItemCollapse};
	__property Eltree::TOnItemExpanding OnItemCollapsing = {read=FOnItemCollapsing, write=FOnItemCollapsing};
	__property Eltree::TOnItemExpandEvent OnItemDeletion = {read=FOnItemDeletion, write=FOnItemDeletion};
	__property Eltree::TOnItemDrawEvent OnItemDraw = {read=FOnItemDraw, write=FOnItemDraw};
	__property Eltree::TOnItemExpandEvent OnItemExpand = {read=FOnItemExpand, write=FOnItemExpand};
	__property Eltree::TOnItemExpanding OnItemExpanding = {read=FOnItemExpanding, write=FOnItemExpanding};
	__property Classes::TNotifyEvent OnItemFocused = {read=FOnItemFocused, write=FOnItemFocused};
	__property Eltree::TOnPicDrawEvent OnItemPicDraw = {read=FOnItemPicDraw, write=FOnItemPicDraw};
	__property Eltree::TElScrollEvent OnScroll = {read=FOnScroll_FTree, write=FOnScroll_FTree};
	__property Eltree::TOnShowHintEvent OnShowLineHint = {read=FOnShowLineHint, write=FOnShowLineHint};
	__property Classes::TNotifyEvent OnTreeResize = {read=FOnResize_FTree, write=FOnResize_FTree};
	__property Eltree::TCellStyleSaveEvent OnCellStyleLoad = {read=FOnCellStyleLoad_FTree, write=FOnCellStyleLoad_FTree};
	__property Eltree::TCellStyleSaveEvent OnCellStyleSave = {read=FOnCellStyleSave_FTree, write=FOnCellStyleSave_FTree};
	__property Eltree::TComboEditShowEvent OnComboEditShow = {read=FOnComboEditShow_FTree, write=FOnComboEditShow_FTree};
	__property Elheader::TElHeaderLookupEvent OnHeaderLookup = {read=FOnHeaderLookup_FTree, write=FOnHeaderLookup_FTree};
	__property Elheader::TElHeaderLookupDoneEvent OnHeaderLookupDone = {read=FOnHeaderLookupDone_FTree, write=FOnHeaderLookupDone_FTree};
	__property Classes::TNotifyEvent OnHeaderResize = {read=FOnHeaderResize_FTree, write=FOnHeaderResize_FTree};
	__property Eltree::TColumnNotifyEvent OnHeaderSectionAutoSize = {read=FOnHeaderSectionAutoSize_FTree, write=FOnHeaderSectionAutoSize_FTree};
	__property Eltree::THeaderSectionEvent OnHeaderSectionCollapse = {read=FOnHeaderSectionCollapse_FTree, write=FOnHeaderSectionCollapse_FTree};
	__property Eltree::THeaderSectionEvent OnHeaderSectionExpand = {read=FOnHeaderSectionExpand_FTree, write=FOnHeaderSectionExpand_FTree};
	__property Eltree::TColumnNotifyEvent OnHeaderSectionFilterCall = {read=FOnHeaderSectionFilterCall_FTree, write=FOnHeaderSectionFilterCall_FTree};
	__property Elscrollbar::TElScrollDrawPartEvent OnHorzScrollDrawPart = {read=FOnHorzScrollDrawPart_FTree, write=FOnHorzScrollDrawPart_FTree};
	__property Elscrollbar::TElScrollHintNeededEvent OnHorzScrollHintNeeded = {read=FOnHorzScrollHintNeeded_FTree, write=FOnHorzScrollHintNeeded_FTree};
	__property Eltree::THotTrackEvent OnHotTrack = {read=FOnHotTrack_FTree, write=FOnHotTrack_FTree};
	__property Eltree::TItemSaveEvent OnItemLoad = {read=FOnItemLoad_FTree, write=FOnItemLoad_FTree};
	__property Eltree::TOnPicDrawEvent OnItemPicDraw2 = {read=FOnItemPicDraw2_FTree, write=FOnItemPicDraw2_FTree};
	__property Eltree::TItemSaveEvent OnItemSave = {read=FOnItemSave_FTree, write=FOnItemSave_FTree};
	__property Eltree::TItemSelChangeEvent OnItemSelectedChange = {read=FOnItemSelectedChange_FTree, write=FOnItemSelectedChange_FTree};
	__property Eltree::TOleDragFinishEvent OnOleDragFinish = {read=FOnOleDragFinish_FTree, write=FOnOleDragFinish_FTree};
	__property Eltree::TOleDragStartEvent OnOleDragStart = {read=FOnOleDragStart_FTree, write=FOnOleDragStart_FTree};
	__property Eldragdrop::TTargetDragEvent OnOleTargetDrag = {read=FOnOleTargetDrag_FTree, write=FOnOleTargetDrag_FTree};
	__property Eldragdrop::TTargetDropEvent OnOleTargetDrop = {read=FOnOleTargetDrop_FTree, write=FOnOleTargetDrop_FTree};
	__property Eltree::TTryEditEvent OnTryEdit = {read=FOnTryEdit_FTree, write=FOnTryEdit_FTree};
	__property Eltree::TValidateComboEvent OnValidateCombo = {read=FOnValidateCombo_FTree, write=FOnValidateCombo_FTree};
	__property Eltree::TOnValidateEvent OnValidateInplaceEdit = {read=FOnValidateInplaceEdit_FTree, write=FOnValidateInplaceEdit_FTree};
	__property Elscrollbar::TElScrollDrawPartEvent OnVertScrollDrawPart = {read=FOnVertScrollDrawPart_FTree, write=FOnVertScrollDrawPart_FTree};
	__property Elscrollbar::TElScrollHintNeededEvent OnVertScrollHintNeeded = {read=FOnVertScrollHintNeeded_FTree, write=FOnVertScrollHintNeeded_FTree};
	__property Classes::TNotifyEvent OnNewTextSelection = {read=FOnNewTextSelection, write=FOnNewTextSelection};
	__property Controls::TKeyEvent OnTreeKeyDown = {read=FOnTreeKeyDown, write=FOnTreeKeyDown};
	__property Controls::TKeyPressEvent OnTreeKeyPress = {read=FOnTreeKeyPress, write=FOnTreeKeyPress};
	__property Controls::TKeyEvent OnTreeKeyUp = {read=FOnTreeKeyUp, write=FOnTreeKeyUp};
	__property Controls::TMouseEvent OnTreeMouseDown = {read=FOnTreeMouseDown, write=FOnTreeMouseDown};
	__property Controls::TMouseMoveEvent OnTreeMouseMove = {read=FOnTreeMouseMove, write=FOnTreeMouseMove};
	__property Controls::TMouseEvent OnTreeMouseUp = {read=FOnTreeMouseUp, write=FOnTreeMouseUp};
	__property TElComboDropEvent OnDrop = {read=FOnDrop, write=FOnDrop};
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property BevelKind  = {default=0};
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
	__property Graphics::TBitmap* CheckBoxGlyph = {read=GetCheckBoxGlyph_FTree, write=SetCheckBoxGlyph_FTree};
	__property bool CustomCheckboxes = {read=GetCustomCheckboxes_FTree, write=SetCustomCheckboxes_FTree, nodefault};
	__property Graphics::TBitmap* RadioButtonGlyph = {read=GetRadioButtonGlyph_FTree, write=SetRadioButtonGlyph_FTree};
	__property bool UnderlineTracked = {read=GetUnderlineTracked_FTree, write=SetUnderlineTracked_FTree, nodefault};
	__property bool AutoProcessSelection = {read=FAutoProcessSelection, write=FAutoProcessSelection, default=1};
	__property bool DoInplaceEdit = {read=GetDoInplaceEdit, write=SetDoInplaceEdit, nodefault};
	__property bool ShowRootButtons = {read=GetShowRootButtons, write=SetShowRootButtons, nodefault};
	__property Eltree::TEditRequestEvent OnEditRequest = {read=FOnEditRequest, write=FOnEditRequest};
	__property Elvclutils::TElFlatBorderType TreeActiveBorderType = {read=GetTreeActiveBorderType, write=SetTreeActiveBorderType, nodefault};
	__property bool TreeFlat = {read=GetTreeFlat, write=SetTreeFlat, nodefault};
	__property Graphics::TColor TreeHeaderActiveFilterColor = {read=GetTreeHeaderActiveFilterColor, write=SetTreeHeaderActiveFilterColor, nodefault};
	__property Graphics::TColor TreeHeaderFilterColor = {read=GetTreeHeaderFilterColor, write=SetTreeHeaderFilterColor, nodefault};
	__property bool TreeHeaderFlat = {read=GetTreeHeaderFlat, write=SetTreeHeaderFlat, nodefault};
	__property int TreeHeaderHeight = {read=GetTreeHeaderHeight, write=SetTreeHeaderHeight, nodefault};
	__property Elvclutils::TElFlatBorderType TreeInactiveBorderType = {read=GetTreeInactiveBorderType, write=SetTreeInactiveBorderType, nodefault};
	__property bool AutoResizeColumns = {read=GetAutoResizeColumns, write=SetAutoResizeColumns, nodefault};
	__property Graphics::TBitmap* TreeBackground = {read=GetTreeBackground, write=SetTreeBackground};
	__property Elvclutils::TElBkGndType TreeBackgroundType = {read=GetTreeBackgroundType, write=SetTreeBackgroundType, nodefault};
	__property bool FlatFocusedScrollbars = {read=GetFlatFocusedScrollbars, write=SetFlatFocusedScrollbars, nodefault};
	__property Graphics::TColor GradientEndColor = {read=GetGradientEndColor, write=SetGradientEndColor, nodefault};
	__property Graphics::TColor GradientStartColor = {read=GetGradientStartColor, write=SetGradientStartColor, nodefault};
	__property int GradientSteps = {read=GetGradientSteps, write=SetGradientSteps, nodefault};
	__property Elscrollbar::TElScrollBarStyles* HorzScrollBarStyles = {read=GetHorzScrollBarStyles, write=SetHorzScrollBarStyles};
	__property Elscrollbar::TElScrollBarStyles* VertScrollBarStyles = {read=GetVertScrollBarStyles, write=SetVertScrollBarStyles};
	__property bool AlwaysKeepFocus = {read=GetAlwaysKeepFocus, write=SetAlwaysKeepFocus, nodefault};
	__property bool AdjustMultilineHeight = {read=GetAdjustMultilineHeight, write=SetAdjustMultilineHeight, nodefault};
	__property bool BarStyleVerticalLines = {read=GetBarStyleVerticalLines, write=SetBarStyleVerticalLines, nodefault};
	__property int ChangeDelay = {read=GetChangeDelay, write=SetChangeDelay, nodefault};
	__property Graphics::TColor HorzDivLinesColor = {read=GetHorzDivLinesColor, write=SetHorzDivLinesColor, nodefault};
	__property Eltree::TDragTargetDraw DragTrgDrawMode = {read=GetDragTrgDrawMode, write=SetDragTrgDrawMode, nodefault};
	__property int DragExpandDelay = {read=GetDragExpandDelay, write=SetDragExpandDelay, nodefault};
	__property Graphics::TColor DragRectAcceptColor = {read=GetDragRectAcceptColor, write=SetDragRectAcceptColor, nodefault};
	__property Graphics::TColor DragRectDenyColor = {read=GetDragRectDenyColor, write=SetDragRectDenyColor, nodefault};
	__property bool ExpandOnDragOver = {read=GetExpandOnDragOver, write=SetExpandOnDragOver, nodefault};
	__property Graphics::TColor FocusedSelectTextColor = {read=GetFocusedSelectTextColor, write=SetFocusedSelectTextColor, nodefault};
	__property Graphics::TColor HeaderColor = {read=GetHeaderColor, write=SetHeaderColor, nodefault};
	__property bool HeaderWrapCaptions = {read=GetHeaderWrapCaptions, write=SetHeaderWrapCaptions, nodefault};
	__property bool HideFocusRect = {read=GetHideFocusRect, write=SetHideFocusRect, nodefault};
	__property Graphics::TColor HideSelectColor = {read=GetHideSelectColor, write=SetHideSelectColor, nodefault};
	__property Graphics::TColor HideSelectTextColor = {read=GetHideSelectTextColor, write=SetHideSelectTextColor, nodefault};
	__property bool TreeHideSelection = {read=GetHideSelection, write=SetHideSelection, nodefault};
	__property bool IncrementalSearch = {read=GetIncrementalSearch, write=SetIncrementalSearch, nodefault};
	__property int ItemIndent = {read=GetItemIndent, write=SetItemIndent, nodefault};
	__property Graphics::TColor LineHintColor = {read=GetLineHintColor, write=SetLineHintColor, nodefault};
	__property int LineHintTimeout = {read=GetLineHintTimeout, write=SetLineHintTimeout, nodefault};
	__property Eltree::TLineHintType LineHintType = {read=GetLineHintType, write=SetLineHintType, nodefault};
	__property bool PlusMinusTransparent = {read=GetPlusMinusTransparent, write=SetPlusMinusTransparent, nodefault};
	__property bool RightClickSelect = {read=GetRightClickSelect, write=SetRightClickSelect, nodefault};
	__property bool RowHotTrack = {read=GetRowHotTrack, write=SetRowHotTrack, nodefault};
	__property bool ScrollbarOpposite = {read=GetScrollbarOpposite, write=SetScrollbarOpposite, nodefault};
	__property Graphics::TColor TrackColor = {read=GetTrackColor, write=SetTrackColor, nodefault};
	__property bool UseCustomScrollBars = {read=GetUseCustomScrollBars, write=SetUseCustomScrollBars, nodefault};
	__property bool VerticalLinesLong = {read=GetVerticalLinesLong, write=SetVerticalLinesLong, nodefault};
	__property bool UseSystemHintColors = {read=GetUseSystemHintColors, write=SetUseSystemHintColors, nodefault};
	__property Elheader::TMeasureSectionEvent OnHeaderSectionMeasure = {read=GetOnHeaderSectionMeasure, write=SetOnHeaderSectionMeasure};
	__property Classes::TNotifyEvent OnAfterSelectionChange = {read=GetOnAfterSelectionChange, write=SetOnAfterSelectionChange};
	__property Eltree::TOnItemCheckedEvent OnItemChecked = {read=GetOnItemChecked, write=SetOnItemChecked};
	__property Classes::TNotifyEvent OnSortBegin = {read=GetOnSortBegin, write=SetOnSortBegin};
	__property Classes::TNotifyEvent OnSortEnd = {read=GetOnSortEnd, write=SetOnSortEnd};
	__property Htmlrender::TElHTMLImageNeededEvent OnHTMLImageNeeded = {read=GetOnHTMLImageNeeded, write=SetOnHTMLImageNeeded};
	__property Graphics::TColor StripedOddColor = {read=GetStripedOddColor, write=SetStripedOddColor, nodefault};
	__property Graphics::TColor StripedEvenColor = {read=GetStripedEvenColor, write=SetStripedEvenColor, nodefault};
	__property bool StripedItems = {read=GetStripedItems, write=SetStripedItems, nodefault};
	__property Eltree::TElHintType HintType = {read=GetHintType, write=SetHintType, nodefault};
	__property Graphics::TColor VertDivLinesColor = {read=GetVertDivLinesColor, write=SetVertDivLinesColor, nodefault};
	__property bool CloseOnClick = {read=FCloseOnClick, write=FCloseOnClick, default=0};
	__property bool SizeableTree = {read=FSizeableTree, write=SetSizeableTree, default=1};
	__property Elheader::TElHeaderSections* HeaderSections = {read=GetHeaderSections, write=SetHeaderSections};
	__property Controls::TImageList* HeaderImages = {read=GetHeaderImages, write=SetHeaderImages};
	__property int MultiSelectLevel = {read=GetMultiSelectLevel, write=SetMultiSelectLevel, nodefault};
	__property int DragScrollInterval = {read=GetDragScrollInterval, write=SetDragScrollInterval, nodefault};
	__property bool MouseFrameSelect = {read=GetMouseFrameSelect, write=SetMouseFrameSelect, nodefault};
	__property bool ShowLeafButton = {read=GetShowLeafButton, write=SetShowLeafButton, nodefault};
	__property Graphics::TBitmap* LeafPicture = {read=GetLeafPicture, write=SetLeafPicture};
	__property Graphics::TFont* HeaderFont = {read=GetHeaderFont, write=SetHeaderFont};
	__property bool HeaderUseTreeFont = {read=GetHeaderUseTreeFont, write=SetHeaderUseTreeFont, nodefault};
	__property bool AdjustDropDownPos = {read=FAdjustDropDownPos, write=FAdjustDropDownPos, default=1};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElTreeCombo(HWND ParentWindow) : Elbtnedit::TCustomElButtonEdit(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TDropdownElTree : public Eltree::TCustomElTree 
{
	typedef Eltree::TCustomElTree inherited;
	
private:
	TElTreeCombo* FOwner;
	MESSAGE void __fastcall WMMouseActivate(Messages::TMessage &Msg);
	
protected:
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TWMNCHitTest &Msg);
	MESSAGE void __fastcall WMExitSizeMove(Messages::TMessage &Message);
	MESSAGE void __fastcall WMEnterSizeMove(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Message);
	virtual void __fastcall UpdateScrollBars(void);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Message);
	
public:
	DYNAMIC bool __fastcall CanFocus(void);
public:
	#pragma option push -w-inl
	/* TCustomElTree.Create */ inline __fastcall virtual TDropdownElTree(Classes::TComponent* AOwner) : Eltree::TCustomElTree(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElTree.CreateClass */ inline __fastcall TDropdownElTree(Classes::TComponent* AOwner, TMetaClass* ItemClass) : Eltree::TCustomElTree(AOwner, ItemClass) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElTree.Destroy */ inline __fastcall virtual ~TDropdownElTree(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TDropdownElTree(HWND ParentWindow) : Eltree::TCustomElTree(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreecombo */
using namespace Eltreecombo;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeCombo
