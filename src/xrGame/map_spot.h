#pragma once

#include "xrUICore/Static/UIStatic.h"
#include "alife_space.h"

class CMapLocation;
class CUIXml;

class CMapSpot : public CUIStatic
{
private:
    typedef CUIStatic inherited;
    CMapLocation* m_map_location;
    int m_location_level;

    CUIStatic* m_border_static;

    bool m_mark_focused;

public:
    bool m_bScale;
    Fvector2 m_scale_bounds;

    Fvector2 m_originSize;

public:
    CMapSpot(CMapLocation*);
    virtual ~CMapSpot();
    virtual void Load(CUIXml* xml, LPCSTR path);
    CMapLocation* MapLocation() { return m_map_location; }
    int get_location_level() { return m_location_level; }
    virtual LPCSTR GetHint();
    virtual void SetWndPos(const Fvector2& pos);
    virtual void Update();
    virtual bool OnMouseDown(int mouse_btn);
    virtual void OnFocusLost();

    void show_static_border(bool status);
    void mark_focused();
};

class CMapSpotPointer : public CMapSpot
{
    typedef CMapSpot inherited;
    xr_string m_pointer_hint;

public:
    CMapSpotPointer(CMapLocation*);
    virtual ~CMapSpotPointer();
    virtual LPCSTR GetHint();
};

class CMiniMapSpot : public CMapSpot
{
    typedef CMapSpot inherited;
    ui_shader m_icon_above, m_icon_normal, m_icon_below;
    Frect m_tex_rect_above, m_tex_rect_normal, m_tex_rect_below;

public:
    CMiniMapSpot(CMapLocation*);
    virtual ~CMiniMapSpot();
    virtual void Load(CUIXml* xml, LPCSTR path);
    virtual void Draw();
};

// -------------------------------------------------------------------------------------------------
class CUIStaticOrig : public CUIStatic
{
protected:
    typedef CUIStatic inherited;
    Fvector2 m_origin_pos;
    Fvector2 m_origin_size;

public:
    IC void SetWndPosOrigin(const Fvector2& pos) { m_origin_pos = pos; }
    IC void SetWndSizeOrigin(const Fvector2& size) { m_origin_size = size; }
    IC const Fvector2& GetWndPosOrigin() const { return m_origin_pos; }
    IC const Fvector2& GetWndSizeOrigin() const { return m_origin_size; }
    void InitWndOrigin();
    void ScaleOrigin(float k);

}; // class CUIStaticOrig

class CComplexMapSpot : public CMapSpot
{
    typedef CMapSpot inherited;

    CUIStaticOrig* m_left_icon;
    CUIStaticOrig* m_right_icon;
    CUIStaticOrig* m_top_icon;
    CUIStaticOrig* m_timer;

    bool m_infinity_time;
    int m_last_delay;
    ALife::_TIME_ID m_timer_finish;

public:
    CComplexMapSpot(CMapLocation*);
    virtual ~CComplexMapSpot();
    virtual void Load(CUIXml* xml, LPCSTR path);
    virtual void Update();
    virtual void SetWndSize(const Fvector2& size);

    void SetLeftTexture(LPCSTR texture_name)
    {
        VERIFY2(m_left_icon, texture_name);
        m_left_icon->InitTexture(texture_name);
    }
    void SetRightTexture(LPCSTR texture_name)
    {
        VERIFY2(m_right_icon, texture_name);
        m_right_icon->InitTexture(texture_name);
    }
    void SetTopTexture(LPCSTR texture_name)
    {
        VERIFY2(m_top_icon, texture_name);
        m_top_icon->InitTexture(texture_name);
    }
    void SetTimerFinish(ALife::_TIME_ID time); // ms game_time
    ALife::_TIME_ID GetTimerFinish() const { return m_timer_finish; }
protected:
    CUIStaticOrig* CreateStaticOrig(CUIXml& xml, LPCSTR ui_path);

}; // class CComplexMapSpot
