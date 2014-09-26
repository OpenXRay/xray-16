// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDBBtnEdit.pas' rev: 6.00

#ifndef ElDBBtnEditHPP
#define ElDBBtnEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ElBtnEdit.hpp>	// Pascal unit
#include <DBCtrls.hpp>	// Pascal unit
#include <DB.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eldbbtnedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElDBButtonEdit;
class PASCALIMPLEMENTATION TElDBButtonEdit : public Elbtnedit::TCustomElButtonEdit 
{
	typedef Elbtnedit::TCustomElButtonEdit inherited;
	
private:
	Dbctrls::TFieldDataLink* FDataLink;
	void __fastcall ActiveChange(System::TObject* Sender);
	void __fastcall DataChange(System::TObject* Sender);
	void __fastcall EditingChange(System::TObject* Sender);
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	void __fastcall ResetMaxLength(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall UpdateData(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall CMGetDataLink(Messages::TMessage &Message);
	
protected:
	virtual void __fastcall Change(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall Loaded(void);
	
public:
	__fastcall virtual TElDBButtonEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBButtonEdit(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	__property Db::TField* Field = {read=GetField};
	__property Text ;
	__property Lines ;
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property AutoSize  = {default=1};
	__property RTLContent ;
	__property BorderSides ;
	__property PasswordChar ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property FlatFocusedScrollBars  = {default=0};
	__property WantTabs  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property ImageForm ;
	__property WordWrap  = {default=1};
	__property ScrollBars  = {default=0};
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Multiline ;
	__property Flat  = {default=0};
	__property ActiveBorderType  = {default=1};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property UseBackground  = {default=0};
	__property Alignment ;
	__property AutoSelect  = {default=0};
	__property Background ;
	__property ButtonCaption ;
	__property ButtonClickSound ;
	__property ButtonDownSound ;
	__property ButtonUpSound ;
	__property ButtonSoundMap ;
	__property ButtonColor ;
	__property ButtonDown ;
	__property ButtonEnabled ;
	__property ButtonFlat ;
	__property ButtonGlyph ;
	__property ButtonHint ;
	__property ButtonIcon ;
	__property ButtonNumGlyphs ;
	__property ButtonPopupPlace ;
	__property ButtonPullDownMenu ;
	__property ButtonShortcut ;
	__property ButtonUseIcon ;
	__property ButtonVisible ;
	__property ButtonWidth ;
	__property OnButtonClick ;
	__property AltButtonCaption ;
	__property AltButtonClickSound ;
	__property AltButtonDownSound ;
	__property AltButtonUpSound ;
	__property AltButtonSoundMap ;
	__property AltButtonColor ;
	__property AltButtonDown ;
	__property AltButtonEnabled ;
	__property AltButtonFlat ;
	__property AltButtonGlyph ;
	__property AltButtonHint ;
	__property AltButtonIcon ;
	__property AltButtonNumGlyphs ;
	__property AltButtonPopupPlace ;
	__property AltButtonPosition  = {default=1};
	__property AltButtonPullDownMenu ;
	__property AltButtonShortcut ;
	__property AltButtonUseIcon ;
	__property AltButtonVisible ;
	__property AltButtonWidth ;
	__property OnAltButtonClick ;
	__property BorderStyle ;
	__property Ctl3D ;
	__property ParentCtl3D  = {default=1};
	__property Enabled  = {default=1};
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property PopupMenu ;
	__property Color  = {default=-2147483643};
	__property ParentColor  = {default=1};
	__property Align  = {default=0};
	__property Font ;
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property ReadOnly  = {default=0};
	__property OnEnter ;
	__property OnExit ;
	__property OnClick ;
	__property OnDblClick ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDock ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDock ;
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBButtonEdit(HWND ParentWindow) : Elbtnedit::TCustomElButtonEdit(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldbbtnedit */
using namespace Eldbbtnedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDBBtnEdit
