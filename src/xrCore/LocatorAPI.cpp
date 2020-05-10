// LocatorAPI.cpp: implementation of the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop // huh?

#pragma warning(push)
#pragma warning(disable : 4995)
#if defined(XR_PLATFORM_WINDOWS)
#include <direct.h>
#include <sys/stat.h>
#endif
#include <fcntl.h>
#pragma warning(pop)

#include "FS_internal.h"
#include "stream_reader.h"
#include "file_stream_reader.h"
#include "xrCore/Threading/Lock.hpp"
#include "Crypto/trivial_encryptor.h"

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
#include "SDL.h"
#include "xrstring.h"
#include <glob.h>
#endif

constexpr size_t VFS_STANDARD_FILE = std::numeric_limits<size_t>::max();

const u32 BIG_FILE_READER_WINDOW_SIZE = 1024 * 1024;

xr_unique_ptr<CLocatorAPI> xr_FS;

#ifdef _EDITOR
static constexpr pcstr FSLTX = "fs.ltx"
#else
static constexpr pcstr FSLTX = "fsgame.ltx";
#endif

struct _open_file
{
    union
    {
        IReader* _reader;
        CStreamReader* _stream_reader;
    };
    shared_str _fn;
    u32 _used;
};

template <typename T>
struct eq_pointer;

template <>
struct eq_pointer<IReader>
{
    IReader* _val;
    eq_pointer(IReader* p) : _val(p) {}
    bool operator()(_open_file& itm) const { return _val == itm._reader; }
};

template <>
struct eq_pointer<CStreamReader>
{
    CStreamReader* _val;
    eq_pointer(CStreamReader* p) : _val(p) {}
    bool operator()(_open_file& itm) const { return _val == itm._stream_reader; }
};

struct eq_fname_free
{
    shared_str _val;
    eq_fname_free(shared_str s) : _val(s) {}
    bool operator()(_open_file& itm) const { return _val == itm._fn && itm._reader == nullptr; }
};

struct eq_fname_check
{
    shared_str _val;
    eq_fname_check(shared_str s) : _val(s) {}
    bool operator()(_open_file& itm) const { return _val == itm._fn && itm._reader != nullptr; }
};

XRCORE_API xr_vector<_open_file> g_open_files;

void _check_open_file(const shared_str& _fname)
{
    auto it = std::find_if(g_open_files.begin(), g_open_files.end(), eq_fname_check(_fname));
    if (it != g_open_files.end())
        Log("file opened at least twice", _fname.c_str());
}

_open_file& find_free_item(const shared_str& _fname)
{
    auto it = std::find_if(g_open_files.begin(), g_open_files.end(), eq_fname_free(_fname));
    if (it == g_open_files.end())
    {
        g_open_files.resize(g_open_files.size() + 1);
        _open_file& _of = g_open_files.back();
        _of._fn = _fname;
        _of._used = 0;
        return _of;
    }
    return *it;
}

void setup_reader(CStreamReader* _r, _open_file& _of) { _of._stream_reader = _r; }
void setup_reader(IReader* _r, _open_file& _of) { _of._reader = _r; }
template <typename T>
void _register_open_file(T* _r, pcstr _fname)
{
    Lock _lock;
    _lock.Enter();

    shared_str f = _fname;
    _check_open_file(f);

    _open_file& _of = find_free_item(_fname);
    setup_reader(_r, _of);
    _of._used += 1;

    _lock.Leave();
}

template <typename T>
void _unregister_open_file(T* _r)
{
    Lock _lock;
    _lock.Enter();

    auto it = std::find_if(g_open_files.begin(), g_open_files.end(), eq_pointer<T>(_r));
    VERIFY(it != g_open_files.end());
    _open_file& _of = *it;
    _of._reader = nullptr;
    _lock.Leave();
}

XRCORE_API void _dump_open_files(int mode)
{
    if (mode == 1)
    {
        for (const auto& file : g_open_files)
        {
            Log("----opened files");
            if (file._reader != nullptr)
                Msg("[%d] fname:%s", file._used, file._fn.c_str());
        }
    }
    else
    {
        Log("----un-used");
        for (const auto& file : g_open_files)
        {
            if (file._reader == nullptr)
                Msg("[%d] fname:%s", file._used, file._fn.c_str());
        }
    }
    Log("----total count = ", g_open_files.size());
}

CLocatorAPI::CLocatorAPI() : bNoRecurse(true), m_auth_code(0),
#ifdef CONFIG_PROFILE_LOCKS
    m_auth_lock(xr_new<Lock>(MUTEX_PROFILE_ID(CLocatorAPI::m_auth_lock)))
#else
    m_auth_lock(xr_new<Lock>())
#endif // CONFIG_PROFILE_LOCKS
{
    m_Flags.zero();
#if defined(XR_PLATFORM_WINDOWS)
    // get page size
    SYSTEM_INFO sys_inf;
    GetSystemInfo(&sys_inf);
    dwAllocGranularity = sys_inf.dwAllocationGranularity;
#elif defined(XR_PLATFORM_LINUX)
    dwAllocGranularity = sysconf(_SC_PAGE_SIZE);
#endif
    m_iLockRescan = 0;
    dwOpenCounter = 0;
}

CLocatorAPI::~CLocatorAPI()
{
    VERIFY(0 == m_iLockRescan);
    _dump_open_files(1);
    delete m_auth_lock;
}

const CLocatorAPI::file* CLocatorAPI::RegisterExternal(pcstr name)
{
    struct stat buffer;
    if (stat(name, &buffer) == -1)
        return nullptr;
    return Register(name, size_t(-1), 0, 0, buffer.st_size, buffer.st_size, u32(buffer.st_mtime));
}

const CLocatorAPI::file* CLocatorAPI::Register(
    pcstr name, size_t vfs, u32 crc, u32 ptr, u32 size_real, u32 size_compressed, u32 modif)
{
    string256 temp_file_name;
    xr_strcpy(temp_file_name, sizeof temp_file_name, name);

    restore_path_separators(temp_file_name);
    //Msg("Register[%d] [%s]",vfs,temp_file_name);

    // Register file
    file desc;
    // desc.name = xr_strlwr(xr_strdup(name));
    desc.name = temp_file_name;
    desc.vfs = vfs;
    desc.crc = crc;
    desc.ptr = ptr;
    desc.size_real = size_real;
    desc.size_compressed = size_compressed;
    desc.modif = modif & ~u32(0x3);
    // Msg("registering file %s - %d", name, size_real);
    // if file already exist - update info
    files_it I = m_files.find(desc);
    if (I != m_files.end())
    {
        //. Msg("-- file already scanned [%s]", I->name);
        desc.name = I->name;

        // sad but true, performance option
        // correct way is to erase and then insert new record:
        const_cast<file&>(*I) = desc;
        return &*I;
    }
    else
    {
        desc.name = xr_strdup(desc.name);
    }

    // otherwise insert file
    auto result = m_files.insert(desc).first;

    // Try to register folder(s)
    string_path temp;
    xr_strcpy(temp, sizeof temp, desc.name);
    string_path path;
    string_path folder;
    while (temp[0])
    {
        _splitpath(temp, path, folder, nullptr, nullptr);
        xr_strcat(path, folder);
        if (!exist(path))
        {
            desc.name = xr_strdup(path);
            desc.vfs = VFS_STANDARD_FILE;
            desc.ptr = 0;
            desc.size_real = 0;
            desc.size_compressed = 0;
            desc.modif = u32(-1);
            std::pair<files_it, bool> I2 = m_files.insert(desc);

            cpcstr failureDescription = "Failed to register file in filesystem.\n"
                "First argument is the file we tried to insert, "
                "second is the file that prevented the insertion.";
            R_ASSERT4(I2.second, failureDescription, path, I2.first->name);
        }
        xr_strcpy(temp, sizeof temp, folder);
        if (xr_strlen(temp))
            temp[xr_strlen(temp) - 1] = 0;
    }
    return &*result;
}

#if defined(XR_PLATFORM_WINDOWS)
IReader* open_chunk(void* ptr, u32 ID, pcstr archiveName, size_t archiveSize, bool shouldDecrypt = false)
{
    u32 dwType = INVALID_SET_FILE_POINTER;
    size_t dwSize = 0;
    DWORD read_byte;
    u32 pt = SetFilePointer(ptr, 0, nullptr, FILE_BEGIN);
    VERIFY(pt != INVALID_SET_FILE_POINTER);

    while (true)
    {
        bool res = ReadFile(ptr, &dwType, 4, &read_byte, nullptr);

        if (read_byte == 0)
            return nullptr;
        //. VERIFY(res&&(read_byte==4));

        u32 tempSize = 0;
        res = ReadFile(ptr, &tempSize, 4, &read_byte, nullptr);
        dwSize = tempSize;

        if (read_byte == 0)
            return nullptr;
        //. VERIFY(res&&(read_byte==4));

        if ((dwType & ~CFS_CompressMark) == ID)
        {
            u8* src_data = xr_alloc<u8>(dwSize);
            res = ReadFile(ptr, src_data, dwSize, &read_byte, nullptr);

            VERIFY(res && (read_byte == dwSize));
            if (dwType & CFS_CompressMark)
            {
                BYTE* dest = nullptr;
                size_t dest_sz = 0;

                if (shouldDecrypt) // Try WW key first
                    g_trivial_encryptor.decode(src_data, dwSize, src_data);

                bool result = _decompressLZ(&dest, &dest_sz, src_data, dwSize, archiveSize);

                if (!result && shouldDecrypt)
                {
                    // Let's try to decode with RU key
                    g_trivial_encryptor.encode(src_data, dwSize, src_data); // rollback
                    g_trivial_encryptor.decode(src_data, dwSize, src_data, trivial_encryptor::key_flag::russian);
                    result = _decompressLZ(&dest, &dest_sz, src_data, dwSize, archiveSize);
                }
                R_ASSERT3(result, "Can't decompress archive", archiveName);

                xr_free(src_data);
                return xr_new<CTempReader>(dest, dest_sz, 0);
            }
            return xr_new<CTempReader>(src_data, dwSize, 0);
        }

        pt = SetFilePointer(ptr, dwSize, nullptr, FILE_CURRENT);
        if (pt == INVALID_SET_FILE_POINTER)
            return nullptr;
    }
    return nullptr;
};
#endif

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
IReader* open_chunk(int fd, u32 ID, pcstr archiveName, size_t archiveSize, bool shouldDecrypt = false)
{
    u32 dwType;
    size_t dwSize = 0;
    ssize_t read_byte;
    ::lseek(fd, 0L, SEEK_SET);

    while (true)
    {
        read_byte = ::read(fd, &dwType, 4);
        if (read_byte == -1)
            return nullptr;

        u32 tempSize = 0;
        read_byte = ::read(fd, &tempSize, 4);
        dwSize = tempSize;
        if (read_byte == -1)
            return nullptr;

        if ((dwType & ~CFS_CompressMark) == ID)
        {
            u8* src_data = xr_alloc<u8>(dwSize);
            read_byte = ::read(fd, src_data, dwSize);

            VERIFY(read_byte == dwSize);
            if (dwType & CFS_CompressMark)
            {
                BYTE* dest = nullptr;
                size_t dest_sz = 0;

                if (shouldDecrypt)
                    g_trivial_encryptor.decode(src_data, dwSize, src_data);

                bool result = _decompressLZ(&dest, &dest_sz, src_data, dwSize, archiveSize);

                if (!result && shouldDecrypt)
                {
                    // Let's try to decode with RU key
                    g_trivial_encryptor.decode(src_data, dwSize, src_data, trivial_encryptor::key_flag::russian);
                    result = _decompressLZ(&dest, &dest_sz, src_data, dwSize, archiveSize);
                }
                R_ASSERT3(result, "Can't decompress archive", archiveName);

                xr_free(src_data);
                return xr_new<CTempReader>(dest, dest_sz, 0);
            }
            return xr_new<CTempReader>(src_data, dwSize, 0);
        }

        if(-1 == ::lseek(fd, dwSize, SEEK_CUR))
            return nullptr;
    }
    return nullptr;
};
#endif

void CLocatorAPI::LoadArchive(archive& A, pcstr entrypoint)
{
    // Create base path
    string_path fs_entry_point;
    bool shouldDecrypt = false;
    fs_entry_point[0] = 0;
    if (A.header)
    {
        shared_str read_path = A.header->r_string("header", "entry_point");
        if (0 == xr_stricmp(read_path.c_str(), "gamedata"))
        {
            read_path = "$fs_root$";
            FS_Path* root = nullptr;
            if (get_path(read_path.c_str(), &root))
            {
                // R_ASSERT3 (root, "path not found ", read_path.c_str());
                xr_strcpy(fs_entry_point, sizeof fs_entry_point, root->m_Path);
            }
            xr_strcat(fs_entry_point, "gamedata//");
        }
        else
        {
            string256 alias_name;
            alias_name[0] = 0;
            R_ASSERT2(*read_path.c_str() == '$', read_path.c_str());

            int count = sscanf(read_path.c_str(), "%[^\\]s", alias_name);
            R_ASSERT2(count == 1, read_path.c_str());

            auto P = m_paths.find(alias_name);

            FS_Path* root = nullptr;
            if (get_path("$fs_root$", &root))
            {
                // R_ASSERT3 (root, "path not found ", alias_name);
                xr_strcpy(fs_entry_point, sizeof fs_entry_point, root->m_Path);
            }
            xr_strcat(fs_entry_point, sizeof fs_entry_point, read_path.c_str() + xr_strlen(alias_name) + 1);
        }
    }
    else
    {
        Msg("~ Found archive without ini header: %s", A.path.c_str());

        if (!strstr(A.path.c_str(), ".xdb"))
        {
            Msg("Assuming that [%s] is encrypted ShoC archive", A.path.c_str());
            shouldDecrypt = true;
        }
        FS_Path* root = nullptr;
        if (get_path("$fs_root$", &root))
        {
            // R_ASSERT3 (root, "path not found ", read_path.c_str());
            xr_strcpy(fs_entry_point, sizeof fs_entry_point, root->m_Path);
        }
        xr_strcat(fs_entry_point, "gamedata" DELIMITER);
    }
    if (entrypoint)
        xr_strcpy(fs_entry_point, sizeof fs_entry_point, entrypoint);

    // Read FileSystem
    A.open();
    IReader* hdr = open_chunk(A.hSrcFile, 1, A.path.c_str(), A.size, shouldDecrypt);

    R_ASSERT(hdr);

    while (!hdr->eof())
    {
        string_path name, full;
        archive_header header;

        u16 buffer_size = hdr->r_u16(); // Read the total length of all crap
        VERIFY(buffer_size < sizeof(name) + sizeof(archive_header) + sizeof(u32));

        hdr->r(&header, sizeof(archive_header)); // Read header

        const size_t name_length = buffer_size - sizeof(archive_header) - sizeof(u32);
        VERIFY(name_length > 0);
        hdr->r(&name, name_length); // Read file name
        name[name_length] = 0;

        u32 ptr = 0;
        hdr->r(&ptr, sizeof(ptr)); // Obtain internal pointer to the file in archive

        strconcat(sizeof full, full, fs_entry_point, name);
        Register(full, A.vfs_idx, header.crc, ptr, header.size_real, header.size_compr, 0);
    }
    hdr->close();
} //-V773

