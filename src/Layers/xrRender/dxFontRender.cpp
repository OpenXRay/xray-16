#include "stdafx.h"

#include "dxFontRender.h"

#include "xrEngine/GameFont.h"
#include "xrCore/Text/StringConversion.hpp"
#include "RenderContext/RenderDevice.h"
#include "ResourceManager/FGResourceManager.h"
#include "FrameGraph/ShaderLoader.h"

extern ENGINE_API bool g_bRendering;
extern ENGINE_API Fvector2 g_current_font_scale;

namespace xray::render::RENDER_NAMESPACE
{
dxFontRender::~dxFontRender()
{
    // DX12: Clean up NVRHI resources
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
    {
        m_vsHandle = nullptr;
        m_psHandle = nullptr;
        m_baseTexture = nullptr;  // Release texture reference

        // Clean up reflection data
        if (m_vsReflection) {
            xr_delete(m_vsReflection);
            m_vsReflection = nullptr;
        }
        if (m_psReflection) {
            xr_delete(m_psReflection);
            m_psReflection = nullptr;
        }
    }

    // Legacy D3D11 path
    pShader.destroy();
    pGeom.destroy();
}

void dxFontRender::Initialize(cpcstr cShader, cpcstr cTexture)
{
    // DX12/FrameGraph: Compile shaders using NVRHI ShaderLoader
    if (GEnv.Backend && GEnv.Backend->GetAPI() == IRenderBackend::API::D3D12)
    {
        auto* shaderLoader = RImplementation.GetShaderLoader();
        if (!shaderLoader)
        {
            Msg("! [dxFontRender] ShaderLoader is NULL for shader: %s", cShader);
            return;
        }

        // Store shader and texture names for later use
        m_shaderName = cShader;
        m_textureName = cTexture;

        // Cache the base texture for rendering
        // For DX12, we defer texture loading - it will be loaded via TextureManager when creating the PSO
        if (cTexture) {
            // Set deferred load flag to prevent _CreateTexture from calling Load()
            bool prevDeferredLoad = RImplementation.Resources->bDeferredLoad;
            RImplementation.Resources->bDeferredLoad = true;

            m_baseTexture = RImplementation.Resources->_CreateTexture(cTexture);

            RImplementation.Resources->bDeferredLoad = prevDeferredLoad;
        }

        // Try to load the requested shader
        // Font shaders use stub_notransform_t.vs for vertex shader (screen-space coordinates)
        auto vsResult = shaderLoader->LoadVertexShader("stub_notransform_t", "main");
        auto psResult = shaderLoader->LoadPixelShader(cShader, "main");

        // If pixel shader not found, try falling back to hud_font
        if (!psResult.handle)
        {
            Msg("* [dxFontRender] Pixel shader '%s' not found, falling back to hud_font", cShader);
            psResult = shaderLoader->LoadPixelShader("hud_font", "main");
        }

        if (vsResult.handle && psResult.handle)
        {
            m_vsHandle = vsResult.handle;
            m_psHandle = psResult.handle;

            // Store reflection data (transfer ownership from ShaderResult)
            m_vsReflection = vsResult.reflection;
            m_psReflection = psResult.reflection;
            vsResult.reflection = nullptr;  // Transfer ownership
            psResult.reflection = nullptr;  // Transfer ownership

            Msg("* [dxFontRender] Compiled font shader: %s (tex: %s)", cShader, cTexture ? cTexture : "none");
        }
        else
        {
            Msg("! [dxFontRender] Failed to compile font shader: %s (VS=%s, PS=%s)",
                cShader,
                vsResult.handle ? "OK" : "FAILED",
                psResult.handle ? "OK" : "FAILED");
        }

        // Create geometry buffer (used for both D3D11 and D3D12)
        pGeom.create(FVF::F_TL, RImplementation.Vertex.Buffer(), RImplementation.QuadIB);

        // Texture loading is deferred to PSO creation time (via TextureManager)
        // Don't call Load() here as it would use legacy D3D11 HW.pDevice

        return;
    }

    // Legacy D3D11 path
    //pShader.create(cShader, cTexture);  // Still commented out for now
    pGeom.create(FVF::F_TL, RImplementation.Vertex.Buffer(), RImplementation.QuadIB);
    m_textureName = cTexture;  // Store for FrameGraph access
}

void dxFontRender::OnRender(CGameFont& owner)
{
    VERIFY(g_bRendering);
    if (pShader)
        RCache.set_Shader(pShader);

    if (!(owner.uFlags & CGameFont::fsValid))
    {
        R_ASSERT(pShader);
        R_constant* C = RCache.get_c(c_sbase)._get(); // get sampler
        CTexture* T = RCache.get_ActiveTexture(C ? C->samp.index : 0);
        R_ASSERT(T);
        owner.vTS.set((int)T->get_Width(), (int)T->get_Height());
        owner.fTCHeight = owner.fHeight / float(owner.vTS.y);
        owner.uFlags |= CGameFont::fsValid;
    }

    for (u32 i = 0; i < owner.strings.size();)
    {
        // calculate first-fit
        int count = 1;

        u32 length = owner.smart_strlen(owner.strings[i].string);
        auto [actionsCount, actionsLength] = owner.get_actions_text_length(owner.strings[i].string);
        length += actionsLength - actionsCount * 2;

        while ((i + count) < owner.strings.size())
        {
            u32 L = owner.smart_strlen(owner.strings[i + count].string);
            auto [aC, aL] = owner.get_actions_text_length(owner.strings[i].string);
            L += aL - aC * 2;

            if ((L + length) < MAX_MB_CHARS)
            {
                count++;
                length += L;
            }
            else
                break;
        }

        // lock AGP memory
        u32 vOffset;
        FVF::TL* v = (FVF::TL*)RImplementation.Vertex.Lock(length * 4, pGeom.stride(), vOffset);
        FVF::TL* start = v;

        // fill vertices
        u32 last = i + count;
        for (; i < last; i++)
        {
            CGameFont::String& PS = owner.strings[i];
            xr_wide_char wsStr[MAX_MB_CHARS];

            const u16 len = owner.IsMultibyte() ? mbhMulti2Wide(wsStr, nullptr, MAX_MB_CHARS, PS.string) : xr_strlen(PS.string);

            if (len)
            {
                float X = float(iFloor(PS.x));
                float Y = float(iFloor(PS.y));
                float S = PS.height * g_current_font_scale.y;
                float Y2 = Y + S;
                float fSize = 0;

                if (PS.align)
                    fSize = owner.IsMultibyte() ? owner.SizeOf_(wsStr) : owner.SizeOf_(PS.string);

                switch (PS.align)
                {
                case CGameFont::alCenter: X -= (iFloor(fSize * 0.5f)) * g_current_font_scale.x; break;
                case CGameFont::alRight: X -= iFloor(fSize); break;
                }

                const u32 clr = PS.c;
                u32 clr2 = PS.c;

                if (owner.uFlags & CGameFont::fsGradient)
                {
                    const u32 r = color_get_R(clr) / 2;
                    const u32 g = color_get_G(clr) / 2;
                    const u32 b = color_get_B(clr) / 2;
                    const u32 a = color_get_A(clr);
                    clr2 = color_rgba(r, g, b, a);
                }

#ifndef USE_DX9 // Vertex shader will cancel a DX9 correction, so make fake offset
                X -= 0.5f;
                Y -= 0.5f;
                Y2 -= 0.5f;
#endif // !USE_DX9

                for (u16 j = 0; j < len; j++)
                {
                    if (owner.IsMultibyte())
                    {
                        if (wsStr[1 + j] == GAME_ACTION_MARK)
                        {
                            static_assert(kLASTACTION < type_max<u8>, "Modify the code to have more than 255 actions.");
                            ++j;
                            const EGameActions actionId = static_cast<EGameActions>(wsStr[1 + j]);

                            cpcstr binding = GetActionBinding(actionId);

                            xr_wide_char wideBinding[MAX_MB_CHARS];
                            const auto bindingLen = mbhMulti2Wide(wideBinding, nullptr, MAX_MB_CHARS, binding);

                            for (size_t k = 0; k < bindingLen; ++k)
                            {
                                const Fvector l = owner.GetCharTC(wideBinding[1 + k]);
                                ImprintChar(l, owner, v, X, Y2, clr2, Y, clr, wsStr, j);
                            }
                        }
                        else
                        {
                            const Fvector l = owner.GetCharTC(wsStr[1 + j]);
                            ImprintChar(l, owner, v, X, Y2, clr2, Y, clr, wsStr, j);
                        }
                    }
                    else
                    {
                        if (PS.string[j] == GAME_ACTION_MARK)
                        {
                            static_assert(kLASTACTION < type_max<u8>, "Modify the code to have more than 255 actions.");
                            ++j;
                            const EGameActions actionId = static_cast<EGameActions>(PS.string[j]);

                            pcstr binding = GetActionBinding(actionId);

                            while (binding[0])
                            {
                                const Fvector l = owner.GetCharTC((u16)(u8)binding[0]);
                                ImprintChar(l, owner, v, X, Y2, clr2, Y, clr, wsStr, j);
                                ++binding;
                            }
                        }
                        else
                        {
                            const Fvector l = owner.GetCharTC((u16)(u8)PS.string[j]);
                            ImprintChar(l, owner, v, X, Y2, clr2, Y, clr, wsStr, j);
                        }
                    }
                }
            }
        }

        // Unlock and draw
        u32 vCount = (u32)(v - start);
        RImplementation.Vertex.Unlock(vCount, pGeom.stride());
        if (vCount)
        {
            // TODO: TEMPORARILY DISABLED
            // The legacy font renderer uses RCache which makes raw D3D11 calls.
            // NVRHI texture upload operations corrupt D3D11 state, causing crashes here.
            // We need to either:
            // 1. Port this to use RenderContext for proper NVRHI state management
            // 2. Save/restore D3D11 state around NVRHI operations
            // For now, disable to allow testing ImGui renderer

            // RCache.set_Geometry(pGeom);
            // RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, vCount, 0, vCount / 2);
        }
    }
}

inline void dxFontRender::ImprintChar(Fvector l, const CGameFont& owner, FVF::TL*& v, float& X, float Y2, u32 clr2, float Y, u32 clr, xr_wide_char* wsStr, int j)
{
    float scw = l.z * g_current_font_scale.x;

    float fTCWidth = l.z / owner.vTS.x;

    if (!fis_zero(l.z))
    {
        //float tu = (l.x / owner.vTS.x) + (0.5f / owner.vTS.x);
        //float tv = (l.y / owner.vTS.y) + (0.5f / owner.vTS.y);
        float tu = (l.x / owner.vTS.x);
        float tv = (l.y / owner.vTS.y);

        v->set(X, Y2, clr2, tu, tv + owner.fTCHeight);
        v++;
        v->set(X, Y, clr, tu, tv);
        v++;
        v->set(X + scw, Y2, clr2, tu + fTCWidth, tv + owner.fTCHeight);
        v++;
        v->set(X + scw, Y, clr, tu + fTCWidth, tv);
        v++;
    }
    X += scw * owner.vInterval.x;
    if (owner.IsMultibyte())
    {
        X -= 2;
        if (IsNeedSpaceCharacter(wsStr[1 + j]))
            X += owner.fXStep;
    }
}
} // namespace xray::render::RENDER_NAMESPACE
