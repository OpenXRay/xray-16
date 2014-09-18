#pragma once
#include "hxgrid/Interface/IAgent.h"
//#include "hxgrid/Interface/IGenericStream.h"


class XRLC_LIGHT_API byte_count
{
	u32					_count;
	
public:
			byte_count	():_count( 0 ){}
	IC u32	count		(){ return _count ; }
	IC void reset		(){ _count = 0; }
	IC void add			(u32 cnt ){ _count+=cnt; }
};



class CReadMemoryBlock
{
	mutable	u32		file_size;
	mutable	u8*		_buffer;
	const	u32		buf_size;
	mutable u32		position;
public:
					CReadMemoryBlock	( const u32	buf_size_, u8* buffer );
					~CReadMemoryBlock	();
	void			r				( void *p, int cnt )const;
	u32				count			()const { return file_size - position; }
	void			*pdata			()const { return (void*) _buffer; }
//IC	void			reset_file_size	( u32 new_size ){file_size = new_size ;}
//IC	u32				get_file_size	( )				{return file_size; }
	void			free_buff		()const { file_size = 0; position = 0; }
	void			alloc			( u32 _file_size )const	
	{ 
		R_ASSERT( buf_size >= _file_size );
		VERIFY(!allocated()); 
		VERIFY( empty() );
		file_size = _file_size; 
	}
	bool			allocated		()const { return ( file_size !=0) ;}
	bool			empty			()const { return (position == 0); }
};

class IReadBlock
{
public:
	virtual	void			r				( void *p, int cnt )const	= 0;
	virtual	u32				count			()const						= 0;
	virtual	void			*pdata			()const						= 0;
};


//class	CMemoryReadBlock
//	//:
//	//public IReadBlock
//{
//	CReadMemoryBlock mem_reader;
//public:
//	CMemoryReadBlock	( const u32	file_size_, u8*	&buff ):mem_reader(file_size_, buff ){};
//	virtual	void			r				( void *p, int cnt )const	{ mem_reader.r(p,cnt); }
//	virtual	u32				count			()const						{ return mem_reader.count(); }
//	virtual	void			*pdata			()const						{ return mem_reader.pdata(); }
//
//	void					free_buff		()const						{ mem_reader.free_buff(); }
//	void					alloc			(u32 _file_size)const		{ mem_reader.alloc(_file_size); }
//	bool					allocated		()const						{ return mem_reader.allocated(); }
//	bool					empty			()const						{ return mem_reader.empty(); }
//};

class IWriteBlock
{
protected:
	u32				size;
public:
					IWriteBlock (u32 _size):size(_size)			{}
	virtual bool	save_to		(LPCSTR fn)						=0;
//	virtual u32		tell		() 								=0;
//	virtual	u8*		pointer		()								=0;
	virtual void	w			(const void* ptr, u32 count)	=0;
	virtual	void	send		(IGenericStream	 *_stream)		=0;
	virtual	u32		rest		()								=0;
};

class CMemoryWriteBlock
//:	public IWriteBlock
{
	//CMemoryWriter	mem_writer;
	u8*			buffer;
	const u32	buffer_size;

//	u8*				data;
	u32				position;

//	u32				mem_size;
//	u32				file_size;


public:
					CMemoryWriteBlock	( u8* _buffer, u32 _size ):buffer(_buffer), buffer_size(_size),position(0){}

//	 bool			save_to				(LPCSTR fn)						{ return mem_writer.save_to(fn); }
	 void			send				(IGenericStream	 *_stream)		;
	 u32			rest				()								{ return buffer_size - tell(); }
	 void			w					(const void* ptr, u32 count)	;
public:	 
	 void			clear				()								{position = 0;}
	 bool			is_empty			()								{return 0==position;}
private:
	u32				tell				() 								{ return position; }
	u8*				pointer				()								{ return buffer; }
};

class CFileWriteBlock:
	public IWriteBlock
{
//	IWriter								*file;
	FILE								*file;
	FILE								*file_map;
//	CVirtualFileRW						*file_map					; 
	LPCSTR								file_name					;
	bool								reopen						;
public:
					CFileWriteBlock ( LPCSTR fn, u32 _size, bool _reopen );
					~CFileWriteBlock( )							;
	virtual bool	save_to			( LPCSTR fn )				{return false;};
	virtual	void	send			(IGenericStream	 *_stream)	;
	virtual	u32		rest			()							;
	void			w_close			()							;
//	virtual u32		tell			() 							;	
//	virtual	u8*		pointer			()							;	
	virtual void	w				(const void* ptr, u32 count);	
};

class XRLC_LIGHT_API INetReader : 
	public IReaderBase<INetReader>,
	public byte_count
			
{
public:
	INetReader		()
	{
		//VERIFY(stream);
	}
	virtual			~INetReader	();
	IC int			elapsed		()	const		{	VERIFY(false); return 0;		};
	void			seek		(u32 pos)		{	VERIFY(false); 		};



	IC int			tell		()	const		{	VERIFY(false); return 0;	};
	//IC void			seek		(int ptr)		{	VERIFY(false); }
	IC int			length		()	const		{	VERIFY(false); return 0; };
//	IC void*		pointer		()	const		{	VERIFY(false); return 0; 	};
	IC void			advance		(int cnt)		{	VERIFY(false);}


virtual	void			r			(void *p,int cnt) = 0;
	

	void			r_string	(char *dest, u32 tgt_sz);
	void			r_stringZ	( char *dest );
	void			r_stringZ	( shared_str &dest );
	u32 			find_chunk  (u32 ID, BOOL* bCompressed = 0);

private:
	typedef IReaderBase<INetReader>	inherited;
	
};





class XRLC_LIGHT_API INetReaderFile:
	public INetReader
{
	FILE					*file;
public:
							INetReaderFile			( LPCSTR file );

virtual						~INetReaderFile			( );
private:
virtual		void			r						(void *p,int cnt);
};
//////////////////////////////////////////////////////////////////////////////
class XRLC_LIGHT_API INetBuffWriter : 
	public IWriter,
	public byte_count
{
protected:
	IWriteBlock				*mem_writter;
	//CMemoryWriteBlock		mem_writter;
private:
	virtual void	seek	(u32 pos)						{ VERIFY(false); }
	virtual u32		tell	()								{ VERIFY(false); return 0; };
	virtual	void	flush	()								{ VERIFY(false); }	
public:

	INetBuffWriter(): mem_writter(0)
	{
		//VERIFY(stream);
	}
	virtual					~INetBuffWriter( );

	virtual	void			send_not_clear	(IGenericStream	 * _stream)		;
	virtual	void			clear			()								;
	virtual	void			save_buffer		( LPCSTR fn )const				;


};
class XRLC_LIGHT_API INetMemoryBuffWriter:
	public IWriter,
	public byte_count
//	public INetBuffWriter
{
	IGenericStream		*stream;
	u32					net_block_write_data_size;
	CMemoryWriteBlock	mem_writter;
public:
	INetMemoryBuffWriter(IGenericStream *_stream, u32 _block_size, u8 *buffer): mem_writter( buffer, _block_size ),
	  stream(_stream),
	  net_block_write_data_size(_block_size)
	{
		//VERIFY(stream);
	}
							~INetMemoryBuffWriter	();
 private:	
//	void					create_block			();
	void					w						( const void* ptr, u32 count )	;
	void					send_and_clear			(  );
private:

	virtual void	seek	(u32 pos)						{ VERIFY(false); }
	virtual u32		tell	()								{ VERIFY(false); return 0; };
	virtual	void	flush	()								{ VERIFY(false); }	
};
class XRLC_LIGHT_API INetReaderGenStream:
	public INetReader
{
public:
	INetReaderGenStream(IGenericStream	 *_stream): stream(_stream){}
	virtual ~INetReaderGenStream();
protected:
	IGenericStream	*stream;
private:
	virtual	void			r			( void *p, int cnt );
};
class XRLC_LIGHT_API INetIWriterGenStream:
	public IWriter
{
	IGenericStream	 *stream;
	u32				 block_size;
public:
	INetIWriterGenStream( IGenericStream	 * _stream, u32 inital_size );
	virtual ~INetIWriterGenStream( );
private:
	virtual void	w										( const void* ptr, u32 count )	;
	virtual void	seek	(u32 pos)						{ VERIFY(false); }
	virtual u32		tell	()								{ VERIFY(false); return 0; };
	virtual	void	flush	()								{ VERIFY(false); }	
};

class XRLC_LIGHT_API INetBlockReader : 
	public INetReaderGenStream
{
		CReadMemoryBlock		mem_reader;
public:
						INetBlockReader			(IGenericStream	 *_stream, u8* buffer, u32 _size_buffer ):INetReaderGenStream(_stream), mem_reader( _size_buffer, buffer ){}
						
		void			load_buffer				(LPCSTR fn);
virtual	void			r						(void *p,int cnt);
		virtual			~INetBlockReader		();
private:
//	u32				_block_size;
//	u8				*&_buffer;
	void			create_block( u32 size );
	typedef INetReader	inherited;
};

////////////////////////////////////////////////////////////////////////////////////

class CGenStreamOnFile: public IGenericStream
{
	//FILE					*file;
	CVirtualFileRW	*file;
public:
			CGenStreamOnFile( CVirtualFileRW	*_file );
			~CGenStreamOnFile(  );
private:
	 //======== BEGIN COM INTERFACE =======
  IUNKNOWN_METHODS_IMPLEMENTATION_INSTANCE()

  virtual BYTE* __stdcall GetBasePointer(){ return (BYTE*) file->pointer() ;}
  virtual BYTE* __stdcall GetCurPointer(){R_ASSERT(false);return 0 ;}
  virtual bool __stdcall isReadOnly(){R_ASSERT(false);return false ;}
  virtual DWORD __stdcall GetLength();
  virtual void __stdcall Write(const void* Data, DWORD count){R_ASSERT(false);}
  virtual DWORD __stdcall Read(void* Data, DWORD count);
  virtual void __stdcall Seek(DWORD pos){R_ASSERT(false);}
  virtual DWORD __stdcall GetPos(){R_ASSERT(false);return false ;}
  virtual void __stdcall Clear(){R_ASSERT(false);}
  virtual void __stdcall FastClear(){R_ASSERT(false);}
  virtual void __stdcall GrowToPos(int DestSize=-1){R_ASSERT(false);}
  virtual void __stdcall Skip(DWORD count){R_ASSERT(false);}
  virtual void __stdcall SetLength(DWORD newLength){R_ASSERT(false);}
  virtual void __stdcall Compact(){R_ASSERT(false);}
  virtual DWORD __stdcall GetVersion() const
	{
		return CGenStreamOnFile::VERSION;
	}
};


