#pragma once
#include "UIWindow.h"

class XRUICORE_API CUIFrameLineWnd : public CUIWindow
{
    typedef CUIWindow inherited;

public:
    CUIFrameLineWnd();
    void InitFrameLineWnd(LPCSTR base_name, Fvector2 pos, Fvector2 size, bool horizontal = true);
    void InitFrameLineWnd(Fvector2 pos, Fvector2 size, bool horizontal = true);
    void InitTexture(LPCSTR tex_name, LPCSTR sh_name = "hud" DELIMITER "default");
    virtual void Draw();

    float GetTextureHeight() const { return m_tex_rect[0].height(); }
    float GetTextureWidth() const { return m_tex_rect[0].width(); }
    void SetTextureColor(u32 cl) { m_texture_color = cl; }
    bool IsHorizontal() { return bHorizontal; }
    void SetHorizontal(bool horiz) { bHorizontal = horiz; }
protected:
    bool bHorizontal;
    bool inc_pos(Frect& rect, int counter, int i, Fvector2& LTp, Fvector2& RBp, Fvector2& LTt, Fvector2& RBt);
    enum
    {
        flFirst = 0, // Left or top
        flBack, // Center texture
        flSecond, // Right or bottom
        flMax
    };
    u32 m_texture_color;
    bool m_bTextureVisible;
    void DrawElements();

    ui_shader m_shader;
    Frect m_tex_rect[flMax];
    shared_str dbg_tex_name;
};
