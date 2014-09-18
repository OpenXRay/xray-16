#include "stdafx.h"
#include "net_stream.h"









class XRLC_LIGHT_API INetFileBuffWriter:
		public INetBuffWriter
{

	virtual void			w				( const void* ptr, u32 count )	;


public:
		void				w_close				(){CFileWriteBlock* fr = dynamic_cast<CFileWriteBlock*>(mem_writter);R_ASSERT(fr);fr->w_close();};

							INetFileBuffWriter	(LPCSTR	_file_name, u32 block_size, bool _reopen );//:INetWriter(),file_name(file_name)
	virtual					~INetFileBuffWriter	();

};





INetReader::~INetReader	()
{

}

void		INetReaderGenStream::	r			(void *p,int cnt)
{
	stream->Read( p, cnt );
}
INetReaderGenStream::~INetReaderGenStream()
{

}
void INetBlockReader::r(void *p,int cnt)
{
	if(cnt==0)
		return;
	u8* pointer = (u8*)p;
	add( cnt );
	for(;;)
	{
		if( mem_reader.allocated() )
		{
			u32 read_cnt = _min( cnt, mem_reader.count() ); 
			mem_reader.r( pointer, read_cnt );
			pointer+=read_cnt;
			cnt-=read_cnt;
		}
		if( mem_reader.allocated() && mem_reader.count() == 0 )
				mem_reader.free_buff();
				//xr_delete( mem_reader );

		if( cnt==0 )
				return;
		R_ASSERT( stream );
		u32 block_size ;
		stream->Read(&block_size, sizeof(block_size) );
		create_block( block_size );
		stream->Read( mem_reader.pdata(), block_size );
	}
}

void		INetBlockReader::			create_block	( u32 size )
{
	VERIFY(!mem_reader.allocated());
/*
	if( !_buffer )
	{
			_buffer = (u8*)	Memory.mem_alloc	( size
#ifdef DEBUG_MEMORY_NAME
			,		"INetBlockReader - storage"
#endif // DEBUG_MEMORY_NAME
			);
		_block_size = size ;
	}
	if( _block_size < size )
	{
			_buffer = (u8*)Memory.mem_realloc	(_buffer,size
#ifdef DEBUG_MEMORY_NAME
			,	"CMemoryWriter - storage"
#endif // DEBUG_MEMORY_NAME
			);
		_block_size = size ;
	}
*/
	//R_ASSERT( _block_size >= size );
	mem_reader.alloc(size);
}

INetBlockReader::~INetBlockReader		()
{
	R_ASSERT(!mem_reader.allocated()||mem_reader.count()==0);
	mem_reader.free_buff();
//	Memory.mem_free( _buffer );
}
/*
	IC void			w_string(const char *p)			{	w(p,(u32)xr_strlen(p));w_u8(13);w_u8(10);	}
	IC void			w_stringZ(const char *p)		{	w(p,(u32)xr_strlen(p)+1);			}
	IC void			w_stringZ(const shared_str& p) 	{	w(*p?*p:"",p.size());w_u8(0);		}
	IC void			w_stringZ(shared_str& p)		{	w(*p?*p:"",p.size());w_u8(0);		}
*/

void	INetReader::r_string	(char *dest, u32 tgt_sz)
{
	
	R_ASSERT( tgt_sz < 1024 );
	char buf[ 1024 ];
	
	buf[ 0 ] = r_u8();
	u32 i = 1;
	for( ; i < 1024 ; ++i )
	{
		
		buf[ i ] = r_u8();
		if( buf[ i-1 ] == 13 && buf[ i ] == 10 )
			break;
	}
	u32 lenght = i-1;
	R_ASSERT2( lenght < (tgt_sz-1) ,"Dest string less than needed." );
	//R_ASSERT	(!IsBadReadPtr((void*)src,sz));

	
	buf[lenght] = 0;

	strncpy_s	(dest,tgt_sz, buf, lenght+1 );

    //dest[sz]	= 0;
}


