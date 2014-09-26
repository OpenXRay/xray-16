#pragma once
#ifndef XR_COMPRESS_H_INCLUDED
#define XR_COMPRESS_H_INCLUDED

class xrCompressor
{
	bool						bFast;
	bool						bStoreFiles;
	IWriter*					fs_pack_writer;
	CMemoryWriter				fs_desc;
	shared_str					target_name;
	IReader*					pPackHeader;
	CInifile*					config_ltx;
	xr_vector<char*>*			files_list;
	xr_vector<char*>*			folders_list;

	struct	ALIAS
	{
		LPCSTR		path;
		u32			crc;
		u32			c_ptr;
		u32			c_size_real;
		u32			c_size_compressed;
	};
	xr_multimap<u32,ALIAS>		aliases;

	xr_vector<shared_str>		exclude_exts;
	bool	testSKIP			(LPCSTR path);
	ALIAS*	testALIAS			(IReader* base, u32 crc, u32& a_tests);
	bool	testEqual			(LPCSTR path, IReader* base);
	bool	testVFS				(LPCSTR path);
	bool	IsFolderAccepted	(CInifile& ltx, LPCSTR path, BOOL& recurse);
	
	void	GatherFiles			(LPCSTR folder);
	
	void	write_file_header	(LPCSTR file_name, const u32 &crc, const u32 &ptr, const u32 &size_real, const u32 &size_compressed);
	void	ClosePack			();
	void	OpenPack			(LPCSTR tgt_folder, int num);
	
	void	PerformWork			();

	void	CompressOne			(LPCSTR path);



	u32						bytesSRC;
	u32						bytesDST;
	u32						filesTOTAL;
	u32						filesSKIP;
	u32						filesVFS;
	u32						filesALIAS;
	CStatTimer				t_compress;
	u8*						c_heap;
	u32						dwTimeStart;

	u32						XRP_MAX_SIZE;

public:
			xrCompressor		();
			~xrCompressor		();
	void	SetFastMode			(bool b)					{bFast=b;}
	void	SetStoreFiles		(bool b)					{bStoreFiles=b;}
	void	SetMaxVolumeSize	(u32 sz)					{XRP_MAX_SIZE=sz;}
	void	SetTargetName		(LPCSTR n)					{target_name=n;}
	void	SetPackHeaderName	(LPCSTR n);

	void	ProcessLTX			(CInifile& ini);
	void	ProcessTargetFolder	();
};

#endif