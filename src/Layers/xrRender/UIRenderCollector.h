#pragma once

#include "Include/xrRender/UIRender.h"
#include "UIGeometryBatch.h"

namespace xray::render::ui
{
using namespace xray::render::fg;

class UIRenderCollector : public IUIRender
{
public:
    UIRenderCollector();
    ~UIRenderCollector() = default;

    void CreateUIGeom() override;
    void DestroyUIGeom() override;

    void SetShader(IUIShader& shader) override;
    void SetAlphaRef(int aref) override;
    void SetScissor(Irect* rect = nullptr) override;

    void PushPoint(float x, float y, float z, u32 C, float u, float v) override;

    void StartPrimitive(u32 iMaxVerts, ePrimitiveType primType, ePointType pointType) override;
    void FlushPrimitive() override;

    LPCSTR UpdateShaderName(LPCSTR tex_name, LPCSTR sh_name) override;

    void CacheSetXformWorld(const Fmatrix& M) override;
    void CacheSetCullMode(CullMode mode) override;

    const xr_vector<UIGeometryBatch>& GetBatches() const { return m_batches; }
    void Clear();

private:
    IUIShader* m_currentUIShader{nullptr};
    int m_currentAlphaRef{0};
    bool m_hasScissor{false};
    Irect m_scissorRect;
    Fmatrix m_xformWorld;
    int m_cullMode{0};

    ePrimitiveType m_primitiveType{ptNone};
    ePointType m_pointType{pttNone};
    u32 m_maxVerts{0};
    xr_vector<UIVertex> m_currentVertices;

    xr_vector<UIGeometryBatch> m_batches;
    UIGeometryBatch* m_currentBatch{nullptr};

    UIPrimitiveType ConvertPrimitiveType(ePrimitiveType primType);
    UIGeometryBatch* GetOrCreateBatch();
};

}
