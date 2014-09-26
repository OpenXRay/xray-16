// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElShellUtils.pas' rev: 6.00

#ifndef ElShellUtilsHPP
#define ElShellUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElStrToken.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <CommCtrl.hpp>	// Pascal unit
#include <ShlObj.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Registry.hpp>	// Pascal unit
#include <ShellAPI.hpp>	// Pascal unit
#include <ActiveX.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------
typedef UNALIGNED _ITEMIDLIST * LPITEMIDLIST;
typedef const UNALIGNED _ITEMIDLIST * LPCITEMIDLIST;
typedef struct _STRRET
{
    UINT uType; // One of the STRRET_* values
    union
    {
        LPWSTR          pOleStr;        // must be freed by caller of GetDisplayNameOf
        LPSTR           pStr;           // NOT USED
        UINT            uOffset;        // Offset into SHITEMID
        char            cStr[MAX_PATH]; // Buffer to fill in (ANSI)
    } DUMMYUNIONNAME;
} STRRET, *LPSTRRET;
typedef struct {
    GUID fmtid;
    DWORD pid;
} SHCOLUMNID, *LPSHCOLUMNID;
typedef const SHCOLUMNID* LPCSHCOLUMNID;
typedef SHCOLUMNID *PShColumnID;
typedef struct _SHELLDETAILS
{
    int     fmt;            // LVCFMT_* value (header only)
    int     cxChar;         // Number of "average" characters (header only)
    STRRET  str;            // String information
} SHELLDETAILS, *LPSHELLDETAILS;
typedef _SHELLDETAILS *PShellDetails;
typedef _SHELLDETAILS  TShellDetails;
DECLARE_INTERFACE_(IShellFolder, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellFolder methods ***
    STDMETHOD(ParseDisplayName)(THIS_ HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
                                ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes) PURE;

    STDMETHOD(EnumObjects)(THIS_ HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList) PURE;

    STDMETHOD(BindToObject)(THIS_ LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv) PURE;
    STDMETHOD(BindToStorage)(THIS_ LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv) PURE;
    STDMETHOD(CompareIDs)(THIS_ LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject)(THIS_ HWND hwndOwner, REFIID riid, void **ppv) PURE;
    STDMETHOD(GetAttributesOf)(THIS_ UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)(THIS_ HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                             REFIID riid, UINT * prgfInOut, void **ppv) PURE;
    STDMETHOD(GetDisplayNameOf)(THIS_ LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName) PURE;
    STDMETHOD(SetNameOf)(THIS_ HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName,
                         DWORD uFlags, LPITEMIDLIST *ppidlOut) PURE;
};
typedef IShellFolder * LPSHELLFOLDER;
DECLARE_DINTERFACE_TYPE(IEnumExtraSearch);

namespace Elshellutils
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TShellFolders { sfoDesktopExpanded, sfoDesktop, sfoPrograms, sfoControlPanel, sfoPrinters, sfoPersonal, sfoFavorites, sfoStartup, sfoRecent, sfoSendto, sfoRecycleBin, sfoStartMenu, sfoDesktopDirectory, sfoMyComputer, sfoNetwork, sfoNetworkNeighborhood, sfoFonts, sfoTemplates, sfoCommonStartMenu, sfoCommonPrograms, sfoCommonStartup, sfoCommonDesktopDirectory, sfoAppData, sfoPrintHood, sfoCustom };
#pragma option pop

class DELPHICLASS TElShellIconCache;
class PASCALIMPLEMENTATION TElShellIconCache : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	Ellist::TElList* FNames;
	Controls::TImageList* FSmallImages;
	Controls::TImageList* FLargeImages;
	HICON DefSmallIcon;
	HICON DefLargeIcon;
	void __fastcall OnItemDelete(System::TObject* Sender, void * Item);
	
protected:
	int __fastcall LookForIcon(char * Name, int Index);
	
public:
	__fastcall TElShellIconCache(void);
	__fastcall virtual ~TElShellIconCache(void);
	int __fastcall AddIcon(_di_IExtractIconA Icon, unsigned Flags);
	int __fastcall AddFromPIDL(Shlobj::PItemIDList PIDL, unsigned Flags, bool OpenIcon);
	__property Controls::TImageList* SmallImages = {read=FSmallImages};
	__property Controls::TImageList* LargeImages = {read=FLargeImages};
};


typedef SHCOLUMNID *PShColumnID;

typedef _SHELLDETAILS *PShellDetails;

typedef _SHELLDETAILS  TShellDetails;

__interface IShellFolder2;
typedef System::DelphiInterface<IShellFolder2> _di_IShellFolder2;
__interface INTERFACE_UUID("{93F2F68C-1D1B-11D3-A30E-00C04F79ABD1}") IShellFolder2  : public IShellFolder 
{
	
public:
	virtual HRESULT __stdcall GetDefaultSearchGUID(/* out */ GUID &pguid) = 0 ;
	virtual HRESULT __stdcall EnumSearches(/* out */ _di_IEnumExtraSearch &ppEnum) = 0 ;
	virtual HRESULT __stdcall GetDefaultColumn(unsigned dwRes, unsigned &pSort, unsigned &pDisplay) = 0 ;
	virtual HRESULT __stdcall GetDefaultColumnState(unsigned iColumn, unsigned &pcsFlags) = 0 ;
	virtual HRESULT __stdcall GetDetailsEx(Shlobj::PItemIDList pidl, const SHCOLUMNID &pscid, System::POleVariant pv) = 0 ;
	virtual HRESULT __stdcall GetDetailsOf(Shlobj::PItemIDList pidl, unsigned iColumn, _SHELLDETAILS &psd) = 0 ;
	virtual HRESULT __stdcall MapNameToSCID(wchar_t * pwszName, SHCOLUMNID &pscid) = 0 ;
};

__interface IShellDetails;
typedef System::DelphiInterface<IShellDetails> _di_IShellDetails;
__interface INTERFACE_UUID("{000214EC-0000-0000-C000-000000000046}") IShellDetails  : public IInterface 
{
	
public:
	virtual HRESULT __stdcall GetDetailsOf(Shlobj::PItemIDList pidl, unsigned iColumn, _SHELLDETAILS &pDetails) = 0 ;
	virtual HRESULT __stdcall ColumnClick(unsigned iColumn) = 0 ;
};

//-- var, const, procedure ---------------------------------------------------
#define SID_IShellDetails "{000214EC-0000-0000-C000-000000000046}"
#define SID_IShellFolder2 "{93F2F68C-1D1B-11D3-A30E-00C04F79ABD1}"
#define SID_IEnumExtraSearch "{0e700be1-9db6-11d1-A1CE-00C04FD75D13}"
extern PACKAGE GUID IID_IShellDetails;
extern PACKAGE GUID IID_IShellFolder2;
extern PACKAGE Shlobj::PItemIDList __fastcall GetFolderPIDL(TShellFolders FolderID, AnsiString CustomName);
extern PACKAGE Shlobj::PItemIDList __fastcall GetFolderPIDL2(TShellFolders FolderID, AnsiString CustomName);
extern PACKAGE void __fastcall FreeIDList(Shlobj::PItemIDList PIDL);
extern PACKAGE bool __fastcall GetPathFromPIDL(Shlobj::PItemIDList PIDL, AnsiString &Path);
extern PACKAGE Shlobj::PItemIDList __fastcall GETPIDLFromPath(AnsiString Path);
extern PACKAGE bool __fastcall IsDesktopPIDL(Shlobj::PItemIDList PIDL);
extern PACKAGE AnsiString __fastcall StrRetToPas(const _STRRET &str, Shlobj::PItemIDList PIDL);
extern PACKAGE void __fastcall StrRetFree(const _STRRET &str);
extern PACKAGE int __fastcall GetCompressedColor(void);
extern PACKAGE TElShellIconCache* __fastcall ShellIconCache(void);
extern PACKAGE Shlobj::PItemIDList __fastcall GetNextItemID(Shlobj::PItemIDList PIDL);
extern PACKAGE bool __fastcall PIDLStartsWith(Shlobj::PItemIDList PIDL, Shlobj::PItemIDList SubPIDL);
extern PACKAGE bool __fastcall PIDLContainsAt(Shlobj::PItemIDList PIDL, Shlobj::PItemIDList SubPIDL, int Pos);
extern PACKAGE int __fastcall CalcPIDLSize(Shlobj::PItemIDList PIDL);
extern PACKAGE bool __fastcall CompareIDLists(Shlobj::PItemIDList IDList1, Shlobj::PItemIDList IDList2);
extern PACKAGE Shlobj::PItemIDList __fastcall ClonePIDL(Shlobj::PItemIDList PIDL);
extern PACKAGE Shlobj::PItemIDList __fastcall GetOwnPIDL(Shlobj::PItemIDList PIDL);
extern PACKAGE Shlobj::PItemIDList __fastcall GetEmptyPIDL(void);
extern PACKAGE Shlobj::PItemIDList __fastcall GetItemIDOnly(Shlobj::PItemIDList PIDL);
extern PACKAGE Shlobj::PItemIDList __fastcall AppendPIDL(Shlobj::PItemIDList ParentPIDL, Shlobj::PItemIDList ChildPIDL);
extern PACKAGE Shlobj::PItemIDList __fastcall GetParentPIDL(Shlobj::PItemIDList PIDL);
extern PACKAGE bool __fastcall FireURL(const AnsiString URL);

}	/* namespace Elshellutils */
using namespace Elshellutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElShellUtils
