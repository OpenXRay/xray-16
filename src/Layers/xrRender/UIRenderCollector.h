// xrRender/UIRenderCollector.h
#pragma once

#include "Include/xrRender/UIRender.h"
#include "UIGeometryBatch.h"

namespace xray::render::ui
{
using namespace xray::render::fg;  // For HW

// Collector that implements IUIRender interface to record UI geometry
// instead of immediately rendering it
class UIRenderCollector : public IUIRender
{
public:
    UIRenderCollector();
    ~UIRenderCollector() = default;

    // IUIRender interface implementation
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

    // Collector-specific methods
    const xr_vector<UIGeometryBatch>& GetBatches() const { return m_batches; }
    void Clear();

private:
    // Current rendering state
    IUIShader* m_currentUIShader{nullptr};  // Backend-agnostic: Current UI shader
    int m_currentAlphaRef{0};
    bool m_hasScissor{false};
    Irect m_scissorRect;
    Fmatrix m_xformWorld;
    int m_cullMode{0};

    // Current primitive being built
    ePrimitiveType m_primitiveType{ptNone};
    ePointType m_pointType{pttNone};
    u32 m_maxVerts{0};
    xr_vector<UIVertex> m_currentVertices;

    // Collected batches
    xr_vector<UIGeometryBatch> m_batches;
    UIGeometryBatch* m_currentBatch{nullptr};

    // Helper methods
    UIPrimitiveType ConvertPrimitiveType(ePrimitiveType primType);
    UIGeometryBatch* GetOrCreateBatch();
};

} // namespace xray::render::ui
