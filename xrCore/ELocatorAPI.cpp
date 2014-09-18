// LocatorAPI.cpp: implementation of the CLocatorAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys\stat.h>
#pragma warning(default:4995)

#include "FS_internal.h"

CLocatorAPI*	xr_FS	= NULL;

#ifdef _EDITOR
#define FSLTX	"fs.ltx"
#else
#define FSLTX	"fsgame.ltx"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CLocatorAPI::CLocatorAPI()
{
    m_Flags.zero		();
	// get page size
	SYSTEM_INFO			sys_inf;
	GetSystemInfo		(&sys_inf);
	dwAllocGranularity	= sys_inf.dwAllocationGranularity;
	dwOpenCounter		= 0;
}

CLocatorAPI::~CLocatorAPI()
{
}

void CLocatorAPI::_initialize	(u32 flags, LPCSTR target_folder, LPCSTR fs_fname)
{
	char _delimiter = '|'; //','
	if (m_Flags.is(flReady))return;

	Log				("Initializing File System...");
	m_Flags.set		(flags,TRUE);

	append_path	("$fs_root$", "", 0, FALSE);

	// append application path

	if (m_Flags.is(flScanAppRoot)){
		append_path		("$app_root$",Core.ApplicationPath,0,FALSE);
    }
	if (m_Flags.is(flTargetFolderOnly)){
		append_path		("$target_folder$",target_folder,0,TRUE);
	}else{
		IReader* F		= r_open((fs_fname&&fs_fname[0])?fs_fname:FSLTX); 
		if (!F&&m_Flags.is(flScanAppRoot))
			F			= r_open("$app_root$",(fs_fname&&fs_fname[0])?fs_fname:FSLTX); 
		R_ASSERT3		(F,"Can't open file:", (fs_fname&&fs_fname[0])?fs_fname:FSLTX);
		// append all pathes    
		string_path		buf;
		string_path		id, temp, root, add, def, capt;
		LPCSTR			lp_add, lp_def, lp_capt;
		string16		b_v;
		while(!F->eof()){
			F->r_string	(buf,sizeof(buf));
			_GetItem(buf,0,id,'=');
			if (id[0]==';') continue;
			_GetItem(buf,1,temp,'=');
			int cnt		= _GetItemCount(temp,_delimiter);  R_ASSERT(cnt>=3);
			u32 fl		= 0;
			_GetItem	(temp,0,b_v,_delimiter);	if (CInifile::IsBOOL(b_v)) fl |= FS_Path::flRecurse;
			_GetItem	(temp,1,b_v,_delimiter);	if (CInifile::IsBOOL(b_v)) fl |= FS_Path::flNotif;
			_GetItem	(temp,2,root,_delimiter);
			_GetItem	(temp,3,add,_delimiter);
			_GetItem	(temp,4,def,_delimiter);
			_GetItem	(temp,5,capt,_delimiter);
			xr_strlwr	(id);			if (!m_Flags.is(flBuildCopy)&&(0==xr_strcmp(id,"$build_copy$"))) continue;
			xr_strlwr	(root);
			lp_add		=(cnt>=4)?xr_strlwr(add):0;
			lp_def		=(cnt>=5)?def:0;
			lp_capt		=(cnt>=6)?capt:0;
			PathPairIt p_it = pathes.find(root);
			std::pair<PathPairIt, bool> I;
			FS_Path* P	= xr_new<FS_Path>((p_it!=pathes.end())?p_it->second->m_Path:root,lp_add,lp_def,lp_capt,fl);
			I			= pathes.insert(mk_pair(xr_strdup(id),P));
			
			R_ASSERT	(I.second);
		}
		r_close			(F);
	};

	m_Flags.set		(flReady,TRUE);

	CreateLog		(0!=strstr(Core.Params,"-nolog"));
}

void CLocatorAPI::_destroy		()
{
	CloseLog		();

	for(PathPairIt p_it=pathes.begin(); p_it!=pathes.end(); p_it++){
		char* str	= LPSTR(p_it->first);
		xr_free		(str);
		xr_delete	(p_it->second);
    }
	pathes.clear	();
}

BOOL CLocatorAPI::file_find(LPCSTR full_name, FS_File& f)
{
    intptr_t		hFile;
    _FINDDATA_T		sFile;
	// find all files    
	if (-1!=(hFile=_findfirst((LPSTR)full_name, &sFile))){
    	f			= FS_File(sFile);
        _findclose	(hFile);
        return		TRUE;
    }else{
    	return		FALSE;
    }
}

BOOL CLocatorAPI::exist			(const char* fn)
{
	return ::GetFileAttributes(fn) != u32(-1);
}

