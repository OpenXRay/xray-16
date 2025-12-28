#include "pch.hpp"

#include "UIFrameLineWnd.h"

#include "Static/UIStatic.h"
#include "XML/UITextureMaster.h"

#include "xrEngine/editor_helper.h"

CUIFrameLineWnd::CUIFrameLineWnd(pcstr window_name) : CUIWindow(window_name) {}

bool CUIFrameLineWnd::InitTexture(pcstr texture, bool fatal /*= true*/)
{
    return InitTextureEx(texture, "hud\\default", fatal);
}

bool CUIFrameLineWnd::InitTextureEx(pcstr texture, pcstr shader, bool fatal /*= true*/)
{
    m_bTextureVisible = false;

    string_path first, second, back;
    strconcat(back, texture, "_back");
    strconcat(first, texture, "_b");
    strconcat(second, texture, "_e");

    const bool back_exist = CUITextureMaster::InitTexture(back, shader, m_shader[flBack], m_tex_rect[flBack]);
    const bool b_exist = CUITextureMaster::InitTexture(first, shader, m_shader[flFirst], m_tex_rect[flFirst]);
    const bool e_exist = CUITextureMaster::InitTexture(second, shader, m_shader[flSecond], m_tex_rect[flSecond]);

    if (back_exist && b_exist && e_exist)
    {
        m_bTextureVisible = true;

#ifndef MASTER_GOLD
        if (bHorizontal)
        {
            if (b_exist && e_exist && !fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flSecond].height()))
                Msg("~ Textures %s_b and %s_e are not similar by height", texture, texture);
            if (b_exist && back_exist && !fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flBack].height()))
                Msg("~ Textures %s_b and %s_back are not similar by height", texture, texture);
        }
        else
        {
            if (b_exist && e_exist && !fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flSecond].width()))
                Msg("~ Textures %s_b and %s_e are not similar by width", texture, texture);
            if (b_exist && back_exist && !fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flBack].width()))
                Msg("~ Textures %s_b and %s_back are not similar by width", texture, texture);
        }
#endif
    }
    else if (fatal)
    {
        R_ASSERT3(back_exist, "Texture needed for CUIFrameLineWnd is missing", back);
        R_ASSERT3(b_exist, "Texture needed for CUIFrameLineWnd is missing", first);
        R_ASSERT3(e_exist, "Texture needed for CUIFrameLineWnd is missing", second);
    }

    return m_bTextureVisible;
}

void CUIFrameLineWnd::Draw()
{
    if (m_bTextureVisible)
        DrawElements();

    inherited::Draw();
}

constexpr Fvector2 pt_offset = { -0.5f, -0.5f };

void draw_rect(Fvector2 LTp, Fvector2 RBp, Fvector2 LTt, Fvector2 RBt, u32 clr, Fvector2 const& ts)
{
    UI().AlignPixel(LTp.x);
    UI().AlignPixel(LTp.y);
    LTp.add(pt_offset);
    UI().AlignPixel(RBp.x);
    UI().AlignPixel(RBp.y);
    RBp.add(pt_offset);
    LTt.div(ts);
    RBt.div(ts);

    GEnv.UIRender->PushPoint(LTp.x, LTp.y, 0, clr, LTt.x, LTt.y);
    GEnv.UIRender->PushPoint(RBp.x, RBp.y, 0, clr, RBt.x, RBt.y);
    GEnv.UIRender->PushPoint(LTp.x, RBp.y, 0, clr, LTt.x, RBt.y);

    GEnv.UIRender->PushPoint(LTp.x, LTp.y, 0, clr, LTt.x, LTt.y);
    GEnv.UIRender->PushPoint(RBp.x, LTp.y, 0, clr, RBt.x, LTt.y);
    GEnv.UIRender->PushPoint(RBp.x, RBp.y, 0, clr, RBt.x, RBt.y);
}

void CUIFrameLineWnd::DrawElements() const
{
    Frect rect;
    GetAbsoluteRect(rect);
    UI().ClientToScreenScaled(rect.lt);
    UI().ClientToScreenScaled(rect.rb);

    float first_len  = bHorizontal ? m_tex_rect[flFirst].width()  : m_tex_rect[flFirst].height();
    float second_len = bHorizontal ? m_tex_rect[flSecond].width() : m_tex_rect[flSecond].height();
    float back_len   = bHorizontal ? m_tex_rect[flBack].width()   : m_tex_rect[flBack].height();

    if (bHorizontal)
    {
        first_len = UI().ClientToScreenScaledX(first_len);
        back_len = UI().ClientToScreenScaledX(back_len);
        second_len = UI().ClientToScreenScaledX(second_len) * UI().get_current_kx();
    }
    else
    {
        first_len = UI().ClientToScreenScaledY(first_len);
        back_len = UI().ClientToScreenScaledY(back_len);
        second_len = UI().ClientToScreenScaledY(second_len);
    }

    u32 total_tiles{};
    u32 back_tiles{};
    float back_remainder{};

    const float total_line_len = bHorizontal ? rect.width() : rect.height();
    const float back_available_len = total_line_len - first_len - second_len;

    if (back_available_len > 0.0f && back_len > 0.0f)
    {
        const float back_tiles_full = back_available_len / back_len;
        total_tiles   += iCeil(back_tiles_full);
        back_tiles     = iFloor(back_tiles_full);
        back_remainder = back_tiles_full - float(back_tiles);
    }
    else if (back_available_len < 0.0f)
    {
        if (bHorizontal)
            rect.x2 -= back_len;
        else
            rect.y2 -= back_len;
    }

    float cursor{};

    const auto draw_tile = [&](const float length, const Frect tex_rect, const RectSegment segment)
    {
        if (fis_zero(length))
            return;

        Fvector2 lt, rb;
        if (bHorizontal)
        {
            float height;
            if (m_bStretchTexture && rect.height() > 0.0f)
                height = rect.rb.y;
            else
                height = rect.lt.y + UI().ClientToScreenScaledY(tex_rect.height());

            lt = { rect.lt.x + cursor, rect.lt.y };
            rb = { lt.x      + length, height    };
        }
        else
        {
            float width;
            if (m_bStretchTexture && rect.width() > 0.0f)
                width = rect.rb.x;
            else
                width = rect.lt.x + UI().ClientToScreenScaledX(tex_rect.width());

            lt = { rect.lt.x, rect.lt.y + cursor };
            rb = { width,     lt.y      + length };
        }
        cursor += length;

        Fvector2 ts{};
        m_shader[segment]->GetBaseTextureResolution(ts);
        GEnv.UIRender->SetShader(*m_shader[segment]);

        draw_rect(lt, rb, tex_rect.lt, tex_rect.rb, m_texture_color, ts);
    };

    bool one_shader = false;
    if (m_shader[flFirst] == m_shader[flBack] && m_shader[flBack] == m_shader[flSecond])
        one_shader = true;

    // first
    const auto first_tiles = one_shader ? total_tiles + 2 : 1;
    GEnv.UIRender->StartPrimitive(6 * first_tiles, IUIRender::ptTriList, UI().m_currentPointType);
    draw_tile(first_len, m_tex_rect[flFirst], flFirst);

    if (!one_shader)
    {
        GEnv.UIRender->FlushPrimitive();
        GEnv.UIRender->StartPrimitive(6 * total_tiles, IUIRender::ptTriList, UI().m_currentPointType);
    }

    // back
    for (u32 i = 0; i < back_tiles; ++i)
    {
        draw_tile(back_len, m_tex_rect[flBack], flBack);
    }

    if (back_remainder > 0.0f)
    {
        Frect remainder_tc = m_tex_rect[flBack];
        if (bHorizontal)
            remainder_tc.rb.x = remainder_tc.lt.x + remainder_tc.width() * back_remainder;
        else
            remainder_tc.rb.y = remainder_tc.lt.y + remainder_tc.height() * back_remainder;

        draw_tile(back_len * back_remainder, remainder_tc, flBack);
    }

    if (!one_shader)
    {
        GEnv.UIRender->FlushPrimitive();
        GEnv.UIRender->StartPrimitive(6, IUIRender::ptTriList, UI().m_currentPointType);
    }

    // second
    draw_tile(second_len, m_tex_rect[flSecond], flSecond);
    GEnv.UIRender->FlushPrimitive();
}

CUIStatic* CUIFrameLineWnd::GetTitleText(bool create_on_demand /*= false*/)
{
    if (create_on_demand && !m_title_text)
    {
        m_title_text = xr_new<CUIStatic>("title");
        m_title_text->SetAutoDelete(true);
        AttachChild(m_title_text);
    }
    return m_title_text;
}

bool CUIFrameLineWnd::FillDebugTree(const CUIDebugState& debugState)
{
    return CUIWindow::FillDebugTree(debugState);
}

void CUIFrameLineWnd::FillDebugInfo()
{
#ifndef MASTER_GOLD
    CUIWindow::FillDebugInfo();

    if (!ImGui::CollapsingHeader(CUIFrameLineWnd::GetDebugType()))
        return;

    ImGui::Checkbox("Enable texture", &m_bTextureVisible);

    ImGui::SameLine();
    ImGui::Checkbox("Stretch texture", &m_bStretchTexture);

    ImGui::SameLine();
    ImGui::Checkbox("Horizontal", &bHorizontal);

    xray::imgui::ColorEdit4("Texture color", m_texture_color);

    const auto showRectAdjust = [&](pcstr label, RectSegment segment)
    {
        Fvector2 ts{};
        m_shader[segment]->GetBaseTextureResolution(ts);

        Frect& tex_rect = m_tex_rect[segment];
        Frect rect;
        rect.lt = tex_rect.lt;
        rect.rb = { tex_rect.width(), tex_rect.height() };

        if (ImGui::DragFloat4(label, reinterpret_cast<float*>(&rect), 1.0f, 0.0f, std::max(ts.x, ts.y)))
        {
            tex_rect.lt = rect.lt;
            tex_rect.rb = rect.lt.add(rect.rb);
            clamp(tex_rect.lt.x, 0.0f, ts.x);
            clamp(tex_rect.lt.y, 0.0f, ts.y);
            clamp(tex_rect.rb.x, 0.0f, ts.x);
            clamp(tex_rect.rb.y, 0.0f, ts.y);
        }
    };

    showRectAdjust("Texture rect: first", flFirst);
    showRectAdjust("Texture rect: middle", flBack);
    showRectAdjust("Texture rect: second", flSecond);
#endif
}
