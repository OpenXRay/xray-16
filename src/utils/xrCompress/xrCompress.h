#pragma once
#ifndef XR_COMPRESS_H_INCLUDED
#define XR_COMPRESS_H_INCLUDED

class xrCompressor
{
    bool bFast{};
    bool bStoreFiles{};
    bool bPackingToXDB;
    IWriter* fs_pack_writer{};
    CMemoryWriter fs_desc;
    shared_str target_name;
    shared_str output_name;
    IReader* pPackHeader{};
    CInifile* config_ltx{};
    xr_vector<char*>* files_list{};
    xr_vector<char*>* folders_list{};

    struct ALIAS
    {
        LPCSTR path;
        u32 crc;
        u32 c_ptr;
        u32 c_size_real;
        u32 c_size_compressed;
    };
    xr_multimap<u32, ALIAS> aliases;

    xr_vector<shared_str> exclude_exts;
    bool testSKIP(LPCSTR path) const;
    ALIAS* testALIAS(IReader* base, u32 crc, u32& a_tests);
    bool testEqual(LPCSTR path, IReader* base);
    bool testVFS(LPCSTR path) const;
    bool IsFolderAccepted(const CInifile& ltx, LPCSTR path, bool& recurse) const;

    void GatherFiles(LPCSTR folder) const;

    void ClosePack();
    void OpenPack(LPCSTR tgt_folder, int num);

    void PerformWork();

    void CompressOne(LPCSTR path);

    u32 bytesSRC{};
    u32 filesTOTAL{};
    u32 filesSKIP{};
    u32 filesVFS{};
    u32 filesALIAS{};
    CStatTimer t_compress;
    u8* c_heap{};
    u32 dwTimeStart{};

    size_t XRP_TARGET_SIZE{ XRP_MAX_SIZE }; // bytes (640Mb)

public:
    static constexpr
    size_t XRP_MAX_SIZE{ 1024u * 1024u * 1900u }; // bytes (1900Mb)

public:
    xrCompressor() = default;
    ~xrCompressor();

    void SetFastMode(bool b) { bFast = b; }
    void SetStoreFiles(bool b) { bStoreFiles = b; }
    void SetPackingToXDB(bool b) { bPackingToXDB = b; }
    void SetTargetName(LPCSTR n) { target_name = n; }
    void SetOutputName(LPCSTR n) { output_name = n; }
    void SetPackHeaderName(LPCSTR n);
    void SetMaxVolumeSize(size_t size)
    {
        if (size > XRP_MAX_SIZE)
        {
            Log("~ Trying to set too big target archive size. Setting it to 1900 MB, which is max.");
            XRP_TARGET_SIZE = XRP_MAX_SIZE;
            return;
        }
        XRP_TARGET_SIZE = size;
    }

    void ProcessLTX(CInifile& ini);
    void ProcessTargetFolder();
};

#endif
