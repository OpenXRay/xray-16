// LocatorAPI.h: interface for the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ELocatorAPIH
#define ELocatorAPIH
#pragma once

#include "LocatorAPI_defs.h"

class XRCORE_API CLocatorAPI  
{
	friend class FS_Path;
public:
private:
	DEFINE_MAP_PRED				(LPCSTR,FS_Path*,PathMap,PathPairIt,pred_str);
	PathMap						pathes;
public:
	enum{
		flNeedRescan			= (1<<0),
		flBuildCopy				= (1<<1),                                
		flReady					= (1<<2),
		flEBuildCopy			= (1<<3),
//.		flEventNotificator      = (1<<4),
		flTargetFolderOnly		= (1<<5),
		flCacheFiles			= (1<<6),
		flScanAppRoot			= (1<<7),
	};    
	Flags32						m_Flags			;
	u32							dwAllocGranularity;
	u32							dwOpenCounter;
public:
								CLocatorAPI		();
								~CLocatorAPI	();
	void						_initialize		(u32 flags, LPCSTR target_folder=0, LPCSTR fs_fname=0);
	void						_destroy		();

	IReader*					r_open			(LPCSTR initial, LPCSTR N);
	IC IReader*					r_open			(LPCSTR N){return r_open(0,N);}
	void						r_close			(IReader* &S);

	IWriter*					w_open			(LPCSTR initial, LPCSTR N);
	IWriter*					w_open_ex		(LPCSTR initial, LPCSTR N);
	IC IWriter*					w_open			(LPCSTR N){return w_open(0,N);}
	IC IWriter*					w_open_ex		(LPCSTR N){return w_open_ex(0,N);}
	void						w_close			(IWriter* &S);

	BOOL						exist			(LPCSTR N);
	BOOL						exist			(LPCSTR path, LPCSTR name);
	BOOL						exist			(string_path& fn, LPCSTR path, LPCSTR name);
	BOOL						exist			(string_path& fn, LPCSTR path, LPCSTR name, LPCSTR ext);

    BOOL 						can_write_to_folder	(LPCSTR path); 
    BOOL 						can_write_to_alias	(LPCSTR path); 
    BOOL						can_modify_file	(LPCSTR fname);
    BOOL						can_modify_file	(LPCSTR path, LPCSTR name);

    BOOL 						dir_delete			(LPCSTR initial, LPCSTR N,BOOL remove_files);
    BOOL 						dir_delete			(LPCSTR full_path,BOOL remove_files){return dir_delete(0,full_path,remove_files);}
    void 						file_delete			(LPCSTR path,LPCSTR nm);
    void 						file_delete			(LPCSTR full_path){file_delete(0,full_path);}
	void 						file_copy			(LPCSTR src, LPCSTR dest);
	void 						file_rename			(LPCSTR src, LPCSTR dest,bool bOwerwrite=true);
    int							file_length			(LPCSTR src);

    time_t 						get_file_age		(LPCSTR nm);
    void 						set_file_age		(LPCSTR nm, time_t age);

    BOOL						path_exist			(LPCSTR path);
    FS_Path*					get_path			(LPCSTR path);
    FS_Path*					append_path			(LPCSTR path_alias, LPCSTR root, LPCSTR add, BOOL recursive);
    LPCSTR						update_path			(string_path& dest, LPCSTR initial, LPCSTR src);

	BOOL						file_find			(LPCSTR full_name, FS_File& f);

	int							file_list			(FS_FileSet& dest, LPCSTR path, u32 flags=FS_ListFiles, LPCSTR mask=0);
//.    void						update_path			(xr_string& dest, LPCSTR initial, LPCSTR src);
};

extern XRCORE_API	CLocatorAPI*					xr_FS;
#define FS (*xr_FS)

#endif // ELocatorAPIH

