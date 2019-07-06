#pragma once
#include "hxgrid/Interface/IAgent.h"

class XRLC_LIGHT_API byte_count
{
    size_t _count;

public:
    byte_count() : _count(0) {}
    IC size_t count() { return _count; }
    IC void reset() { _count = 0; }
    IC void add(size_t cnt) { _count += cnt; }
};

class CReadMemoryBlock
{
    mutable size_t file_size;
    mutable u8* _buffer;
    const size_t buf_size;
    mutable size_t position;

public:
    CReadMemoryBlock(const size_t buf_size_, u8* buffer);
    ~CReadMemoryBlock();
    void r(void* p, size_t cnt) const;
    size_t count() const { return file_size - position; }
    void* pdata() const { return (void*)_buffer; }
    // IC	void			reset_file_size	( size_t new_size ){file_size = new_size ;}
    // IC	size_t				get_file_size	( )				{return file_size; }
    void free_buff() const
    {
        file_size = 0;
        position = 0;
    }
    void alloc(size_t _file_size) const
    {
        R_ASSERT(buf_size >= _file_size);
        VERIFY(!allocated());
        VERIFY(empty());
        file_size = _file_size;
    }
    bool allocated() const { return (file_size != 0); }
    bool empty() const { return (position == 0); }
};

class IReadBlock
{
public:
    virtual void r(void* p, size_t cnt) const = 0;
    virtual size_t count() const = 0;
    virtual void* pdata() const = 0;
};

// class	CMemoryReadBlock
//	//:
//	//public IReadBlock
//{
//	CReadMemoryBlock mem_reader;
// public:
//	CMemoryReadBlock	( const size_t	file_size_, u8*	&buff ):mem_reader(file_size_, buff ){};
//	virtual	void			r				( void *p, size_t cnt )const	{ mem_reader.r(p,cnt); }
//	virtual	size_t				count			()const						{ return mem_reader.count(); }
//	virtual	void			*pdata			()const						{ return mem_reader.pdata(); }
//
//	void					free_buff		()const						{ mem_reader.free_buff(); }
//	void					alloc			(size_t _file_size)const		{ mem_reader.alloc(_file_size); }
//	bool					allocated		()const						{ return mem_reader.allocated(); }
//	bool					empty			()const						{ return mem_reader.empty(); }
//};

class IWriteBlock
{
protected:
    size_t size;

public:
    IWriteBlock(size_t _size) : size(_size) {}
    virtual bool save_to(LPCSTR fn) = 0;
    //	virtual size_t		tell		() 								=0;
    //	virtual	u8*		pointer		()								=0;
    virtual void w(const void* ptr, size_t count) = 0;
    virtual void send(IGenericStream* _stream) = 0;
    virtual size_t rest() = 0;
};

class CMemoryWriteBlock
//:	public IWriteBlock
{
    // CMemoryWriter	mem_writer;
    u8* buffer;
    const size_t buffer_size;

    //	u8*				data;
    size_t position;

    //	size_t				mem_size;
    //	size_t				file_size;

public:
    CMemoryWriteBlock(u8* _buffer, size_t _size) : buffer(_buffer), buffer_size(_size), position(0) {}
    //	 bool			save_to				(LPCSTR fn)						{ return mem_writer.save_to(fn); }
    void send(IGenericStream* _stream);
    size_t rest() { return buffer_size - tell(); }
    void w(const void* ptr, size_t count);

public:
    void clear() { position = 0; }
    bool is_empty() { return 0 == position; }
private:
    size_t tell() { return position; }
    u8* pointer() { return buffer; }
};

class CFileWriteBlock : public IWriteBlock
{
    //	IWriter								*file;
    FILE* file;
    FILE* file_map;
    //	CVirtualFileRW						*file_map					;
    LPCSTR file_name;
    bool reopen;

public:
    CFileWriteBlock(LPCSTR fn, size_t _size, bool _reopen);
    ~CFileWriteBlock();
    bool save_to(LPCSTR fn) override { return false; };
    void send(IGenericStream* _stream) override;
    size_t rest() override;
    void w_close();
    //	virtual size_t		tell			() 							;
    //	virtual	u8*		pointer			()							;
    void w(const void* ptr, size_t count) override;
};

class XRLC_LIGHT_API INetReader : public IReaderBase<INetReader>, public byte_count

{
public:
    INetReader()
    {
        // VERIFY(stream);
    }
    virtual ~INetReader();
    IC intptr_t elapsed() const
    {
        VERIFY(false);
        return 0;
    };
    void seek(size_t pos) { VERIFY(false); };
    IC size_t tell() const
    {
        VERIFY(false);
        return 0;
    };
    // IC void			seek		(size_t ptr)		{	VERIFY(false); }
    IC size_t length() const
    {
        VERIFY(false);
        return 0;
    };
    //	IC void*		pointer		()	const		{	VERIFY(false); return 0; 	};
    IC void advance(size_t cnt) { VERIFY(false); }
    void r(void* p, size_t cnt) override = 0;

    void r_string(char* dest, size_t tgt_sz);
    void r_stringZ(char* dest);
    void r_stringZ(shared_str& dest);
    size_t find_chunk(u32 ID, bool* bCompressed = nullptr);

private:
    using inherited = IReaderBase<INetReader>;
};

class XRLC_LIGHT_API INetReaderFile : public INetReader
{
    FILE* file;

public:
    INetReaderFile(LPCSTR file);

    virtual ~INetReaderFile();

private:
    void r(void* p, size_t cnt) override;
};
//////////////////////////////////////////////////////////////////////////////
class XRLC_LIGHT_API INetBuffWriter : public IWriter, public byte_count
{
protected:
    IWriteBlock* mem_writter;
    // CMemoryWriteBlock		mem_writter;
private:
    virtual void seek(size_t pos) { VERIFY(false); }

    size_t tell() override
    {
        VERIFY(false);
        return 0;
    };

    void flush() override
    { VERIFY(false); }
public:
    INetBuffWriter() : mem_writter(0)
    {
        // VERIFY(stream);
    }
    virtual ~INetBuffWriter();

    virtual void send_not_clear(IGenericStream* _stream);
    virtual void clear();
    virtual void save_buffer(LPCSTR fn) const;
};
class XRLC_LIGHT_API INetMemoryBuffWriter : public IWriter, public byte_count
//	public INetBuffWriter
{
    IGenericStream* stream;
    size_t net_block_write_data_size;
    CMemoryWriteBlock mem_writter;

public:
    INetMemoryBuffWriter(IGenericStream* _stream, size_t _block_size, u8* buffer)
        : mem_writter(buffer, _block_size), stream(_stream), net_block_write_data_size(_block_size)
    {
        // VERIFY(stream);
    }
    ~INetMemoryBuffWriter();

private:
    //	void					create_block			();
    void w(const void* ptr, size_t count);
    void send_and_clear();

private:
    virtual void seek(size_t pos) { VERIFY(false); }

    size_t tell() override
    {
        VERIFY(false);
        return 0;
    };

    void flush() override
    { VERIFY(false); }
};
class XRLC_LIGHT_API INetReaderGenStream : public INetReader
{
public:
    INetReaderGenStream(IGenericStream* _stream) : stream(_stream) {}
    virtual ~INetReaderGenStream();

protected:
    IGenericStream* stream;

private:
    void r(void* p, size_t cnt) override;
};
class XRLC_LIGHT_API INetIWriterGenStream : public IWriter
{
    IGenericStream* stream;
    size_t block_size;

public:
    INetIWriterGenStream(IGenericStream* _stream, size_t inital_size);
    virtual ~INetIWriterGenStream();

private:
    virtual void w(const void* ptr, size_t count);
    virtual void seek(size_t pos) { VERIFY(false); }

    size_t tell() override
    {
        VERIFY(false);
        return 0;
    };

    void flush() override
    { VERIFY(false); }
};

class XRLC_LIGHT_API INetBlockReader : public INetReaderGenStream
{
    CReadMemoryBlock mem_reader;

public:
    INetBlockReader(IGenericStream* _stream, u8* buffer, size_t _size_buffer)
        : INetReaderGenStream(_stream), mem_reader(_size_buffer, buffer)
    {
    }

    void load_buffer(LPCSTR fn);
    void r(void* p, size_t cnt) override;
    virtual ~INetBlockReader();

private:
    //	size_t				_block_size;
    //	u8				*&_buffer;
    void create_block(size_t size);
    typedef INetReader inherited;
};

////////////////////////////////////////////////////////////////////////////////////

class CGenStreamOnFile : public IGenericStream
{
    // FILE					*file;
    CVirtualFileRW* file;

public:
    CGenStreamOnFile(CVirtualFileRW* _file);
    ~CGenStreamOnFile();

private:
    //======== BEGIN COM INTERFACE =======
    IUNKNOWN_METHODS_IMPLEMENTATION_INSTANCE()

    virtual BYTE* __stdcall GetBasePointer() { return (BYTE*)file->pointer(); }
    virtual BYTE* __stdcall GetCurPointer()
    {
        R_ASSERT(false);
        return 0;
    }
    virtual bool __stdcall isReadOnly()
    {
        R_ASSERT(false);
        return false;
    }
    virtual DWORD __stdcall GetLength();
    virtual void __stdcall Write(const void* Data, DWORD count) { R_ASSERT(false); }
    virtual DWORD __stdcall Read(void* Data, DWORD count);
    virtual void __stdcall Seek(DWORD pos) { R_ASSERT(false); }
    virtual DWORD __stdcall GetPos()
    {
        R_ASSERT(false);
        return false;
    }
    virtual void __stdcall Clear() { R_ASSERT(false); }
    virtual void __stdcall FastClear() { R_ASSERT(false); }
    virtual void __stdcall GrowToPos(int DestSize = -1) { R_ASSERT(false); }
    virtual void __stdcall Skip(DWORD count) { R_ASSERT(false); }
    virtual void __stdcall SetLength(DWORD newLength) { R_ASSERT(false); }
    virtual void __stdcall Compact() { R_ASSERT(false); }
    virtual DWORD __stdcall GetVersion() const { return CGenStreamOnFile::VERSION; }
};
