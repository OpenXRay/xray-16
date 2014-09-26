// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDBCtrls.pas' rev: 6.00

#ifndef ElDBCtrlsHPP
#define ElDBCtrlsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElScrollBar.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElGroupBox.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Mask.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElPromptDlg.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElCheckItemGrp.hpp>	// Pascal unit
#include <ElCheckCtl.hpp>	// Pascal unit
#include <ElMaskEdit.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElToolbar.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <DBCtrls.hpp>	// Pascal unit
#include <DB.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eldbctrls
{
//-- type declarations -------------------------------------------------------
typedef TElWideStrings TElFStrings;
;

typedef TElWideStringList TElFStringList;
;

class DELPHICLASS TElDBEdit;
class PASCALIMPLEMENTATION TElDBEdit : public Elmaskedit::TCustomElMaskEdit 
{
	typedef Elmaskedit::TCustomElMaskEdit inherited;
	
private:
	Dbctrls::TFieldDataLink* FDataLink;
	bool FFocused;
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall ResetMaxLength(void);
	void __fastcall DataChange(System::TObject* Sender);
	void __fastcall ActiveChange(System::TObject* Sender);
	void __fastcall EditingChange(System::TObject* Sender);
	void __fastcall UpdateData(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Message);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall CMGetDataLink(Messages::TMessage &Message);
	void __fastcall SetFocused(bool Value);
	bool __fastcall GetReadOnly(void);
	HIDESBASE void __fastcall SetReadOnly(bool Value);
	HIDESBASE MESSAGE void __fastcall WMCut(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMPaste(Messages::TMessage &Message);
	MESSAGE void __fastcall WMUndo(Messages::TMessage &Message);
	
protected:
	DYNAMIC void __fastcall Change(void);
	virtual bool __fastcall EditCanModify(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall Reset(void);
	
public:
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	__fastcall virtual TElDBEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBEdit(void);
	__property Db::TField* Field = {read=GetField};
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property ActiveBorderType  = {default=1};
	__property Alignment  = {default=0};
	__property Background ;
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property Transparent  = {default=0};
	__property UseBackground  = {default=0};
	__property BorderSides ;
	__property HandleDialogKeys  = {default=1};
	__property ImageForm ;
	__property UseXPThemes  = {default=1};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property AutoSelect  = {default=1};
	__property AutoSize  = {default=1};
	__property BiDiMode ;
	__property BorderStyle  = {default=1};
	__property CharCase  = {default=0};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Cursor  = {default=0};
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property HideSelection  = {default=1};
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property MaxLength  = {default=0};
	__property OEMConvert  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PasswordChar  = {default=0};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property Visible  = {default=1};
	__property OnChange ;
	__property OnClick ;
	__property OnContextPopup ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDock ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDock ;
	__property OnStartDrag ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBEdit(HWND ParentWindow) : Elmaskedit::TCustomElMaskEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElDBMemo;
class PASCALIMPLEMENTATION TElDBMemo : public Elactrls::TElAdvancedMemo 
{
	typedef Elactrls::TElAdvancedMemo inherited;
	
private:
	bool FAutoDisplay;
	Dbctrls::TFieldDataLink* FDataLink;
	bool FFocused;
	bool FMemoLoaded;
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall SetAutoDisplay(bool Value);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Message);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall CMGetDataLink(Messages::TMessage &Message);
	void __fastcall DataChange(System::TObject* Sender);
	void __fastcall EditingChange(System::TObject* Sender);
	bool __fastcall GetReadOnly(void);
	void __fastcall SetFocused(bool Value);
	HIDESBASE void __fastcall SetReadOnly(bool Value);
	void __fastcall UpdateData(System::TObject* Sender);
	MESSAGE void __fastcall WMCut(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Message);
	MESSAGE void __fastcall WMPaste(Messages::TMessage &Message);
	MESSAGE void __fastcall WMUndo(Messages::TMessage &Message);
	
protected:
	DYNAMIC void __fastcall Change(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	virtual void __fastcall LoadMemo(void);
	__fastcall virtual TElDBMemo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBMemo(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	__property Db::TField* Field = {read=GetField};
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property bool AutoDisplay = {read=FAutoDisplay, write=SetAutoDisplay, default=1};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBMemo(HWND ParentWindow) : Elactrls::TElAdvancedMemo(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElDBCheckBox;
class PASCALIMPLEMENTATION TElDBCheckBox : public Elcheckctl::TElCheckBox 
{
	typedef Elcheckctl::TElCheckBox inherited;
	
private:
	Dbctrls::TFieldDataLink* FDataLink;
	WideString FValueCheck;
	WideString FValueUncheck;
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	bool __fastcall GetReadOnly(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall SetReadOnly(bool Value);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall CMGetDataLink(Messages::TMessage &Message);
	void __fastcall DataChange(System::TObject* Sender);
	Stdctrls::TCheckBoxState __fastcall GetFieldState(void);
	void __fastcall SetValueCheck(const WideString Value);
	void __fastcall SetValueUncheck(const WideString Value);
	void __fastcall UpdateData(System::TObject* Sender);
	bool __fastcall ValueMatch(const AnsiString ValueList, const AnsiString Value);
	
protected:
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall Toggle(void);
	
public:
	__fastcall virtual TElDBCheckBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBCheckBox(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	DYNAMIC void __fastcall Click(void);
	__property Db::TField* Field = {read=GetField};
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property WideString ValueChecked = {read=FValueCheck, write=SetValueCheck};
	__property WideString ValueUnchecked = {read=FValueUncheck, write=SetValueUncheck};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBCheckBox(HWND ParentWindow) : Elcheckctl::TElCheckBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElDBRadioGroup;
class PASCALIMPLEMENTATION TElDBRadioGroup : public Elcheckitemgrp::TCustomElRadioGroup 
{
	typedef Elcheckitemgrp::TCustomElRadioGroup inherited;
	
private:
	Dbctrls::TFieldDataLink* FDataLink;
	Classes::TNotifyEvent FOnChange;
	WideString FValue;
	Elunicodestrings::TElWideStrings* FValues;
	bool FInSetValue;
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	void __fastcall DataChange(System::TObject* Sender);
	WideString __fastcall GetButtonValue(int Index);
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	bool __fastcall GetReadOnly(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	HIDESBASE void __fastcall SetItems(Elunicodestrings::TElWideStrings* Value);
	void __fastcall SetReadOnly(bool Value);
	void __fastcall SetValue(WideString Value);
	void __fastcall SetValues(Elunicodestrings::TElWideStrings* Value);
	void __fastcall UpdateData(System::TObject* Sender);
	
protected:
	virtual bool __fastcall CanModify(void);
	DYNAMIC void __fastcall Change(void);
	DYNAMIC void __fastcall Click(void);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	__property Dbctrls::TFieldDataLink* DataLink = {read=FDataLink};
	
public:
	__fastcall virtual TElDBRadioGroup(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBRadioGroup(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	__property Db::TField* Field = {read=GetField};
	__property WideString Value = {read=FValue, write=SetValue};
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Elunicodestrings::TElWideStrings* Values = {read=FValues, write=SetValues};
	__property Items  = {write=SetItems};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property Align  = {default=0};
	__property Alignment  = {default=1};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property OnEndDock ;
	__property OnStartDock ;
	__property BorderSides ;
	__property Caption ;
	__property CaptionColor  = {default=536870911};
	__property CheckBoxChecked  = {default=1};
	__property Color  = {default=-2147483633};
	__property Columns  = {default=1};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Flat ;
	__property FlatAlways ;
	__property Font ;
	__property Hints ;
	__property ImageForm ;
	__property IsHTML  = {default=0};
	__property MoneyFlat  = {default=0};
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatActiveColor ;
	__property MoneyFlatDownColor ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowCheckBox  = {default=0};
	__property ShowFocus ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property CheckSound ;
	__property SoundMap ;
	__property Glyph ;
	__property Images ;
	__property UseCustomGlyphs  = {default=0};
	__property UseImageList  = {default=0};
	__property OnClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnStartDrag ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBRadioGroup(HWND ParentWindow) : Elcheckitemgrp::TCustomElRadioGroup(ParentWindow) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TElNavButtonRole { nbrFirst, nbrPrior, nbrNext, nbrLast, nbrInsert, nbrDelete, nbrEdit, nbrPost, nbrCancel, nbrRefresh, nbrSearch, nbrSetFilter, nbrRemoveFilter, nbrClear, nbrOpen, nbrClose, nbrFindFirst, nbrFindPrior, nbrFindNext, nbrFindLast, nbrCustom };
#pragma option pop

class DELPHICLASS TElDBNavButton;
class PASCALIMPLEMENTATION TElDBNavButton : public Eltoolbar::TCustomElToolButton 
{
	typedef Eltoolbar::TCustomElToolButton inherited;
	
protected:
	TElNavButtonRole FRole;
	void __fastcall SetRole(TElNavButtonRole Value);
	virtual WideString __fastcall GetArrowThemedClassName();
	virtual int __fastcall GetArrowThemePartID(void);
	virtual int __fastcall GetArrowThemeStateID(void);
	virtual WideString __fastcall GetThemedClassName();
	virtual int __fastcall GetThemePartID(void);
	virtual int __fastcall GetThemeStateID(void);
	virtual void __fastcall SetUseImageList(bool newValue);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall SetImageList(Controls::TImageList* newValue);
	virtual void __fastcall SetImageIndex(int newValue);
	
public:
	virtual void __fastcall AClick(bool Arrow);
	__fastcall virtual TElDBNavButton(Classes::TComponent* AOwner);
	
__published:
	__property TElNavButtonRole Role = {read=FRole, write=SetRole, default=20};
	__property Wrap ;
	__property LargeGlyph ;
	__property NumLargeGlyphs ;
	__property Glyph ;
	__property NumGlyphs ;
	__property OwnerSettings  = {default=1};
	__property PullDownMenu ;
	__property PopupPlace  = {default=0};
	__property DisableAutoPopup  = {default=0};
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
	/* TCustomElToolButton.Destroy */ inline __fastcall virtual ~TElDBNavButton(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElDBNavigator;
class DELPHICLASS TElNavDataLink;
class PASCALIMPLEMENTATION TElNavDataLink : public Db::TDataLink 
{
	typedef Db::TDataLink inherited;
	
private:
	TElDBNavigator* FNavigator;
	
protected:
	virtual void __fastcall EditingChanged(void);
	virtual void __fastcall DataSetChanged(void);
	virtual void __fastcall ActiveChanged(void);
	
public:
	__fastcall TElNavDataLink(TElDBNavigator* ANav);
	__fastcall virtual ~TElNavDataLink(void);
};


class PASCALIMPLEMENTATION TElDBNavigator : public Eltoolbar::TElToolBar 
{
	typedef Eltoolbar::TElToolBar inherited;
	
private:
	AnsiString FDeleteRecordQuestion;
	TElNavDataLink* FDataLink;
	Db::TDataSource* __fastcall GetDataSource(void);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	
protected:
	bool FConfirmDelete;
	Classes::TNotifyEvent FOnSearch;
	bool FIsToolbar;
	Controls::TImageList* FIntImageList;
	void __fastcall ActiveChanged(void);
	void __fastcall DataChanged(void);
	void __fastcall EditingChanged(void);
	virtual void __fastcall DoSearch(void);
	virtual TMetaClass* __fastcall GetButtonClass(void);
	void __fastcall SetIsToolbar(bool Value);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall Loaded(void);
	HIDESBASE MESSAGE void __fastcall CMControlChange(Controls::TCMControlChange &Msg);
	
public:
	__fastcall virtual TElDBNavigator(Classes::TComponent* AOwner);
	TElDBNavButton* __fastcall FindButtonByRole(TElNavButtonRole Role);
	HIDESBASE TElDBNavButton* __fastcall AddButton(TElNavButtonRole Role);
	__fastcall virtual ~TElDBNavigator(void);
	
__published:
	__property AnsiString DeleteRecordQuestion = {read=FDeleteRecordQuestion, write=FDeleteRecordQuestion};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property bool ConfirmDelete = {read=FConfirmDelete, write=FConfirmDelete, default=1};
	__property Classes::TNotifyEvent OnSearch = {read=FOnSearch, write=FOnSearch};
	__property bool IsToolbar = {read=FIsToolbar, write=SetIsToolbar, default=1};
	__property BtnOffsHorz  = {default=3};
	__property BtnOffsVert  = {default=3};
	__property BevelOuter  = {default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBNavigator(HWND ParentWindow) : Eltoolbar::TElToolBar(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElWideDBEdit;
class PASCALIMPLEMENTATION TElWideDBEdit : public Eledits::TCustomElEdit 
{
	typedef Eledits::TCustomElEdit inherited;
	
private:
	Dbctrls::TFieldDataLink* FDataLink;
	bool FFocused;
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall ResetMaxLength(void);
	void __fastcall DataChange(System::TObject* Sender);
	void __fastcall ActiveChange(System::TObject* Sender);
	void __fastcall EditingChange(System::TObject* Sender);
	void __fastcall UpdateData(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Message);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall CMGetDataLink(Messages::TMessage &Message);
	void __fastcall SetFocused(bool Value);
	bool __fastcall GetReadOnly(void);
	HIDESBASE void __fastcall SetReadOnly(bool Value);
	HIDESBASE MESSAGE void __fastcall WMCut(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMPaste(Messages::TMessage &Message);
	MESSAGE void __fastcall WMUndo(Messages::TMessage &Message);
	
protected:
	virtual void __fastcall Change(void);
	bool __fastcall EditCanModify(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall Reset(void);
	
public:
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	__fastcall virtual TElWideDBEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElWideDBEdit(void);
	__property Db::TField* Field = {read=GetField};
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property AutoSize  = {default=1};
	__property Alignment ;
	__property Background ;
	__property BorderSides ;
	__property CharCase  = {default=0};
	__property UseBackground  = {default=0};
	__property RTLContent ;
	__property PasswordChar ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property FlatFocusedScrollBars  = {default=0};
	__property WantTabs  = {default=0};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property TopMargin  = {default=1};
	__property BorderStyle ;
	__property AutoSelect  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property Lines  = {stored=false};
	__property Text ;
	__property ImageForm ;
	__property ActiveBorderType  = {default=1};
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property WordWrap  = {default=1};
	__property ScrollBars  = {default=0};
	__property VertScrollBarStyles ;
	__property HorzScrollBarStyles ;
	__property UseCustomScrollBars ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property BiDiMode ;
	__property Cursor  = {default=0};
	__property ImeMode  = {default=3};
	__property ImeName ;
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
	__property OnStartDrag ;
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElWideDBEdit(HWND ParentWindow) : Eledits::TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElWideDBMemo;
class PASCALIMPLEMENTATION TElWideDBMemo : public Eledits::TCustomElEdit 
{
	typedef Eledits::TCustomElEdit inherited;
	
private:
	bool FAutoDisplay;
	Dbctrls::TFieldDataLink* FDataLink;
	bool FFocused;
	bool FMemoLoaded;
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall SetAutoDisplay(bool Value);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Message);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall CMGetDataLink(Messages::TMessage &Message);
	void __fastcall DataChange(System::TObject* Sender);
	void __fastcall EditingChange(System::TObject* Sender);
	bool __fastcall GetReadOnly(void);
	void __fastcall SetFocused(bool Value);
	HIDESBASE void __fastcall SetReadOnly(bool Value);
	void __fastcall UpdateData(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall WMCut(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMPaste(Messages::TMessage &Message);
	MESSAGE void __fastcall WMUndo(Messages::TMessage &Message);
	
protected:
	virtual void __fastcall Change(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	virtual void __fastcall LoadMemo(void);
	__fastcall virtual TElWideDBMemo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElWideDBMemo(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	__property Db::TField* Field = {read=GetField};
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property bool AutoDisplay = {read=FAutoDisplay, write=SetAutoDisplay, default=1};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property AutoSize  = {default=1};
	__property Alignment ;
	__property Background ;
	__property BorderSides ;
	__property CharCase  = {default=0};
	__property UseBackground  = {default=0};
	__property RTLContent ;
	__property PasswordChar ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property FlatFocusedScrollBars  = {default=0};
	__property WantTabs  = {default=0};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property TopMargin  = {default=1};
	__property BorderStyle ;
	__property AutoSelect  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property Lines  = {stored=false};
	__property Text ;
	__property ImageForm ;
	__property ActiveBorderType  = {default=1};
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property WordWrap  = {default=1};
	__property ScrollBars  = {default=0};
	__property VertScrollBarStyles ;
	__property HorzScrollBarStyles ;
	__property UseCustomScrollBars ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property BiDiMode ;
	__property Cursor  = {default=0};
	__property ImeMode  = {default=3};
	__property ImeName ;
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
	__property OnStartDrag ;
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElWideDBMemo(HWND ParentWindow) : Eledits::TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldbctrls */
using namespace Eldbctrls;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDBCtrls
