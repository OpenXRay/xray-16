#include "stdafx.h"
#include "map_spot.h"
#include "map_location.h"

#include "ui/UIXmlInit.h"
#include "ui/UIMApWnd.h"
#include "Level.h"
#include "ui/UIInventoryUtilities.h"
#include "xrEngine/xr_object.h"
#include "Common/object_broker.h"
#include "ui/UITextureMaster.h"
#include "ui/UIHelper.h"

#include "Include/xrRender/UIShader.h"
#include "gametaskmanager.h"
#include "gametask.h"

CMapSpot::CMapSpot(CMapLocation* ml) : m_map_location(ml), m_mark_focused(false)
{
    m_bScale = false;
    m_location_level = 0;
    m_border_static = nullptr;
    m_scale_bounds.set(-1.0f, -1.0f);
}

CMapSpot::~CMapSpot() {}
void CMapSpot::Load(CUIXml* xml, LPCSTR path)
{
    CUIXmlInit::InitStatic(*xml, path, 0, this);
    if (!Heading())
    {
        SetWidth(GetWidth() * UI().get_current_kx());
        SetStretchTexture(true);
    }

    int i = xml->ReadAttribInt(path, 0, "scale", 0);
    m_bScale = (i == 1);
    m_scale_bounds.x = xml->ReadAttribFlt(path, 0, "scale_min", -1.0f);
    if (m_bScale)
    {
        m_scale_bounds.y = xml->ReadAttribFlt(path, 0, "scale_max", -1.0f);
        R_ASSERT2(m_scale_bounds.x > 0 && m_scale_bounds.y > 0, path);
    }
    m_location_level = xml->ReadAttribInt(path, 0, "location_level", 0);

    m_originSize = GetWndSize();

    string512 str;
    strconcat(sizeof(str), str, path, ":static_border");
    if (xml->NavigateToNode(str))
    {
        m_border_static = UIHelper::CreateStatic(*xml, str, this);
        m_border_static->Show(false);
        if (!Heading())
        {
            m_border_static->SetWidth(m_border_static->GetWidth() * UI().get_current_kx());
            m_border_static->SetStretchTexture(true);
        }
    }
    m_mark_focused = false;
}

LPCSTR CMapSpot::GetHint() { return MapLocation()->GetHint(); };
void CMapSpot::SetWndPos(const Fvector2& pos) { inherited::SetWndPos(pos); }
void CMapSpot::Update()
{
    inherited::Update();
    if (m_bCursorOverWindow)
    {
        if (Device.dwTimeGlobal > (m_dwFocusReceiveTime + 500))
        {
            GetMessageTarget()->SendMessage(this, MAP_SHOW_HINT, NULL);
        }
    }
}

