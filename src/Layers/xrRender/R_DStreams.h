#ifndef r_DStreamsH
#define r_DStreamsH
#pragma once

#ifdef USE_OGL
enum
{
	LOCKFLAGS_FLUSH		= GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
	LOCKFLAGS_APPEND	= GL_MAP_WRITE_BIT // TODO: Implement buffer object appending using glBufferSubData
};
#else
enum
{
	LOCKFLAGS_FLUSH		= D3DLOCK_DISCARD,
	LOCKFLAGS_APPEND	= D3DLOCK_NOOVERWRITE
};
#endif // USE_OGL

class  ECORE_API _VertexStream
{
private :
#ifdef USE_OGL
	GLuint						pVB;
#else
	ID3DVertexBuffer*			pVB;
#endif // USE_OGL
	u32							mSize;			// size in bytes
	u32							mPosition;		// position in bytes
	u32							mDiscardID;		// ID of discard - usually for caching
public:
#ifdef USE_OGL
	GLuint						old_pVB;
#else
	ID3DVertexBuffer*			old_pVB;
#endif // USE_OGL
#ifdef DEBUG
	u32							dbg_lock;
#endif
private:
	void						_clear			();
public:
	void						Create			();
	void						Destroy			();
	void						reset_begin		();
	void						reset_end		();

#ifdef USE_OGL
	IC GLuint					Buffer() { return pVB; }
#else
	IC ID3DVertexBuffer*		Buffer() { return pVB; }
#endif // USE_OGL
	IC u32						DiscardID()		{ return mDiscardID;	}
	IC void						Flush()			{ mPosition=mSize;		}

	void*						Lock			( u32 vl_Count, u32 Stride, u32& vOffset );
	void						Unlock			( u32 Count, u32 Stride);
	u32							GetSize()		{ return mSize;}

	_VertexStream();
	~_VertexStream()			{ Destroy();	};
};

class  ECORE_API _IndexStream
{
private :
#ifdef USE_OGL
	GLuint						pIB;
#else
	ID3DIndexBuffer*			pIB;
#endif // USE_OGL
	u32							mSize;		// real size (usually mCount, aligned on 512b boundary)
	u32							mPosition;
	u32							mDiscardID;
public:
#ifdef USE_OGL
	GLuint						old_pIB;
#else
	ID3DIndexBuffer*			old_pIB;
#endif // USE_OGL
private:
	void						_clear	()
	{
		pIB			= NULL;
		mSize		= 0;
		mPosition	= 0;
		mDiscardID	= 0;
	}
public:
	void						Create			();
	void						Destroy			();
	void						reset_begin		();
	void						reset_end		();

#ifdef USE_OGL
	IC GLuint					Buffer() { return pIB; }
#else
	IC ID3DIndexBuffer*			Buffer() { return pIB; }
#endif // USE_OGL
	IC u32						DiscardID()		{ return mDiscardID;	}
	void						Flush()			{ mPosition=mSize;		}

	u16*						Lock			( u32 Count, u32& vOffset );
	void						Unlock			(u32 RealCount);

	_IndexStream()				{ _clear();		};
	~_IndexStream()				{ Destroy();	};
};
#endif