BOOL CLocatorAPI::exist			(const char* path, const char* name)
{
	string_path		temp;       
    update_path		(temp,path,name);
	return exist	(temp);
}

BOOL CLocatorAPI::exist			(string_path& fn, const char* path, const char* name)
{
    update_path		(fn,path,name);
	return exist	(fn);
}

BOOL CLocatorAPI::exist			(string_path& fn, const char* path, const char* name, const char* ext)
{
	string_path 	nm;
	strconcat		(sizeof(nm),nm,name,ext);
    update_path		(fn,path,nm);
	return exist	(fn);
}

bool ignore_name(const char* _name)
{
	// ignore processing ".svn" folders
	return ( _name[0]=='.' && _name[1]=='s' && _name[2]=='v' && _name[3]=='n' && _name[4]==0) ;
}

typedef void	(__stdcall * TOnFind)	(_finddata_t&, void*);
void Recurse	(LPCSTR, bool, TOnFind, void*);
void ProcessOne	(LPCSTR path, _finddata_t& F, bool root_only, TOnFind on_find_cb, void* data)
{
	string_path	N;
	strcpy		(N,path);
	strcat		(N,F.name);
	xr_strlwr	(N);

	if (ignore_name(N))					return;
    
	if (F.attrib&_A_HIDDEN)				return;

	if (F.attrib&_A_SUBDIR) {
    	if (root_only)					return;
		if (0==xr_strcmp(F.name,"."))	return;
		if (0==xr_strcmp(F.name,"..")) 	return;
		strcat		(N,"\\");
	    strcpy		(F.name,N);
        on_find_cb	(F,data);
		Recurse		(F.name,root_only,on_find_cb,data);
	} else {
	    strcpy		(F.name,N);
        on_find_cb	(F,data);
	}
}

void Recurse(LPCSTR path, bool root_only, TOnFind on_find_cb, void* data)
{
	xr_string		fpath	= path;
	fpath			+= "*.*";

    // begin search
    _finddata_t		sFile;
    intptr_t		hFile;

	// find all files    
	if (-1==(hFile=_findfirst((LPSTR)fpath.c_str(), &sFile)))
    	return;
    do{
    	ProcessOne	(path,sFile, root_only, on_find_cb, data);
    }while(_findnext(hFile,&sFile)==0);
	_findclose		( hFile );
}

struct file_list_cb_data
{
	size_t 		base_len;
    u32 		flags;
    SStringVec* masks;
    FS_FileSet* dest;
};

void __stdcall file_list_cb(_finddata_t& entry, void* data)
{
	file_list_cb_data*	D		= (file_list_cb_data*)data;

    LPCSTR end_symbol 			= entry.name+xr_strlen(entry.name)-1;
    if ((*end_symbol)!='\\'){
        // file
        if ((D->flags&FS_ListFiles) == 0)	return;
        LPCSTR entry_begin 		= entry.name+D->base_len;
        if ((D->flags&FS_RootOnly)&&strstr(entry_begin,"\\"))	return;	// folder in folder
        // check extension
        if (D->masks){
            bool bOK			= false;
            for (SStringVecIt it=D->masks->begin(); it!=D->masks->end(); it++)
                if (PatternMatch(entry_begin,it->c_str())){bOK=true; break;}
            if (!bOK)			return;
        }
        xr_string fn			= entry_begin;
        // insert file entry
        if (D->flags&FS_ClampExt) fn=EFS.ChangeFileExt(fn,"");
        D->dest->insert			(FS_File(fn,entry));
    } else {
        // folder
        if ((D->flags&FS_ListFolders) == 0)	return;
        LPCSTR entry_begin 		= entry.name+D->base_len;
        D->dest->insert			(FS_File(entry_begin,entry));
    }
}

int CLocatorAPI::file_list(FS_FileSet& dest, LPCSTR path, u32 flags, LPCSTR mask)
{
	R_ASSERT		(path);
	VERIFY			(flags);
               
	string_path		fpath;
	if (path_exist(path))
    	update_path	(fpath,path,"");
    else
    	strcpy(fpath,path);

    // build mask
	SStringVec 		masks;
	_SequenceToList	(masks,mask);

    file_list_cb_data data;
    data.base_len	= xr_strlen(fpath);
    data.flags		= flags;
    data.masks		= masks.empty()?0:&masks;
    data.dest		= &dest;

    Recurse			(fpath,!!(flags&FS_RootOnly),file_list_cb,&data);
	return dest.size();
}

