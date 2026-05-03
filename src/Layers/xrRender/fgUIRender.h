#pragma once

#include <nvrhi/nvrhi.h>
#include "Include/xrRender/UIRender.h"
#include "Layers/xrRender/UIGeometryBatch.h"

namespace xray::render
{
class MaterialCache;
}

namespace xray::render::fg
{
class RenderDevice;

class FGUIRender : public IUIRender
{
public:
    FGUIRender();
    virtual ~FGUIRender();

    void CreateUIGeom() override {}

    void DestroyUIGeom() override {}

    void SetShader(IUIShader& shader) override;
    void SetAlphaRef(int aref) override;
    void SetScissor(Irect* rect = nullptr) override;

    void PushPoint(float x, float y, float z, u32 C, float u, float v) override;

    void StartPrimitive(u32 iMaxVerts, ePrimitiveType primType, ePointType pointType) override;
    void FlushPrimitive() override;

    LPCSTR UpdateShaderName(LPCSTR tex_name, LPCSTR sh_name) override;

    void CacheSetXformWorld(const Fmatrix& M) override;
    void CacheSetCullMode(CullMode mode) override;

    void Initialize(RenderDevice* device, render::MaterialCache* matCache);
    void Shutdown();

    const xr_vector<ui::UIGeometryBatch>& GetBatches() const
    {
        return m_batches;
    }

    void Clear();

    void Draw(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, u32 screenWidth, u32 screenHeight);

private:
    bool CreateBuffers();
    void EnsureBufferCapacity(size_t vertexCount, size_t indexCount);
    void UploadBatchGeometry(nvrhi::ICommandList* cmdList, const ui::UIGeometryBatch& batch, u32& vertexOffset, u32& indexOffset);
    void RenderBatchWithShader(nvrhi::ICommandList* cmdList, const ui::UIGeometryBatch& batch, render::MaterialPSO* pso, nvrhi::IFramebuffer* framebuffer,
        u32 screenWidth, u32 screenHeight, u32 vertexOffset, u32 indexOffset);

    ui::UIPrimitiveType ConvertPrimitiveType(ePrimitiveType primType);
    ui::UIGeometryBatch* GetOrCreateBatch(ui::UIPrimitiveType primType);

    IUIShader* m_currentUIShader = nullptr;
    int m_currentAlphaRef = 0;
    bool m_hasScissor = false;
    Irect m_scissorRect{};
    Fmatrix m_xformWorld;
    int m_cullMode = 0;

    ePrimitiveType m_primitiveType = ptNone;
    ePointType m_pointType = pttNone;
    u32 m_maxVerts = 0;
    xr_vector<ui::UIVertex> m_currentVertices;

    xr_vector<ui::UIGeometryBatch> m_batches;

    RenderDevice* m_device = nullptr;
    render::MaterialCache* m_matCache = nullptr;
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;
    nvrhi::BufferHandle m_constantBuffer;
    size_t m_vertexBufferSize = 0;
    size_t m_indexBufferSize = 0;
    bool m_initialized = false;
};
} // namespace xray::render::fg
