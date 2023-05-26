#include "stdafx.h"
#include "ELocatorAPI.h"

#pragma warning(disable : 4995)
#include <io.h>
#include <direct.h>
#pragma warning(default : 4995)

extern XRCORE_API void CloseLog();

#include "xrCore/FS_internal.h"

// hack
struct _utimbuf
{
    time_t actime; // access time
    time_t modtime; // modification time
};

#define _CRT_NO_TIME_T 1
#include <sys/utime.h>

ECORE_API ELocatorAPI* xr_EditorFS = nullptr;

ELocatorAPI::ELocatorAPI()
{
	m_Flags.zero();
	// get page size
	SYSTEM_INFO sys_inf;
	GetSystemInfo(&sys_inf);
	dwAllocGranularity = sys_inf.dwAllocationGranularity;
	dwOpenCounter = 0;
}

void ELocatorAPI::InitFS(u32 flags)
{
	const char* FSLTX = "fs.ltx";

	char _delimiter = '|'; //','
	if (m_Flags.is(flReady))
		return;

	Log("Initializing File System...");
	m_Flags.set(flags, TRUE);
	string_path tmpAppPath;

	if (m_Flags.is(flScanAppRoot))
	{
		string_path tmpFsPath;
		xr_strcpy(tmpAppPath, sizeof(tmpAppPath), Core.ApplicationPath);
		xr_strcpy(tmpFsPath, sizeof(tmpAppPath), tmpAppPath);
		xr_strcat(tmpFsPath, sizeof(tmpAppPath), FSLTX);

		while (xr_strlen(tmpAppPath) && !exist(tmpFsPath))
		{
			// remove last slash if exists
			if (tmpAppPath[xr_strlen(tmpAppPath) - 1] == '\\')
				tmpAppPath[xr_strlen(tmpAppPath) - 1] = '\0';

			// go one folder up
			if (strrchr(tmpAppPath, '\\'))
				*(strrchr(tmpAppPath, '\\') + 1) = '\0';

			// update path to FSLTX
			xr_strcpy(tmpFsPath, sizeof(tmpAppPath), tmpAppPath);
			xr_strcat(tmpFsPath, sizeof(tmpAppPath), FSLTX);

			// remove last slash
			tmpAppPath[xr_strlen(tmpAppPath) - 1] = '\0';
		}

		append_path("$app_root$", tmpAppPath, 0, FALSE);
		append_path("$fs_root$", tmpAppPath, 0, FALSE);
	}
	else
		append_path("$fs_root$", "", 0, FALSE);

	xr_strcat(tmpAppPath, "\\");
	IReader *F = r_open(FSLTX);

	if (!F && m_Flags.is(flScanAppRoot))
		F = r_open("$app_root$", FSLTX);

	R_ASSERT3(F, "Can't open file:", FSLTX);
	// append all pathes
	string_path buf;
	string_path id, temp, root, add, def, capt;
	LPCSTR lp_add, lp_def, lp_capt;
	string16 b_v;
	Core.SocSdk = true;

	while (!F->eof())
	{
		F->r_string(buf, sizeof(buf));
		_GetItem(buf, 0, id, '=');

		if (id[0] == ';')
			continue;

		if (!strcmp(id, "$server_data_root$"))
			Core.SocSdk = false;

		_GetItem(buf, 1, temp, '=');
		int cnt = _GetItemCount(temp, _delimiter);
		R_ASSERT(cnt >= 3);
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
		if (!m_Flags.is(flBuildCopy) && (0 == xr_strcmp(id, "$build_copy$")))
			continue;
		xr_strlwr(root);
		lp_add = (cnt >= 4) ? xr_strlwr(add) : 0;
		lp_def = (cnt >= 5) ? def : 0;
		lp_capt = (cnt >= 6) ? capt : 0;

		PathPairIt p_it = pathes.find(root);
		const char* rootDir;

		if (p_it != pathes.end())
			rootDir = p_it->second->m_Path;
		else
		{
			// to find path when working directory is not sdk root, and root not specified
			rootDir = tmpAppPath;
			lp_add = root;
		}

		std::pair<PathPairIt, bool> I;
		FS_Path *P = xr_new<FS_Path>(rootDir, lp_add, lp_def, lp_capt, fl);
		I = pathes.insert(std::make_pair(xr_strdup(id), P));

		R_ASSERT(I.second);
	}

	r_close(F);

	m_Flags.set(flReady, TRUE);
	CreateLog(0 != strstr(Core.Params, "-nolog"));
}

void ELocatorAPI::DestroyFS()
{
	CloseLog();

	for (PathPairIt p_it = pathes.begin(); p_it != pathes.end(); p_it++)
	{
		char *str = LPSTR(p_it->first);
		xr_free(str);
		xr_delete(p_it->second);
	}
	pathes.clear();
}

