#ifndef LocatorAPI_defsH
#define LocatorAPI_defsH

#pragma once

enum FS_List
{
    FS_ListFiles = (1 << 0),
    FS_ListFolders = (1 << 1),
    FS_ClampExt = (1 << 2),
    FS_RootOnly = (1 << 3),
    FS_forcedword = u32(-1)
};

class XRCORE_API FS_Path
{
public:
    enum
    {
        flRecurse = (1 << 0),
        flNotif = (1 << 1),
        flNeedRescan = (1 << 2),
    };

public:
    pstr m_Path;
    pstr m_Root;
    pstr m_Add;
    pstr m_DefExt;
    pstr m_FilterCaption;
    Flags32 m_Flags;

public:
    FS_Path(LPCSTR _Root, LPCSTR _Add, LPCSTR _DefExt = 0, LPCSTR _FilterString = 0, u32 flags = 0);
    ~FS_Path();
    LPCSTR _update(string_path& dest, LPCSTR src) const;
    void _set(LPCSTR add);
    void _set_root(LPCSTR root);

    void __stdcall rescan_path_cb();
};

#ifdef _EDITOR
namespace std
{
struct _finddata_t;
};
#define _FINDDATA_T std::_finddata_t
#else
struct _finddata64i32_t;
#define _FINDDATA_T _finddata64i32_t
#endif

struct XRCORE_API FS_File
{
    enum
    {
        flSubDir = (1 << 0),
        flVFS = (1 << 1),
    };
    unsigned attrib;
    time_t time_write;
    long size;
    xr_string name; // low-case name
    void set(const xr_string& nm, long sz, time_t modif, unsigned attr);

public:
    FS_File() : attrib(0), time_write(0), size(0) {}
    FS_File(const xr_string& nm);
    FS_File(const _FINDDATA_T& f);
    FS_File(const xr_string& nm, const _FINDDATA_T& f);
    FS_File(const xr_string& nm, long sz, time_t modif, unsigned attr);
    bool operator<(const FS_File& _X) const { return xr_strcmp(name.c_str(), _X.name.c_str()) < 0; }
};
using FS_FileSet = xr_set<FS_File>;

extern bool XRCORE_API PatternMatch(LPCSTR s, LPCSTR mask);

#endif // LocatorAPI_defsH
