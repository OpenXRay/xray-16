#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/ResourceManager.h"
#include "../xrRender/R_DStreams.h"

#include "glRenderDeviceRender.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int		rsDVB_Size = 512 + 1024;
int		rsDIB_Size = 512;

void _VertexStream::Create()
{
	//glRenderDeviceRender::Instance().Resources->Evict		();
	DEV->Evict();

	mSize = rsDVB_Size * 1024;
	glGenBuffers(1, &pVB);
	R_ASSERT(pVB);

	glBindBuffer(GL_ARRAY_BUFFER, pVB);
	CHK_GL(glBufferData(GL_ARRAY_BUFFER, mSize, nullptr, GL_DYNAMIC_DRAW));

	mPosition = 0;
	mDiscardID = 0;

	Msg("* DVB created: %dK", mSize / 1024);
}

void _VertexStream::Destroy()
{
	glDeleteBuffers(1, &pVB);
	_clear();
}

void* _VertexStream::Lock(u32 vl_Count, u32 Stride, u32& vOffset)
{
#ifdef DEBUG
	PGO(Msg("PGO:VB_LOCK:%d", vl_Count));
	VERIFY(0 == dbg_lock);
	dbg_lock++;
#endif

	// Ensure there is enough space in the VB for this data
	u32	bytes_need = vl_Count*Stride;
	R_ASSERT2((bytes_need <= mSize) && vl_Count, make_string("bytes_need = %d, mSize = %d, vl_Count = %d", bytes_need, mSize, vl_Count));

	// Vertex-local info
	u32 vl_mSize = mSize / Stride;
	u32 vl_mPosition = mPosition / Stride;

	// Check if there is need to flush and perform lock
	void* pData = nullptr;
	glBindBuffer(GL_ARRAY_BUFFER, pVB);
	if ((vl_Count + vl_mPosition) >= vl_mSize)
	{
		// FLUSH-LOCK
		mPosition = 0;
		vOffset = 0;
		mDiscardID++;

		CHK_GL(pData = glMapBufferRange(GL_ARRAY_BUFFER, mPosition, bytes_need, LOCKFLAGS_FLUSH));
	}
	else {
		// APPEND-LOCK
		mPosition = vl_mPosition*Stride;
		vOffset = vl_mPosition;

		CHK_GL(pData = glMapBufferRange(GL_ARRAY_BUFFER, mPosition, bytes_need, LOCKFLAGS_APPEND));
	}
	VERIFY(pData);

	return pData;
}

void	_VertexStream::Unlock(u32 Count, u32 Stride)
{
#ifdef DEBUG
	PGO(Msg("PGO:VB_UNLOCK:%d", Count));
	VERIFY(1 == dbg_lock);
	dbg_lock--;
#endif
	mPosition += Count*Stride;

	VERIFY(pVB);
	CHK_GL(glUnmapBuffer(GL_ARRAY_BUFFER));
}

void	_VertexStream::reset_begin()
{
	old_pVB = pVB;
	Destroy();
}
void	_VertexStream::reset_end()
{
	Create();
	//old_pVB				= NULL;
}

_VertexStream::_VertexStream()
{
	_clear();
};

void _VertexStream::_clear()
{
	pVB = NULL;
	mSize = 0;
	mPosition = 0;
	mDiscardID = 0;
#ifdef DEBUG
	dbg_lock = 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
void	_IndexStream::Create()
{
	//dxRenderDeviceRender::Instance().Resources->Evict		();
	DEV->Evict();

	mSize = rsDIB_Size * 1024;
	glGenBuffers(1, &pIB);
	R_ASSERT(pIB);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIB);
	CHK_GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, mSize, nullptr, GL_DYNAMIC_DRAW));

	mPosition = 0;
	mDiscardID = 0;

	Msg("* DIB created: %dK", mSize / 1024);
}

void	_IndexStream::Destroy()
{
	glDeleteBuffers(1, &pIB);
	_clear();
}

u16*	_IndexStream::Lock(u32 Count, u32& vOffset)
{
	PGO(Msg("PGO:IB_LOCK:%d", Count));
	vOffset = 0;
	void* pLockedData = nullptr;

	// Ensure there is enough space in the VB for this data
	R_ASSERT((2 * Count <= mSize) && Count);

	// If either user forced us to flush,
	// or there is not enough space for the index data,
	// then flush the buffer contents
	GLbitfield flags = LOCKFLAGS_APPEND;
	if (2 * (Count + mPosition) >= mSize)
	{
		mPosition = 0;						// clear position
		flags = LOCKFLAGS_FLUSH;			// discard its contens
		mDiscardID++;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIB);
	CHK_GL(pLockedData = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, mPosition * 2, Count * 2, flags));
	VERIFY(pLockedData);

	vOffset = mPosition;

	return					LPWORD(pLockedData);
}

void	_IndexStream::Unlock(u32 RealCount)
{
	PGO(Msg("PGO:IB_UNLOCK:%d", RealCount));
	mPosition += RealCount;
	VERIFY(pIB);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIB);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

void	_IndexStream::reset_begin()
{
	old_pIB = pIB;
	Destroy();
}
void	_IndexStream::reset_end()
{
	Create();
	//old_pIB				= NULL;
}
