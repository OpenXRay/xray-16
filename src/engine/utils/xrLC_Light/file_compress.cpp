#include "stdafx.h"
#include "../../3rd party/bugtrap/zlib/zlib.h"
#pragma comment(lib, "zlib.lib")
void compress( LPCSTR f_in, LPCSTR f_out );
void decompress( LPCSTR f_in, LPCSTR f_out );

void compress( LPCSTR f_in, LPCSTR f_out )
{
	FILE * file = fopen( f_in, "rb" );
	u32 buff_size = 1024*1024/2;

	
	void *buff = _alloca( buff_size );
	gzFile z_file = gzopen(f_out, "wb");
	u32 const length	= _filelength( _fileno( file ) );


	for (int n = length/buff_size, i = 0; i<n; ++i) {
		fread					( buff, 1, buff_size, file );
		gzwrite					( z_file, buff, buff_size );
	}
	buff_size				= length % buff_size;
	if (buff_size!=0) {
		fread					( buff, 1, buff_size, file );
		gzwrite					( z_file, buff, buff_size );
	}
	fclose( file );
	gzclose( z_file );
}

void decompress( LPCSTR f_in, LPCSTR f_out )
{

	FILE * file = fopen( f_out, "wb" );
	u32 buff_size = 1024*1024/2;

	//u32 const length	= _filelength( _fileno( file ) );

	void *buff = _alloca( buff_size );
	gzFile z_file = gzopen(f_in, "rb");
	

	for (;;) {
		u32 read = gzread		( z_file, buff, buff_size );
		fwrite					( buff, 1, read, file );
		if(read<buff_size)
			break;
	}

	fclose( file );
	gzclose( z_file );
	
}




void compress( LPCSTR f_in_out )
{
 	string_path	tmp;
 	strconcat( sizeof(tmp),tmp, f_in_out, "___ctmp" );
	compress( f_in_out, tmp );

	if ( GetFileAttributes(f_in_out) != u32(-1) ) 
			unlink(f_in_out);
    // physically rename file
    VerifyPath			(f_in_out);
    rename				(tmp,f_in_out);

	
}

void decompress( LPCSTR f_in_out )
{
 	string_path	tmp;
 	strconcat( sizeof(tmp),tmp, f_in_out, "___dtmp" );
	decompress( f_in_out, tmp );
	if ( GetFileAttributes(f_in_out) != u32(-1) ) 
			unlink(f_in_out);
    // physically rename file
    VerifyPath			(f_in_out);
    rename				(tmp,f_in_out);
}