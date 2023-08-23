#pragma once
#include "UIWindow.h"

class XRUICORE_API CUIFrameLineWnd : public CUIWindow
{
    typedef CUIWindow inherited;

public:
    enum RectSegment
    {
        flFirst = 0, // Left or top
        flBack, // Center texture
        flSecond, // Right or bottom
        flMax
    };

    CUIFrameLineWnd(pcstr window_name);

    bool InitFrameLineWnd(LPCSTR base_name, Fvector2 pos, Fvector2 size, bool horizontal = true, bool fatal = true);
    void InitFrameLineWnd(Fvector2 pos, Fvector2 size, bool horizontal = true);
    bool InitTexture(pcstr texture, bool fatal = true);
    bool InitTexture(pcstr texture, pcstr shader = "hud" DELIMITER "default", bool fatal = true);

    virtual void Draw();

    float GetTextureHeight() const { return m_tex_rect[0].height(); }
    float GetTextureWidth() const { return m_tex_rect[0].width(); }
    void SetTextureColor(u32 cl) { m_texture_color = cl; }
    bool IsHorizontal() { return bHorizontal; }
    void SetHorizontal(bool horiz) { bHorizontal = horiz; }

    void SetTextureVisible(bool value) { m_bTextureVisible = value; }
    void SetShader(ui_shader sh) { m_shader = sh; }
    void SetTextureRect(Frect rect, RectSegment idx)
    {
        R_ASSERT(idx >= flFirst && idx <= flSecond);
        m_tex_rect[idx] = rect;
    }

    pcstr GetDebugType() override { return "CUIFrameLineWnd"; }

protected:
    bool bHorizontal;
    bool inc_pos(Frect& rect, int counter, int i, Fvector2& LTp, Fvector2& RBp, Fvector2& LTt, Fvector2& RBt);

    u32 m_texture_color;
    bool m_bTextureVisible;
    void DrawElements();

    ui_shader m_shader;
    Frect m_tex_rect[flMax];
    shared_str dbg_tex_name;
};
