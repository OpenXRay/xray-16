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
        pcstr initial, LPSTR buffer, int sz_buf, bool bMulti = false, pcstr offset = 0, int start_flt_ext = -1);

public:
    EFS_Utils();
    virtual ~EFS_Utils();
    void _initialize() {}
    void _destroy() {}
    pcstr GenerateName(pcstr base_path, pcstr base_name, pcstr def_ext, LPSTR out_name, u32 const out_name_size);

    bool GetOpenName(pcstr initial, string_path& buffer, int sz_buf, bool bMulti = false, pcstr offset = 0,
        int start_flt_ext = -1);
    bool GetOpenName(pcstr initial, xr_string& buf, bool bMulti = false, pcstr offset = 0, int start_flt_ext = -1);

    bool GetSaveName(pcstr initial, string_path& buffer, pcstr offset = 0, int start_flt_ext = -1);
    bool GetSaveName(pcstr initial, xr_string& buf, pcstr offset = 0, int start_flt_ext = -1);

    void MarkFile(pcstr fn, bool bDeleteSource);

    xr_string AppendFolderToName(xr_string& tex_name, int depth, BOOL full_name);

    pcstr AppendFolderToName(LPSTR tex_name, u32 const tex_name_size, int depth, BOOL full_name);
    pcstr AppendFolderToName(pcstr src_name, LPSTR dest_name, u32 const dest_name_size, int depth, BOOL full_name);

    xr_string ChangeFileExt(pcstr src, pcstr ext);
    xr_string ChangeFileExt(const xr_string& src, pcstr ext);

    static xr_string ExtractFileName(pcstr src);
    static xr_string ExtractFilePath(pcstr src);
    static xr_string ExtractFileExt(pcstr src);
    static xr_string ExcludeBasePath(pcstr full_path, pcstr excl_path);
};
extern XRCORE_API xr_unique_ptr<EFS_Utils> xr_EFS;
#define EFS (*xr_EFS)

#endif /*_INCDEF_FileSystem_H_*/
