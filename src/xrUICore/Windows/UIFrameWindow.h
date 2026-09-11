#pragma once

#include "UIWindow.h"

class CUIStatic;

class XRUICORE_API CUIFrameWindow : public CUIWindow, public ITextureOwner
{
    typedef CUIWindow inherited;

public:
    CUIFrameWindow(pcstr window_name);

    virtual void SetTextureRect(const Frect& r){};
    virtual const Frect& GetTextureRect() const { return m_tex_rect[fmBK]; }
    virtual void SetWndSize(const Fvector2& size);

    virtual void SetTextureColor(u32 color) { m_texture_color = color; }
    virtual u32 GetTextureColor() const { return m_texture_color; }
    virtual bool InitTexture(pcstr texture, bool fatal = true);
    virtual bool InitTextureEx(pcstr texture, pcstr shader, bool fatal = true);

    virtual void SetStretchTexture(bool stretch) {}
    virtual bool GetStretchTexture() { return false; };
    virtual void Draw();

    CUIStatic* GetTitleText(bool create_on_demand = false);

    pcstr GetDebugType() override { return "CUIFrameWindow"; }

protected:

    enum EFramePart
    {
        fmBK = 0,
        fmL,
        fmR,
        fmT,
        fmB,
        fmLT,
        fmRB,
        fmRT,
        fmLB,
        fmMax
    };

    ui_shader m_shader;
    Frect m_tex_rect[fmMax];
    u32 m_texture_color;
    bool m_bTextureVisible;

    CUIStatic* m_title_text{};

    void DrawElements();
    bool get_points(Frect const& r, int i, Fvector2& LTp, Fvector2& RBp, Fvector2& LTt, Fvector2& RBt);
    void draw_tile_line(Frect rect, int i, bool b_horz, Fvector2 const& ts);
    void draw_tile_rect(Frect rect, int i, Fvector2 const& ts);

private:
    DECLARE_SCRIPT_REGISTER_FUNCTION(CUIWindow);
};
