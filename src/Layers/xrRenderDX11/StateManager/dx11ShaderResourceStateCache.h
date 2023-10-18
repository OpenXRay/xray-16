#ifndef dx11ShaderResourceStateCache_included
#define dx11ShaderResourceStateCache_included
#pragma once

class dx11ShaderResourceStateCache
{
public:
    dx11ShaderResourceStateCache();

    void ResetDeviceState();

    void Apply(u32 context_id);

    void SetPSResource(u32 uiSlot, ID3DShaderResourceView* pRes);
    void SetGSResource(u32 uiSlot, ID3DShaderResourceView* pRes);
    void SetVSResource(u32 uiSlot, ID3DShaderResourceView* pRes);
    void SetDSResource(u32 uiSlot, ID3DShaderResourceView* pRes);
    void SetHSResource(u32 uiSlot, ID3DShaderResourceView* pRes);
    void SetCSResource(u32 uiSlot, ID3DShaderResourceView* pRes);

private:
    ID3DShaderResourceView* m_PSViews[CTexture::mtMaxPixelShaderTextures];
    ID3DShaderResourceView* m_GSViews[CTexture::mtMaxGeometryShaderTextures];
    ID3DShaderResourceView* m_VSViews[CTexture::mtMaxVertexShaderTextures];
    ID3DShaderResourceView* m_HSViews[CTexture::mtMaxHullShaderTextures];
    ID3DShaderResourceView* m_DSViews[CTexture::mtMaxDomainShaderTextures];
    ID3DShaderResourceView* m_CSViews[CTexture::mtMaxComputeShaderTextures];

    u32 m_uiMinPSView;
    u32 m_uiMaxPSView;

    u32 m_uiMinGSView;
    u32 m_uiMaxGSView;

    u32 m_uiMinVSView;
    u32 m_uiMaxVSView;

    u32 m_uiMinHSView;
    u32 m_uiMaxHSView;

    u32 m_uiMinDSView;
    u32 m_uiMaxDSView;

    u32 m_uiMinCSView;
    u32 m_uiMaxCSView;

    bool m_bUpdatePSViews;
    bool m_bUpdateGSViews;
    bool m_bUpdateVSViews;
    bool m_bUpdateHSViews;
    bool m_bUpdateDSViews;
    bool m_bUpdateCSViews;
};

#endif //	dx11ShaderResourceStateCache_included
