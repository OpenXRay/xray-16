#include "StdAfx.h"
#include "UIFrameRect.h"
#include "HUDManager.h"
#include "ui/UITextureMaster.h"

CUIFrameRect::CUIFrameRect()
{
    uFlags.zero();
    m_itm_mask.one();
}

void CUIFrameRect::InitTextureEx(LPCSTR texture, LPCSTR shader)
{
    string_path buf;
    strcpy_s(buf, texture);

    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_back"), shader, &frame[CUIFrameRect::fmBK]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_l"), shader, &frame[CUIFrameRect::fmL]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_r"), shader, &frame[CUIFrameRect::fmR]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_t"), shader, &frame[CUIFrameRect::fmT]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_b"), shader, &frame[CUIFrameRect::fmB]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_lt"), shader, &frame[CUIFrameRect::fmLT]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_rt"), shader, &frame[CUIFrameRect::fmRT]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_rb"), shader, &frame[CUIFrameRect::fmRB]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_lb"), shader, &frame[CUIFrameRect::fmLB]);
}

void CUIFrameRect::InitTexture(LPCSTR texture) { InitTextureEx(texture, "hud" DELIMITER "default"); }
void CUIFrameRect::UpdateSize(bool recall)
{
    //	VERIFY(g_bRendering);
    // texture size
    Fvector2 ts;
    float rem_x, rem_y;
    int tile_x, tile_y;

    Fvector2 _bk, _lt, _lb, _rb, _rt, _l, _r, _t, _b;

    _bk.set(frame[fmBK].GetOriginalRect().width(), frame[fmBK].GetOriginalRect().height());
    _lt.set(frame[fmLT].GetOriginalRect().width(), frame[fmLT].GetOriginalRect().height());
    _lb.set(frame[fmLB].GetOriginalRect().width(), frame[fmLB].GetOriginalRect().height());
    _rb.set(frame[fmRB].GetOriginalRect().width(), frame[fmRB].GetOriginalRect().height());
    _rt.set(frame[fmRT].GetOriginalRect().width(), frame[fmRT].GetOriginalRect().height());
    _l.set(frame[fmL].GetOriginalRect().width(), frame[fmL].GetOriginalRect().height());
    _r.set(frame[fmR].GetOriginalRect().width(), frame[fmR].GetOriginalRect().height());
    _t.set(frame[fmT].GetOriginalRect().width(), frame[fmT].GetOriginalRect().height());
    _b.set(frame[fmB].GetOriginalRect().width(), frame[fmB].GetOriginalRect().height());

    Fvector2 wnd_pos = GetWndPos();
    frame[fmLT].SetPos(wnd_pos.x, wnd_pos.y);
    frame[fmRT].SetPos(wnd_pos.x + m_wndSize.x - _rt.x, wnd_pos.y);
    frame[fmLB].SetPos(wnd_pos.x, wnd_pos.y + m_wndSize.y - _lb.y);
    frame[fmRB].SetPos(wnd_pos.x + m_wndSize.x - _rb.x, wnd_pos.y + m_wndSize.y - _rb.y);

    float size_top = m_wndSize.x - _lt.x - _rt.x;
    float size_bottom = m_wndSize.x - _lb.x - _rb.x;
    float size_left = m_wndSize.y - _lt.y - _lb.y;
    float size_right = m_wndSize.y - _rt.y - _rb.y;

    bool resizing = false;
    Fvector2 new_size = GetWndSize();
    if (size_top < 0.0f || size_bottom < 0.0f)
    {
        Fvector2 n1;
        n1.x = _lt.x + _rt.x;
        n1.y = _lb.x + _rb.x;
        new_size.x = _max(n1.x, n1.y);
        resizing = true;
    }
    if (size_left < 0.0f || size_right < 0.0f)
    {
        Fvector2 n2;
        n2.x = _lt.y + _lb.y;
        n2.y = _rt.y + _rb.y;
        new_size.y = _max(n2.x, n2.y);
        resizing = true;
    }

    if (resizing)
    {
        SetWndSize(new_size);
        VERIFY(!recall);
        UpdateSize(true);
        return;
    }

    //Фон
    ts.set(_bk.x, _bk.y);
    rem_x = fmod(size_top, ts.x);
    rem_x = _max(rem_x, 0.0f);
    rem_y = fmod(size_left, ts.y);
    rem_y = _max(rem_y, 0.0f);
    tile_x = iFloor(size_top / ts.x);
    tile_x = _max(tile_x, 0);
    tile_y = iFloor(size_left / ts.y);
    tile_y = _max(tile_y, 0);

    frame[fmBK].SetPos(wnd_pos.x + _lt.x, wnd_pos.y + _lt.y);
    frame[fmBK].SetTile(tile_x, tile_y, rem_x, rem_y);

    //Обрамление
    ts.set(_t.x, _t.y);
    rem_x = fmod(size_top, ts.x);
    tile_x = iFloor(size_top / ts.x);
    tile_x = _max(tile_x, 0);
    frame[fmT].SetPos(wnd_pos.x + _lt.x, wnd_pos.y);
    frame[fmT].SetTile(tile_x, 1, rem_x, 0);

    ts.set(_b.x, _b.y);
    rem_x = fmod(size_bottom, ts.x);
    tile_x = iFloor(size_bottom / ts.x);
    tile_x = _max(tile_x, 0);
    frame[fmB].SetPos(wnd_pos.x + _lb.x, wnd_pos.y + m_wndSize.y - ts.y);
    frame[fmB].SetTile(tile_x, 1, rem_x, 0);

    ts.set(_l.x, _l.y);
    rem_y = fmod(size_left, ts.y);
    tile_y = iFloor(size_left / ts.y);
    tile_y = _max(tile_y, 0);
    frame[fmL].SetPos(wnd_pos.x, wnd_pos.y + _lt.y);
    frame[fmL].SetTile(1, tile_y, 0, rem_y);

    ts.set(_r.x, _r.y);
    rem_y = fmod(size_right, ts.y);
    tile_y = iFloor(size_right / ts.y);
    tile_y = _max(tile_y, 0);
    frame[fmR].SetPos(wnd_pos.x + m_wndSize.x - ts.x, wnd_pos.y + _rt.y);
    frame[fmR].SetTile(1, tile_y, 0, rem_y);

    uFlags.set(flValidSize, TRUE);
}

