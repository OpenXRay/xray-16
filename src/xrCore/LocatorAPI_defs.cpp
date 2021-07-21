#include "stdafx.h"
#pragma hdrstop

#include "LocatorAPI_defs.h"
#pragma warning(push)
#pragma warning(disable : 4995)
#if defined(XR_PLATFORM_WINDOWS)
#include <io.h>
#include <direct.h>
#include <sys\stat.h>
#endif
#include <fcntl.h>
#pragma warning(pop)

//////////////////////////////////////////////////////////////////////
// FS_File
//////////////////////////////////////////////////////////////////////
FS_File::FS_File(const xr_string& nm, long sz, time_t modif, unsigned attr) { set(nm, sz, modif, attr); }
FS_File::FS_File(const xr_string& nm) { set(nm, 0, 0, 0); }
FS_File::FS_File(const _FINDDATA_T& f) { set(f.name, f.size, f.time_write, (f.attrib & _A_SUBDIR) ? flSubDir : 0); }
FS_File::FS_File(const xr_string& nm, const _FINDDATA_T& f)
{
    set(nm, f.size, f.time_write, (f.attrib & _A_SUBDIR) ? flSubDir : 0);
}

void FS_File::set(const xr_string& nm, long sz, time_t modif, unsigned attr)
{
    name = nm;
    xr_fs_strlwr(name);
    size = sz;
    time_write = modif;
    attrib = attr;
}

//////////////////////////////////////////////////////////////////////
// FS_Path
//////////////////////////////////////////////////////////////////////
FS_Path::FS_Path(LPCSTR _Root, LPCSTR _Add, LPCSTR _DefExt, LPCSTR _FilterCaption, u32 flags)
{
    // VERIFY (_Root&&_Root[0]);
    string_path temp;
    xr_strcpy(temp, sizeof(temp), _Root);
    if (_Add)
        xr_strcat(temp, _Add);
    if (temp[0] && temp[xr_strlen(temp) - 1] != _DELIMITER && temp[xr_strlen(temp) - 1] != '/')
        xr_strcat(temp, DELIMITER);
    restore_path_separators(temp);
    m_Path = xr_fs_strlwr(xr_strdup(temp));
    m_DefExt = _DefExt ? xr_fs_strlwr(xr_strdup(_DefExt)) : 0;
    m_FilterCaption = _FilterCaption ? xr_fs_strlwr(xr_strdup(_FilterCaption)) : 0;
    m_Add = _Add ? xr_fs_strlwr(xr_strdup(_Add)) : 0;
    m_Root = _Root ? xr_fs_strlwr(xr_strdup(_Root)) : 0;
    m_Flags.assign(flags);
#ifdef _EDITOR
    // Editor(s)/User(s) wants pathes already created in "real" file system :)
    VerifyPath(m_Path);
#endif
}

FS_Path::~FS_Path()
{
    xr_free(m_Root);
    xr_free(m_Path);
    xr_free(m_Add);
    xr_free(m_DefExt);
    xr_free(m_FilterCaption);
}

void FS_Path::_set(LPCSTR add)
{
    // m_Add
    R_ASSERT(add);
    xr_free(m_Add);
    m_Add = xr_fs_strlwr(xr_strdup(add));

    // m_Path
    string_path temp;
    strconcat(sizeof(temp), temp, m_Root, m_Add);
    if (temp[xr_strlen(temp) - 1] != _DELIMITER)
        xr_strcat(temp, DELIMITER);
    xr_free(m_Path);
    m_Path = xr_fs_strlwr(xr_strdup(temp));
}

void FS_Path::_set_root(LPCSTR root)
{
    string_path temp;
    xr_strcpy(temp, root);
    if (*temp && temp[xr_strlen(temp) - 1] != _DELIMITER)
        xr_strcat(temp, DELIMITER);
    xr_free(m_Root);
    m_Root = xr_fs_strlwr(xr_strdup(temp));

    // m_Path
    strconcat(sizeof(temp), temp, m_Root, m_Add ? m_Add : "");
    if (*temp && temp[xr_strlen(temp) - 1] != _DELIMITER)
        xr_strcat(temp, DELIMITER);
    xr_free(m_Path);
    m_Path = xr_fs_strlwr(xr_strdup(temp));
}

LPCSTR FS_Path::_update(string_path& dest, LPCSTR src) const
{
    R_ASSERT(dest);
    R_ASSERT(src);
    string_path temp;
    xr_strcpy(temp, sizeof(temp), src);

#ifdef XR_PLATFORM_LINUX
    string_path fullPath;
    strconcat(fullPath, m_Path, temp);
    if (FS.exist(fullPath, FSType::External) || FS.exist(fullPath, FSType::Virtual))
    {
        xr_strcpy(dest, fullPath);
        return dest;
    }
#endif

    xr_strlwr(temp);
    strconcat(sizeof(dest), dest, m_Path, temp);
    return xr_fs_strlwr(dest);
}
/*
void FS_Path::_update(xr_string& dest, LPCSTR src)const
{
R_ASSERT(src);
dest = xr_string(m_Path)+src;
xr_strlwr (dest);
}*/
void FS_Path::rescan_path_cb()
{
    m_Flags.set(flNeedRescan, TRUE);
    FS.m_Flags.set(CLocatorAPI::flNeedRescan, TRUE);
}

bool XRCORE_API PatternMatch(LPCSTR s, LPCSTR mask)
{
    LPCSTR cp = 0;
    LPCSTR mp = 0;
    for (; *s && *mask != '*'; mask++, s++)
        if (*mask != *s && *mask != '?')
            return false;
    for (;;)
    {
        if (!*s)
        {
            while (*mask == '*')
                mask++;
            return !*mask;
        }
        if (*mask == '*')
        {
            if (!*++mask)
                return true;
            mp = mask;
            cp = s + 1;
            continue;
        }
        if (*mask == *s || *mask == '?')
        {
            mask++, s++;
            continue;
        }
        mask = mp;
        s = cp++;
    }
}
