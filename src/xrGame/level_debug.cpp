#include "StdAfx.h"
#include "level_debug.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/GameFont.h"
#include "Level.h"

#ifdef DEBUG
#include "debug_renderer.h"
// Lain: added
#include "debug_text_tree.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "xrUICore/ui_base.h"
#endif

#ifdef DEBUG

// Lain: added text_tree
CLevelDebug::CLevelDebug() : m_p_texttree(new debug::text_tree()), m_texttree_offs(0) {}
CLevelDebug::~CLevelDebug()
{
    xr_delete(m_p_texttree);

    free_mem();
}

// Lain: added
void CLevelDebug::log_debug_info() { debug::log_text_tree(*m_p_texttree); }
void CLevelDebug::debug_info_up()
{
    if (m_texttree_offs)
    {
        m_texttree_offs--;
    }
}

void CLevelDebug::debug_info_down() { m_texttree_offs++; }
void CLevelDebug::draw_debug_text()
{
    int column_size = 1024 / 3;
    int y_start = 50;
    int x_start = 5;

    if (!smart_cast<CBaseMonster*>(Level().CurrentEntity()))
    {
        bool debug_actor_view = false;
        if (!ai_dbg::get_var("actor_view", debug_actor_view))
            debug_actor_view = false;

        debug::text_tree* actor_view = m_p_texttree->find_node("ActorView");
        if (debug_actor_view && actor_view)
        {
            debug::draw_text_tree(*actor_view, 2, x_start, y_start, m_texttree_offs, column_size, 80,
                color_xrgb(0, 255, 0), color_xrgb(255, 255, 0));
        }
        return;
    }

    if (m_p_texttree->find_node("General"))
    {
        debug::draw_text_tree(*m_p_texttree->find_node("General"), 2, x_start, y_start, m_texttree_offs, column_size,
            80, color_xrgb(0, 255, 0), color_xrgb(255, 255, 0));
    }

    if (m_p_texttree->find_node("Brain"))
    {
        debug::draw_text_tree(*m_p_texttree->find_node("Brain"), 2, x_start * 2 + column_size, y_start, m_texttree_offs,
            column_size, 80, color_xrgb(0, 255, 0), color_xrgb(255, 255, 0));
    }

    if (m_p_texttree->find_node("Controllers"))
    {
        debug::draw_text_tree(*m_p_texttree->find_node("Controllers"), 2, x_start * 3 + column_size * 2, y_start,
            m_texttree_offs, column_size, 80, color_xrgb(0, 255, 0), color_xrgb(255, 255, 0));
    }
}

debug::text_tree& CLevelDebug::get_text_tree() { return *m_p_texttree; }
CLevelDebug::CObjectInfo& CLevelDebug::object_info(IGameObject* obj, LPCSTR class_name)
{
    OBJECT_INFO_MAP_IT obj_it = m_objects_info.find(obj);
    if (obj_it != m_objects_info.end())
    {
        CLASS_INFO_MAP_IT class_it = obj_it->second.find(class_name);

        if (class_it != obj_it->second.end())
        {
            return (*(class_it->second));
        }
        else
        {
            CObjectInfo* new_info = new CObjectInfo();
            obj_it->second.insert(std::make_pair(class_name, new_info));
            return (*(new_info));
        }
    }
    else
    {
        CLASS_INFO_MAP temp_map;

        CObjectInfo* new_info = new CObjectInfo();
        temp_map.insert(std::make_pair(class_name, new_info));
        m_objects_info.insert(std::make_pair(obj, temp_map));

        return (*(new_info));
    }
}

CLevelDebug::CTextInfo& CLevelDebug::text(void* class_ptr, LPCSTR class_name)
{
    SKey key(class_ptr, class_name);

    TEXT_INFO_MAP_IT it = m_text_info.find(key);
    if (it != m_text_info.end())
    {
        return (*it->second);
    }
    else
    {
        CTextInfo* new_info = new CTextInfo();
        m_text_info.insert(std::make_pair(key, new_info));
        return (*(new_info));
    }
}

CLevelDebug::CLevelInfo& CLevelDebug::level_info(void* class_ptr, LPCSTR class_name)
{
    SKey key(class_ptr, class_name);

    LEVEL_INFO_MAP_IT it = m_level_info.find(key);
    if (it != m_level_info.end())
    {
        return (*it->second);
    }
    else
    {
        CLevelInfo* new_info = new CLevelInfo();
        m_level_info.insert(std::make_pair(key, new_info));
        return (*(new_info));
    }
}

void CLevelDebug::free_mem()
{
    // free object info
    for (OBJECT_INFO_MAP_IT it_obj = m_objects_info.begin(); it_obj != m_objects_info.end(); ++it_obj)
    {
        for (CLASS_INFO_MAP_IT it_class = it_obj->second.begin(); it_class != it_obj->second.end(); ++it_class)
        {
            xr_delete(it_class->second);
        }
    }

    // free text info
    for (TEXT_INFO_MAP_IT it = m_text_info.begin(); it != m_text_info.end(); ++it)
    {
        xr_delete(it->second);
    }

    // free text info
    for (LEVEL_INFO_MAP_IT it = m_level_info.begin(); it != m_level_info.end(); ++it)
    {
        xr_delete(it->second);
    }
}