void CUIFrameRect::Draw()
{
    if (!uFlags.is(flValidSize))
    {
        VERIFY(g_bRendering);
        UpdateSize();
    }

    for (int k = 0; k < fmMax; ++k)
        if (m_itm_mask.test(u16(1 << k)))
            frame[k].Render();
}

void CUIFrameRect::Update() {}
void CUIFrameRect::SetWndSize(const Fvector2& size)
{
    CUISimpleWindow::SetWndSize(size);
    uFlags.set(flValidSize, false);
}

void CUIFrameRect::SetWndRect(const Frect& rect)
{
    CUISimpleWindow::SetWndRect(rect);
    uFlags.set(flValidSize, false);
}

void CUIFrameRect::SetWndPos(const Fvector2& pos)
{
    Fvector2 _old_pos = GetWndPos();
    if (_old_pos.similar(pos, EPS))
        return;

    CUISimpleWindow::SetWndPos(pos);
    uFlags.set(flValidSize, false);
}

void CUIFrameRect::SetHeight(float height)
{
    CUISimpleWindow::SetHeight(height);
    uFlags.set(flValidSize, false);
}

void CUIFrameRect::SetWidth(float width)
{
    CUISimpleWindow::SetWidth(width);
    uFlags.set(flValidSize, false);
}

void CUIFrameRect::Draw(float x, float y)
{
    Fvector2 p = GetWndPos();
    float dx = p.x - x;
    float dy = p.y - y;
    if (!fis_zero(dx) || !fis_zero(dy))
        SetWndPos(Fvector2().set(x, y));

    Draw();
}

void CUIFrameRect::SetTextureColor(u32 cl)
{
    for (int i = 0; i < fmMax; ++i)
        frame[i].SetColor(cl);
}