void CLocatorAPI::archive::open()
{
#if defined(XR_PLATFORM_WINDOWS)
    // Open the file
    if (hSrcFile && hSrcMap)
        return;

    hSrcFile = CreateFile(*path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    R_ASSERT(hSrcFile != INVALID_HANDLE_VALUE);
    hSrcMap = CreateFileMapping(hSrcFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    R_ASSERT(hSrcMap != INVALID_HANDLE_VALUE);
    size = GetFileSize(hSrcFile, nullptr);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    // Open the file
    if (hSrcFile)
        return;

    pstr conv_path = xr_strdup(path.c_str());
    convert_path_separators(conv_path);
    hSrcFile = ::open(conv_path, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    xr_free(conv_path);
    R_ASSERT(hSrcFile != -1);
    struct stat file_info;
    ::fstat(hSrcFile, &file_info);
    size = file_info.st_size;
#endif
    R_ASSERT(size > 0);
}

void CLocatorAPI::archive::close()
{
#if defined(XR_PLATFORM_WINDOWS)
    CloseHandle(hSrcMap);
    hSrcMap = nullptr;
    CloseHandle(hSrcFile);
    hSrcFile = nullptr;
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    ::close(hSrcFile);
    hSrcFile = -1;
#endif
}

void CLocatorAPI::ProcessArchive(pcstr _path)
{
    // find existing archive
    shared_str path = _path;

    for (const auto& it : m_archives)
        if (it.path == path)
            return;

    m_archives.push_back(archive());
    archive& A = m_archives.back();
    A.vfs_idx = m_archives.size() - 1;
    A.path = path;

    A.open();

    // Read header
    bool bProcessArchiveLoading = true;
    IReader* hdr = open_chunk(A.hSrcFile, CFS_HeaderChunkID, A.path.c_str(), A.size);

    if (hdr)
    {
        A.header = xr_new<CInifile>(hdr, "archive_header");
        hdr->close();
        bProcessArchiveLoading = A.header->r_bool("header", "auto_load");
    }
    if (bProcessArchiveLoading || strstr(Core.Params, "-auto_load_arch"))
        LoadArchive(A);
    else
        A.close();
}

void CLocatorAPI::unload_archive(CLocatorAPI::archive& A)
{
    files_it I = m_files.begin();
    for (; I != m_files.end(); ++I)
    {
        const file& entry = *I;
        if (entry.vfs == A.vfs_idx)
        {
#ifndef MASTER_GOLD
            Msg("unregistering file [%s]", I->name);
#endif // #ifndef MASTER_GOLD
            auto str = pstr(I->name);
            xr_free(str);
            m_files.erase(I);
            break;
        }
    }
    A.close();
}

bool CLocatorAPI::load_all_unloaded_archives()
{
    bool res = false;
    for (auto& archive : m_archives)
    {
#if defined(XR_PLATFORM_WINDOWS)
        if (archive.hSrcFile == nullptr)
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
        if (archive.hSrcFile == 0)
#endif
        {
            LoadArchive(archive);
            res = true;
        }
    }

    return res;
}

void CLocatorAPI::ProcessOne(pcstr path, const _finddata_t& entry)
{
    string_path N;
#if defined(XR_PLATFORM_WINDOWS)
    xr_strcpy(N, sizeof N, path);
    xr_strcat(N, entry.name);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    xr_strcpy(N, sizeof N, entry.name);
#endif

    if (entry.attrib & _A_HIDDEN)
        return;

    if (entry.attrib & _A_SUBDIR)
    {
        if (bNoRecurse)
            return;
        if (0 == xr_strcmp(entry.name, "."))
            return;
        if (0 == xr_strcmp(entry.name, ".."))
            return;
        if(path[xr_strlen(path) - 1] != _DELIMITER || path[xr_strlen(path) - 1] != '/')
            xr_strcat(N, DELIMITER);
        Register(N, VFS_STANDARD_FILE, 0, 0, entry.size, entry.size, (u32)entry.time_write);
        Recurse(N);
    }
    else
    {
        if (strext(N) && (0 == strncmp(strext(N), ".db", 3) || 0 == strncmp(strext(N), ".xdb", 4)))
            ProcessArchive(N);
        else
            Register(N, VFS_STANDARD_FILE, 0, 0, entry.size, entry.size, (u32)entry.time_write);
    }
}

IC bool pred_str_ff(const _finddata_t& x, const _finddata_t& y) { return xr_strcmp(x.name, y.name) < 0; }
bool ignore_name(const char* _name)
{
    if (!strcmp(_name, "Thumbs.db"))
        return true; // ignore windows hidden Thumbs.db
    if (!strcmp(_name, ".svn"))
        return true; // ignore ".svn" folders
    if (!strcmp(_name, ".vs"))
        return true; // ignore ".vs" folders
    const size_t len = strlen(_name);
#define ENDS_WITH(n) (len>sizeof(n) && !strcmp(_name+len-(sizeof(n)-1), n))
    if (ENDS_WITH(".VC.db"))
        return true;
    if (ENDS_WITH(".VC.opendb"))
        return true;
    if (ENDS_WITH(".sln"))
        return true;
    if (ENDS_WITH(".pdb"))
        return true;
    if (ENDS_WITH(".ipdb"))
        return true;
    if (ENDS_WITH(".iobj"))
        return true;
#undef ENDS_WITH
    return false;
}

// we need to check for file existance
// because Unicode file names can
// be interpolated by FindNextFile()

bool ignore_path(pcstr _path)
{
#if defined(XR_PLATFORM_WINDOWS)
    HANDLE h = CreateFile(_path, 0, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_NO_BUFFERING, nullptr);

    if (h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(h);
        return false;
    }
    else
        return true;
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    pstr conv_path = xr_strdup(_path);
    convert_path_separators(conv_path);
    int h = ::open(conv_path, O_RDONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    xr_free(conv_path);
    if (h != -1)
    {
        ::close(h);
        return false;
    }
    else
        return true;
#endif
}

bool CLocatorAPI::Recurse(pcstr path)
{
    string_path scanPath = { 0 };
    xr_strcpy(scanPath, sizeof scanPath, path);
    xr_strcat(scanPath, ".xrignore");
    struct stat buffer;
    if (!stat(scanPath, &buffer))
        return true;
    xr_strcpy(scanPath, sizeof scanPath, path);
    xr_strcat(scanPath, "*");
    _finddata_t findData;
    convert_path_separators(scanPath);
#ifdef XR_PLATFORM_WINDOWS
    intptr_t handle = _findfirst(scanPath, &findData);
    if (handle == -1)
        return false;
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    glob_t globbuf;

    globbuf.gl_offs = 256;
    int result = glob(scanPath, GLOB_NOSORT, NULL, &globbuf);

    if(0 != result)
        return false;

    intptr_t handle = globbuf.gl_pathc - 1;
#endif

    rec_files.reserve(256);
    size_t oldSize = rec_files.size();
    intptr_t done = handle;
    while (done != -1)
    {
#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
        xr_strcpy(findData.name, globbuf.gl_pathv[handle - done]);
        struct stat fi;
        stat(findData.name, &fi);
        findData.size = fi.st_size;
        findData.time_access = fi.st_atim.tv_sec;
        findData.time_create = fi.st_ctim.tv_sec;
        findData.time_write = fi.st_mtim.tv_sec;
        switch (fi.st_mode & S_IFMT)
        {
        case S_IFDIR: findData.attrib = _A_SUBDIR; break;
        case S_IFREG: findData.attrib = 0;         break; // File
        default:      findData.attrib = _A_HIDDEN; break; // Skip
        }
        restore_path_separators(findData.name);
#endif
        string1024 fullPath;
        bool ignore = false;
        if (m_Flags.test(flNeedCheck))
        {
#ifdef XR_PLATFORM_WINDOWS
            xr_strcpy(fullPath, sizeof fullPath, path);
            xr_strcat(fullPath, findData.name);
            ignore = ignore_name(findData.name) || ignore_path(fullPath);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
            xr_strcpy(fullPath, sizeof fullPath, findData.name); // glob return full path to file
            ignore = ignore_name(findData.name);
#endif
        }
        else
        {
            ignore = ignore_name(findData.name);
        }
        if (!ignore)
            rec_files.push_back(findData);
#ifdef XR_PLATFORM_WINDOWS
        done = _findnext(handle, &findData);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
        done--;
#endif
    }
#ifdef XR_PLATFORM_WINDOWS
    _findclose(handle);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    globfree(&globbuf);
#endif
    size_t newSize = rec_files.size();
    if (newSize > oldSize)
    {
        std::sort(rec_files.begin() + oldSize, rec_files.end(), pred_str_ff);
        for (size_t i = oldSize; i < newSize; i++) // Don't replace this with range-based for!
            ProcessOne(path, rec_files[i]); // only index-based for can work correctly here
        rec_files.erase(rec_files.begin() + oldSize, rec_files.end());
    }
    // insert self
    if (path && path[0] != 0)
        Register(path, VFS_STANDARD_FILE, 0, 0, 0, 0, 0);

    return true;
}

bool file_handle_internal(pcstr file_name, size_t& size, int& file_handle);
void* FileDownload(pcstr file_name, const int& file_handle, size_t& file_size);

void CLocatorAPI::setup_fs_path(pcstr fs_name, string_path& fs_path)
{
    xr_strcpy(fs_path, fs_name ? fs_name : "");
    pstr slash = strrchr(fs_path, _DELIMITER);
    if (!slash)
        slash = strrchr(fs_path, '/');
    if (!slash)
    {
        xr_strcpy(fs_path, "");
        return;
    }

    *(slash + 1) = 0;
}

void CLocatorAPI::setup_fs_path(pcstr fs_name)
{
    string_path fs_path;
    setup_fs_path(fs_name, fs_path);

    string_path full_current_directory;
    
#if defined(XR_PLATFORM_WINDOWS)
    _fullpath(full_current_directory, fs_path, sizeof full_current_directory);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    if (SDL_strlen(fs_path) != 0)
    {
        char *tmp_path = realpath(fs_path, NULL);
        CHECK_OR_EXIT(tmp_path && tmp_path[0],
            make_string("Cannot get realpath for \"%s\": %s", fs_path, strerror(errno)));
        SDL_strlcpy(full_current_directory, tmp_path, sizeof full_current_directory);
        free(tmp_path);
    }
    else
        SDL_strlcpy(full_current_directory, SDL_GetPrefPath("GSC", "SCOP"), sizeof full_current_directory);
#endif

    FS_Path* path = xr_new<FS_Path>(full_current_directory, "", "", "", 0);

#ifdef DEBUG
    Msg("initial $fs_root$ = %s", full_current_directory);
#endif

    m_paths.insert(std::make_pair(xr_strdup("$fs_root$"), path));
}

pcstr one_folder_back_and_rescan(FS_Path* path)
{
    string_path newPath = {};
    xr_strcpy(newPath, sizeof(newPath), path->m_Path);
    newPath[xr_strlen(newPath) - 1] = 0;
    if (strrchr(newPath, _DELIMITER))
        * (strrchr(newPath, _DELIMITER) + 1) = 0;

    FS_Path* pFSRoot = FS.get_path("$fs_root$");
    pFSRoot->_set_root(newPath);
    return pFSRoot->m_Path;
}

IReader* CLocatorAPI::setup_fs_ltx(pcstr fs_name)
{
    pcstr fs_file_name = FSLTX;
    if (fs_name && *fs_name)
        fs_file_name = fs_name;

    Log("using fs-ltx", fs_file_name);

    setup_fs_path(fs_name);

    // if (m_Flags.is(flTargetFolderOnly)) {
    // append_path ("$fs_root$", "", 0, FALSE);
    // return (0);
    // }

    IReader* result = nullptr;
    int file_handle;
    size_t file_size;

    bool fsltx_is_available = file_handle_internal(fs_file_name, file_size, file_handle);

    if (!fsltx_is_available)
    {
        fs_file_name = one_folder_back_and_rescan(get_path("$fs_root$"));
        fsltx_is_available = file_handle_internal(fs_file_name, file_size, file_handle);
    }

    else if (!fsltx_is_available)
    {
        fs_file_name = one_folder_back_and_rescan(get_path("$app_root$"));
        fsltx_is_available = file_handle_internal(fs_file_name, file_size, file_handle);
    }

#ifndef MASTER_GOLD
    else if (!fsltx_is_available)
    {
        fs_file_name = one_folder_back_and_rescan(get_path("$fs_root$"));
        fsltx_is_available = file_handle_internal(fs_file_name, file_size, file_handle);
    }

    else if (!fsltx_is_available)
    {
        fs_file_name = one_folder_back_and_rescan(get_path("$fs_root$"));
        fsltx_is_available = file_handle_internal(fs_file_name, file_size, file_handle);
    }
#endif // MASTER_GOLD

    CHECK_OR_EXIT(fsltx_is_available,
        make_string("Cannot open file \"%s\".\nCheck your working folder.", fs_file_name));

#ifdef DEBUG
    Msg("final $fs_root$ = %s", get_path("$fs_root$")->m_Path);
#endif

    void* buffer = FileDownload(fs_file_name, file_handle, file_size);
    result = xr_new<CTempReader>(buffer, file_size, 0);

#ifdef DEBUG
    if (result && m_Flags.is(flBuildCopy | flReady))
        copy_file_to_build(result, fs_file_name);
#endif // DEBUG

    if (m_Flags.test(flDumpFileActivity))
        _register_open_file(result, fs_file_name);

    return result;
}

void CLocatorAPI::_initialize(u32 flags, pcstr target_folder, pcstr fs_name)
{
    char _delimiter = '|'; //','
    if (m_Flags.is(flReady))
        return;
    CTimer t;
    t.Start();
    Log("Initializing File System...");
    const size_t M1 = Memory.mem_usage();

    m_Flags.set(flags, true);

    // scan root directory
    bNoRecurse = true;
    string4096 buf;

    // append application path
    if (m_Flags.is(flScanAppRoot))
        append_path("$app_root$", Core.ApplicationPath, nullptr, false);

    //-----------------------------------------------------------
    // append application data path
    // target folder
    if (m_Flags.is(flTargetFolderOnly))
    {
        append_path("$target_folder$", target_folder, nullptr, true);
    }
    else
    {
        IReader* pFSltx = setup_fs_ltx(fs_name);

        FS_Path* pFSRoot = get_path("$fs_root$");
        rescan_path(pFSRoot->m_Path, false);

        // append all pathes
        string_path id, root, add, def, capt;
        pcstr lp_add, lp_def, lp_capt;
        string16 b_v;
        string4096 temp;

        while (!pFSltx->eof())
        {
            pFSltx->r_string(buf, sizeof buf);
            if (buf[0] == ';')
                continue;

            _GetItem(buf, 0, id, '=');

            if (!m_Flags.is(flBuildCopy) && 0 == xr_strcmp(id, "$build_copy$"))
                continue;

            _GetItem(buf, 1, temp, '=');
            int cnt = _GetItemCount(temp, _delimiter);
            R_ASSERT2(cnt >= 3, temp);
            u32 fl = 0;
            _GetItem(temp, 0, b_v, _delimiter);

            if (CInifile::isBool(b_v))
                fl |= FS_Path::flRecurse;

            _GetItem(temp, 1, b_v, _delimiter);
            if (CInifile::isBool(b_v))
                fl |= FS_Path::flNotif;

            _GetItem(temp, 2, root, _delimiter);
            _GetItem(temp, 3, add, _delimiter);
            _GetItem(temp, 4, def, _delimiter);
            _GetItem(temp, 5, capt, _delimiter);
            xr_strlwr(id);

            lp_add = cnt >= 4 ? add : 0;
            lp_def = cnt >= 5 ? def : 0;
            lp_capt = cnt >= 6 ? capt : 0;

            auto p_it = m_paths.find(root);

            FS_Path* P = xr_new<FS_Path>(p_it != m_paths.end() ? p_it->second->m_Path : root, lp_add, lp_def, lp_capt, fl);
            bNoRecurse = !(fl & FS_Path::flRecurse);
            Recurse(P->m_Path);
            auto I = m_paths.insert(std::make_pair(xr_strdup(id), P));
#ifndef DEBUG
            m_Flags.set(flCacheFiles, false);
#endif // DEBUG

            CHECK_OR_EXIT(I.second,
                "The file 'fsgame.ltx' is corrupted (it contains duplicated lines).\n"
                "Please reinstall the game or fix the problem manually.");
        }
        r_close(pFSltx);
        R_ASSERT(path_exist("$app_data_root$"));
    };

    const size_t M2 = Memory.mem_usage();
    Msg("FS: %d files cached %d archives, %dKb memory used.", m_files.size(), m_archives.size(), (M2 - M1) / 1024);

    m_Flags.set(flReady, true);

    Msg("Init FileSystem %f sec", t.GetElapsed_sec());
    //-----------------------------------------------------------
    if (strstr(Core.Params, "-overlaypath"))
    {
        string1024 c_newAppPathRoot;
        sscanf(strstr(Core.Params, "-overlaypath ") + 13, "%[^ ] ", c_newAppPathRoot);
        FS_Path* pLogsPath = get_path("$logs$");
        FS_Path* pAppdataPath = get_path("$app_data_root$");

        if (pLogsPath)
            pLogsPath->_set_root(c_newAppPathRoot);
        if (pAppdataPath)
        {
            pAppdataPath->_set_root(c_newAppPathRoot);
            rescan_path(pAppdataPath->m_Path, pAppdataPath->m_Flags.is(FS_Path::flRecurse));
        }
    }

    rec_files.clear();
    //-----------------------------------------------------------

    CreateLog(nullptr != strstr(Core.Params, "-nolog"));
    xrDebug::OnFilesystemInitialized();
}

void CLocatorAPI::_destroy()
{
    CloseLog();

    for (auto& it : m_files)
    {
        auto str = pstr(it.name);
        xr_free(str);
    }
    m_files.clear();

    for (auto& it : m_paths)
    {
        auto str = pstr(it.first);
        xr_free(str);
        xr_delete(it.second);
    }
    m_paths.clear();

    for (auto& it : m_archives)
    {
        xr_delete(it.header);
        it.close();
    }
    m_archives.clear();
}

const CLocatorAPI::file* CLocatorAPI::GetFileDesc(pcstr path)
{
    auto it = file_find_it(path);
    return it != m_files.end() ? &*it : nullptr;
}

FileStatus CLocatorAPI::exist(pcstr fn, FSType fsType /*= FSType::Virtual*/)
{
    if ((fsType | FSType::Virtual) == FSType::Virtual)
    {
        files_it it = file_find_it(fn);
        if (it != m_files.end())
            return FileStatus(true, false);
    }
    if ((fsType | FSType::External) == FSType::External)
    {
        struct stat buffer;
        buffer.st_size;
        return FileStatus(stat(fn, &buffer) == 0, true);
    }
    return FileStatus(false, false);
}

FileStatus CLocatorAPI::exist(pcstr path, pcstr name, FSType fsType /*= FSType::Virtual*/)
{
    string_path temp;
    update_path(temp, path, name);
    return exist(temp, fsType);
}

FileStatus CLocatorAPI::exist(string_path& fn, pcstr path, pcstr name, FSType fsType /*= FSType::Virtual*/)
{
    update_path(fn, path, name);
    return exist(fn, fsType);
}

FileStatus CLocatorAPI::exist(
    string_path& fn, pcstr path, pcstr name, pcstr ext, FSType fsType /*= FSType::Virtual*/)
{
    string_path nm;
    strconcat(sizeof nm, nm, name, ext);
    update_path(fn, path, nm);
    return exist(fn, fsType);
}

xr_vector<pstr>* CLocatorAPI::file_list_open(pcstr initial, pcstr folder, u32 flags)
{
    string_path N;
    R_ASSERT(initial && initial[0]);
    update_path(N, initial, folder);
    return file_list_open(N, flags);
}

xr_vector<pstr>* CLocatorAPI::file_list_open(pcstr _path, u32 flags)
{
    R_ASSERT(_path);
    VERIFY(flags);
    check_pathes();

    string_path N;

    if (path_exist(_path))
        update_path(N, _path, "");
    else
        xr_strcpy(N, sizeof N, _path);

    file desc;
    desc.name = N;
    files_it I = m_files.find(desc);
    if (I == m_files.end())
        return nullptr;

    xr_vector<char*>* dest = xr_new<xr_vector<char*>>();

    size_t base_len = xr_strlen(N);
    for (++I; I != m_files.end(); ++I)
    {
        const file& entry = *I;
        if (0 != strncmp(entry.name, N, base_len))
            break; // end of list
        const char* end_symbol = entry.name + xr_strlen(entry.name) - 1;
        if (*end_symbol != _DELIMITER)
        {
            // file
            if ((flags & FS_ListFiles) == 0)
                continue;

            const char* entry_begin = entry.name + base_len;
            if (flags & FS_RootOnly && strchr(entry_begin, _DELIMITER))
                continue; // folder in folder
            dest->push_back(xr_strdup(entry_begin));
            pstr fname = dest->back();
            if (flags & FS_ClampExt)
                if (nullptr != strext(fname))
                    *strext(fname) = 0;
        }
        else
        {
            // folder
            if ((flags & FS_ListFolders) == 0)
                continue;
            const char* entry_begin = entry.name + base_len;

            if (flags & FS_RootOnly && strchr(entry_begin, _DELIMITER) != end_symbol)
                continue; // folder in folder

            dest->push_back(xr_strdup(entry_begin));
        }
    }
    return dest;
}

void CLocatorAPI::file_list_close(xr_vector<pstr>*& lst)
{
    if (lst)
    {
        for (xr_vector<char*>::iterator I = lst->begin(); I != lst->end(); ++I)
            xr_free(*I);
        xr_delete(lst);
    }
}

size_t CLocatorAPI::file_list(FS_FileSet& dest, pcstr path, u32 flags /*= FS_ListFiles*/, pcstr mask /*= nullptr*/)
{
    R_ASSERT(path);
    VERIFY(flags);
    check_pathes();

    string_path N;
    if (path_exist(path))
        update_path(N, path, "");
    else
        xr_strcpy(N, sizeof N, path);

    file desc;
    desc.name = N;
    files_it I = m_files.find(desc);
    if (I == m_files.end())
        return 0;

    SStringVec masks;
    _SequenceToList(masks, mask);
    bool b_mask = !masks.empty();

    size_t base_len = xr_strlen(N);
    for (++I; I != m_files.end(); ++I)
    {
        const file& entry = *I;
        if (0 != strncmp(entry.name, N, base_len))
            break; // end of list
        pcstr end_symbol = entry.name + xr_strlen(entry.name) - 1;
        if (*end_symbol != _DELIMITER)
        {
            // file
            if ((flags & FS_ListFiles) == 0)
                continue;
            LPCSTR entry_begin = entry.name + base_len;
            if (flags & FS_RootOnly && strchr(entry_begin, _DELIMITER))
                continue; // folder in folder
            // check extension
            if (b_mask)
            {
                bool bOK = false;
                for (const auto& it : masks)
                {
                    if (PatternMatch(entry_begin, it.c_str()))
                    {
                        bOK = true;
                        break;
                    }
                }
                if (!bOK)
                    continue;
            }
            FS_File file;
            if (flags & FS_ClampExt)
                file.name = EFS.ChangeFileExt(entry_begin, "");
            else
                file.name = entry_begin;
            u32 fl = entry.vfs != VFS_STANDARD_FILE ? FS_File::flVFS : 0;
            file.size = entry.size_real;
            file.time_write = entry.modif;
            file.attrib = fl;
            dest.insert(file);
        }
        else
        {
            // folder
            if ((flags & FS_ListFolders) == 0)
                continue;
            LPCSTR entry_begin = entry.name + base_len;

            if (flags & FS_RootOnly && strchr(entry_begin, _DELIMITER) != end_symbol)
                continue; // folder in folder
            u32 fl = FS_File::flSubDir | (entry.vfs ? FS_File::flVFS : 0);
            dest.insert(FS_File(entry_begin, entry.size_real, entry.modif, fl));
        }
    }
    return dest.size();
}

void CLocatorAPI::check_cached_files(pstr fname, const size_t& fname_size, const file& desc, pcstr& source_name)
{
    string_path fname_copy;
    if (m_paths.size() <= 1)
        return;

    if (!path_exist("$server_root$"))
        return;

    LPCSTR path_base = get_path("$server_root$")->m_Path;
    size_t len_base = xr_strlen(path_base);
    LPCSTR path_file = fname;
    const size_t len_file = xr_strlen(path_file);
    if (len_file <= len_base)
        return;

    if (len_base == 1 && *path_base == _DELIMITER)
        len_base = 0;

    if (0 != memcmp(path_base, fname, len_base))
        return;

    bool bCopy = false;

    string_path fname_in_cache;
    update_path(fname_in_cache, "$cache$", path_file + len_base);
    files_it fit = file_find_it(fname_in_cache);
    if (fit != m_files.end())
    {
        // use
        const file& fc = *fit;
        if (fc.size_real == desc.size_real && fc.modif == desc.modif)
        {
            // use
        }
        else
        {
            // copy & use
            Msg("copy: db[%X],cache[%X] - '%s', ", desc.modif, fc.modif, fname);
            bCopy = true;
        }
    }
    else
    {
        // copy & use
        bCopy = true;
    }

    // copy if need
    if (bCopy)
    {
        IReader* _src;
        if (desc.size_real < 256 * 1024)
            _src = xr_new<CFileReader>(fname);
        else
            _src = xr_new<CVirtualFileReader>(fname);
        IWriter* _dst = xr_new<CFileWriter>(fname_in_cache, false);
        _dst->w(_src->pointer(), _src->length());
        xr_delete(_dst);
        xr_delete(_src);
        set_file_age(fname_in_cache, desc.modif);
        Register(fname_in_cache, VFS_STANDARD_FILE, 0, 0, desc.size_real, desc.size_real, desc.modif);
    }

    // Use
    source_name = &fname_copy[0];
    xr_strcpy(fname_copy, sizeof fname_copy, fname);
    xr_strcpy(fname, fname_size, fname_in_cache);
}

void CLocatorAPI::file_from_cache_impl(IReader*& R, LPSTR fname, const file& desc)
{
    if (desc.size_real < 16 * 1024)
    {
        R = xr_new<CFileReader>(fname);
        return;
    }

    R = xr_new<CVirtualFileReader>(fname);
}

void CLocatorAPI::file_from_cache_impl(CStreamReader*& R, LPSTR fname, const file& desc)
{
    CFileStreamReader* r = xr_new<CFileStreamReader>();
    r->construct(fname, BIG_FILE_READER_WINDOW_SIZE);
    R = r;
}

template <typename T> void CLocatorAPI::file_from_cache(T*& R, pstr fname, const size_t& fname_size, const file& desc, pcstr& source_name)
{
#ifdef DEBUG
    if (m_Flags.is(flCacheFiles))
        check_cached_files(fname, fname_size, desc, source_name);
#endif // DEBUG

    file_from_cache_impl(R, fname, desc);
}

void CLocatorAPI::file_from_archive(IReader*& R, pcstr fname, const file& desc)
{
    // Archived one
    archive& A = m_archives[desc.vfs];
    size_t start = desc.ptr / dwAllocGranularity * dwAllocGranularity;
    size_t end = (desc.ptr + desc.size_compressed) / dwAllocGranularity;
    if ((desc.ptr + desc.size_compressed) % dwAllocGranularity)
        end += 1;
    end *= dwAllocGranularity;
    if (end > A.size)
        end = A.size;
    const size_t sz = end - start;
#if defined(XR_PLATFORM_WINDOWS)
    u8* ptr = (u8*)MapViewOfFile(A.hSrcMap, FILE_MAP_READ, 0, start, sz);
    VERIFY3(ptr, "cannot create file mapping on file", fname);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    u8* ptr = (u8*)::mmap(NULL, sz, PROT_READ, MAP_SHARED, A.hSrcFile, start);
    VERIFY3(ptr && ptr != MAP_FAILED, "cannot create file mapping on file", fname);
#endif

    string1024 temp;
    xr_sprintf(temp, sizeof temp, "%s:%s", *A.path, fname);

#ifdef FS_DEBUG
    register_file_mapping(ptr, sz, temp);
#endif // DEBUG

    size_t ptr_offs = desc.ptr - start;
    if (desc.size_real == desc.size_compressed)
    {
        R = xr_new<CPackReader>(ptr, ptr + ptr_offs, desc.size_real);
        return;
    }

    // Compressed
    u8* dest = xr_alloc<u8>(desc.size_real);
    rtc_decompress(dest, desc.size_real, ptr + ptr_offs, desc.size_compressed);
    R = xr_new<CTempReader>(dest, desc.size_real, 0);
#if defined(XR_PLATFORM_WINDOWS)
    UnmapViewOfFile(ptr);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    ::munmap(ptr, sz);
#endif

#ifdef FS_DEBUG
    unregister_file_mapping(ptr, sz);
#endif // DEBUG
}

void CLocatorAPI::file_from_archive(CStreamReader*& R, pcstr fname, const file& desc)
{
    archive& A = m_archives[desc.vfs];
    R_ASSERT2(desc.size_compressed == desc.size_real,
        make_string("cannot use stream reading for compressed data %s, do not compress data to be streamed", fname));

    R = xr_new<CStreamReader>();
#if defined(XR_PLATFORM_WINDOWS)
    R->construct(A.hSrcMap, desc.ptr, desc.size_compressed, A.size, BIG_FILE_READER_WINDOW_SIZE);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    R->construct(A.hSrcFile, desc.ptr, desc.size_compressed, A.size, BIG_FILE_READER_WINDOW_SIZE);
#endif
}

void CLocatorAPI::copy_file_to_build(IWriter* W, IReader* r) { W->w(r->pointer(), r->length()); }
void CLocatorAPI::copy_file_to_build(IWriter* W, CStreamReader* r)
{
    const size_t buffer_size = r->length();
    u8* buffer = xr_alloc<u8>(buffer_size);
    r->r(buffer, buffer_size);
    W->w(buffer, buffer_size);
    xr_free(buffer);
    r->seek(0);
}

template <typename T>
void CLocatorAPI::copy_file_to_build(T*& r, pcstr source_name)
{
    string_path cpy_name;
    string_path e_cpy_name;
    FS_Path* P;
    // if (!(source_name==strstr(source_name,(P=get_path("$server_root$"))->m_Path)||
    // source_name==strstr(source_name,(P=get_path("$server_data_root$"))->m_Path)))
    // return;

    string_path fs_root;
    update_path(fs_root, "$fs_root$", "");
    pcstr const position = strstr(source_name, fs_root);
    if (position == source_name)
        update_path(cpy_name, "$build_copy$", source_name + xr_strlen(fs_root));
    else
        update_path(cpy_name, "$build_copy$", source_name);

    IWriter* W = w_open(cpy_name);
    if (!W)
    {
        Log("!Can't build:", source_name);
        return;
    }

    copy_file_to_build(W, r);
    w_close(W);
    set_file_age(cpy_name, get_file_age(source_name));
    if (!m_Flags.is(flEBuildCopy))
        return;

    pcstr ext = strext(cpy_name);
    if (!ext)
        return;

    IReader* R = nullptr;
    if (0 == xr_strcmp(ext, ".dds"))
    {
        P = get_path("$game_textures$");
        update_path(e_cpy_name, "$textures$", source_name + xr_strlen(P->m_Path));
        // tga
        *strext(e_cpy_name) = 0;
        xr_strcat(e_cpy_name, ".tga");
        r_close(R = r_open(e_cpy_name));
        // thm
        *strext(e_cpy_name) = 0;
        xr_strcat(e_cpy_name, ".thm");
        r_close(R = r_open(e_cpy_name));
        return;
    }

    if (0 == xr_strcmp(ext, ".ogg"))
    {
        P = get_path("$game_sounds$");
        update_path(e_cpy_name, "$sounds$", source_name + xr_strlen(P->m_Path));
        // wav
        *strext(e_cpy_name) = 0;
        xr_strcat(e_cpy_name, ".wav");
        r_close(R = r_open(e_cpy_name));
        // thm
        *strext(e_cpy_name) = 0;
        xr_strcat(e_cpy_name, ".thm");
        r_close(R = r_open(e_cpy_name));
        return;
    }

    if (0 == xr_strcmp(ext, ".object"))
    {
        xr_strcpy(e_cpy_name, sizeof e_cpy_name, source_name);
        // object thm
        *strext(e_cpy_name) = 0;
        xr_strcat(e_cpy_name, ".thm");
        R = r_open(e_cpy_name);
        if (R)
            r_close(R);
    }
}

bool CLocatorAPI::check_for_file(pcstr path, pcstr _fname, string_path& fname, const file*& desc)
{
    check_pathes();

    // correct path
    xr_strcpy(fname, _fname);
    if (path && path[0])
        update_path(fname, path, fname);

    // Search entry
    file desc_f;
    desc_f.name = fname;
    files_it I = m_files.find(desc_f);
    if (I == m_files.end())
    {
        if (!exist(fname, FSType::External))
            return false;
        const file* extFile = RegisterExternal(fname);
        if (!extFile)
            return false;
        desc = extFile;
    }
    else
        desc = &*I;
    ++dwOpenCounter;
    return true;
}

template <typename T>
T* CLocatorAPI::r_open_impl(pcstr path, pcstr _fname)
{
    T* R = nullptr;
    string_path fname;
    const file* desc = nullptr;
    pcstr source_name = &fname[0];

    if (!check_for_file(path, _fname, fname, desc))
        return nullptr;

    // OK, analyse
    if (VFS_STANDARD_FILE == desc->vfs)
        file_from_cache(R, fname, sizeof fname, *desc, source_name);
    else
        file_from_archive(R, fname, *desc);

#ifdef DEBUG
    if (R && m_Flags.is(flBuildCopy | flReady))
        copy_file_to_build(R, source_name);
#endif // DEBUG

    if (m_Flags.test(flDumpFileActivity))
        _register_open_file(R, fname);

    return (R);
}

CStreamReader* CLocatorAPI::rs_open(pcstr path, pcstr _fname) { return r_open_impl<CStreamReader>(path, _fname); }
IReader* CLocatorAPI::r_open(pcstr path, pcstr _fname) { return r_open_impl<IReader>(path, _fname); }
void CLocatorAPI::r_close(IReader*& fs)
{
    if (m_Flags.test(flDumpFileActivity))
        _unregister_open_file(fs);

    xr_delete(fs);
}

void CLocatorAPI::r_close(CStreamReader*& fs)
{
    if (m_Flags.test(flDumpFileActivity))
        _unregister_open_file(fs);

    fs->close();
}

IWriter* CLocatorAPI::w_open(pcstr path, pcstr _fname)
{
    string_path fname;
    xr_strcpy(fname, _fname);
    if (path && path[0])
        update_path(fname, path, fname);
    CFileWriter* W = xr_new<CFileWriter>(fname, false);
#ifdef _EDITOR
    if (!W->valid())
        xr_delete(W);
#endif
    return W;
}

IWriter* CLocatorAPI::w_open_ex(pcstr path, pcstr _fname)
{
    string_path fname;
    xr_strcpy(fname, _fname);
    if (path && path[0])
        update_path(fname, path, fname);
    CFileWriter* W = xr_new<CFileWriter>(fname, true);
#ifdef _EDITOR
    if (!W->valid())
        xr_delete(W);
#endif
    return W;
}

void CLocatorAPI::w_close(IWriter*& S)
{
    if (S)
    {
        R_ASSERT(S->fName.size());
        string_path fname;
        xr_strcpy(fname, sizeof fname, S->fName.c_str());
        bool bReg = S->valid();
        xr_delete(S);

        if (bReg)
        {
#if defined(XR_PLATFORM_WINDOWS)
            struct _stat st;
            _stat(fname, &st);
            Register(fname, VFS_STANDARD_FILE, 0, 0, st.st_size, st.st_size, (u32)st.st_mtime);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
            struct stat st;
            ::stat(fname, &st);
            Register(fname, VFS_STANDARD_FILE, 0, 0, st.st_size, st.st_size, (u32)st.st_mtime);
#endif
        }
    }
}

CLocatorAPI::files_it CLocatorAPI::file_find_it(pcstr fname)
{
    check_pathes();

    file desc_f;
    string_path file_name;
    VERIFY(xr_strlen(fname) * sizeof(char) < sizeof(file_name));
    xr_strcpy(file_name, sizeof file_name, fname);
    desc_f.name = file_name;

    return m_files.find(desc_f);
}

bool CLocatorAPI::dir_delete(pcstr initial, pcstr nm, bool remove_files)
{
    string_path fpath;
    if (initial && initial[0])
        update_path(fpath, initial, nm);
    else
        xr_strcpy(fpath, sizeof fpath, nm);

    files_set folders;
    files_it I;
    // remove files
    I = file_find_it(fpath);
    if (I != m_files.end())
    {
        size_t base_len = xr_strlen(fpath);
        for (; I != m_files.end();)
        {
            files_it cur_item = I;
            const file& entry = *cur_item;
            I = cur_item;
            ++I;
            if (0 != strncmp(entry.name, fpath, base_len))
                break; // end of list
            const char* end_symbol = entry.name + xr_strlen(entry.name) - 1;
            if (*end_symbol != _DELIMITER)
            {
                // const char* entry_begin = entry.name+base_len;
                if (!remove_files)
                    return false;
                xr_unlink(entry.name);
                m_files.erase(cur_item);
            }
            else
            {
                folders.insert(entry);
            }
        }
    }
    // remove folders
    files_set::reverse_iterator r_it = folders.rbegin();
    for (; r_it != folders.rend(); ++r_it)
    {
        const char* end_symbol = r_it->name + xr_strlen(r_it->name) - 1;
        if (*end_symbol == _DELIMITER)
        {
            _rmdir(r_it->name);
            m_files.erase(*r_it);
        }
    }
    return true;
}

void CLocatorAPI::file_delete(pcstr path, pcstr nm)
{
    string_path fname;
    if (path && path[0])
        update_path(fname, path, nm);
    else
        xr_strcpy(fname, sizeof fname, nm);

    const files_it I = file_find_it(fname);
    if (I != m_files.end())
    {
        // remove file
        xr_unlink(I->name);
        auto str = pstr(I->name);
        xr_free(str);
        m_files.erase(I);
    }
}

void CLocatorAPI::file_copy(pcstr src, pcstr dest)
{
    if (exist(src))
    {
        IReader* S = r_open(src);
        if (S)
        {
            IWriter* D = w_open(dest);
            if (D)
            {
                D->w(S->pointer(), S->length());
                w_close(D);
            }
            r_close(S);
        }
    }
}

void CLocatorAPI::file_rename(pcstr src, pcstr dest, bool overwrite)
{
    files_it S = file_find_it(src);
    if (S != m_files.end())
    {
        files_it D = file_find_it(dest);
        if (D != m_files.end())
        {
            if (!overwrite)
                return;
            xr_unlink(D->name);
            auto str = pstr(D->name);
            xr_free(str);
            m_files.erase(D);
        }

        file new_desc = *S;
        // remove existing item
        auto str = pstr(S->name);
        xr_free(str);
        m_files.erase(S);
        // insert updated item
        new_desc.name = xr_strdup(dest);
        m_files.insert(new_desc);

        // physically rename file
        VerifyPath(dest);
        pstr conv_dest = xr_strdup(dest);
        convert_path_separators(conv_dest);
        rename(src, conv_dest);
        xr_free(conv_dest);
    }
}

int CLocatorAPI::file_length(pcstr src)
{
    files_it it = file_find_it(src);
    if (it != m_files.end())
        return it->size_real;
    struct stat buffer;
    if (stat(src, &buffer) != -1)
        return buffer.st_size;
    return -1;
}

bool CLocatorAPI::path_exist(pcstr path)
{
    return m_paths.find(path) != m_paths.end();
}

FS_Path* CLocatorAPI::append_path(pcstr path_alias, pcstr root, pcstr add, bool recursive)
{
    VERIFY(root /**&&root[0]**/);
    VERIFY(false == path_exist(path_alias));
    FS_Path* P = xr_new<FS_Path>(root, add, nullptr, nullptr, 0);
    bNoRecurse = !recursive;
    Recurse(P->m_Path);
    m_paths.insert(std::make_pair(xr_strdup(path_alias), P));
    return P;
}

FS_Path* CLocatorAPI::get_path(pcstr path)
{
    const auto P = m_paths.find(path);
    R_ASSERT2(P != m_paths.end(), path);
    return P->second;
}

bool CLocatorAPI::get_path(pcstr path, FS_Path** outPath)
{
    const auto P = m_paths.find(path);

    if (P != m_paths.end())
    {
        if (outPath)
            *outPath = P->second;
        return true;
    }

    return false;
}

pcstr CLocatorAPI::update_path(string_path& dest, pcstr initial, pcstr src, bool crashOnNotFound /*= true*/)
{
    FS_Path* path;
    if (!get_path(initial, &path))
    {
        R_ASSERT3(!crashOnNotFound, "Failed to find FS path", initial);
        return nullptr;
    }

    return path->_update(dest, src);
}
/*
void CLocatorAPI::update_path(xr_string& dest, pcstr initial, pcstr src)
{
    return get_path(initial)->_update(dest,src);
}
}*/

u32 CLocatorAPI::get_file_age(pcstr nm)
{
    check_pathes();

    files_it I = file_find_it(nm);
    return I != m_files.end() ? I->modif : u32(-1);
}

void CLocatorAPI::set_file_age(pcstr nm, u32 age)
{
    check_pathes();

    // set file
    _utimbuf tm;
    tm.actime = age;
    tm.modtime = age;
    int res = _utime(nm, &tm);
    if (0 != res)
    {
        Msg("!Can't set file age: '%s'. Error: '%s'", nm, _sys_errlist[errno]);
    }
    else
    {
        // update record
        files_it I = file_find_it(nm);
        if (I != m_files.end())
        {
            file& F = (file&)*I;
            F.modif = age;
        }
    }
}

void CLocatorAPI::rescan_path(pcstr full_path, bool bRecurse)
{
    file desc;
    desc.name = full_path;
    files_it I = m_files.lower_bound(desc);
    if (I == m_files.end())
        return;

    size_t base_len = xr_strlen(full_path);
    for (; I != m_files.end();)
    {
        files_it cur_item = I;
        const file& entry = *cur_item;
        I = cur_item;
        ++I;
        if (0 != strncmp(entry.name, full_path, base_len))
            break; // end of list
        if (entry.vfs != VFS_STANDARD_FILE)
            continue;
        pcstr entry_begin = entry.name + base_len;
        if (!bRecurse && strchr(entry_begin, _DELIMITER))
            continue;
        // erase item
        auto str = pstr(cur_item->name);
        xr_free(str);
        m_files.erase(cur_item);
    }
    bNoRecurse = !bRecurse;
    Recurse(full_path);
}

void CLocatorAPI::rescan_pathes()
{
    m_Flags.set(flNeedRescan, false);
    for (const auto& it : m_paths)
    {
        FS_Path* P = it.second;
        if (P->m_Flags.is(FS_Path::flNeedRescan))
        {
            rescan_path(P->m_Path, P->m_Flags.is(FS_Path::flRecurse));
            P->m_Flags.set(FS_Path::flNeedRescan, false);
        }
    }
}

void CLocatorAPI::lock_rescan() { m_iLockRescan++; }
void CLocatorAPI::unlock_rescan()
{
    m_iLockRescan--;
    VERIFY(m_iLockRescan >= 0);
    if (0 == m_iLockRescan && m_Flags.is(flNeedRescan))
        rescan_pathes();
}

void CLocatorAPI::check_pathes()
{
    if (m_Flags.is(flNeedRescan) && 0 == m_iLockRescan)
    {
        lock_rescan();
        rescan_pathes();
        unlock_rescan();
    }
}

bool CLocatorAPI::can_write_to_folder(pcstr path)
{
    if (path && path[0])
    {
        string_path temp;
        pcstr fn = "$!#%TEMP%#!$.$$$";
        strconcat(sizeof temp, temp, path, path[xr_strlen(path) - 1] != _DELIMITER ? DELIMITER : "", fn);
        FILE* hf = fopen(temp, "wb");
        if (hf == nullptr)
            return false;
        fclose(hf);
        xr_unlink(temp);
        return true;
    }
    return false;
}

bool CLocatorAPI::can_write_to_alias(pcstr path)
{
    string_path temp;
    update_path(temp, path, "");
    return can_write_to_folder(temp);
}

bool CLocatorAPI::can_modify_file(pcstr fname)
{
    FILE* hf = fopen(fname, "r+b");
    if (hf)
    {
        fclose(hf);
        return true;
    }
    else
    {
        return false;
    }
}

bool CLocatorAPI::can_modify_file(pcstr path, pcstr name)
{
    string_path temp;
    update_path(temp, path, name);
    return can_modify_file(temp);
}
