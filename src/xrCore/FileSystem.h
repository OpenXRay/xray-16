//----------------------------------------------------
// file: FileSystem.h
//----------------------------------------------------

#ifndef FileSystemH
#define FileSystemH

#define BACKUP_FILE_LEVEL 5

class XRCORE_API EFS_Utils
{
protected:
    bool GetOpenNameInternal(
        const char* initial, char* buffer, size_t sz_buf, bool bMulti = false, const char* offset = 0, int start_flt_ext = -1);

public:
    EFS_Utils();
    virtual ~EFS_Utils();
    void _initialize() {}
    void _destroy() {}
    const char* GenerateName(const char* base_path, const char* base_name, const char* def_ext, char* out_name, size_t const out_name_size);

    bool GetOpenName(const char* initial, string_path& buffer, int sz_buf, bool bMulti = false, const char* offset = 0,
        int start_flt_ext = -1);
    bool GetOpenName(const char* initial, xr_string& buf, bool bMulti = false, const char* offset = 0, int start_flt_ext = -1);

    bool GetSaveName(const char* initial, string_path& buffer, const char* offset = 0, int start_flt_ext = -1);
    bool GetSaveName(const char* initial, xr_string& buf, const char* offset = 0, int start_flt_ext = -1);

    void MarkFile(const char* fn, bool bDeleteSource);

    xr_string AppendFolderToName(xr_string& tex_name, int depth, bool full_name);

    const char* AppendFolderToName(char* tex_name, size_t const tex_name_size, int depth, bool full_name);
    const char* AppendFolderToName(const char* src_name, char* dest_name, size_t const dest_name_size, int depth, bool full_name);

    xr_string ChangeFileExt(const char* src, const char* ext);
    xr_string ChangeFileExt(const xr_string& src, const char* ext);

    static xr_string ExtractFileName(const char* src);
    static xr_string ExtractFilePath(const char* src);
    static xr_string ExtractFileExt(const char* src);
    static xr_string ExcludeBasePath(const char* full_path, const char* excl_path);
};
extern XRCORE_API xr_unique_ptr<EFS_Utils> xr_EFS;
#define EFS (*xr_EFS)

#endif /*_INCDEF_FileSystem_H_*/
