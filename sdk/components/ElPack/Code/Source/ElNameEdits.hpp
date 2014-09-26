// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElNameEdits.pas' rev: 6.00

#ifndef ElNameEditsHPP
#define ElNameEditsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <CommDlg.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ElBtnEdit.hpp>	// Pascal unit
#include <ElShellUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <ElFolderDlg.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elnameedits
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElFolderNameEdit;
class PASCALIMPLEMENTATION TElFolderNameEdit : public Elbtnedit::TCustomElButtonEdit 
{
	typedef Elbtnedit::TCustomElButtonEdit inherited;
	
private:
	Elfolderdlg::TElFolderDialog* FileDlg;
	Elfolderdlg::TBrowseForFolderOptions __fastcall GetOptions(void);
	AnsiString __fastcall GetTitle();
	void __fastcall SetOptions(Elfolderdlg::TBrowseForFolderOptions Value);
	void __fastcall SetTitle(const AnsiString Value);
	void __fastcall SetRootFolder(Elshellutils::TShellFolders Value);
	Elshellutils::TShellFolders __fastcall GetRootFolder(void);
	
protected:
	AnsiString FDialogTitle;
	void __fastcall BtnClick(System::TObject* Sender);
	virtual void __fastcall CreateHandle(void);
	virtual void __fastcall Loaded(void);
	
public:
	__fastcall virtual TElFolderNameEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFolderNameEdit(void);
	
__published:
	__property Elfolderdlg::TBrowseForFolderOptions Options = {read=GetOptions, write=SetOptions, nodefault};
	__property AnsiString Title = {read=GetTitle, write=SetTitle};
	__property Elshellutils::TShellFolders RootFolder = {read=GetRootFolder, write=SetRootFolder, nodefault};
	__property AnsiString DialogTitle = {read=FDialogTitle, write=FDialogTitle};
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property BorderSides ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property ImageForm ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Text ;
	__property Flat  = {default=0};
	__property ActiveBorderType  = {default=1};
	__property InactiveBorderType  = {default=3};
	__property UseBackground  = {default=0};
	__property Alignment ;
	__property AutoSelect  = {default=0};
	__property Multiline ;
	__property Background ;
	__property ButtonClickSound ;
	__property ButtonDownSound ;
	__property ButtonUpSound ;
	__property ButtonSoundMap ;
	__property ButtonColor ;
	__property ButtonFlat ;
	__property ButtonHint ;
	__property ButtonShortcut ;
	__property ButtonGlyph ;
	__property ButtonIcon ;
	__property ButtonUseIcon ;
	__property ButtonNumGlyphs ;
	__property ButtonWidth ;
	__property AltButtonCaption ;
	__property AltButtonClickSound ;
	__property AltButtonDownSound ;
	__property AltButtonUpSound ;
	__property AltButtonSoundMap ;
	__property AltButtonDown ;
	__property AltButtonEnabled ;
	__property AltButtonFlat ;
	__property AltButtonGlyph ;
	__property AltButtonHint ;
	__property AltButtonIcon ;
	__property AltButtonUseIcon ;
	__property AltButtonNumGlyphs ;
	__property AltButtonPopupPlace ;
	__property AltButtonPosition  = {default=1};
	__property AltButtonPullDownMenu ;
	__property AltButtonShortcut ;
	__property AltButtonVisible ;
	__property AltButtonWidth ;
	__property OnAltButtonClick ;
	__property AutoSize  = {default=1};
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
	/* TWinControl.CreateParented */ inline __fastcall TElFolderNameEdit(HWND ParentWindow) : Elbtnedit::TCustomElButtonEdit(ParentWindow) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TElFileDialogType { fdtOpen, fdtSave };
#pragma option pop

class DELPHICLASS TElFileNameEdit;
class PASCALIMPLEMENTATION TElFileNameEdit : public Elbtnedit::TCustomElButtonEdit 
{
	typedef Elbtnedit::TCustomElButtonEdit inherited;
	
private:
	Dialogs::TOpenDialog* FileDlg;
	int __fastcall GetFilterIndex(void);
	void __fastcall SetHistoryList(Classes::TStrings* Value);
	void __fastcall SetInitialDir(const AnsiString Value);
	AnsiString __fastcall GetDefaultExt();
	void __fastcall SetDefaultExt(const AnsiString Value);
	Classes::TStrings* __fastcall GetFiles(void);
	Classes::TStrings* __fastcall GetHistoryList(void);
	AnsiString __fastcall GetInitialDir();
	Dialogs::TOpenOptions __fastcall GetOptions(void);
	void __fastcall SetOptions(Dialogs::TOpenOptions Value);
	void __fastcall SetFilterIndex(int Value);
	AnsiString __fastcall GetTitle();
	void __fastcall SetTitle(const AnsiString Value);
	AnsiString __fastcall GetFilter();
	void __fastcall SetFilter(const AnsiString Value);
	
protected:
	AnsiString FDialogTitle;
	TElFileDialogType FDialogType;
	void __fastcall BtnClick(System::TObject* Sender);
	virtual void __fastcall CreateHandle(void);
	virtual void __fastcall Loaded(void);
	
public:
	__fastcall virtual TElFileNameEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFileNameEdit(void);
	__property Classes::TStrings* Files = {read=GetFiles};
	__property Classes::TStrings* HistoryList = {read=GetHistoryList, write=SetHistoryList};
	
__published:
	__property AnsiString DefaultExt = {read=GetDefaultExt, write=SetDefaultExt};
	__property AnsiString Filter = {read=GetFilter, write=SetFilter};
	__property int FilterIndex = {read=GetFilterIndex, write=SetFilterIndex, default=1};
	__property AnsiString InitialDir = {read=GetInitialDir, write=SetInitialDir};
	__property Dialogs::TOpenOptions Options = {read=GetOptions, write=SetOptions, default=524292};
	__property AnsiString Title = {read=GetTitle, write=SetTitle};
	__property AnsiString DialogTitle = {read=FDialogTitle, write=FDialogTitle};
	__property TElFileDialogType DialogType = {read=FDialogType, write=FDialogType, nodefault};
	__property TopMargin  = {default=1};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property BorderSides ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property ImageForm ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Text ;
	__property Flat  = {default=0};
	__property ActiveBorderType  = {default=1};
	__property InactiveBorderType  = {default=3};
	__property UseBackground  = {default=0};
	__property Alignment ;
	__property AutoSelect  = {default=0};
	__property Multiline ;
	__property Background ;
	__property ButtonClickSound ;
	__property ButtonDownSound ;
	__property ButtonUpSound ;
	__property ButtonSoundMap ;
	__property ButtonColor ;
	__property ButtonFlat ;
	__property ButtonHint ;
	__property ButtonShortcut ;
	__property ButtonGlyph ;
	__property ButtonIcon ;
	__property ButtonUseIcon ;
	__property ButtonNumGlyphs ;
	__property ButtonWidth ;
	__property AltButtonCaption ;
	__property AltButtonClickSound ;
	__property AltButtonDownSound ;
	__property AltButtonUpSound ;
	__property AltButtonSoundMap ;
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
	__property AutoSize  = {default=1};
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
	/* TWinControl.CreateParented */ inline __fastcall TElFileNameEdit(HWND ParentWindow) : Elbtnedit::TCustomElButtonEdit(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elnameedits */
using namespace Elnameedits;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElNameEdits
