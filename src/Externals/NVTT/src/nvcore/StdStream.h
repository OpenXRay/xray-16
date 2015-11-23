#ifndef NV_STDSTREAM_H
#define NV_STDSTREAM_H

#include <nvcore/Stream.h>

#include <stdio.h> // fopen
#include <string.h> // memcpy
#include <exception> // std::exception

namespace nv
{

// Portable version of fopen.
inline FILE * fileOpen(const char * fileName, const char * mode)
{
	nvCheck(fileName != NULL);
#if NV_CC_MSVC && _MSC_VER >= 1400
	FILE * fp;
	if (fopen_s(&fp, fileName, mode) == 0) {
		return fp;
	}
	return NULL;
#else
	return fopen(fileName, mode);
#endif
}


/// Base stdio stream.
class NVCORE_CLASS StdStream : public Stream
{
	NV_FORBID_COPY(StdStream);
public:

	/// Ctor.
	StdStream( FILE * fp, bool autoclose=true ) : 
		m_fp(fp), m_autoclose(autoclose) { }
	
	/// Dtor. 
	virtual ~StdStream()
	{
		if( m_fp != NULL && m_autoclose ) {
			fclose( m_fp );
		}
	}


	/** @name Stream implementation. */
	//@{
		virtual void seek( uint pos )
		{
			nvDebugCheck(m_fp != NULL);
			nvDebugCheck(pos < size());
			fseek(m_fp, pos, SEEK_SET);
		}
		
		virtual uint tell() const
		{
			nvDebugCheck(m_fp != NULL);
			return ftell(m_fp);
		}
		
		virtual uint size() const
		{
			nvDebugCheck(m_fp != NULL);
			uint pos = ftell(m_fp);
			fseek(m_fp, 0, SEEK_END);
			uint end = ftell(m_fp);
			fseek(m_fp, pos, SEEK_SET);
			return end;
		}
		
		virtual bool isError() const
		{
			return m_fp == NULL || ferror( m_fp ) != 0;
		}

		virtual void clearError()
		{
			nvDebugCheck(m_fp != NULL);
			clearerr(m_fp);
		}
		
		virtual bool isAtEnd() const
		{
			nvDebugCheck(m_fp != NULL);
			return feof( m_fp ) != 0;
		}
		
		/// Always true.
		virtual bool isSeekable() const { return true; }
	//@}

protected:

	FILE * m_fp;
	bool m_autoclose;

};


/// Standard output stream.
class NVCORE_CLASS StdOutputStream : public StdStream
{
	NV_FORBID_COPY(StdOutputStream);
public:

	/// Construct stream by file name.
	StdOutputStream( const char * name ) :
		StdStream(fileOpen(name, "wb")) { }

	/// Construct stream by file handle.
	StdOutputStream( FILE * fp, bool autoclose=true ) : StdStream(fp, autoclose)
	{
	}

	/** @name Stream implementation. */
	//@{
		/// Write data.
		virtual uint serialize( void * data, uint len )
		{
			nvDebugCheck(data != NULL);
			nvDebugCheck(m_fp != NULL);
			return (uint)fwrite(data, 1, len, m_fp);
		}
		
		virtual bool isLoading() const
		{
			return false;
		}
		
		virtual bool isSaving() const
		{
			return true;
		}
	//@}

};


/// Standard input stream.
class NVCORE_CLASS StdInputStream : public StdStream
{
	NV_FORBID_COPY(StdInputStream);
public:

	/// Construct stream by file name.
	StdInputStream( const char * name ) : 
		StdStream(fileOpen(name, "rb")) { }

	/// Construct stream by file handle.
	StdInputStream( FILE * fp, bool autoclose=true ) : StdStream(fp, autoclose)
	{
	}

	/** @name Stream implementation. */
	//@{
		/// Read data.
		virtual uint serialize( void * data, uint len )
		{
			nvDebugCheck(data != NULL);
			nvDebugCheck(m_fp != NULL);
			return (uint)fread(data, 1, len, m_fp);
		}
		
		virtual bool isLoading() const
		{
			return true;
		}
		
		virtual bool isSaving() const
		{
			return false;
		}
	//@}
};



/// Memory input stream.
class NVCORE_CLASS MemoryInputStream : public Stream
{
	NV_FORBID_COPY(MemoryInputStream);
public:

	/// Ctor.
	MemoryInputStream( const uint8 * mem, uint size ) : 
		m_mem(mem), m_ptr(mem), m_size(size) { }

	/** @name Stream implementation. */
	//@{
		/// Read data.
		virtual uint serialize( void * data, uint len )
		{
			nvDebugCheck(data != NULL);
			nvDebugCheck(!isError());

			uint left = m_size - tell();
			if (len > left) len = left;
			
			memcpy( data, m_ptr, len );
			m_ptr += len;
				
			return len;
		}
		
		virtual void seek( uint pos )
		{
			nvDebugCheck(!isError());
			m_ptr = m_mem + pos;
			nvDebugCheck(!isError());
		}
		
		virtual uint tell() const
		{
			nvDebugCheck(m_ptr >= m_mem);
			return uint(m_ptr - m_mem);
		}
		
		virtual uint size() const
		{
			return m_size;
		}
		
		virtual bool isError() const
		{
			return m_mem == NULL || m_ptr > m_mem + m_size || m_ptr < m_mem;
		}
		
		virtual void clearError()
		{
			// Nothing to do.
		}

		virtual bool isAtEnd() const
		{
			return m_ptr == m_mem + m_size;
		}
		
		/// Always true.
		virtual bool isSeekable() const
		{
			return true;
		}
		
		virtual bool isLoading() const
		{
			return true;
		}
		
		virtual bool isSaving() const
		{
			return false;
		}
	//@}

	
private:

	const uint8 * m_mem;
	const uint8 * m_ptr;
	uint m_size;

};


/// Protected input stream.
class NVCORE_CLASS ProtectedStream : public Stream
{
	NV_FORBID_COPY(ProtectedStream);
public:

	/// Ctor.
	ProtectedStream( Stream & s ) : m_s(&s), m_autodelete(false)
	{ 
	}

	/// Ctor.
	ProtectedStream( Stream * s, bool autodelete = true ) : 
		m_s(s), m_autodelete(autodelete) 
	{
		nvDebugCheck(m_s != NULL);
	}

	/// Dtor.
	virtual ~ProtectedStream()
	{
		if( m_autodelete ) {
			delete m_s;
		}
	}

	/** @name Stream implementation. */
	//@{
		/// Read data.
		virtual uint serialize( void * data, uint len )
		{
			nvDebugCheck(data != NULL);
			len = m_s->serialize( data, len );
			
			if( m_s->isError() ) {
				throw std::exception();
			}

			return len;
		}
		
		virtual void seek( uint pos )
		{
			m_s->seek( pos );
			
			if( m_s->isError() ) {
				throw std::exception();
			}
		}
		
		virtual uint tell() const
		{
			return m_s->tell();
		}
		
		virtual uint size() const
		{
			return m_s->size();
		}
		
		virtual bool isError() const
		{
			return m_s->isError();
		}

		virtual void clearError()
		{
			m_s->clearError();
		}

		virtual bool isAtEnd() const
		{
			return m_s->isAtEnd();
		}
		
		virtual bool isSeekable() const
		{
			return m_s->isSeekable();
		}
		
		virtual bool isLoading() const
		{
			return m_s->isLoading();
		}
		
		virtual bool isSaving() const
		{
			return m_s->isSaving();
		}
	//@}

	
private:
	
	Stream * const m_s;
	bool const m_autodelete;

};

} // nv namespace


#endif // NV_STDSTREAM_H