void	INetReader::r_stringZ	( char *dest )
{
	
	//R_ASSERT( tgt_sz < 1024 );
	
	u32 i = 0;
	for( ; i < 1024 ; ++i )
	{
		dest[i] = r_u8();

		if( dest[i] == 0  )
			break;
	}

}
void	INetReader::r_stringZ	( shared_str &dest )
{
	char buf[ 1024 ];
	r_stringZ( buf );
	dest._set( buf );
}
void	INetBlockReader::load_buffer (LPCSTR fn)
{
	//xr_delete(mem_reader);
	mem_reader.free_buff();
	IReader* fs			= FS.r_open(fn);
	 if(fs)
	 {
		//mem_reader = xr_new<CReadMemoryBlock>( fs->length() );
	    create_block( fs->length() );
		fs->r( mem_reader.pdata(), fs->length() );
		FS.r_close(fs);// ->close();
	 }
}
void	INetMemoryBuffWriter::	send_and_clear()
{
	if( mem_writter.is_empty() )
		return;
	//send_not_clear( stream );
	//R_ASSERT(mem_writter);
	mem_writter.send( stream );
	//clear();
	//xr_delete( mem_writter );
	mem_writter.clear();
}

void	INetBuffWriter::	send_not_clear(IGenericStream* _stream)
{
 	R_ASSERT(mem_writter);
	mem_writter->send( _stream );
}

void	INetBuffWriter::	clear		  ()
{
	xr_delete( mem_writter );
}

void INetMemoryBuffWriter::w(const void* ptr, u32 count)
{
	if(count==0)
		return;
	add( count );
	const u8* pointer = (const u8*)ptr;
	for(;;)
	{
		VERIFY(mem_writter.rest()>=0);
		if( mem_writter.rest() != 0 )
		{
			u32 write_cnt = _min( count,  mem_writter.rest()  ); 
			mem_writter.w( pointer, write_cnt );
			count-=write_cnt;
			pointer+=write_cnt;
		} 
		else
		{
			R_ASSERT(u32(-1)!=net_block_write_data_size);

			send_and_clear ();
		}
		//	create_block();

		//VERIFY(mem_writter->rest()>=0);
		//if( !mem_writter.is_empty() && mem_writter.rest()==0 )
		//{
		//	R_ASSERT(u32(-1)!=net_block_write_data_size);

		//	send_and_clear ();
		//	create_block();

		//}
			
		if( count == 0 )
				return;
		
	}
}

void INetFileBuffWriter::w(const void* ptr, u32 count)
{
	VERIFY( mem_writter );
	add( count );
	mem_writter->w( ptr, count );
}

//void INetMemoryBuffWriter::create_block()
//{
//	VERIFY(!mem_writter);
//	mem_writter = xr_new<CMemoryWriteBlock>( net_block_write_data_size );
//}
INetMemoryBuffWriter::			~INetMemoryBuffWriter	()
{
	if(!mem_writter.is_empty())
		send_and_clear ();
	//R_ASSERT(!mem_writter);
}
INetBuffWriter::~INetBuffWriter()
{
	R_ASSERT(!mem_writter);
}

void	INetBuffWriter::save_buffer		( LPCSTR fn )const
{
	if(mem_writter)
		mem_writter->save_to(fn);
}

INetFileBuffWriter::INetFileBuffWriter(LPCSTR	_file_name, u32 block_size, bool _reopen ):INetBuffWriter()
{
	
	mem_writter = xr_new<CFileWriteBlock>( _file_name, block_size, _reopen );
}

INetFileBuffWriter::~INetFileBuffWriter()
{
	xr_delete(mem_writter);
}
/*
			data = (BYTE*)	Memory.mem_realloc	(data,mem_size
#ifdef DEBUG_MEMORY_NAME
			,	"CMemoryWriter - storage"
#endif // DEBUG_MEMORY_NAME
			);
*/
void CReadMemoryBlock::r(void *p,int cnt)const
{	
	R_ASSERT( (position+cnt) <=file_size);
	CopyMemory	( p, _buffer+position, cnt );
	position	+=cnt;
	
}

CReadMemoryBlock::CReadMemoryBlock	( const u32	buff_size_, u8* buffer ): buf_size(buff_size_), file_size( 0 ), position( 0 ), _buffer( buffer )
{
/*
	data = (u8*)	Memory.mem_alloc	(file_size_
#ifdef DEBUG_MEMORY_NAME
			,		"CReadMemoryBlock - storage"
#endif // DEBUG_MEMORY_NAME
			);
*/
	//data = buffer;
}

CReadMemoryBlock::	~CReadMemoryBlock	()
{
	//Memory.mem_free( data );
}

#include "../../xrCore/FS_impl.h"
u32 INetReader::find_chunk				(u32 ID, BOOL* bCompressed)
{
	R_ASSERT( false );
	return inherited::find_chunk(ID, bCompressed);
}




