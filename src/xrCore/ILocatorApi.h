#pragma once
#include "LocatorAPI_defs.h"
#include "xrCommon/xr_smart_pointers.h"

enum class FSType
{
    Virtual = 1,
    External = 2,
    Any = Virtual | External,
};

class FileStatus
{
public:
    bool Exists;
    bool External; // File can be accessed only as external

    FileStatus(bool exists, bool external) : Exists(exists), External(external) {}

    operator bool() const { return Exists; }
};

class XRCORE_API CStreamReader;
class XRCORE_API ILocatorAPI
{
    friend class FS_Path;

public:

    struct archive
    {
        size_t size = 0;
        size_t vfs_idx = size_t(-1);
        shared_str path;
        u32 modif = 0;
#if defined(XR_PLATFORM_WINDOWS)
        void *hSrcFile = nullptr;
        void *hSrcMap = nullptr;
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
        int hSrcFile = 0;
#else
#   error Select or add implementation for your platform
#endif
        CInifile* header = nullptr;
        
        archive() = default;
        void open();
        void close();
    };

    enum : u32
    {
        flNeedRescan = (1 << 0),
        flBuildCopy = (1 << 1),
        flReady = (1 << 2),
        flEBuildCopy = (1 << 3),
        flEventNotificator = (1 << 4),
        flTargetFolderOnly = (1 << 5),
        flCacheFiles = (1 << 6),
        flScanAppRoot = (1 << 7),
        flNeedCheck = (1 << 8),
        flDumpFileActivity = (1 << 9),
    };

    using archives_vec = xr_vector<archive>;
    archives_vec m_archives;

    virtual void LoadArchive(archive& A, pcstr entrypoint = nullptr) { R_ASSERT(false); }
    virtual bool load_all_unloaded_archives() { R_ASSERT(false); return false; }
    virtual void unload_archive(archive& A) { R_ASSERT(false); }

    virtual ~ILocatorAPI() = default;
    virtual void _initialize(u32 flags, pcstr target_folder = 0, pcstr fs_fname = 0) = 0;
    virtual void _destroy() = 0;

    virtual IReader* r_open(pcstr initial, pcstr N) = 0;
    virtual void r_close(IReader*& S) = 0;
    IC IReader* r_open(pcstr N) { return r_open(0, N); }

    virtual IWriter* w_open(pcstr initial, pcstr N) = 0;
    virtual IWriter* w_open_ex(pcstr initial, pcstr N) = 0;
    virtual void w_close(IWriter*& S) = 0;
    IC IWriter* w_open(pcstr N) { return w_open(0, N); }
    IC IWriter* w_open_ex(pcstr N) { return w_open_ex(0, N); }

    virtual FileStatus exist(pcstr N, FSType fsType = FSType::Virtual) = 0;
    virtual FileStatus exist(pcstr path, pcstr name, FSType fsType = FSType::Virtual) = 0;
    virtual FileStatus exist(string_path& fn, pcstr path, pcstr name, FSType fsType = FSType::Virtual) = 0;
    virtual FileStatus exist(string_path& fn, pcstr path, pcstr name, pcstr ext, FSType fsType = FSType::Virtual) = 0;

    virtual bool can_write_to_folder(pcstr path) = 0;
    virtual bool can_write_to_alias(pcstr path) = 0;
    virtual bool can_modify_file(pcstr fname) = 0;
    virtual bool can_modify_file(pcstr path, pcstr name) = 0;

    virtual bool dir_delete(pcstr initial, pcstr N, bool remove_files) = 0;
    virtual bool dir_delete(pcstr full_path, bool remove_files) { return dir_delete(0, full_path, remove_files); }
    virtual void file_delete(pcstr path, pcstr nm) = 0;
    virtual void file_delete(pcstr full_path) { file_delete(0, full_path); }
    virtual void file_copy(pcstr src, pcstr dest) = 0;
    virtual void file_rename(pcstr src, pcstr dest, bool bOwerwrite = true) = 0;
    virtual int file_length(pcstr src) = 0;

    virtual u32 get_file_age(pcstr nm) = 0;
    virtual void set_file_age(pcstr nm, u32 age) = 0;

    virtual bool path_exist(pcstr path) = 0;
    virtual FS_Path* get_path(pcstr path) = 0;
    virtual FS_Path* append_path(pcstr path_alias, pcstr root, pcstr add, bool recursive) = 0;
    virtual pcstr update_path(string_path& dest, pcstr initial, pcstr src, bool crashOnNotFound = true) = 0;

    virtual bool file_find(pcstr full_name, FS_File& f) { R_ASSERT(false); return false; };
    virtual size_t file_list(FS_FileSet& dest, pcstr path, u32 flags = FS_ListFiles, pcstr mask = 0) = 0;
    virtual void r_close(CStreamReader*& fs) { R_ASSERT(false); }
    virtual CStreamReader* rs_open(pcstr initial, pcstr N) { R_ASSERT(false); return nullptr; };

    virtual xr_vector<LPSTR>* file_list_open(pcstr initial, pcstr folder, u32 flags = FS_ListFiles) { R_ASSERT(false); return nullptr; };
    virtual xr_vector<LPSTR>* file_list_open(pcstr path, u32 flags = FS_ListFiles) { R_ASSERT(false); return nullptr; };
    virtual void file_list_close(xr_vector<LPSTR>*& lst) { R_ASSERT(false); };

    virtual void auth_generate(xr_vector<shared_str>& ignore, xr_vector<shared_str>& important) { R_ASSERT(false); };
    virtual u64 auth_get() { R_ASSERT(false); return 0; };
    virtual void auth_runtime(void*) { R_ASSERT(false); };

    virtual void rescan_path(pcstr full_path, bool bRecurse) { R_ASSERT(false); }
    virtual void rescan_pathes() { R_ASSERT(false); }

    Flags32 m_Flags;
    u32 dwAllocGranularity;
    u32 dwOpenCounter;
};

extern XRCORE_API xr_unique_ptr<ILocatorAPI> xr_FS;
#define FS (*xr_FS)
