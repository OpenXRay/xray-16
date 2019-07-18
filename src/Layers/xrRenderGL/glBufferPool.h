#pragma once

#ifdef USE_OGL

class CGLBuffer
{
	friend class CBufferPool;
    s32      m_RefCount = 1;
public:
    GLuint   m_Buffer = 0;
public:
             CGLBuffer() = default;
    virtual  ~CGLBuffer();

    IC void  AddRef()        { ++m_RefCount; };
    void     Release();
    void     DeleteBuffer();
};
using IGLVertexBuffer = CGLBuffer;
using IGLIndexBuffer = CGLBuffer;

class CBufferPool
{
	xr_vector<IGLVertexBuffer*> m_VertexBuffers;
	xr_vector<IGLIndexBuffer*>  m_IndexBuffers;
public:
	                            CBufferPool() = default;
    virtual                     ~CBufferPool();

    void                        CreateVertexBuffer(IGLVertexBuffer* &pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
    void                        CreateIndexBuffer(IGLIndexBuffer* &pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);

    void                        DeleteVertexBuffer(IGLVertexBuffer* &pBuffer);
    void                        DeleteIndexBuffer(IGLIndexBuffer* &pBuffer);
};

extern CBufferPool GLBuffers;

#endif // USE_OGL