void CLevelDebug::draw_object_info()
{
    // handle all of the objects
    for (OBJECT_INFO_MAP_IT it = m_objects_info.begin(); it != m_objects_info.end(); ++it)
    {
        // если объект невалидный - удалить информацию
        if (!it->first || it->first->getDestroy())
        {
            for (CLASS_INFO_MAP_IT it_class = it->second.begin(); it_class != it->second.end(); ++it_class)
            {
                xr_delete(it_class->second);
            }
            m_objects_info.erase(it);
            break;
        }

        Fmatrix res;
        res.mul(Device.mFullTransform, it->first->XFORM());

        Fvector4 v_res;

        float delta_height = 0.f;

        // handle all of the classes
        for (CLASS_INFO_MAP_IT class_it = it->second.begin(); class_it != it->second.end(); ++class_it)
        {
            // get up on 2 meters
            res.transform(v_res, class_it->second->get_shift_pos());

            // check if the object in sight
            if (v_res.z < 0 || v_res.w < 0)
                continue;
            if (v_res.x < -1.f || v_res.x > 1.f || v_res.y < -1.f || v_res.y > 1.f)
                continue;

            // get real (x,y)
            float x = (1.f + v_res.x) / 2.f * (Device.dwWidth);
            float y = (1.f - v_res.y) / 2.f * (Device.dwHeight) - delta_height;
            float start_y = y;

            // handle all of the text inside class
            class_it->second->draw_info(x, y);

            delta_height = start_y - y;
        }
    }
}

void CLevelDebug::draw_text()
{
    // handle all of the classes
    for (TEXT_INFO_MAP_IT it = m_text_info.begin(); it != m_text_info.end(); ++it)
    {
        it->second->draw_text();
    }
}

void CLevelDebug::draw_level_info()
{
    // handle all of the classes
    for (LEVEL_INFO_MAP_IT it = m_level_info.begin(); it != m_level_info.end(); ++it)
    {
        it->second->draw_info();
    }
}

//////////////////////////////////////////////////////////////////////////
// CObjectInfo
//////////////////////////////////////////////////////////////////////////

void CLevelDebug::CObjectInfo::add_item(LPCSTR text, u32 color, u32 id)
{
    inherited::add_item(SInfoItem(text, color, id));
}

struct DrawInfoPredicate
{
    float x;
    float y;
    float delta_height;

    DrawInfoPredicate(float coord_x, float coord_y, float delta_h)
    {
        x = coord_x;
        y = coord_y;
        delta_height = delta_h;
    }

    void operator()(const CLevelDebug::SInfoItem& s)
    {
        UI().Font().pFontMedium->SetAligment(CGameFont::alLeft);
        UI().Font().pFontMedium->SetColor(s.color);
        UI().Font().pFontMedium->OutSet(x, y -= delta_height);
        UI().Font().pFontMedium->OutNext(*(s.text));
    }
};

void CLevelDebug::CObjectInfo::draw_info(float x, float& y)
{
    DrawInfoPredicate pred(x, y, m_delta_height);
    process(pred);
    y = pred.y;
}

//////////////////////////////////////////////////////////////////////////
// CTextInfo
//////////////////////////////////////////////////////////////////////////

void CLevelDebug::CTextInfo::add_item(LPCSTR text, float x, float y, u32 color, u32 id)
{
    inherited::add_item(STextItem(text, x, y, color, id));
}

struct DrawTextPredicate
{
    void operator()(const CLevelDebug::STextItem& s)
    {
        UI().Font().pFontMedium->SetAligment(CGameFont::alLeft);
        UI().Font().pFontMedium->SetColor(s.color);
        UI().Font().pFontMedium->OutSet(s.x, s.y);
        UI().Font().pFontMedium->OutNext(*(s.text));
    }
};

void CLevelDebug::CTextInfo::draw_text()
{
    DrawTextPredicate pred;
    process(pred);
}

//////////////////////////////////////////////////////////////////////////
// CLevelInfo
//////////////////////////////////////////////////////////////////////////

void CLevelDebug::CLevelInfo::add_item(const Fvector& pos, u32 color, u32 id)
{
    inherited::add_item(SLevelItem(pos, color, id));
}

void CLevelDebug::CLevelInfo::add_item(const Fvector& pos1, const Fvector& pos2, u32 color, u32 id)
{
    inherited::add_item(SLevelItem(pos1, pos2, color, id));
}

void CLevelDebug::CLevelInfo::add_item(const Fvector& pos, float radius, u32 color, u32 id)
{
    inherited::add_item(SLevelItem(pos, radius, color, id));
}

struct DrawLevelPredicate
{
    void operator()(CLevelDebug::SLevelItem s)
    {
        if (s.ptype == CLevelDebug::SLevelItem::ePoint)
        {
            Level().debug_renderer().draw_aabb(s.position1, 0.35f, 0.35f, 0.35f, s.color);

            Fvector upV;
            upV = s.position1;
            upV.y += 5.0f;

            Level().debug_renderer().draw_line(Fidentity, s.position1, upV, s.color);
        }
        else if (s.ptype == CLevelDebug::SLevelItem::eLine)
        {
            Level().debug_renderer().draw_line(Fidentity, s.position1, s.position2, s.color);
        }
        else if (s.ptype == CLevelDebug::SLevelItem::eBox)
        {
            Level().debug_renderer().draw_aabb(s.position1, s.radius, s.radius, s.radius, s.color);
        }
    }
};

void CLevelDebug::CLevelInfo::draw_info()
{
    DrawLevelPredicate pred;
    process(pred);
}

void CLevelDebug::on_destroy_object(IGameObject* obj)
{
    // handle all of the objects
    for (OBJECT_INFO_MAP_IT it = m_objects_info.begin(); it != m_objects_info.end(); ++it)
    {
        // если объект невалидный - удалить информацию
        if (it->first == obj)
        {
            for (CLASS_INFO_MAP_IT it_class = it->second.begin(); it_class != it->second.end(); ++it_class)
            {
                xr_delete(it_class->second);
            }
            m_objects_info.erase(it);
            break;
        }
    }
}

#endif
