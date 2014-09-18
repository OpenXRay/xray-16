#include "stdafx.h"

#ifndef MOD_COMPRESS

string_path target_folder;
string_path new_folder, old_folder;

struct file_comparer{
	enum modes{eDontCheckFileAge=1, eDontCheckCRC=2, eDontCheckBinary=4, eDontCheckFileSize=8};
	Flags32		m_flags;
	string_path m_short_name;
	string_path m_full_name;
	CLocatorAPI* m_fs_new;
	CLocatorAPI* m_fs_old;
	IReader*	 m_freader;
	u32			m_crc32;
	u32			m_file_size;

	file_comparer(char* c, CLocatorAPI* fs1, CLocatorAPI* fs2, Flags32 flag ){
		m_flags = flag;
		m_fs_new=fs1;
		m_fs_old=fs2;
		m_freader=0;
		m_crc32=0;
//		xr_strcpy(m_short_name,c+xr_strlen(arget_folder)+1);
		xr_strcpy(m_full_name,c);

		const CLocatorAPI::file* f = m_fs_new->exist("$target_folder$",m_full_name);
		if(f)
			m_file_size = f->size_real;
	}

	bool operator ()(char* o){
		//compare file names
		int eq = xr_strcmp(m_full_name,o);
		if(0!=eq)
			return false;
		

		
		if( !m_flags.test(eDontCheckFileSize) ){
			//compare file size
			const CLocatorAPI::file* f = m_fs_old->exist("$target_folder$",o);
			u32 file_size = f->size_real;

			if ( (f->vfs==0xffffffff) && (file_size != m_file_size) )
				return false;
		};
		//compare file crc
		if( !m_crc32 && !m_flags.test(eDontCheckCRC) ){
			IReader* r	=	m_fs_new->r_open	("$target_folder$",m_full_name);
			m_crc32		=	crc32		(r->pointer(),r->length());
			m_fs_new->r_close(r);
		};

		if( !m_flags.test(eDontCheckCRC) ){
			IReader* r_	= m_fs_old->r_open("$target_folder$",o);
			u32 crc32_	=  crc32	(r_->pointer(),r_->length());
			m_fs_old->r_close(r_);
			if(m_crc32!=crc32_)
				return false;
		}

		if( !m_flags.test(eDontCheckBinary) ){
			//compare files binary content
			IReader* f1		=	m_fs_new->r_open	("$target_folder$",m_full_name);
			IReader* f2		=	m_fs_old->r_open	("$target_folder$",o);

			int res = memcmp(f1->pointer(),f2->pointer(),f1->length());
			m_fs_new->r_close(f1);
			m_fs_old->r_close(f2);

			if(0!=res)
				return false;
		}
		return true;
	}
};



int ProcessDifference()
{
	LPCSTR params = GetCommandLine();
	Flags32 _flags;
	_flags.zero();
	if(strstr(params,"-diff /?")){
		printf("HELP:\n");
		printf("xrCompress.exe -diff <new_data> <old_data> -out <diff_resulf> [options]\n");
		printf("<new_data>, <old_data> and <diff_resulf> values must be a folder name\n");
		printf("[options] are set of:\n");
		printf("-nofileage		do not perform file age checking\n");
		printf("-crc			do not perform crc32 checking\n");
		printf("-nobinary		do not perform binary content checking\n");
		printf("-nosize			do not perform file size checking\n");
		return 3;
	}

	CLocatorAPI* FS_new = NULL;
	CLocatorAPI* FS_old = NULL;
	

	xr_vector<char*>*	file_list_old		= NULL;
	xr_vector<char*>*	folder_list_old		= NULL;

	xr_vector<char*>*	file_list_new		= NULL;
	xr_vector<char*>*	folder_list_new		= NULL;

	sscanf					(strstr(params,"-diff ")+6,"%[^ ] ",new_folder);
	sscanf					(strstr(params,"-diff ")+6+xr_strlen(new_folder)+1,"%[^ ] ",old_folder);
	sscanf					(strstr(params,"-out ")+5,"%[^ ] ",target_folder);
	
	if(strstr(params,"-nofileage")){
		_flags.set(file_comparer::eDontCheckFileAge, TRUE);
	};
	if(strstr(params,"-nocrc")){
		_flags.set(file_comparer::eDontCheckCRC, TRUE);
	};
	if(strstr(params,"-nobinary")){
		_flags.set(file_comparer::eDontCheckBinary, TRUE);
	};
	if(strstr(params,"-nosize")){
		_flags.set(file_comparer::eDontCheckFileSize, TRUE);
	};

	FS_new = xr_new<CLocatorAPI>	();
	FS_new->_initialize(CLocatorAPI::flTargetFolderOnly,new_folder);
	file_list_new	= FS_new->file_list_open	("$target_folder$",FS_ListFiles);
	folder_list_new	= FS_new->file_list_open	("$target_folder$",FS_ListFolders);

	FS_old = xr_new<CLocatorAPI>	();
	FS_old->_initialize(CLocatorAPI::flTargetFolderOnly,old_folder);
	file_list_old	= FS_old->file_list_open	("$target_folder$",FS_ListFiles);
	folder_list_old	= FS_old->file_list_open	("$target_folder$",FS_ListFolders);

	xr_vector<LPCSTR> target_file_list;
	target_file_list.reserve(file_list_new->size());

	for(u32 i=0; i<file_list_new->size();++i){
		file_comparer fc(file_list_new->at(i),FS_new, FS_old,_flags);
		xr_vector<char*>::iterator it = std::find_if(file_list_old->begin(),file_list_old->end(),fc);
		if(it != file_list_old->end()){
			printf("skip file %s\n",file_list_new->at(i));
		}else
			target_file_list.push_back(file_list_new->at(i));
	}

	string_path out_path;
	string_path stats;
	u32 total = target_file_list.size();
	for(u32 i=0; i<total; ++i)
	{
		LPCSTR fn = target_file_list[i];
		xr_sprintf(stats,"%d of %d (%3.1f%%)", i, total, 100.0f*((float)i/(float)total));
		SetConsoleTitle		(stats);

		strconcat(sizeof(out_path),out_path,target_folder,"\\",fn);
		VerifyPath(out_path);
		IReader* r = FS_new->r_open("$target_folder$",fn);
		IWriter* w = FS_old->w_open(out_path);
		w->w(r->pointer(),r->length());
		FS_new->r_close(r);
		FS_old->w_close(w);
	}


	FS_new->file_list_close(file_list_new);
	FS_new->file_list_close(folder_list_new);

	FS_old->file_list_close(file_list_old);
	FS_old->file_list_close(folder_list_old);

	xr_delete(FS_new);
	xr_delete(FS_old);

	return 0;
}
#endif