IReader* CLocatorAPI::r_open	(LPCSTR path, LPCSTR _fname)
{
	IReader* R		= 0;

	// correct path
	string_path		fname;
	strcpy			(fname,_fname);
	xr_strlwr		(fname);
	if (path&&path[0]) update_path(fname,path,fname);

	// Search entry
    FS_File			desc;
    if (!file_find(fname,desc)) return NULL;

	dwOpenCounter	++;

	LPCSTR	source_name 	= &fname[0];

    // open file
    if (desc.size<256*1024)	R = xr_new<CFileReader>			(fname);
    else			  		R = xr_new<CVirtualFileReader>	(fname);
    
#ifdef DEBUG
	if ( R && m_Flags.is(flBuildCopy|flReady) ){
		string_path	cpy_name;
		string_path	e_cpy_name;
		FS_Path* 	P; 
		if (source_name==strstr(source_name,(P=get_path("$server_root$"))->m_Path)||
        	source_name==strstr(source_name,(P=get_path("$server_data_root$"))->m_Path)){
			update_path			(cpy_name,"$build_copy$",source_name+xr_strlen(P->m_Path));
			IWriter* W = w_open	(cpy_name);
            if (W){
                W->w				(R->pointer(),R->length());
                w_close				(W);
                set_file_age(cpy_name,get_file_age(source_name));
                if (m_Flags.is(flEBuildCopy)){
                    LPCSTR ext		= strext(cpy_name);
                    if (ext){
                        IReader* R		= 0;
                        if (0==xr_strcmp(ext,".dds")){
                            P			= get_path("$game_textures$");               
                            update_path	(e_cpy_name,"$textures$",source_name+xr_strlen(P->m_Path));
                            // tga
                            *strext		(e_cpy_name) = 0;
                            strcat		(e_cpy_name,".tga");
                            r_close		(R=r_open(e_cpy_name));
                            // thm
                            *strext		(e_cpy_name) = 0;
                            strcat		(e_cpy_name,".thm");
                            r_close		(R=r_open(e_cpy_name));
                        }else if (0==xr_strcmp(ext,".ogg")){
                            P			= get_path("$game_sounds$");                               
                            update_path	(e_cpy_name,"$sounds$",source_name+xr_strlen(P->m_Path));
                            // wav
                            *strext		(e_cpy_name) = 0;
                            strcat		(e_cpy_name,".wav");
                            r_close		(R=r_open(e_cpy_name));
                            // thm
                            *strext		(e_cpy_name) = 0;
                            strcat		(e_cpy_name,".thm");
                            r_close		(R=r_open(e_cpy_name));
                        }else if (0==xr_strcmp(ext,".object")){
                            strcpy		(e_cpy_name,source_name);
                            // object thm
                            *strext		(e_cpy_name) = 0;
                            strcat		(e_cpy_name,".thm");
                            R			= r_open(e_cpy_name);
                            if (R)		r_close	(R);
                        }
                    }
                }
            }else{
            	Log			("!Can't build:",source_name);
            }
		}
	}
#endif // DEBUG
	return R;
}

void	CLocatorAPI::r_close	(IReader* &fs)
{
	xr_delete	(fs);
}

IWriter* CLocatorAPI::w_open	(LPCSTR path, LPCSTR _fname)
{
	string_path	fname;
	xr_strlwr(strcpy(fname,_fname));//,".$");
	if (path&&path[0]) update_path(fname,path,fname);
    CFileWriter* W 	= xr_new<CFileWriter>(fname,false); 
#ifdef _EDITOR
	if (!W->valid()) xr_delete(W);
#endif    
	return W;
}

IWriter* CLocatorAPI::w_open_ex	(LPCSTR path, LPCSTR _fname)
{
	string_path	fname;
	xr_strlwr(strcpy(fname,_fname));//,".$");
	if (path&&path[0]) update_path(fname,path,fname);
    CFileWriter* W 	= xr_new<CFileWriter>(fname,true); 
#ifdef _EDITOR
	if (!W->valid()) xr_delete(W);
#endif    
	return W;
}

void	CLocatorAPI::w_close(IWriter* &S)
{
	if (S){
        R_ASSERT	(S->fName.size());
        xr_delete	(S);
    }
}

struct dir_delete_cb_data
{
	FS_FileSet*		folders;
    BOOL			remove_files;
};

void __stdcall dir_delete_cb(_finddata_t& entry, void* data)
{
	dir_delete_cb_data*	D		= (dir_delete_cb_data*)data;

    if (entry.attrib&_A_SUBDIR)	D->folders->insert	(FS_File(entry));
    else if (D->remove_files)	FS.file_delete		(entry.name);
}

