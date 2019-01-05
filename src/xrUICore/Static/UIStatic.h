#pragma once
#include "xrUICore/Static/UILanimController.h"
#include "xrUICore/Static/UIStaticItem.h"
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Lines/UILines.h"

class CUIFrameWindow;
class CLAItem;
class CUIXml;

struct lanim_cont
{
    CLAItem* m_lanim;
    float m_lanim_start_time;
    float m_lanim_delay_time;
    Flags8 m_lanimFlags;
    void set_defaults();
};

struct lanim_cont_xf : public lanim_cont
{
    Fvector2 m_origSize;
    void set_defaults();
};

class XRUICORE_API CUIStatic : public CUIWindow, public ITextureOwner, public CUILightAnimColorConrollerImpl
{
    friend class CUIXmlInitBase;

private:
    typedef CUIWindow inherited;
    lanim_cont_xf m_lanim_xform;
    void EnableHeading_int(bool b) { m_bHeading = b; }
public:
    CUIStatic();
    virtual ~CUIStatic();

    virtual void Draw();
    virtual void Update();
    virtual void OnFocusLost();

    virtual pcstr GetText() { return TextItemControl()->GetText(); }
    virtual void SetText(pcstr txt) { TextItemControl()->SetText(txt); }
    virtual void SetTextST(pcstr txt) { TextItemControl()->SetTextST(txt); }

    void SetTextColor_script(int a, int r, int g, int b)
    {
        TextItemControl()->SetTextColor(color_argb(a, r, g, b));
    }

    u32 GetTextAlign_script()
    {
        return static_cast<u32>(TextItemControl()->GetTextAlignment());
    }

    void SetTextAlign_script(u32 align)
    {
        TextItemControl()->SetTextAlignment((CGameFont::EAligment)align);
        TextItemControl()->GetFont()->SetAligment((CGameFont::EAligment)align);
    }

    virtual void SetTextX(float text_x) { TextItemControl()->m_TextOffset.x = text_x; }
    virtual void SetTextY(float text_y) { TextItemControl()->m_TextOffset.y = text_y; }
    virtual float GetTextX() { return TextItemControl()->m_TextOffset.x; }
    virtual float GetTextY() { return TextItemControl()->m_TextOffset.y; }

    virtual void SetColor(u32 color) { m_UIStaticItem.SetColor(color); }
    virtual u32 GetColor() const { return m_UIStaticItem.GetColor(); }

    virtual void CreateShader(LPCSTR tex, LPCSTR sh = "hud" DELIMITER "default");
    ui_shader& GetShader() { return m_UIStaticItem.GetShader(); };
    virtual void SetTextureColor(u32 color) { m_UIStaticItem.SetTextureColor(color); }
    virtual u32 GetTextureColor() const { return m_UIStaticItem.GetTextureColor(); }
    virtual void SetTextureRect(const Frect& r) { m_UIStaticItem.SetTextureRect(r); }
    virtual const Frect& GetTextureRect() const { return m_UIStaticItem.GetTextureRect(); }
    virtual void InitTexture(LPCSTR tex_name);
    virtual void InitTextureEx(LPCSTR tex_name, LPCSTR sh_name = "hud" DELIMITER "default");
    CUIStaticItem* GetStaticItem() { return &m_UIStaticItem; }
    void SetTextureRect_script(Frect* pr) { m_UIStaticItem.SetTextureRect(*pr); }
    const Frect* GetTextureRect_script() { return &m_UIStaticItem.GetTextureRect(); }
    void SetHeadingPivot(const Fvector2& p, const Fvector2& offset, bool fixedLT)
    {
        m_UIStaticItem.SetHeadingPivot(p, offset, fixedLT);
    }
    void ResetHeadingPivot() { m_UIStaticItem.ResetHeadingPivot(); }
    virtual void SetTextureOffset(float x, float y) { m_TextureOffset.set(x, y); }
    Fvector2 GetTextureOffeset() const { return m_TextureOffset; }
    void TextureOn() { m_bTextureEnable = true; }
    void TextureOff() { m_bTextureEnable = false; }
    // own
    void SetXformLightAnim(LPCSTR lanim, bool bCyclic);
    void ResetXformAnimation();

    virtual void DrawTexture();
    virtual void DrawText();

    void AdjustHeightToText();
    void AdjustWidthToText();

    void SetShader(const ui_shader& sh);
    CUIStaticItem& GetUIStaticItem() { return m_UIStaticItem; }
    void SetStretchTexture(bool stretch_texture) { m_bStretchTexture = stretch_texture; }
    bool GetStretchTexture() { return m_bStretchTexture; }
    void SetEllipsis(int pos, int indent) { TextItemControl()->SetEllipsis(pos != 0); }
    void SetHeading(float f) { m_fHeading = f; };
    float GetHeading() { return m_fHeading; }
    bool Heading() { return m_bHeading; }
    void EnableHeading(bool b) { m_bHeading = b; }
    void SetConstHeading(bool b) { m_bConstHeading = b; };
    bool GetConstHeading() { return m_bConstHeading; }
    virtual void ColorAnimationSetTextureColor(u32 color, bool only_alpha);
    virtual void ColorAnimationSetTextColor(u32 color, bool only_alpha);

protected:
    CUILines* m_pTextControl;

    bool m_bStretchTexture;
    bool m_bTextureEnable;
    CUIStaticItem m_UIStaticItem;

    bool m_bHeading;
    bool m_bConstHeading;
    float m_fHeading;

    Fvector2 m_TextureOffset;

public:
    CUILines* TextItemControl();
    shared_str m_stat_hint_text;
};

class XRUICORE_API CUITextWnd : public CUIWindow, public CUILightAnimColorConrollerImpl
{
    typedef CUIWindow inherited;
    CUILines m_lines;

public:
    CUITextWnd();
    virtual ~CUITextWnd(){};
    virtual void Draw();
    virtual void Update();

    void AdjustHeightToText();
    void AdjustWidthToText();

    void SetText(LPCSTR txt) { TextItemControl().SetText(txt); }
    void SetTextST(LPCSTR txt) { TextItemControl().SetTextST(txt); }
    LPCSTR GetText() { return TextItemControl().GetText(); }
    void SetFont(CGameFont* F) { TextItemControl().SetFont(F); }
    CGameFont* GetFont() { return TextItemControl().GetFont(); }
    void SetTextColor(u32 color) { TextItemControl().SetTextColor(color); }
    u32 GetTextColor() { return TextItemControl().GetTextColor(); }
    void SetTextComplexMode(bool mode = true) { TextItemControl().SetTextComplexMode(mode); }
    void SetTextAlignment(ETextAlignment al) { TextItemControl().SetTextAlignment(al); }
    void SetVTextAlignment(EVTextAlignment al) { TextItemControl().SetVTextAlignment(al); }
    void SetEllipsis(bool mode) { TextItemControl().SetEllipsis(mode); }
    void SetCutWordsMode(bool mode) { TextItemControl().SetCutWordsMode(mode); }
    void SetTextOffset(float x, float y)
    {
        TextItemControl().m_TextOffset.x = x;
        TextItemControl().m_TextOffset.y = y;
    }

    virtual void ColorAnimationSetTextColor(u32 color, bool only_alpha);

    CUILines& TextItemControl() { return m_lines; }
};
