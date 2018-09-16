#include "StdAfx.h"
#include "UIProgressShape.h"

#include "UIStatic.h"
#include "Include/xrRender/UIShader.h"
#include "Include/xrRender/UIRender.h"

CUIProgressShape::CUIProgressShape()
{
    m_bText = false;
    //	m_pTexture		= new CUIStatic();
    //	AttachChild		(m_pTexture);
    m_blend = true;
    m_angle_begin = 0.0f;
    m_angle_end = PI_MUL_2;
    m_stage = 0.f;
};

CUIProgressShape::~CUIProgressShape()
{
    //	xr_delete		(m_pTexture);
}

void CUIProgressShape::SetPos(float pos) { m_stage = pos; }
void CUIProgressShape::SetPos(int pos, int max)
{
    m_stage = float(pos) / float(max);
    if (m_bText)
    {
        string256 _buff;
        TextItemControl()->SetText(xr_itoa(pos, _buff, 10));
    }
}

void CUIProgressShape::SetTextVisible(bool b) { m_bText = b; }
void _make_rot_pos(Fvector2& pt, float sin_a, float cos_a, float R1, float R2)
{
    pt.x = -R1 * sin_a;
    pt.y = -R2 * cos_a;
}

void _make_rot_tex(Fvector2& pt, float src, float sin_a, float cos_a)
{
    pt.x = src * sin_a;
    pt.y = src * cos_a;
}

float calc_color(u32 idx, u32 total, float stage, float max_stage, bool blend)
{
    float kk = (stage / max_stage) * (float(total + 1));
    if (blend)
    {
        return (1 / (exp((float(idx) - kk) * 0.9f) + 1.0f));
    }

    if ((float)idx < kk)
    {
        return 1.0f;
    }
    return 0.0f;
}

void CUIProgressShape::Draw()
{
    if (m_bText)
        DrawText();

    GEnv.UIRender->SetShader(*GetShader());
    Fvector2 tsize;
    GEnv.UIRender->GetActiveTextureResolution(tsize);

    GEnv.UIRender->StartPrimitive(m_sectorCount * 3, IUIRender::ptTriList, UI().m_currentPointType);

    Frect pos_rect;
    GetAbsoluteRect(pos_rect);
    UI().ClientToScreenScaled(pos_rect.lt, pos_rect.x1, pos_rect.y1);
    UI().ClientToScreenScaled(pos_rect.rb, pos_rect.x2, pos_rect.y2);

    Fvector2 center_pos;
    pos_rect.getcenter(center_pos);

    Frect tex_rect = GetUIStaticItem().GetTextureRect();

    tex_rect.lt.x /= tsize.x;
    tex_rect.lt.y /= tsize.y;
    tex_rect.rb.x /= tsize.x;
    tex_rect.rb.y /= tsize.y;

    Fvector2 center_tex;
    tex_rect.getcenter(center_tex);

    float radius_pos = pos_rect.width() / 2.0f;

    float radius_tex = tex_rect.width() / 2.0f;

    float curr_angle = m_angle_begin;
    float sin_a = _sin(curr_angle);
    float cos_a = _cos(curr_angle);
    Fvector2 start_pos_pt, prev_pos_pt;
    Fvector2 start_tex_pt, prev_tex_pt;

    start_pos_pt.set(0.0f, -radius_pos);
    prev_pos_pt = start_pos_pt;

    start_tex_pt.set(0.0f, -radius_tex);
    prev_tex_pt = start_tex_pt;

    _make_rot_tex(prev_pos_pt, start_pos_pt.y, sin_a, cos_a);
    _make_rot_tex(prev_tex_pt, start_tex_pt.y, sin_a, cos_a);

    float angle_range = PI_MUL_2;
    if (m_bClockwise)
    {
        angle_range = -abs(m_angle_end - m_angle_begin);
    }
    else
    {
        angle_range = abs(m_angle_end - m_angle_begin);
    }

    for (u32 i = 0; i < m_sectorCount; ++i)
    {
        float ffff = calc_color(i + 1, m_sectorCount, m_stage, 1.0f, m_blend);
        u32 color = color_argb_f(ffff, 1.0f, 1.0f, 1.0f);

        GEnv.UIRender->PushPoint(center_pos.x, center_pos.y, 0, color, center_tex.x, center_tex.y);

        Fvector2 tp;
        tp.set(prev_pos_pt);
        tp.add(center_pos);

        Fvector2 tx;
        tx.set(prev_tex_pt);
        tx.add(center_tex);

        Fvector2 tp1;
        Fvector2 tx1;
        tp1.set(tp);
        tx1.set(tx);

        curr_angle += angle_range / float(m_sectorCount);

        sin_a = _sin(curr_angle);
        cos_a = _cos(curr_angle);

        _make_rot_tex(prev_pos_pt, start_pos_pt.y, sin_a, cos_a);
        _make_rot_tex(prev_tex_pt, start_tex_pt.y, sin_a, cos_a);

        tp.set(prev_pos_pt);
        tp.add(center_pos);

        tx.set(prev_tex_pt);
        tx.add(center_tex);

        if (m_bClockwise)
        {
            GEnv.UIRender->PushPoint(tp1.x, tp1.y, 0, color, tx1.x, tx1.y);
            GEnv.UIRender->PushPoint(tp.x, tp.y, 0, color, tx.x, tx.y);
        }
        else
        {
            GEnv.UIRender->PushPoint(tp.x, tp.y, 0, color, tx.x, tx.y);
            GEnv.UIRender->PushPoint(tp1.x, tp1.y, 0, color, tx1.x, tx1.y);
        }
    }

    GEnv.UIRender->FlushPrimitive();
}