BOOL ELocatorAPI::file_find(LPCSTR full_name, FS_File &f)
{
	intptr_t hFile;
	_FINDDATA_T sFile;
	// find all files
	if (-1 != (hFile = _findfirst((LPSTR)full_name, &sFile)))
	{
		f = FS_File(sFile);
		_findclose(hFile);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

const bool ELocatorAPI::exist(const char *fn)
{
	return ::GetFileAttributes(fn) != u32(-1);
}

const bool ELocatorAPI::exist(const char *path, const char *name)
{
	string_path temp;
	update_path(temp, path, name);
	return exist(temp);
}

const bool ELocatorAPI::exist(string_path &fn, const char *path, const char *name)
{
	update_path(fn, path, name);
	return exist(fn);
}

const bool ELocatorAPI::exist(string_path &fn, const char *path, const char *name, const char *ext)
{
	string_path nm;
	strconcat(sizeof(nm), nm, name, ext);
	update_path(fn, path, nm);
	return exist(fn);
}

bool ignore_name(const char *_name)
{
	// ignore processing ".svn" folders
	return (_name[0] == '.' && _name[1] == 's' && _name[2] == 'v' && _name[3] == 'n' && _name[4] == 0);
}

typedef void(__stdcall *TOnFind)(_finddata_t &, void *);
void Recurse(LPCSTR, bool, TOnFind, void *);
void ProcessOne(LPCSTR path, _finddata_t &F, bool root_only, TOnFind on_find_cb, void *data)
{
	string_path N;
	strcpy(N, path);
	strcat(N, F.name);
	xr_strlwr(N);

	if (ignore_name(N))
		return;

	if (F.attrib & _A_HIDDEN)
		return;

	if (F.attrib & _A_SUBDIR)
	{
		if (root_only)
			return;
		if (0 == xr_strcmp(F.name, "."))
			return;
		if (0 == xr_strcmp(F.name, ".."))
			return;
		strcat(N, "\\");
		strcpy(F.name, N);
		on_find_cb(F, data);
		Recurse(F.name, root_only, on_find_cb, data);
	}
	else
	{
		strcpy(F.name, N);
		on_find_cb(F, data);
	}
}

void Recurse(LPCSTR path, bool root_only, TOnFind on_find_cb, void *data)
{
	xr_string fpath = path;
	fpath += "*.*";

	// begin search
	_finddata_t sFile;
	intptr_t hFile;

	// find all files
	if (-1 == (hFile = _findfirst((LPSTR)fpath.c_str(), &sFile)))
		return;
	do
	{
		ProcessOne(path, sFile, root_only, on_find_cb, data);
	} while (_findnext(hFile, &sFile) == 0);
	_findclose(hFile);
}

struct file_list_cb_data
{
	size_t base_len;
	u32 flags;
	SStringVec *masks;
	FS_FileSet *dest;
};

void __stdcall file_list_cb(_finddata_t &entry, void *data)
{
	file_list_cb_data *D = (file_list_cb_data *)data;

	LPCSTR end_symbol = entry.name + xr_strlen(entry.name) - 1;
	if ((*end_symbol) != '\\')
	{
		// file
		if ((D->flags & FS_ListFiles) == 0)
			return;
		LPCSTR entry_begin = entry.name + D->base_len;
		if ((D->flags & FS_RootOnly) && strstr(entry_begin, "\\"))
			return; // folder in folder
		// check extension
		if (D->masks)
		{
			bool bOK = false;
			for (SStringVecIt it = D->masks->begin(); it != D->masks->end(); it++)
				if (PatternMatch(entry_begin, it->c_str()))
				{
					bOK = true;
					break;
				}
			if (!bOK)
				return;
		}
		xr_string fn = entry_begin;
		// insert file entry
		if (D->flags & FS_ClampExt)
			fn = EFS.ChangeFileExt(fn, "");
		D->dest->insert(FS_File(fn, entry));
	}
	else
	{
		// folder
		if ((D->flags & FS_ListFolders) == 0)
			return;
		LPCSTR entry_begin = entry.name + D->base_len;
		D->dest->insert(FS_File(entry_begin, entry));
	}
}

int ELocatorAPI::file_list(FS_FileSet &dest, LPCSTR path, u32 flags, LPCSTR mask)
{
	R_ASSERT(path);
	VERIFY(flags);

	string_path fpath;
	if (path_exist(path))
		update_path(fpath, path, "");
	else
		strcpy(fpath, path);

	// build mask
	SStringVec masks;
	_SequenceToList(masks, mask);

	file_list_cb_data data;
	data.base_len = xr_strlen(fpath);
	data.flags = flags;
	data.masks = masks.empty() ? 0 : &masks;
	data.dest = &dest;

	Recurse(fpath, !!(flags & FS_RootOnly), file_list_cb, &data);
	return dest.size();
}

IReader *ELocatorAPI::r_open(LPCSTR path, LPCSTR _fname)
{
	IReader *R = 0;

	// correct path
	string_path fname;
	strcpy(fname, _fname);
	xr_strlwr(fname);
	if (path && path[0])
		update_path(fname, path, fname);

	// Search entry
	FS_File desc;
	if (!file_find(fname, desc))
		return NULL;

	dwOpenCounter++;

	LPCSTR source_name = &fname[0];

	// open file
	if (desc.size < 256 * 1024)
		R = xr_new<CFileReader>(fname);
	else
		R = xr_new<CVirtualFileReader>(fname);

#ifdef DEBUG
	if (R && m_Flags.is(flBuildCopy | flReady))
	{
		string_path cpy_name;
		string_path e_cpy_name;
		FS_Path *P;
		if (source_name == strstr(source_name, (P = get_path("$server_root$"))->m_Path) ||
			source_name == strstr(source_name, (P = get_path("$server_data_root$"))->m_Path))
		{
			update_path(cpy_name, "$build_copy$", source_name + xr_strlen(P->m_Path));
			IWriter *W = w_open(cpy_name);
			if (W)
			{
				W->w(R->pointer(), R->length());
				w_close(W);
				set_file_age(cpy_name, get_file_age(source_name));
				if (m_Flags.is(flEBuildCopy))
				{
					LPCSTR ext = strext(cpy_name);
					if (ext)
					{
						IReader *R = 0;
						if (0 == xr_strcmp(ext, ".dds"))
						{
							P = get_path("$game_textures$");
							update_path(e_cpy_name, "$textures$", source_name + xr_strlen(P->m_Path));
							// tga
							*strext(e_cpy_name) = 0;
							strcat(e_cpy_name, ".tga");
							r_close(R = r_open(e_cpy_name));
							// thm
							*strext(e_cpy_name) = 0;
							strcat(e_cpy_name, ".thm");
							r_close(R = r_open(e_cpy_name));
						}
						else if (0 == xr_strcmp(ext, ".ogg"))
						{
							P = get_path("$game_sounds$");
							update_path(e_cpy_name, "$sounds$", source_name + xr_strlen(P->m_Path));
							// wav
							*strext(e_cpy_name) = 0;
							strcat(e_cpy_name, ".wav");
							r_close(R = r_open(e_cpy_name));
							// thm
							*strext(e_cpy_name) = 0;
							strcat(e_cpy_name, ".thm");
							r_close(R = r_open(e_cpy_name));
						}
						else if (0 == xr_strcmp(ext, ".object"))
						{
							strcpy(e_cpy_name, source_name);
							// object thm
							*strext(e_cpy_name) = 0;
							strcat(e_cpy_name, ".thm");
							R = r_open(e_cpy_name);
							if (R)
								r_close(R);
						}
					}
				}
			}
			else
			{
				Log("!Can't build:", source_name);
			}
		}
	}
#endif // DEBUG
	return R;
}

void ELocatorAPI::r_close(IReader *&fs)
{
	xr_delete(fs);
}

IWriter *ELocatorAPI::w_open(LPCSTR path, LPCSTR _fname)
{
	string_path fname;
	xr_strlwr(strcpy(fname, _fname)); //,".$");
	if (path && path[0])
		update_path(fname, path, fname);
	CFileWriter *W = xr_new<CFileWriter>(fname, false);
#ifdef _EDITOR
	if (!W->valid())
		xr_delete(W);
#endif
	return W;
}

IWriter *ELocatorAPI::w_open_ex(LPCSTR path, LPCSTR _fname)
{
	string_path fname;
	xr_strlwr(strcpy(fname, _fname)); //,".$");
	if (path && path[0])
		update_path(fname, path, fname);
	CFileWriter *W = xr_new<CFileWriter>(fname, true);
#ifdef _EDITOR
	if (!W->valid())
		xr_delete(W);
#endif
	return W;
}

void ELocatorAPI::w_close(IWriter *&S)
{
	if (S)
	{
		R_ASSERT(S->fName.size());
		xr_delete(S);
	}
}

struct dir_delete_cb_data
{
	FS_FileSet *folders;
	BOOL remove_files;
};

void __stdcall dir_delete_cb(_finddata_t &entry, void *data)
{
	dir_delete_cb_data *D = (dir_delete_cb_data *)data;

	if (entry.attrib & _A_SUBDIR)
		D->folders->insert(FS_File(entry));
	else if (D->remove_files)
		FS.file_delete(entry.name);
}

BOOL ELocatorAPI::dir_delete(LPCSTR initial, LPCSTR nm, BOOL remove_files)
{
	string_path fpath;
	if (initial && initial[0])
		update_path(fpath, initial, nm);
	else
		strcpy(fpath, nm);

	FS_FileSet folders;
	folders.insert(FS_File(fpath));

	// recurse find
	dir_delete_cb_data data;
	data.folders = &folders;
	data.remove_files = remove_files;
	Recurse(fpath, false, dir_delete_cb, &data);

	// remove folders
	FS_FileSet::reverse_iterator r_it = folders.rbegin();
	for (; r_it != folders.rend(); r_it++)
		_rmdir(r_it->name.c_str());
	return TRUE;
}

void ELocatorAPI::file_delete(LPCSTR path, LPCSTR nm)
{
	string_path fname;
	if (path && path[0])
		update_path(fname, path, nm);
	else
		strcpy(fname, nm);
	unlink(fname);
}

void ELocatorAPI::file_copy(LPCSTR src, LPCSTR dest)
{
	if (exist(src))
	{
		IReader *S = r_open(src);
		if (S)
		{
			IWriter *D = w_open(dest);
			if (D)
			{
				D->w(S->pointer(), S->length());
				w_close(D);
			}
			r_close(S);
		}
	}
}

void ELocatorAPI::file_rename(LPCSTR src, LPCSTR dest, bool bOwerwrite)
{
	if (bOwerwrite && exist(dest))
		unlink(dest);
	// physically rename file
	VerifyPath(dest);
	rename(src, dest);
}

int ELocatorAPI::file_length(LPCSTR src)
{
	FS_File F;
	return (file_find(src, F)) ? F.size : -1;
}

BOOL ELocatorAPI::path_exist(LPCSTR path)
{
	PathPairIt P = pathes.find(path);
	return (P != pathes.end());
}

FS_Path *ELocatorAPI::append_path(LPCSTR path_alias, LPCSTR root, LPCSTR add, BOOL recursive)
{
	VERIFY(root /*&&root[0]*/);
	VERIFY(false == path_exist(path_alias));
	FS_Path *P = xr_new<FS_Path>(root, add, LPCSTR(0), LPCSTR(0), 0);
	pathes.insert(std::make_pair(xr_strdup(path_alias), P));
	return P;
}

FS_Path *ELocatorAPI::get_path(LPCSTR path)
{
	PathPairIt P = pathes.find(path);
	R_ASSERT2(P != pathes.end(), path);
	return P->second;
}

LPCSTR ELocatorAPI::update_path(string_path &dest, LPCSTR initial, LPCSTR src)
{
	return get_path(initial)->_update(dest, src);
}
/*
void ELocatorAPI::update_path(xr_string& dest, LPCSTR initial, LPCSTR src)
{
	return get_path(initial)->_update(dest,src);
} */

time_t ELocatorAPI::get_file_age(LPCSTR nm)
{
	FS_File F;
	return (file_find(nm, F)) ? F.time_write : -1;
}

void ELocatorAPI::set_file_age(LPCSTR nm, time_t age)
{
	// set file
	_utimbuf tm;
	tm.actime = age;
	tm.modtime = age;
    int res = _utime32(nm, reinterpret_cast<__utimbuf32*>(&tm));
	if (0 != res)
		Msg("!Can't set file age: '%s'. Error: '%s'", nm, _sys_errlist[errno]);
}

BOOL ELocatorAPI::can_write_to_folder(LPCSTR path)
{
	if (path && path[0])
	{
		string_path temp;
		LPCSTR fn = "$!#%TEMP%#!$.$$$";
		strconcat(sizeof(temp), temp, path, path[xr_strlen(path) - 1] != '\\' ? "\\" : "", fn);
		FILE *hf = fopen(temp, "wb");
		if (hf == 0)
			return FALSE;
		else
		{
			fclose(hf);
			unlink(temp);
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}

BOOL ELocatorAPI::can_write_to_alias(LPCSTR path)
{
	string_path temp;
	update_path(temp, path, "");
	return can_write_to_folder(temp);
}

BOOL ELocatorAPI::can_modify_file(LPCSTR fname)
{
	FILE *hf = fopen(fname, "r+b");
	if (hf)
	{
		fclose(hf);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL ELocatorAPI::can_modify_file(LPCSTR path, LPCSTR name)
{
	string_path temp;
	update_path(temp, path, name);
	return can_modify_file(temp);
}
