// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElFolderDlg.pas' rev: 6.00

#ifndef ElFolderDlgHPP
#define ElFolderDlgHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElShellUtils.hpp>	// Pascal unit
#include <ShlObj.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ActiveX.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------
typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
typedef struct _browseinfoA {
    HWND hwndOwner;
    PItemIDList pidlRoot;
    LPTSTR pszDisplayName;
    LPCTSTR lpszTitle;
    UINT ulFlags;
    BFFCALLBACK lpfn;
    LPARAM lParam;
    int iImage;
} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO;

namespace Elfolderdlg
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TBrowseForFolderOption { bfoFileSysDirsOnly, bfoDontGoBelowDomain, bfoStatusText, bfoFileSysAncestors, bfoBrowseForComputer, bfBrowseForPrinter, bfoBrowseIncludeFiles };
#pragma option pop

typedef Set<TBrowseForFolderOption, bfoFileSysDirsOnly, bfoBrowseIncludeFiles>  TBrowseForFolderOptions;

class DELPHICLASS TElFolderDialog;
class PASCALIMPLEMENTATION TElFolderDialog : public Dialogs::TCommonDialog 
{
	typedef Dialogs::TCommonDialog inherited;
	
private:
	void *FXDefWndProc;
	void *ObjInstance;
	_browseinfoA FBrowseInfo;
	Stdctrls::TButton* FCustBtn;
	AnsiString FCustomButtonCaption;
	AnsiString FDialogTitle;
	char FDisplayName[261];
	AnsiString FFolder;
	_ITEMIDLIST *FFolderPIDL;
	int FHandle;
	Classes::TNotifyEvent FOnChange;
	Classes::TNotifyEvent FOnCustomButtonClick;
	TBrowseForFolderOptions FOptions;
	Controls::TWinControl* FParent;
	Elshellutils::TShellFolders FRootFolder;
	bool FShowCustomButton;
	bool JustInit;
	bool OriginalSelect;
	AnsiString __fastcall GetFolder();
	Controls::TWinControl* __fastcall GetParent(void);
	void __fastcall SetFolder(const AnsiString Value);
	void __fastcall SetParent(Controls::TWinControl* Value);
	void __fastcall SetRootFolder(Elshellutils::TShellFolders Value);
	
protected:
	AnsiString FCustomRootFolder;
	int __fastcall Perform(unsigned Msg, int WParam, int LParam);
	
public:
	__fastcall virtual TElFolderDialog(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFolderDialog(void);
	virtual void __fastcall DefaultHandler(void *Message);
	virtual bool __fastcall Execute(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall SetSelectionPIDL(Shlobj::PItemIDList PIDL);
	void __fastcall WinInitialized(int Param);
	void __fastcall WinSelChanged(int Param);
	__property int Handle = {read=FHandle, nodefault};
	__property Shlobj::PItemIDList SelectionPIDL = {read=FFolderPIDL};
	
__published:
	__property AnsiString DialogTitle = {read=FDialogTitle, write=FDialogTitle};
	__property AnsiString CustomButtonCaption = {read=FCustomButtonCaption, write=FCustomButtonCaption};
	__property AnsiString Folder = {read=GetFolder, write=SetFolder};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Classes::TNotifyEvent OnCustomButtonClick = {read=FOnCustomButtonClick, write=FOnCustomButtonClick};
	__property TBrowseForFolderOptions Options = {read=FOptions, write=FOptions, nodefault};
	__property Controls::TWinControl* Parent = {read=GetParent, write=SetParent};
	__property Elshellutils::TShellFolders RootFolder = {read=FRootFolder, write=SetRootFolder, nodefault};
	__property bool ShowCustomButton = {read=FShowCustomButton, write=FShowCustomButton, nodefault};
	__property AnsiString CustomRootFolder = {read=FCustomRootFolder, write=FCustomRootFolder};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elfolderdlg */
using namespace Elfolderdlg;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElFolderDlg
