#include "stdafx.h"
#include "glBufferPool.h"
#include "glBufferUtils.h"

CBufferPool GLBuffers;

CGLBuffer::~CGLBuffer()
{
    DeleteBuffer();
}

void CGLBuffer::Release()
{
    VERIFY(m_RefCount > 0);

    --m_RefCount;
    if (m_RefCount <= 0)
        DeleteBuffer();
}

void CGLBuffer::DeleteBuffer()
{
    if (m_Buffer)
        glDeleteBuffers(1, &m_Buffer);
}

CBufferPool::~CBufferPool()
{
    for (CGLBuffer* &buff : m_VertexBuffers)
        xr_delete(buff);
    m_VertexBuffers.clear();

    for (CGLBuffer* &buff : m_IndexBuffers)
        xr_delete(buff);
    m_IndexBuffers.clear();
}

void CBufferPool::CreateVertexBuffer(IGLVertexBuffer* &pBuffer, const void* pData, UINT DataSize, bool bImmutable)
{
    R_ASSERT(nullptr == pBuffer);

    pBuffer = new IGLVertexBuffer();
    glBufferUtils::CreateVertexBuffer(&pBuffer->m_Buffer, pData, DataSize, bImmutable);
    m_VertexBuffers.emplace_back(pBuffer);
}

void CBufferPool::CreateIndexBuffer(IGLIndexBuffer* &pBuffer, const void* pData, UINT DataSize, bool bImmutable)
{
    R_ASSERT(nullptr == pBuffer);

    pBuffer = new IGLIndexBuffer();
    glBufferUtils::CreateIndexBuffer(&pBuffer->m_Buffer, pData, DataSize, bImmutable);
    m_IndexBuffers.emplace_back(pBuffer);
}

void CBufferPool::DeleteVertexBuffer(IGLVertexBuffer* &pBuffer)
{
    pBuffer->Release();
    if (pBuffer->m_RefCount <= 0)
    {
        auto it = std::find(m_VertexBuffers.begin(), m_VertexBuffers.end(), pBuffer);
        if (it != m_VertexBuffers.end())
            m_VertexBuffers.erase(it);
        xr_delete(pBuffer);
    }
    pBuffer = nullptr;
}

void CBufferPool::DeleteIndexBuffer(IGLIndexBuffer* &pBuffer)
{
    pBuffer->Release();
    if (pBuffer->m_RefCount <= 0)
    {
        auto it = std::find(m_IndexBuffers.begin(), m_IndexBuffers.end(), pBuffer);
        if (it != m_IndexBuffers.end())
            m_IndexBuffers.erase(it);
        xr_delete(pBuffer);
    }
    pBuffer = nullptr;
}