bool CMapSpot::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1)
    {
        CGameTask* t = Level().GameTaskManager().HasGameTask(m_map_location, true);
        if (t)
        {
            GetMessageTarget()->SendMessage(this, MAP_SELECT_SPOT);
            return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

void CMapSpot::OnFocusLost()
{
    inherited::OnFocusLost();
    GetMessageTarget()->SendMessage(this, MAP_HIDE_HINT, NULL);
}

void CMapSpot::show_static_border(bool status)
{
    if (m_border_static)
    {
        m_border_static->Show(status);
    }
}

void CMapSpot::mark_focused() { m_mark_focused = true; }
// -------------------------------------------------------------------------------------------------

CMapSpotPointer::CMapSpotPointer(CMapLocation* ml) : inherited(ml) {}
CMapSpotPointer::~CMapSpotPointer() {}
LPCSTR CMapSpotPointer::GetHint() { return NULL; }
//////////////////////////////////////////////////
CMiniMapSpot::CMiniMapSpot(CMapLocation* ml) : inherited(ml) {}
CMiniMapSpot::~CMiniMapSpot() {}
void CMiniMapSpot::Load(CUIXml* xml, LPCSTR path)
{
    inherited::Load(xml, path);

    string256 buf;

    Frect base_rect;
    base_rect.x1 = 0;
    base_rect.y1 = 0;
    base_rect.x2 = xml->ReadAttribFlt(path, 0, "width", 0);
    base_rect.y2 = xml->ReadAttribFlt(path, 0, "height", 0);

    Frect _stored_rect = m_UIStaticItem.GetTextureRect();

    strconcat(sizeof(buf), buf, path, ":texture_above");
    XML_NODE n = xml->NavigateToNode(buf, 0);
    if (n)
    {
        LPCSTR texture = xml->Read(buf, 0, NULL);
        CUITextureMaster::InitTexture(texture, &m_UIStaticItem);
        if (strchr(texture, '\\'))
        {
            float x = xml->ReadAttribFlt(buf, 0, "x", base_rect.x1);
            float y = xml->ReadAttribFlt(buf, 0, "y", base_rect.y1);
            float width = xml->ReadAttribFlt(buf, 0, "width", base_rect.width());
            float height = xml->ReadAttribFlt(buf, 0, "height", base_rect.height());
            m_tex_rect_above.set(x, y, x + width, y + height);
        }
        else
            m_tex_rect_above = m_UIStaticItem.GetTextureRect();

        m_icon_above = m_UIStaticItem.GetShader();
    }

    strconcat(sizeof(buf), buf, path, ":texture_below");
    n = xml->NavigateToNode(buf, 0);
    if (n)
    {
        LPCSTR texture = xml->Read(buf, 0, NULL);
        CUITextureMaster::InitTexture(texture, &m_UIStaticItem);
        if (strchr(texture, '\\'))
        {
            float x = xml->ReadAttribFlt(buf, 0, "x", base_rect.x1);
            float y = xml->ReadAttribFlt(buf, 0, "y", base_rect.y1);
            float width = xml->ReadAttribFlt(buf, 0, "width", base_rect.width());
            float height = xml->ReadAttribFlt(buf, 0, "height", base_rect.height());
            m_tex_rect_below.set(x, y, x + width, y + height);
        }
        else
            m_tex_rect_below = m_UIStaticItem.GetTextureRect();

        m_icon_below = m_UIStaticItem.GetShader();
    }
    strconcat(sizeof(buf), buf, path, ":texture");
    n = xml->NavigateToNode(buf, 0);
    if (n)
    {
        LPCSTR texture = xml->Read(buf, 0, NULL);
        CUITextureMaster::InitTexture(texture, &m_UIStaticItem);
        if (strchr(texture, '\\'))
        {
            float x = xml->ReadAttribFlt(buf, 0, "x", base_rect.x1);
            float y = xml->ReadAttribFlt(buf, 0, "y", base_rect.y1);
            float width = xml->ReadAttribFlt(buf, 0, "width", base_rect.width());
            float height = xml->ReadAttribFlt(buf, 0, "height", base_rect.height());
            m_tex_rect_normal.set(x, y, x + width, y + height);
        }
        else
            m_tex_rect_normal = m_UIStaticItem.GetTextureRect();

        m_icon_normal = m_UIStaticItem.GetShader();
    }

    m_UIStaticItem.SetTextureRect(_stored_rect);
}

void CMiniMapSpot::Draw()
{
    IGameObject* O = Level().CurrentViewEntity();
    if (O && m_icon_above->inited() && m_icon_below->inited())
    {
        float ml_y = MapLocation()->GetLastPosition().y;
        float d = O->Position().y - ml_y;

        if (d > 1.8f)
        {
            GetUIStaticItem().SetShader(m_icon_below);
            GetUIStaticItem().SetTextureRect(m_tex_rect_below);
        }
        else if (d < -1.8f)
        {
            GetUIStaticItem().SetShader(m_icon_above);
            GetUIStaticItem().SetTextureRect(m_tex_rect_above);
        }
        else
        {
            GetUIStaticItem().SetShader(m_icon_normal);
            GetUIStaticItem().SetTextureRect(m_tex_rect_normal);
        }
    };

    inherited::Draw();
}

/// ---------------------------------------------------------------------------------------------
void CUIStaticOrig::InitWndOrigin()
{
    m_origin_pos = GetWndPos();
    m_origin_size = GetWndSize();
}

void CUIStaticOrig::ScaleOrigin(float k)
{
    SetWndPos(Fvector2().set(m_origin_pos.x * k, m_origin_pos.y * k));
    SetWndSize(Fvector2().set(m_origin_size.x * k, m_origin_size.y * k));
}

CComplexMapSpot::CComplexMapSpot(CMapLocation* ml) : inherited(ml)
{
    m_infinity_time = false;
    m_last_delay = 0;
    m_timer_finish = 0;
    m_left_icon = NULL;
    m_right_icon = NULL;
    m_top_icon = NULL;
    m_timer = NULL;
}

CComplexMapSpot::~CComplexMapSpot() {}
CUIStaticOrig* CComplexMapSpot::CreateStaticOrig(CUIXml& xml, LPCSTR ui_path)
{
    CUIStaticOrig* ui = new CUIStaticOrig();
    AttachChild(ui);
    ui->SetAutoDelete(true);
    CUIXmlInit::InitStatic(xml, ui_path, 0, ui);
    ui->InitWndOrigin();
    return ui;
}

void CComplexMapSpot::Load(CUIXml* xml, LPCSTR path) // complex_spot_template
{
    inherited::Load(xml, path);

    XML_NODE stored_root = xml->GetLocalRoot();
    XML_NODE node = xml->NavigateToNode(path, 0);
    xml->SetLocalRoot(node);

    m_left_icon = CreateStaticOrig(*xml, "left_icon");
    m_right_icon = CreateStaticOrig(*xml, "right_icon");
    m_top_icon = CreateStaticOrig(*xml, "top_icon");
    m_timer = CreateStaticOrig(*xml, "timer");

    xml->SetLocalRoot(stored_root);
}

void CComplexMapSpot::SetTimerFinish(ALife::_TIME_ID time) // ms
{
    if (time <= 0)
    {
        m_timer_finish = 0;
        m_infinity_time = true;
        m_timer->Show(false);
    }
    else
    {
        m_timer_finish = time;
        m_infinity_time = false;
        m_timer->Show(true);
    }
}

void CComplexMapSpot::Update()
{
    inherited::Update();

    m_last_delay += Device.dwTimeDelta;
    if (m_last_delay > 310)
    {
        m_last_delay = 0;
        if (Level().GetGameTime() > m_timer_finish)
        {
            /*if ( !m_infinity_time )
            {
                MapLocation()->DisableSpot();
            }*/
        }
        else
        {
            if (!m_infinity_time)
            {
                ALife::_TIME_ID dt = m_timer_finish - Level().GetGameTime();
                m_timer->TextItemControl()->SetText(
                    GetTimeAsString(dt, InventoryUtilities::etpTimeToMinutes, ':', false).c_str());
            }
        }
    }

    if (MapLocation()->SpotEnabled())
    {
        m_timer->Show((m_timer->GetWndSize().x > 5.0f) && (Level().GetGameTime() < m_timer_finish));
    }
}

void CComplexMapSpot::SetWndSize(const Fvector2& size)
{
    inherited::SetWndSize(size);

    if (m_originSize.x == 0.0f || m_originSize.y == 0.0f)
    {
        return;
    }
    float k = size.x / m_originSize.x;

    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
    {
        CUIStaticOrig* static_orig = smart_cast<CUIStaticOrig*>(*it);
        if (static_orig)
        {
            static_orig->ScaleOrigin(k);
        }
    }

    if (m_timer)
    {
        float xp = 0.5f * (GetWndSize().x - m_timer->GetWndSize().x);
        m_timer->SetWndPos(Fvector2().set(xp, m_timer->GetWndPos().y));
    }
}
