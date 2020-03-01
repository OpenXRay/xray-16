#pragma once
#include "xrUICore/ui_defs.h"
#include "xrCore/_flags.h"
#include "xrCore/_vector2.h"
#ifdef DEBUG
#include "xrCore/xrstring.h"
#endif

class XRUICORE_API CUIStaticItem
{
protected:
public:
    enum
    {
        flValidSize = (1 << 0),
        flValidTextureRect = (1 << 1),
        flValidHeadingPivot = (1 << 2),
        flFixedLTWhileHeading = (1 << 3),
    };

    Frect TextureRect;
    Fvector2 vHeadingPivot;
    Fvector2 vHeadingOffset;
    Flags8 uFlags;

    ui_shader hShader;
    Fvector2 vPos;
    Fvector2 vSize;
    u32 dwColor;
#ifdef DEBUG
    shared_str dbg_tex_name;
#endif

    void CreateShader(LPCSTR tex, LPCSTR sh = "hud" DELIMITER "default");
    void SetShader(const ui_shader& sh) { hShader = sh; };
    void Init(LPCSTR tex, LPCSTR sh, float left, float top);
    void Render();
    void Render(float angle);

    IC void SetPos(float left, float top) { vPos.set(left, top); }
    IC float GetPosX() { return vPos.x; }
    IC float GetPosY() { return vPos.y; }
    IC void SetTextureColor(u32 clr) { dwColor = clr; }
    IC u32 GetTextureColor() const { return dwColor; }
    void SetColor(u32 clr) { dwColor = clr; }
    void SetColor(Fcolor clr) { dwColor = clr.get(); }
    u32 GetColor() const { return dwColor; }
    u32& GetColorRef() { return dwColor; }
    ui_shader& GetShader() { return hShader; }
public:
    CUIStaticItem();
    IC void SetSize(const Fvector2& sz)
    {
        vSize.set(sz);
        uFlags.set(flValidSize, TRUE);
    }
    void SetTextureRect(const Frect& r)
    {
        TextureRect = r;
        uFlags.set(flValidTextureRect, TRUE);
    }
    const Frect& GetTextureRect() const { return TextureRect; };
    IC Fvector2 GetSize() { return vSize; }
    void SetHeadingPivot(const Fvector2& p, const Fvector2& offset, bool fixedLT);
    void ResetHeadingPivot();
    IC bool GetFixedLTWhileHeading() const { return !!uFlags.test(flFixedLTWhileHeading); }
    Fvector2 GetHeadingPivot() { return vHeadingPivot; }
private:
    void RenderInternal(const Fvector2& pos);
    void RenderInternal(float angle);
};
