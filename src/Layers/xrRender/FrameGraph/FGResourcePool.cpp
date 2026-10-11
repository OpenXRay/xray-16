#include "stdafx.h"
#include "FGResourcePool.h"
#include "PassResourceCache.h"

namespace xray::render::framegraph {

FGResourcePool::FGResourcePool(resources::FGResourceManager* resourceManager)
    : m_resourceManager(resourceManager)
{
    VERIFY(m_resourceManager);
}

FGResourcePool::~FGResourcePool() {
    Reset();
}

resources::TextureHandle FGResourcePool::AllocateTexture(const resources::TextureDesc& desc) {
    for (auto& pooled : m_texturePool) {
        if (!pooled.inUse && AreTexturesCompatible(pooled.desc, desc)) {
            pooled.inUse = true;
            pooled.lastUsedFrame = m_currentFrame;
            return pooled.handle;
        }
    }

    resources::TextureHandle handle = m_resourceManager->GetTextureManager()->CreateTexture(desc);
    if (handle.IsValid())
        m_texturePool.push_back({ handle, desc, true, m_currentFrame });
    return handle;
}

void FGResourcePool::FreeTexture(resources::TextureHandle handle) {
    for (auto& pooled : m_texturePool) {
        if (pooled.handle == handle) {
            R_ASSERT(pooled.inUse);
            pooled.inUse = false;
            pooled.lastUsedFrame = m_currentFrame;
            return;
        }
    }
    R_ASSERT2(false, "framegraph texture freed outside the resource pool");
}

resources::BufferHandle FGResourcePool::AllocateBuffer(const resources::BufferDesc& desc) {
    for (auto& pooled : m_bufferPool) {
        if (!pooled.inUse && AreBuffersCompatible(pooled.desc, desc)) {
            pooled.inUse = true;
            pooled.lastUsedFrame = m_currentFrame;
            return pooled.handle;
        }
    }

    resources::BufferHandle handle = m_resourceManager->GetBufferManager()->CreateBuffer(desc);
    if (handle.IsValid())
        m_bufferPool.push_back({ handle, desc, true, m_currentFrame });
    return handle;
}

void FGResourcePool::FreeBuffer(resources::BufferHandle handle) {
    for (auto& pooled : m_bufferPool) {
        if (pooled.handle == handle) {
            R_ASSERT(pooled.inUse);
            pooled.inUse = false;
            pooled.lastUsedFrame = m_currentFrame;
            return;
        }
    }
    R_ASSERT2(false, "framegraph buffer freed outside the resource pool");
}

bool FGResourcePool::AreTexturesCompatible(const resources::TextureDesc& a, const resources::TextureDesc& b) {
    return a.type == b.type &&
           a.width == b.width &&
           a.height == b.height &&
           a.depth == b.depth &&
           a.arraySize == b.arraySize &&
           a.mipLevels == b.mipLevels &&
           a.sampleCount == b.sampleCount &&
           a.format == b.format &&
           a.isRenderTarget == b.isRenderTarget &&
           a.isDepthStencil == b.isDepthStencil &&
           a.isUAV == b.isUAV &&
           a.isSRGB == b.isSRGB;
}

bool FGResourcePool::AreBuffersCompatible(const resources::BufferDesc& a, const resources::BufferDesc& b) {
    return a.type == b.type &&
           a.usage == b.usage &&
           a.size == b.size &&
           a.stride == b.stride &&
           a.cpuAccess == b.cpuAccess &&
           a.gpuWrite == b.gpuWrite &&
           a.indirectArgs == b.indirectArgs;
}

void FGResourcePool::Reset() {
    resources::TextureManager* textures = m_resourceManager->GetTextureManager();
    for (auto& pooled : m_texturePool) {
        GetPassResourceCache().InvalidateTexture(textures->GetNVRHITexture(pooled.handle));
        textures->Release(pooled.handle);
    }
    m_texturePool.clear();

    resources::BufferManager* buffers = m_resourceManager->GetBufferManager();
    for (auto& pooled : m_bufferPool) {
        GetPassResourceCache().InvalidateBuffer(buffers->GetNVRHIBuffer(pooled.handle));
        buffers->Release(pooled.handle);
    }
    m_bufferPool.clear();
}

void FGResourcePool::Tick() {
    ++m_currentFrame;

    resources::TextureManager* textures = m_resourceManager->GetTextureManager();
    for (size_t i = 0; i < m_texturePool.size();) {
        PooledTexture& pooled = m_texturePool[i];
        if (pooled.inUse || m_currentFrame - pooled.lastUsedFrame < kEvictAfterFrames) {
            ++i;
            continue;
        }
        GetPassResourceCache().InvalidateTexture(textures->GetNVRHITexture(pooled.handle));
        textures->Release(pooled.handle);
        pooled = m_texturePool.back();
        m_texturePool.pop_back();
    }

    resources::BufferManager* buffers = m_resourceManager->GetBufferManager();
    for (size_t i = 0; i < m_bufferPool.size();) {
        PooledBuffer& pooled = m_bufferPool[i];
        if (pooled.inUse || m_currentFrame - pooled.lastUsedFrame < kEvictAfterFrames) {
            ++i;
            continue;
        }
        GetPassResourceCache().InvalidateBuffer(buffers->GetNVRHIBuffer(pooled.handle));
        buffers->Release(pooled.handle);
        pooled = m_bufferPool.back();
        m_bufferPool.pop_back();
    }
}

}
