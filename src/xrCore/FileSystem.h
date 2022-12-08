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
        LPCSTR initial, pstr buffer, size_t sz_buf, bool bMulti = false, LPCSTR offset = 0, int start_flt_ext = -1);

public:
    void _initialize() {}
    void _destroy() {}
    LPCSTR GenerateName(LPCSTR base_path, LPCSTR base_name, LPCSTR def_ext, pstr out_name, size_t const out_name_size);

    bool GetOpenName(LPCSTR initial, string_path& buffer, int sz_buf, bool bMulti = false, LPCSTR offset = 0,
        int start_flt_ext = -1);
    bool GetOpenName(LPCSTR initial, xr_string& buf, bool bMulti = false, LPCSTR offset = 0, int start_flt_ext = -1);

    bool GetSaveName(LPCSTR initial, string_path& buffer, LPCSTR offset = 0, int start_flt_ext = -1);
    bool GetSaveName(LPCSTR initial, xr_string& buf, LPCSTR offset = 0, int start_flt_ext = -1);

    void MarkFile(LPCSTR fn, bool bDeleteSource);

    xr_string AppendFolderToName(xr_string& tex_name, int depth, BOOL full_name);

    LPCSTR AppendFolderToName(pstr tex_name, size_t const tex_name_size, int depth, BOOL full_name);
    LPCSTR AppendFolderToName(LPCSTR src_name, pstr dest_name, size_t const dest_name_size, int depth, BOOL full_name);

    xr_string ChangeFileExt(LPCSTR src, LPCSTR ext);
    xr_string ChangeFileExt(const xr_string& src, LPCSTR ext);

    static xr_string ExtractFileName(LPCSTR src);
    static xr_string ExtractFilePath(LPCSTR src);
    static xr_string ExtractFileExt(LPCSTR src);
    static xr_string ExcludeBasePath(LPCSTR full_path, LPCSTR excl_path);
};
extern XRCORE_API xr_unique_ptr<EFS_Utils> xr_EFS;
#define EFS (*xr_EFS)

#endif /*_INCDEF_FileSystem_H_*/
