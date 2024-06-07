#pragma once
#include "UIWindow.h"

class XRUICORE_API CUIFrameLineWnd : public CUIWindow, public ITextureOwner
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

    bool InitFrameLineWnd(pcstr base_name, Fvector2 pos, Fvector2 size, bool horizontal = true, bool fatal = true);
    void InitFrameLineWnd(Fvector2 pos, Fvector2 size, bool horizontal = true);
    bool InitTexture(pcstr texture, bool fatal = true) override;
    bool InitTextureEx(pcstr texture, pcstr shader = "hud" DELIMITER "default", bool fatal = true) override;

    void Draw() override;

    void SetTextureRect(const Frect rect, RectSegment idx)
    {
        VERIFY(idx >= flFirst && idx < flMax);
        if (idx >= flMax)
            return;
        m_tex_rect[idx] = rect;
    }

    void SetTextureRect(const Frect& r) override
    {
        VERIFY2(false, "This overload is not supposed to be called!!!");
        m_tex_rect[flBack] = r;
    }

    const Frect& GetTextureRect() const override
    {
        VERIFY2(false, "This overload is not supposed to be called!!!");
        return m_tex_rect[flBack];
    }

    void SetTextureColor(u32 cl) override { m_texture_color = cl; }
    u32 GetTextureColor() const override { return m_texture_color; }

    void SetStretchTexture(bool /*stretch*/) override {}
    bool GetStretchTexture() override { return false; }

    void SetTextureVisible(bool value) { m_bTextureVisible = value; }
    void SetShader(const ui_shader& sh) { m_shader = sh; }

    bool IsHorizontal() const { return bHorizontal; }
    void SetHorizontal(bool horiz) { bHorizontal = horiz; }

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