void CMemoryWriteBlock::send(IGenericStream	 *_stream)
{
	u32 block_size = tell();
	_stream->Write( &block_size, sizeof( block_size) );
	_stream->Write( pointer(), block_size );
}
CFileWriteBlock::CFileWriteBlock ( LPCSTR fn, u32 _size, bool _reopen  ):IWriteBlock( _size ), file_name( fn ),file(0),file_map(0),reopen(_reopen)
{
	
	if(reopen)
		return;
	string_path		lfile_name;
	FS.update_path	( lfile_name, "$level$", fn );
	file			= fopen( lfile_name, "wb");
	VERIFY			(file);

}

CFileWriteBlock::~CFileWriteBlock ( )
{

	fclose				(file);
	if (file_map)
		fclose			(file_map);
	if(reopen)
		return;
	string_path			 N;
	FS.update_path		( N, "$level$", file_name );
	if(FS.exist( N ))
		FS.file_delete( N );
}


void	CFileWriteBlock::w(const void* ptr, u32 count)
{
	
	fwrite	(const_cast<void*>(ptr), 1, count, file);

}

void	CFileWriteBlock::send( IGenericStream	 *_stream )	
{


	R_ASSERT				(file_map);
	fseek					( file_map, 0, SEEK_SET );
	u32 const length		= _filelength( _fileno( file_map ) );
	R_ASSERT				( length );

	u32 const position		= _stream->GetPos();
	u32 block_size			= size;
	//_stream->SetLength		(position + length + ((int(length) - 1)/block_size + 1)*sizeof(block_size));
	_stream->SetLength		( position + length );
	_stream->Seek			(position);

	
	void* block				= _alloca(block_size);

	for (int n = length/block_size, i = 0; i<n; ++i) {
		fread				( block, 1, block_size, file_map );
	//	_stream->Write		( &block_size, sizeof( block_size) );
		_stream->Write		( block, block_size );
	}

	block_size				= length % block_size;
	if (block_size==0) {
//		xr_free				(block);
		return;
	}

	fread					( block, 1, block_size, file_map );
//	_stream->Write			( &block_size, sizeof( block_size) );
	_stream->Write			( block, block_size );
}

u32		CFileWriteBlock::rest()
{
	VERIFY( file );
	return size - ftell( file_map ) ;
}

void	CFileWriteBlock::w_close()
{
	R_ASSERT			(!file_map);
	if(!reopen)
	{
		VERIFY				(file);
		fclose				(file);
	}
	string_path			 lfile_name;
	FS.update_path		( lfile_name, "$level$", file_name );
	file_map			= fopen( lfile_name, "rb" );
}


INetReaderFile::INetReaderFile( LPCSTR file_name ): file(0)
{
	file = fopen( file_name, "rb" );// FS.r_open( file_name );
}

INetReaderFile::~INetReaderFile( )
{
	 fclose(file);//FS.r_close( file );
}

void	INetReaderFile::r(void *p,int cnt)
{
	//file->r( p, cnt );
	fread					( p, 1, cnt, file );
}

CGenStreamOnFile::CGenStreamOnFile( CVirtualFileRW	*_file ):file(_file)
{
	VERIFY(file);
	//string_path			 lfile_name;
	//FS.update_path		( lfile_name, "$level$", file_name );
	//file = fopen( lfile_name, "rb" );
}
CGenStreamOnFile::~CGenStreamOnFile( )
{
	// fclose(file);
}
DWORD __stdcall CGenStreamOnFile::GetLength()
{
	return file->length();
	//return _filelength( _fileno( file ) );
}
DWORD __stdcall CGenStreamOnFile::Read(void* Data, DWORD count)
{
	R_ASSERT(false);
	//fread( Data, 1, count, file );
	return count;
}



INetIWriterGenStream::INetIWriterGenStream( IGenericStream	 *_stream, u32 inital_size ):
stream( _stream ), block_size( inital_size )
{
	R_ASSERT( _stream );
	u32 const position		= _stream->GetPos();
	_stream->SetLength		( position + inital_size );
	_stream->Seek			( position );
}
INetIWriterGenStream::~INetIWriterGenStream( )
{
	

}

void	INetIWriterGenStream::w( const void* ptr, u32 count )
{
	
	u32 const position		= stream->GetPos();
	u32 const length		= stream->GetLength();

	if( position + count > length )
	{
		stream->SetLength		( position + block_size );
		stream->Seek			( position );
	}

	stream->Write			( ptr, count );
}



void		CMemoryWriteBlock::w(const void* ptr, u32 count)	
{ 
	VERIFY( rest() >= count );

	R_ASSERT(position+count <= buffer_size);
	//mem_writer.w(ptr,count);

	CopyMemory	(buffer+position,ptr,count);
	position		+=count;
	//if (position>file_size) file_size=position;//?

}