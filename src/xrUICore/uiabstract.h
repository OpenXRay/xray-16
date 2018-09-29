#pragma once

#pragma warning(disable : 4511)
#pragma warning(disable : 4512)

#include "Common/Noncopyable.hpp"
#include "xrEngine/GameFont.h"

typedef CGameFont::EAligment ETextAlignment;

typedef enum { valTop = 0, valCenter, valBotton } EVTextAlignment;

class ITextureOwner
{
public:
    virtual ~ITextureOwner() {}
    virtual void InitTexture(LPCSTR texture) = 0;
    virtual void InitTextureEx(LPCSTR texture, LPCSTR shader) = 0;
    virtual void SetTextureRect(const Frect& r) = 0;
    virtual const Frect& GetTextureRect() const = 0;
    virtual void SetTextureColor(u32 color) = 0;
    virtual u32 GetTextureColor() const = 0;
    virtual void SetStretchTexture(bool stretch) = 0;
    virtual bool GetStretchTexture() = 0;
};

// Window
enum EWindowAlignment
{
    waNone = 0,
    waLeft = 1,
    waRight = 2,
    waTop = 4,
    waBottom = 8,
    waCenter = 16
};

class CUISimpleWindow : public Noncopyable
{
public:
    CUISimpleWindow() : m_bShowMe(false)
    {
        m_alignment = waNone;
        m_wndPos.set(0, 0);
        m_wndSize.set(0, 0);
    }

    virtual void SetWndPos(const Fvector2& pos) { m_wndPos.set(pos.x, pos.y); }
    IC const Fvector2& GetWndPos() const { return m_wndPos; }
    virtual void SetWndSize(const Fvector2& size) { m_wndSize = size; }
    IC const Fvector2& GetWndSize() const { return m_wndSize; }
    virtual void SetWndRect(const Frect& rect)
    {
        m_wndPos.set(rect.lt);
        rect.getsize(m_wndSize);
    }

    virtual void SetHeight(float height) { m_wndSize.y = height; }
    IC float GetHeight() const { return m_wndSize.y; }
    virtual void SetWidth(float width) { m_wndSize.x = width; }
    IC float GetWidth() const { return m_wndSize.x; }
    IC void SetVisible(bool vis) { m_bShowMe = vis; }
    IC bool GetVisible() const { return m_bShowMe; }
    IC void SetAlignment(EWindowAlignment al) { m_alignment = al; };
    IC EWindowAlignment GetAlignment() const { return m_alignment; };
    IC Frect GetWndRect() const
    {
        Frect r;
        GetWndRect(r);
        return r;
    }
    IC void GetWndRect(Frect& res) const
    {
        switch (m_alignment)
        {
        case waNone: res.set(m_wndPos.x, m_wndPos.y, m_wndPos.x + m_wndSize.x, m_wndPos.y + m_wndSize.y); break;
        case waCenter:
        {
            float half_w = m_wndSize.x / 2.0f;
            float half_h = m_wndSize.y / 2.0f;
            res.set(m_wndPos.x - half_w, m_wndPos.y - half_h, m_wndPos.x + half_w, m_wndPos.y + half_h);
        }
        break;
        default: NODEFAULT;
        };
    }
    void MoveWndDelta(float dx, float dy)
    {
        m_wndPos.x += dx;
        m_wndPos.y += dy;
    }
    void MoveWndDelta(const Fvector2& d) { MoveWndDelta(d.x, d.y); };
protected:
    bool m_bShowMe;
    Fvector2 m_wndPos;
    Fvector2 m_wndSize;
    EWindowAlignment m_alignment;
};

class CUISelectable
{
protected:
    bool m_bSelected;

public:
    CUISelectable() : m_bSelected(false) {}
    bool GetSelected() const { return m_bSelected; }
    virtual void SetSelected(bool b) { m_bSelected = b; };
};
