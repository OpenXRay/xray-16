#pragma once
#include "xrCore/LocatorAPI_defs.h"

class ELocatorAPI : Noncopyable, public ILocatorAPI
{
	friend class FS_Path;

public:
private:
	DEFINE_MAP_PRED(pcstr, FS_Path *, PathMap, PathPairIt, pred_str);
	PathMap pathes;

public:
	ELocatorAPI();
	~ELocatorAPI() = default;

	void _initialize(u32 flags, pcstr target_folder = 0, pcstr fs_fname = 0) override;
    void _destroy();

	IReader *r_open(pcstr initial, pcstr N);
	IC IReader *r_open(pcstr N) { return r_open(0, N); }
	void r_close(IReader *&S);

	IWriter *w_open(pcstr initial, pcstr N);
	IWriter *w_open_ex(pcstr initial, pcstr N);
	IC IWriter *w_open(pcstr N) { return w_open(0, N); }
	IC IWriter *w_open_ex(pcstr N) { return w_open_ex(0, N); }
	void w_close(IWriter *&S);

	FileStatus exist(pcstr N, FSType fsType = FSType::Virtual);
    FileStatus exist(pcstr path, pcstr name, FSType fsType = FSType::Virtual);
    FileStatus exist(string_path& fn, pcstr path, pcstr name, FSType fsType = FSType::Virtual);
    FileStatus exist(string_path& fn, pcstr path, pcstr name, pcstr ext, FSType fsType = FSType::Virtual);

	bool can_write_to_folder(pcstr path);
	bool can_write_to_alias(pcstr path);
	bool can_modify_file(pcstr fname);
	bool can_modify_file(pcstr path, pcstr name);

	bool dir_delete(pcstr initial, pcstr N, bool remove_files);
	bool dir_delete(pcstr full_path, bool remove_files) { return dir_delete(0, full_path, remove_files); }
	void file_delete(pcstr path, pcstr nm);
	void file_delete(pcstr full_path) { file_delete(0, full_path); }
	void file_copy(pcstr src, pcstr dest);
	void file_rename(pcstr src, pcstr dest, bool bOwerwrite = true);
	int file_length(pcstr src);

	u32 get_file_age(pcstr nm);
	void set_file_age(pcstr nm, u32 age);

	bool path_exist(pcstr path);
	FS_Path *get_path(pcstr path);
	FS_Path *append_path(pcstr path_alias, pcstr root, pcstr add, bool recursive);
    pcstr update_path(string_path& dest, pcstr initial, pcstr src, bool crashOnNotFound = false);

	bool file_find(pcstr full_name, FS_File &f);
	size_t file_list(FS_FileSet &dest, pcstr path, u32 flags = FS_ListFiles, pcstr mask = 0);
};