BOOL CLocatorAPI::dir_delete(LPCSTR initial, LPCSTR nm, BOOL remove_files)
{
	string_path			fpath;
	if (initial&&initial[0])
    	update_path	(fpath,initial,nm);
    else
    	strcpy		(fpath,nm);

    FS_FileSet 			folders;
    folders.insert		(FS_File(fpath));

    // recurse find
    dir_delete_cb_data	data;
	data.folders		= &folders;
    data.remove_files	= remove_files;
    Recurse				(fpath,false,dir_delete_cb,&data);

    // remove folders
    FS_FileSet::reverse_iterator r_it = folders.rbegin();
    for (;r_it!=folders.rend();r_it++)
		_rmdir			(r_it->name.c_str());
    return TRUE;
}                                                

void CLocatorAPI::file_delete(LPCSTR path, LPCSTR nm)
{
	string_path	fname;
	if (path&&path[0])
    	update_path	(fname,path,nm);
    else
    	strcpy		(fname,nm);
    unlink			(fname);
}

void CLocatorAPI::file_copy(LPCSTR src, LPCSTR dest)
{
	if (exist(src)){
        IReader* S		= r_open(src);
        if (S){
            IWriter* D	= w_open(dest);
            if (D){
                D->w	(S->pointer(),S->length());
                w_close	(D);
            }
            r_close		(S);
        }
	}
}

void CLocatorAPI::file_rename(LPCSTR src, LPCSTR dest, bool bOwerwrite)
{
	if (bOwerwrite&&exist(dest)) unlink(dest);
    // physically rename file
    VerifyPath			(dest);
    rename				(src,dest);
}

int	CLocatorAPI::file_length(LPCSTR src)
{
	FS_File F;
    return (file_find(src,F))?F.size:-1;
}

BOOL CLocatorAPI::path_exist(LPCSTR path)
{
    PathPairIt P 			= pathes.find(path); 
    return					(P!=pathes.end());
}

FS_Path* CLocatorAPI::append_path(LPCSTR path_alias, LPCSTR root, LPCSTR add, BOOL recursive)
{
	VERIFY			(root/*&&root[0]*/);
	VERIFY			(false==path_exist(path_alias));
	FS_Path* P		= xr_new<FS_Path>(root,add,LPCSTR(0),LPCSTR(0),0);
	pathes.insert	(mk_pair(xr_strdup(path_alias),P));
	return P;
}

FS_Path* CLocatorAPI::get_path(LPCSTR path)
{
    PathPairIt P 			= pathes.find(path); 
    R_ASSERT2(P!=pathes.end(),path);
    return P->second;
}

LPCSTR CLocatorAPI::update_path(string_path& dest, LPCSTR initial, LPCSTR src)
{
    return get_path(initial)->_update(dest,src);
}
/*
void CLocatorAPI::update_path(xr_string& dest, LPCSTR initial, LPCSTR src)
{
    return get_path(initial)->_update(dest,src);
} */

time_t CLocatorAPI::get_file_age(LPCSTR nm)
{
	FS_File F;
    return (file_find(nm,F))?F.time_write:-1;
}

void CLocatorAPI::set_file_age(LPCSTR nm, time_t age)
{
    // set file
    _utimbuf	tm;
    tm.actime	= age;
    tm.modtime	= age;
    int res 	= _utime(nm,&tm);
    if (0!=res)	Msg("!Can't set file age: '%s'. Error: '%s'",nm,_sys_errlist[errno]);
}

BOOL CLocatorAPI::can_write_to_folder(LPCSTR path)
{
	if (path&&path[0]){
		string_path		temp;       
        LPCSTR fn		= "$!#%TEMP%#!$.$$$";
	    strconcat		(sizeof(temp),temp,path,path[xr_strlen(path)-1]!='\\'?"\\":"",fn);
		FILE* hf		= fopen	(temp, "wb");
		if (hf==0)		return FALSE;
        else{
        	fclose 		(hf);
	    	unlink		(temp);
            return 		TRUE;
        }
    }else{
    	return 			FALSE;
    }
}

BOOL CLocatorAPI::can_write_to_alias(LPCSTR path)
{
	string_path			temp;       
    update_path			(temp,path,"");
	return can_write_to_folder(temp);
}

BOOL CLocatorAPI::can_modify_file(LPCSTR fname)
{
	FILE* hf			= fopen	(fname, "r+b");
    if (hf){	
    	fclose			(hf);
        return 			TRUE;
    }else{
    	return 			FALSE;
    }
}

BOOL CLocatorAPI::can_modify_file(LPCSTR path, LPCSTR name)
{
	string_path			temp;       
    update_path			(temp,path,name);
	return can_modify_file(temp);
}
