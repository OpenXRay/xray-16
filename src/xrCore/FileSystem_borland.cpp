//----------------------------------------------------
// file: FileSystem.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "FileSystem.h"
#ifdef WINDOWS
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <Shlobj.h>

//#pragma comment(lib, "OSDialogB.lib")

int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
        SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
    return 0;
}

bool EFS_Utils::GetOpenName(LPCSTR initial, xr_string& buffer, bool bMulti, LPCSTR offset, int start_flt_ext)
{
    char buf[255 * 255]; // max files to select
    xr_strcpy(buf, buffer.c_str());

    /*
    char* g_SHBF_Folder =("C:\\Program Files");
    TCHAR path[_MAX_PATH];
    BROWSEINFO info={NULL,NULL,path,"title",BIF_USENEWUI,BrowseCallbackProc, (LPARAM)g_SHBF_Folder };
    SHBrowseForFolder (&info);
    */
    /*
     {
     HANDLE hDialog = OSDInit(true, "SDITEST", 0, 0, 0, 0, 0, 0);
     if(hDialog)
     {
     OSDRET osResult=OSDDoModal(hDialog, 0);
     OSDRelease(hDialog);
     }

     }
     */
    // bool bRes = false;
    bool bRes = GetOpenNameInternal(initial, buf, sizeof(buf), bMulti, offset, start_flt_ext);

    if (bRes)
        buffer = (char*)buf;

    return bRes;
}

bool EFS_Utils::GetSaveName(LPCSTR initial, xr_string& buffer, LPCSTR offset, int start_flt_ext)
{
    string_path buf;
    xr_strcpy(buf, sizeof(buf), buffer.c_str());
    bool bRes = GetSaveName(initial, buf, offset, start_flt_ext);
    if (bRes)
        buffer = buf;

    return bRes;
}
//----------------------------------------------------

void EFS_Utils::MarkFile(LPCSTR fn, bool bDeleteSource)
{
    xr_string ext = strext(fn);
    ext.insert(1, "~");
    xr_string backup_fn = EFS.ChangeFileExt(fn, ext.c_str());
    if (bDeleteSource)
    {
        FS.file_rename(fn, backup_fn.c_str(), true);
    }
    else
    {
        FS.file_copy(fn, backup_fn.c_str());
    }
}

xr_string EFS_Utils::AppendFolderToName(xr_string& tex_name, int depth, BOOL full_name)
{
    string1024 nm;
    xr_strcpy(nm, tex_name.c_str());
    tex_name = AppendFolderToName(nm, sizeof(nm), depth, full_name);
    return tex_name;
}
#endif
