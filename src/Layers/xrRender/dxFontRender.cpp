#include "stdafx.h"
#include "dxFontRender.h"
#include "xrEngine/GameFont.h"
#include "xrCore/Text/Utf8Utils.hpp"

extern ENGINE_API bool g_bRendering;

namespace xray::render::RENDER_NAMESPACE
{
dxFontRender::~dxFontRender()
{
    pShader.destroy();
    pGeom.destroy();
    pTexture.destroy();
}

void dxFontRender::Initialize(cpcstr cShader, cpcstr cTexture)
{
    if (pTexture._get() == nullptr)
        pTexture.create(cTexture);

    pShader.create(cShader, cTexture);
    pGeom.create(FVF::F_TL, RImplementation.Vertex.Buffer(), RImplementation.QuadIB);
}

void dxFontRender::OnRender(CGameFont& owner)
{
    VERIFY(g_bRendering);

    // Guard against an uninitialized texture (e.g. a legacy font whose .dds/.ini
    // failed to load). Skip rendering rather than crash on a null dereference.
    if (pTexture == nullptr || pShader == nullptr)
        return;

    RCache.set_Shader(pShader);

    auto fWidth = (float)std::max(m_atlasWidth ? m_atlasWidth : pTexture->get_Width(), 4u);
    auto fHeight = (float)std::max(m_atlasHeight ? m_atlasHeight : pTexture->get_Height(), 4u);

    for (CGameFont::String& str : owner.strings)
    {
        if (str.string[0])
        {
            const bool isUtf8 = XRay::Utf8::IsValid(str.string);
            const char* text = isUtf8 ? str.string : str.string_utf8.c_str();

            // Single pass over the text to count codepoints and (when needed for
            // alignment) accumulate the pixel width. This avoids a separate
            // full-string scan in owner.WidthOf(text) below.
            int codepointCount = 0;
            float textWidth = 0.0f;
            const bool needWidth = (str.align == CGameFont::alCenter || str.align == CGameFont::alRight);
            if (needWidth)
            {
                const float spacing = owner.GetLetterSpacing();
                for (const char* p = text; *p;)
                {
                    size_t cpLen = 0;
                    const int cp = (int)XRay::Utf8::Decode(p, cpLen);
                    p += cpLen ? cpLen : 1;
                    ++codepointCount;
                    textWidth += owner.WidthOf(cp) + spacing;
                }
                if (codepointCount > 0)
                    textWidth -= spacing;
            }
            else
            {
                codepointCount = (int)XRay::Utf8::LengthCodepoints(text);
            }

            // lock AGP memory
            u32 vOffset;
            FVF::TL* vertexes = (FVF::TL*)RImplementation.Vertex.Lock(codepointCount * 4, pGeom.stride(), vOffset);
            FVF::TL* start = vertexes;

            float X = float(iFloor(str.x));
            float Y = float(iFloor(str.y));
            float Y2 = Y + str.height;

            if (str.align)
            {
                const float width = needWidth ? textWidth : (float)owner.WidthOf(text);

                switch (str.align)
                {
                case CGameFont::alCenter:
                    X -= iFloor(width * 0.5f);
                    break;
                case CGameFont::alRight:
                    X -= iFloor(width);
                    break;
                }
            }

            u32 clr, clr2;
            clr2 = clr = str.c;
            if (str.gradient)
                clr2 = str.gradientColor;

            X -= 0.5f;
            Y -= 0.5f;
            Y2 -= 0.5f;

            bool firstGlyph = true;
            // Apply the global text scale to per-glyph metrics here too, so the
            // rendered advance/offset matches what WidthOf() reported for layout
            // and alignment. Without this, alRight/alCenter text drifts to the
            // right and glyph baselines look uneven when scale != 1.0.
            const float scale = g_text_scale;
            for (const char* p = text; *p;)
            {
                size_t cpLen = 0;
                const int cp = (int)XRay::Utf8::Decode(p, cpLen);
                p += cpLen ? cpLen : 1;

                const CGameFont::Glyph* glyphInfo = owner.GetGlyphInfo(cp);
                if (glyphInfo == nullptr)
                    continue;

                if (!firstGlyph)
                    X += glyphInfo->Abc.abcA * scale;
                firstGlyph = false;

                float GlyphY = Y + glyphInfo->yOffset * scale;
                float GlyphY2 = Y2 + glyphInfo->yOffset * scale;

                float X2 = X + glyphInfo->Abc.abcB * scale;

                float u1 = float(glyphInfo->TextureCoord.left) / fWidth;
                float u2 = float(glyphInfo->TextureCoord.right) / fWidth;

                float v1 = float(glyphInfo->TextureCoord.top) / fHeight;
                float v2 = float(glyphInfo->TextureCoord.bottom) / fHeight;

                if (str.gradientMode == CGameFont::gm_horz)
                {
                    vertexes->set(X, GlyphY2, clr, u1, v2);
                    ++vertexes;
                    vertexes->set(X, GlyphY, clr, u1, v1);
                    ++vertexes;
                    vertexes->set(X2, GlyphY2, clr2, u2, v2);
                    ++vertexes;
                    vertexes->set(X2, GlyphY, clr2, u2, v1);
                    ++vertexes;
                }
                else if (str.gradientMode == CGameFont::gm_back)
                {
                    vertexes->set(X, GlyphY2, clr2, u1, v2);
                    ++vertexes;
                    vertexes->set(X, GlyphY, clr2, u1, v1);
                    ++vertexes;
                    vertexes->set(X2, GlyphY2, clr, u2, v2);
                    ++vertexes;
                    vertexes->set(X2, GlyphY, clr, u2, v1);
                    ++vertexes;
                }
                else if (str.gradientMode == CGameFont::gm_down)
                {
                    vertexes->set(X, GlyphY2, clr, u1, v2);
                    ++vertexes;
                    vertexes->set(X, GlyphY, clr2, u1, v1);
                    ++vertexes;
                    vertexes->set(X2, GlyphY2, clr, u2, v2);
                    ++vertexes;
                    vertexes->set(X2, GlyphY, clr2, u2, v1);
                    ++vertexes;
                }
                else
                {
                    vertexes->set(X, GlyphY2, clr2, u1, v2);
                    ++vertexes;
                    vertexes->set(X, GlyphY, clr, u1, v1);
                    ++vertexes;
                    vertexes->set(X2, GlyphY2, clr2, u2, v2);
                    ++vertexes;
                    vertexes->set(X2, GlyphY, clr2, u2, v1);
                    ++vertexes;
                }
                X = X2 + glyphInfo->Abc.abcC * scale + owner.GetLetterSpacing() * scale;
            }

            // Unlock and draw
            u32 vertexesCount = (u32)(vertexes - start);
            RImplementation.Vertex.Unlock(vertexesCount, pGeom.stride());

            if (vertexesCount > 0)
            {
                RCache.set_Geometry(pGeom);
                RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, vertexesCount, 0, vertexesCount / 2);
            }
        }
    }
}

void dxFontRender::CreateFontAtlas(u32 width, u32 height, const char* name, void* bitmap)
{
    m_atlasWidth = width;
    m_atlasHeight = height;
    Msg("* Font atlas '%s' created: %dx%d", name, width, height);

#if !defined(USE_OGL)
    ID3DTexture2D* pSurface = nullptr;
#endif
    ZoneScoped;

#if defined(USE_DX11)
    D3D_TEXTURE2D_DESC descFontAtlas;
    ZeroMemory(&descFontAtlas, sizeof(D3D_TEXTURE2D_DESC));
    descFontAtlas.Width = width;
    descFontAtlas.Height = height;
    descFontAtlas.MipLevels = 1;
    descFontAtlas.ArraySize = 1;
    descFontAtlas.SampleDesc.Count = 1;
    descFontAtlas.SampleDesc.Quality = 0;
    descFontAtlas.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    descFontAtlas.Usage = D3D_USAGE_IMMUTABLE;
    descFontAtlas.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    descFontAtlas.CPUAccessFlags = 0;
    descFontAtlas.MiscFlags = 0;

    D3D_SUBRESOURCE_DATA FontData;
    FontData.pSysMem = bitmap;
    FontData.SysMemSlicePitch = 0;
    FontData.SysMemPitch = width * 4;

    R_CHK(HW.pDevice->CreateTexture2D(&descFontAtlas, &FontData, &pSurface));
#elif defined(USE_OGL)
    const GLuint oldTextureID = static_cast<GLuint>(m_oglTextureID);
    if (oldTextureID != 0)
        glDeleteTextures(1, &oldTextureID);
    GLuint pTextureID = 0;
    glGenTextures(1, &pTextureID);
    m_oglTextureID = static_cast<u32>(pTextureID);
    glBindTexture(GL_TEXTURE_2D, pTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, bitmap);
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    D3DLOCKED_RECT LockedRect = {};
    R_CHK(HW.pDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pSurface, nullptr));
    R_CHK(pSurface->LockRect(0, &LockedRect, nullptr, 0));

    memcpy(LockedRect.pBits, bitmap, width * height * 4);

    R_CHK(pSurface->UnlockRect(0));
#endif

    pTexture.create(name);

#if defined(USE_OGL)
    pTexture->surface_set(GL_TEXTURE_2D, pTextureID);
#else
    pTexture->surface_set(pSurface);
    _RELEASE(pSurface);
#endif
}
} // namespace xray::render::RENDER_NAMESPACE
