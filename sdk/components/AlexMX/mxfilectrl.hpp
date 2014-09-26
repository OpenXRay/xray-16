// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxFileCtrl.pas' rev: 6.00

#ifndef mxFileCtrlHPP
#define mxFileCtrlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Buttons.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxfilectrl
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TFileAttr { ftReadOnly, ftHidden, ftSystem, ftVolumeID, ftDirectory, ftArchive, ftNormal };
#pragma option pop

typedef Set<TFileAttr, ftReadOnly, ftNormal>  TFileType;

#pragma option push -b-
enum TDriveType { dtUnknown, dtNoDrive, dtFloppy, dtFixed, dtNetwork, dtCDROM, dtRAM };
#pragma option pop

class DELPHICLASS TFileListBox;
class DELPHICLASS TDirectoryListBox;
class DELPHICLASS TDriveComboBox;
#pragma option push -b-
enum TTextCase { tcLowerCase, tcUpperCase };
#pragma option pop

class PASCALIMPLEMENTATION TDriveComboBox : public Stdctrls::TCustomComboBox 
{
	typedef Stdctrls::TCustomComboBox inherited;
	
private:
	TDirectoryListBox* FDirList;
	char FDrive;
	TTextCase FTextCase;
	void __fastcall SetDirListBox(TDirectoryListBox* Value);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	void __fastcall SetDrive(char NewDrive);
	void __fastcall SetTextCase(TTextCase NewTextCase);
	void __fastcall ReadBitmaps(void);
	void __fastcall ResetItemHeight(void);
	
protected:
	Graphics::TBitmap* FloppyBMP;
	Graphics::TBitmap* FixedBMP;
	Graphics::TBitmap* NetworkBMP;
	Graphics::TBitmap* CDROMBMP;
	Graphics::TBitmap* RAMBMP;
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	DYNAMIC void __fastcall Click(void);
	virtual void __fastcall BuildList(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TDriveComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TDriveComboBox(void);
	__property Text ;
	__property char Drive = {read=FDrive, write=SetDrive, nodefault};
	
__published:
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property TDirectoryListBox* DirList = {read=FDirList, write=SetDirListBox};
	__property DragMode  = {default=0};
	__property DragCursor  = {default=-12};
	__property Enabled  = {default=1};
	__property Font ;
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property TTextCase TextCase = {read=FTextCase, write=SetTextCase, default=0};
	__property Visible  = {default=1};
	__property OnChange ;
	__property OnClick ;
	__property OnContextPopup ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnDropDown ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TDriveComboBox(HWND ParentWindow) : Stdctrls::TCustomComboBox(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TDirectoryListBox : public Stdctrls::TCustomListBox 
{
	typedef Stdctrls::TCustomListBox inherited;
	
private:
	TFileListBox* FFileList;
	TDriveComboBox* FDriveCombo;
	Stdctrls::TLabel* FDirLabel;
	bool FInSetDir;
	bool FPreserveCase;
	bool FCaseSensitive;
	char __fastcall GetDrive(void);
	void __fastcall SetFileListBox(TFileListBox* Value);
	void __fastcall SetDirLabel(Stdctrls::TLabel* Value);
	void __fastcall SetDirLabelCaption(void);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	void __fastcall SetDrive(char Value);
	void __fastcall DriveChange(char NewDrive);
	void __fastcall SetDir(const AnsiString NewDirectory);
	virtual void __fastcall SetDirectory(const AnsiString NewDirectory);
	void __fastcall ResetItemHeight(void);
	
protected:
	Graphics::TBitmap* ClosedBMP;
	Graphics::TBitmap* OpenedBMP;
	Graphics::TBitmap* CurrentBMP;
	AnsiString FDirectory;
	Classes::TNotifyEvent FOnChange;
	virtual void __fastcall Change(void);
	DYNAMIC void __fastcall DblClick(void);
	virtual void __fastcall ReadBitmaps(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	int __fastcall ReadDirectoryNames(const AnsiString ParentDirectory, Classes::TStringList* DirectoryList);
	virtual void __fastcall BuildList(void);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TDirectoryListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TDirectoryListBox(void);
	AnsiString __fastcall DisplayCase(const AnsiString S);
	int __fastcall FileCompareText(const AnsiString A, const AnsiString B);
	AnsiString __fastcall GetItemPath(int Index);
	void __fastcall OpenCurrent(void);
	HIDESBASE void __fastcall Update(void);
	__property char Drive = {read=GetDrive, write=SetDrive, nodefault};
	__property AnsiString Directory = {read=FDirectory, write=SetDirectory};
	__property bool PreserveCase = {read=FPreserveCase, nodefault};
	__property bool CaseSensitive = {read=FCaseSensitive, nodefault};
	
__published:
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Columns  = {default=0};
	__property Constraints ;
	__property Ctl3D ;
	__property Stdctrls::TLabel* DirLabel = {read=FDirLabel, write=SetDirLabel};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property TFileListBox* FileList = {read=FFileList, write=SetFileListBox};
	__property Font ;
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property IntegralHeight  = {default=0};
	__property ItemHeight ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property OnClick ;
	__property OnContextPopup ;
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
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TDirectoryListBox(HWND ParentWindow) : Stdctrls::TCustomListBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TFilterComboBox;
class PASCALIMPLEMENTATION TFilterComboBox : public Stdctrls::TCustomComboBox 
{
	typedef Stdctrls::TCustomComboBox inherited;
	
private:
	AnsiString FFilter;
	TFileListBox* FFileList;
	Classes::TStringList* MaskList;
	bool __fastcall IsFilterStored(void);
	AnsiString __fastcall GetMask();
	void __fastcall SetFilter(const AnsiString NewFilter);
	void __fastcall SetFileListBox(TFileListBox* Value);
	
protected:
	DYNAMIC void __fastcall Change(void);
	virtual void __fastcall CreateWnd(void);
	DYNAMIC void __fastcall Click(void);
	void __fastcall BuildList(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TFilterComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TFilterComboBox(void);
	__property AnsiString Mask = {read=GetMask};
	__property Text ;
	
__published:
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property DragMode  = {default=0};
	__property DragCursor  = {default=-12};
	__property Enabled  = {default=1};
	__property TFileListBox* FileList = {read=FFileList, write=SetFileListBox};
	__property AnsiString Filter = {read=FFilter, write=SetFilter, stored=IsFilterStored};
	__property Font ;
	__property ImeName ;
	__property ImeMode  = {default=3};
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property OnChange ;
	__property OnClick ;
	__property OnContextPopup ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnDropDown ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TFilterComboBox(HWND ParentWindow) : Stdctrls::TCustomComboBox(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TFileListBox : public Stdctrls::TCustomListBox 
{
	typedef Stdctrls::TCustomListBox inherited;
	
private:
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	char __fastcall GetDrive(void);
	AnsiString __fastcall GetFileName();
	bool __fastcall IsMaskStored(void);
	void __fastcall SetDrive(char Value);
	void __fastcall SetFileEdit(Stdctrls::TEdit* Value);
	void __fastcall SetDirectory(const AnsiString NewDirectory);
	void __fastcall SetFileType(TFileType NewFileType);
	void __fastcall SetMask(const AnsiString NewMask);
	void __fastcall SetFileName(const AnsiString NewFile);
	void __fastcall SetShowGlyphs(bool Value);
	void __fastcall ResetItemHeight(void);
	
protected:
	AnsiString FDirectory;
	AnsiString FMask;
	TFileType FFileType;
	Stdctrls::TEdit* FFileEdit;
	TDirectoryListBox* FDirList;
	TFilterComboBox* FFilterCombo;
	Graphics::TBitmap* ExeBMP;
	Graphics::TBitmap* DirBMP;
	Graphics::TBitmap* UnknownBMP;
	Classes::TNotifyEvent FOnChange;
	int FLastSel;
	bool FShowGlyphs;
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall ReadBitmaps(void);
	DYNAMIC void __fastcall Click(void);
	virtual void __fastcall Change(void);
	virtual void __fastcall ReadFileNames(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual AnsiString __fastcall GetFilePath();
	
public:
	__fastcall virtual TFileListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TFileListBox(void);
	HIDESBASE void __fastcall Update(void);
	virtual void __fastcall ApplyFilePath(const AnsiString EditText);
	__property char Drive = {read=GetDrive, write=SetDrive, nodefault};
	__property AnsiString Directory = {read=FDirectory, write=ApplyFilePath};
	__property AnsiString FileName = {read=GetFilePath, write=ApplyFilePath};
	
__published:
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property ExtendedSelect  = {default=1};
	__property Stdctrls::TEdit* FileEdit = {read=FFileEdit, write=SetFileEdit};
	__property TFileType FileType = {read=FFileType, write=SetFileType, default=64};
	__property Font ;
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property IntegralHeight  = {default=0};
	__property ItemHeight ;
	__property AnsiString Mask = {read=FMask, write=SetMask, stored=IsMaskStored};
	__property MultiSelect  = {default=0};
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property bool ShowGlyphs = {read=FShowGlyphs, write=SetShowGlyphs, default=0};
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property OnClick ;
	__property OnContextPopup ;
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
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TFileListBox(HWND ParentWindow) : Stdctrls::TCustomListBox(ParentWindow) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TSelectDirOpt { sdAllowCreate, sdPerformCreate, sdPrompt };
#pragma option pop

typedef Set<TSelectDirOpt, sdAllowCreate, sdPrompt>  TSelectDirOpts;

//-- var, const, procedure ---------------------------------------------------
static const Shortint WNTYPE_DRIVE = 0x1;
extern PACKAGE AnsiString __fastcall MinimizeName(const AnsiString Filename, Graphics::TCanvas* Canvas, int MaxLen);
extern PACKAGE void __fastcall ProcessPath(const AnsiString EditText, char &Drive, AnsiString &DirPart, AnsiString &FilePart);
extern PACKAGE bool __fastcall SelectDirectory(AnsiString &Directory, TSelectDirOpts Options, int HelpCtx)/* overload */;
extern PACKAGE bool __fastcall SelectDirectory(const AnsiString Caption, const WideString Root, /* out */ AnsiString &Directory)/* overload */;
extern PACKAGE bool __fastcall DirectoryExists(const AnsiString Name);
extern PACKAGE bool __fastcall ForceDirectories(AnsiString Dir);

}	/* namespace Mxfilectrl */
using namespace Mxfilectrl;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxFileCtrl
