#include "StdAfx.h"
#include "map_location.h"
#include "map_spot.h"
#include "map_manager.h"

#include "Level.h"
#include "xrEngine/xr_object.h"
#include "ai_space.h"
#include "xrAICore/Navigation/game_graph.h"
#include "xrServer.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ui/UIXmlInit.h"
#include "ui/UIMap.h"
#include "alife_simulator.h"
#include "xrAICore/Navigation/graph_engine.h"
#include "Actor.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "alife_object_registry.h"
#include "relation_registry.h"
#include "InventoryOwner.h"
#include "Common/object_broker.h"
#include "string_table.h"
#include "level_changer.h"
#include "actor_memory.h"
#include "visual_memory_manager.h"
#include "location_manager.h"
#include "GameTask.h"
#include "GametaskManager.h"
#include "ActorHelmet.h"
#include "Inventory.h"

static class CUISpotXmlManager : public CUIResetNotifier, public pureAppEnd
{
    CUIXml m_xml;
    bool m_loaded{};

public:
    void Load()
    {
        if (m_loaded)
            return;
        m_loaded = m_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "map_spots.xml");
    }

    void OnAppEnd() override
    {
        m_xml.ClearInternal();
        m_loaded = false;
    }

    void OnUIReset() override
    {
        OnAppEnd();
        if (g_pGameLevel)
            Load();
    }

    operator CUIXml*()
    {
        return &m_xml;
    }

    CUIXml* operator->()
    {
        return &m_xml;
    }
} g_uiSpotXml;

CMapLocation::CMapLocation(LPCSTR type, u16 object_id)
{
    m_flags.zero();

    m_level_spot = NULL;
    m_level_spot_pointer = NULL;
    m_minimap_spot = NULL;
    m_minimap_spot_pointer = NULL;
    m_complex_spot = NULL;
    m_complex_spot_pointer = NULL;

    m_level_map_spot_border = NULL;
    m_mini_map_spot_border = NULL;
    m_complex_spot_border = NULL;

    m_level_map_spot_border_na = NULL;
    m_mini_map_spot_border_na = NULL;
    m_complex_spot_border_na = NULL;

    m_objectID = object_id;
    m_actual_time = 0;
    m_owner_se_object = (ai().get_alife()) ? ai().alife().objects().object(m_objectID, true) : NULL;
    m_flags.set(eHintEnabled, TRUE);
    LoadSpot(type);

    DisablePointer();

    EnableSpot();
    m_cached.m_Position.set(10000, 10000);
    m_cached.m_updatedFrame = u32(-1);
    m_cached.m_graphID = GameGraph::_GRAPH_ID(-1);
    if (!IsGameTypeSingle())
        m_cached.m_LevelName = Level().name();
}

CMapLocation::~CMapLocation() {}
void CMapLocation::destroy()
{
    delete_data(m_level_spot);
    delete_data(m_level_spot_pointer);
    delete_data(m_minimap_spot);
    delete_data(m_minimap_spot_pointer);
    delete_data(m_complex_spot);
    delete_data(m_complex_spot_pointer);

    delete_data(m_level_map_spot_border);
    delete_data(m_mini_map_spot_border);
    delete_data(m_complex_spot_border);

    delete_data(m_level_map_spot_border_na);
    delete_data(m_mini_map_spot_border_na);
    delete_data(m_complex_spot_border_na);
}

void CMapLocation::LoadSpot(LPCSTR type)
{
    g_uiSpotXml.Load();

    string512 path_base, path;
    xr_strcpy(path_base, type);
    R_ASSERT3(g_uiSpotXml->NavigateToNode(path_base, 0), "XML node not found in file map_spots.xml", path_base);
    LPCSTR s = g_uiSpotXml->ReadAttrib(path_base, 0, "hint", "no hint");
    SetHint(s);

    s = g_uiSpotXml->ReadAttrib(path_base, 0, "store", NULL);
    if (s)
    {
        m_flags.set(eSerailizable, TRUE);
    }

    s = g_uiSpotXml->ReadAttrib(path_base, 0, "no_offline", NULL);
    if (s)
    {
        m_flags.set(eHideInOffline, TRUE);
    }

    m_ttl = g_uiSpotXml->ReadAttribInt(path_base, 0, "ttl", 0);
    if (m_ttl > 0)
    {
        m_flags.set(eTTL, TRUE);
        UpdateTTL();
    }

    s = g_uiSpotXml->ReadAttrib(path_base, 0, "pos_to_actor", NULL);
    if (s)
    {
        m_flags.set(ePosToActor, TRUE);
    }

    strconcat(sizeof(path), path, path_base, ":level_map");
    XML_NODE node = g_uiSpotXml->NavigateToNode(path, 0);
    if (node)
    {
        LPCSTR str = g_uiSpotXml->ReadAttrib(path, 0, "spot", "");
        if (xr_strlen(str))
        {
            if (!m_level_spot)
            {
                m_level_spot = xr_new<CMapSpot>(this);
            }
            m_level_spot->Load(g_uiSpotXml, str);
        }
        else
        {
            xr_delete(m_level_spot);
        }

        m_spot_border_names[0] = g_uiSpotXml->ReadAttrib(path, 0, "spot_a", "level_map_spot_border");
        m_spot_border_names[1] = g_uiSpotXml->ReadAttrib(path, 0, "spot_na", "");

        str = g_uiSpotXml->ReadAttrib(path, 0, "pointer", "");
        if (xr_strlen(str))
        {
            if (!m_level_spot_pointer)
            {
                m_level_spot_pointer = xr_new<CMapSpotPointer>(this);
            }
            m_level_spot_pointer->Load(g_uiSpotXml, str);
        }
        else
        {
            xr_delete(m_level_spot_pointer);
        }
    }
    else
    {
        xr_delete(m_level_spot);
        xr_delete(m_level_spot_pointer);
    }

    strconcat(sizeof(path), path, path_base, ":mini_map");
    node = g_uiSpotXml->NavigateToNode(path, 0);
    if (node)
    {
        LPCSTR str = g_uiSpotXml->ReadAttrib(path, 0, "spot", "");
        if (xr_strlen(str))
        {
            if (!m_minimap_spot)
            {
                m_minimap_spot = xr_new<CMiniMapSpot>(this);
            }
            m_minimap_spot->Load(g_uiSpotXml, str);
        }
        else
        {
            xr_delete(m_minimap_spot);
        }
        m_spot_border_names[2] = g_uiSpotXml->ReadAttrib(path, 0, "spot_a", "mini_map_spot_border");
        m_spot_border_names[3] = g_uiSpotXml->ReadAttrib(path, 0, "spot_na", "");

        str = g_uiSpotXml->ReadAttrib(path, 0, "pointer", "");
        if (xr_strlen(str))
        {
            if (!m_minimap_spot_pointer)
            {
                m_minimap_spot_pointer = xr_new<CMapSpotPointer>(this);
            }
            m_minimap_spot_pointer->Load(g_uiSpotXml, str);
        }
        else
        {
            xr_delete(m_minimap_spot_pointer);
        }
    }
    else
    {
        xr_delete(m_minimap_spot);
        xr_delete(m_minimap_spot_pointer);
    }

    strconcat(sizeof(path), path, path_base, ":complex_spot");
    node = g_uiSpotXml->NavigateToNode(path, 0);
    if (node)
    {
        LPCSTR str = g_uiSpotXml->ReadAttrib(path, 0, "spot", "");
        if (xr_strlen(str))
        {
            if (!m_complex_spot)
            {
                m_complex_spot = xr_new<CComplexMapSpot>(this);
            }
            m_complex_spot->Load(g_uiSpotXml, str);
        }
        else
        {
            xr_delete(m_complex_spot);
        }
        m_spot_border_names[4] = g_uiSpotXml->ReadAttrib(path, 0, "spot_a", "complex_map_spot_border");
        m_spot_border_names[5] = g_uiSpotXml->ReadAttrib(path, 0, "spot_na", "");

        str = g_uiSpotXml->ReadAttrib(path, 0, "pointer", "");
        if (xr_strlen(str))
        {
            if (!m_complex_spot_pointer)
            {
                m_complex_spot_pointer = xr_new<CMapSpotPointer>(this);
            }
            m_complex_spot_pointer->Load(g_uiSpotXml, str);
        }
        else
        {
            xr_delete(m_complex_spot_pointer);
        }
    }
    else
    {
        xr_delete(m_complex_spot);
        xr_delete(m_complex_spot_pointer);
    }

    if (m_minimap_spot == NULL && m_level_spot == NULL && m_complex_spot == NULL)
    {
        DisableSpot();
    }
}

void CMapLocation::CalcPosition()
{
    if (m_flags.test(ePosToActor) && Level().CurrentEntity())
    {
        m_position_global = Level().CurrentEntity()->Position();
        m_cached.m_Position.set(m_position_global.x, m_position_global.z);
        return;
    }

    IGameObject* pObject = Level().Objects.net_Find(m_objectID);
    if (!pObject)
    {
        if (m_owner_se_object)
        {
            m_position_global = m_owner_se_object->draw_level_position();
            m_cached.m_Position.set(m_position_global.x, m_position_global.z);
        }
    }
    else
    {
        m_position_global = pObject->Position();
        m_cached.m_Position.set(m_position_global.x, m_position_global.z);
    }
}

const Fvector2& CMapLocation::CalcDirection()
{
    if (Level().CurrentViewEntity() && Level().CurrentViewEntity()->ID() == m_objectID)
    {
        m_cached.m_Direction.set(Device.vCameraDirection.x, Device.vCameraDirection.z);
    }
    else
    {
        IGameObject* pObject = Level().Objects.net_Find(m_objectID);
        if (!pObject)
            m_cached.m_Direction.set(0.0f, 0.0f);
        else
        {
            const Fvector& op = pObject->Direction();
            m_cached.m_Direction.set(op.x, op.z);
        }
    }

    if (m_flags.test(ePosToActor))
    {
        IGameObject* pObject = Level().Objects.net_Find(m_objectID);
        if (pObject)
        {
            Fvector2 dcp, obj_pos;
            dcp.set(Device.vCameraPosition.x, Device.vCameraPosition.z);
            obj_pos.set(pObject->Position().x, pObject->Position().z);
            m_cached.m_Direction.sub(obj_pos, dcp);
            m_cached.m_Direction.normalize_safe();
        }
    }
    return m_cached.m_Direction;
}

void CMapLocation::CalcLevelName()
{
    if (m_owner_se_object && ai().get_game_graph())
    {
        if (m_cached.m_graphID != m_owner_se_object->m_tGraphID)
        {
            m_cached.m_LevelName = ai().game_graph()
                                       .header()
                                       .level(ai().game_graph().vertex(m_owner_se_object->m_tGraphID)->level_id())
                                       .name();
            m_cached.m_graphID = m_owner_se_object->m_tGraphID;
        }
    }
    else
    {
        m_cached.m_LevelName = Level().name();
    }
}

bool CMapLocation::Update() // returns actual
{
    R_ASSERT(m_cached.m_updatedFrame != Device.dwFrame);

    if (m_flags.test(eTTL))
    {
        if (m_actual_time < Device.dwTimeGlobal)
        {
            m_cached.m_Actuality = false;
            m_cached.m_updatedFrame = Device.dwFrame;
            return m_cached.m_Actuality;
        }
    }

    IGameObject* pObject = Level().Objects.net_Find(m_objectID);

    if (m_owner_se_object || (!IsGameTypeSingle() && pObject))
    {
        m_cached.m_Actuality = true;
        if (IsGameTypeSingle())
            CalcLevelName();

        CalcPosition();
    }
    else
        m_cached.m_Actuality = false;

    m_cached.m_updatedFrame = Device.dwFrame;
    return m_cached.m_Actuality;
}

void CMapLocation::UpdateTTL()
{
    if (!m_flags.test(eTTL))
        return;
    m_actual_time = Device.dwTimeGlobal + m_ttl * 1000;
}

extern xr_vector<CLevelChanger*> g_lchangers;
xr_vector<u32> map_point_path;

void CMapLocation::UpdateSpot(CUICustomMap* map, CMapSpot* sp)
{
    if (map->MapName() == GetLevelName())
    {
        bool b_alife = !!ai().get_alife();

        if (b_alife && m_flags.test(eHideInOffline) && !m_owner_se_object->m_bOnline)
        {
            return;
        }

        if (b_alife && m_owner_se_object->m_flags.test(CSE_ALifeObject::flVisibleForMap) == FALSE)
        {
            return;
        }

        if (IsGameTypeSingle())
        {
            CGameTask* ml_task = Level().GameTaskManager().HasGameTask(this, true);
            if (ml_task)
            {
                CGameTask* storyTask = Level().GameTaskManager().ActiveTask(eTaskTypeStoryline);
                CGameTask* additionalTask = Level().GameTaskManager().ActiveTask(eTaskTypeAdditional);
                const bool border_show = ml_task == storyTask || ml_task == additionalTask;
                if (m_minimap_spot)
                {
                    m_minimap_spot->show_static_border(border_show);
                }
                if (m_level_spot)
                {
                    m_level_spot->show_static_border(border_show);
                }
                if (m_complex_spot)
                {
                    m_complex_spot->show_static_border(border_show);
                }
            }
        }

        // update spot position
        Fvector2 position = GetPosition();

        m_position_on_map =
            map->ConvertRealToLocal(position, (map->Heading()) ? false : true); // for visibility calculating

        sp->SetWndPos(m_position_on_map);

        const Frect& wnd_rect = sp->GetWndRect();

        if (map->IsRectVisible(wnd_rect))
        {
            // update heading if needed
            if (sp->Heading() && !sp->GetConstHeading())
            {
                Fvector2 dir_global = CalcDirection();
                float h = dir_global.getH();
                float h_ = map->GetHeading() + h;
                sp->SetHeading(h_);
            }
            map->AttachChild(sp);
        }

        if (IsGameTypeSingle())
        {
            CMapSpot* s = GetSpotBorder(sp);
            if (s)
            {
                s->SetWndPos(sp->GetWndPos());
                map->AttachChild(s);
            }
        }

        bool b_pointer = (GetSpotPointer(sp) && map->NeedShowPointer(wnd_rect));

        if (map->Heading())
        {
            m_position_on_map = map->ConvertRealToLocal(position, true); // for drawing
            sp->SetWndPos(m_position_on_map);
        }

        if (b_pointer)
            UpdateSpotPointer(map, GetSpotPointer(sp));
    }
    else if (Level().name() == map->MapName() && GetSpotPointer(sp))
    {
        GameGraph::_GRAPH_ID dest_graph_id;

        dest_graph_id = m_owner_se_object->m_tGraphID;

        map_point_path.clear();

        VERIFY(Actor());
        GraphEngineSpace::CGameVertexParams params(Actor()->locations().vertex_types(), flt_max);
        bool res = ai().graph_engine().search(
            ai().game_graph(), Actor()->ai_location().game_vertex_id(), dest_graph_id, &map_point_path, params);

        if (res)
        {
            xr_vector<u32>::reverse_iterator it = map_point_path.rbegin();
            xr_vector<u32>::reverse_iterator it_e = map_point_path.rend();

            xr_vector<CLevelChanger*>::iterator lit = g_lchangers.begin();
            // xr_vector<CLevelChanger*>::iterator lit_e = g_lchangers.end();
            bool bDone = false;
            // for(; (it!=it_e)&&(!bDone) ;++it){
            //	for(lit=g_lchangers.begin();lit!=lit_e; ++lit){

            //		if((*it)==(*lit)->ai_location().game_vertex_id() )
            //		{
            //			bDone = true;
            //			break;
            //		}

            //	}
            //}
            static bool bbb = false;
            if (!bDone && bbb)
            {
                Msg("! Error. Path from actor to selected map spot does not contain level changer :(");
                Msg("Path:");
                xr_vector<u32>::iterator it = map_point_path.begin();
                xr_vector<u32>::iterator it_e = map_point_path.end();
                for (; it != it_e; ++it)
                {
                    //					Msg("%d-%s",(*it),ai().game_graph().vertex(*it));
                    Msg("[%d] level[%s]", (*it),
                        *ai().game_graph().header().level(ai().game_graph().vertex(*it)->level_id()).name());
                }
                Msg("- Available LevelChangers:");
                xr_vector<CLevelChanger *>::iterator lit, lit_e;
                lit_e = g_lchangers.end();
                for (lit = g_lchangers.begin(); lit != lit_e; ++lit)
                {
                    GameGraph::_GRAPH_ID gid = (*lit)->ai_location().game_vertex_id();
                    Msg("[%d]", gid);
                    Fvector p = ai().game_graph().vertex(gid)->level_point();
                    Msg("lch_name=%s pos=%f %f %f",
                        *ai().game_graph().header().level(ai().game_graph().vertex(gid)->level_id()).name(), p.x, p.y,
                        p.z);
                }
            };
            if (bDone)
            {
                Fvector2 position;
                position.set((*lit)->Position().x, (*lit)->Position().z);
                m_position_on_map = map->ConvertRealToLocal(position, false);
                UpdateSpotPointer(map, GetSpotPointer(sp));
            }
            else
            {
                xr_vector<u32>::reverse_iterator it = map_point_path.rbegin();
                xr_vector<u32>::reverse_iterator it_e = map_point_path.rend();
                for (; (it != it_e) && (!bDone); ++it)
                {
                    if (*ai().game_graph().header().level(ai().game_graph().vertex(*it)->level_id()).name() ==
                        Level().name())
                        break;
                }
                if (it != it_e)
                {
                    Fvector p = ai().game_graph().vertex(*it)->level_point();
                    if (Actor()->Position().distance_to_sqr(p) > 45.0f * 45.0f)
                    {
                        Fvector2 position;
                        position.set(p.x, p.z);
                        m_position_on_map = map->ConvertRealToLocal(position, false);
                        UpdateSpotPointer(map, GetSpotPointer(sp));
                    }
                }
            }
        }
    }
}

void CMapLocation::UpdateSpotPointer(CUICustomMap* map, CMapSpotPointer* sp)
{
    if (sp->GetParent())
        return; // already is child
    float heading;
    Fvector2 pointer_pos;
    if (map->GetPointerTo(m_position_on_map, sp->GetWidth() / 2, pointer_pos, heading))
    {
        sp->SetWndPos(pointer_pos);
        sp->SetHeading(heading);

        map->AttachChild(sp);

        Fvector2 tt = map->ConvertLocalToReal(m_position_on_map, map->BoundRect());
        Fvector ttt;
        ttt.set(tt.x, 0.0f, tt.y);

        if (IsGameTypeSingle())
        {
            float dist_to_target = Level().CurrentEntity()->Position().distance_to(ttt);
            CGameTask* task = Level().GameTaskManager().HasGameTask(this, true);
            if (task)
            {
                map->SetPointerDistance(dist_to_target);
            }

            u32 clr = sp->GetTextureColor();
            u32 a = 0xff;
            if (dist_to_target >= 0.0f && dist_to_target < 10.0f)
                a = 255;
            else if (dist_to_target >= 10.0f && dist_to_target < 50.0f)
                a = 200;
            else if (dist_to_target >= 50.0f && dist_to_target < 100.0f)
                a = 150;
            else
                a = 100;

            sp->SetTextureColor(subst_alpha(clr, a));
        }
    }
}

void CMapLocation::UpdateMiniMap(CUICustomMap* map)
{
    CMapSpot* sp = m_minimap_spot;
    if (!sp)
        return;
    if (SpotEnabled())
        UpdateSpot(map, sp);
}

void CMapLocation::UpdateLevelMap(CUICustomMap* map)
{
    CComplexMapSpot* csp = m_complex_spot;
    if (csp && SpotEnabled())
    {
        UpdateSpot(map, csp);
        return;
    }

    CMapSpot* sp = m_level_spot;
    if (sp && SpotEnabled())
    {
        UpdateSpot(map, sp);
    }
}

void CMapLocation::save(IWriter& stream)
{
    stream.w_stringZ(m_hint);
    stream.w_u32(m_flags.flags);
    stream.w_stringZ(m_owner_task_id);
}

void CMapLocation::load(IReader& stream)
{
    xr_string str;
    stream.r_stringZ(str);
    SetHint(str.c_str());
    m_flags.flags = stream.r_u32();

    stream.r_stringZ(str);
    m_owner_task_id = str.c_str();
}

void CMapLocation::SetHint(const shared_str& hint)
{
    if (hint == "disable_hint")
    {
        m_flags.set(eHintEnabled, FALSE);
        m_hint = "";
        return;
    }
    m_hint = hint;
};

LPCSTR CMapLocation::GetHint()
{
    if (!HintEnabled())
    {
        return NULL;
    }
    return StringTable().translate(m_hint).c_str();
};

CMapSpotPointer* CMapLocation::GetSpotPointer(CMapSpot* sp)
{
    R_ASSERT(sp);
    if (!PointerEnabled())
    {
        return NULL;
    }
    if (sp == m_level_spot)
    {
        return m_level_spot_pointer;
    }
    else if (sp == m_minimap_spot)
    {
        return m_minimap_spot_pointer;
    }
    else if (sp == m_complex_spot)
    {
        return m_complex_spot_pointer;
    }

    return NULL;
}

CMapSpot* CMapLocation::GetSpotBorder(CMapSpot* sp)
{
    R_ASSERT(sp);
    if (PointerEnabled())
    {
        if (sp == m_level_spot)
        {
            if (NULL == m_level_map_spot_border)
            {
                m_level_map_spot_border = xr_new<CMapSpot>(this);
                m_level_map_spot_border->Load(g_uiSpotXml, m_spot_border_names[0].c_str());
            }
            return m_level_map_spot_border;
        }
        else if (sp == m_minimap_spot)
        {
            if (NULL == m_mini_map_spot_border)
            {
                m_mini_map_spot_border = xr_new<CMapSpot>(this);
                m_mini_map_spot_border->Load(g_uiSpotXml, m_spot_border_names[2].c_str());
            }
            return m_mini_map_spot_border;
        }
        else if (sp == m_complex_spot)
        {
            if (NULL == m_complex_spot_border)
            {
                m_complex_spot_border = xr_new<CMapSpot>(this);
                m_complex_spot_border->Load(g_uiSpotXml, m_spot_border_names[4].c_str());
            }
            return m_complex_spot_border;
        }
    }
    else
    { // inactive state
        if (sp == m_level_spot)
        {
            if (NULL == m_level_map_spot_border_na && m_spot_border_names[1].size())
            {
                m_level_map_spot_border_na = xr_new<CMapSpot>(this);
                m_level_map_spot_border_na->Load(g_uiSpotXml, m_spot_border_names[1].c_str());
            }
            return m_level_map_spot_border_na;
        }
        else if (sp == m_minimap_spot)
        {
            if (NULL == m_mini_map_spot_border_na && m_spot_border_names[3].size())
            {
                m_mini_map_spot_border_na = xr_new<CMapSpot>(this);
                m_mini_map_spot_border_na->Load(g_uiSpotXml, m_spot_border_names[3].c_str());
            }
            return m_mini_map_spot_border_na;
        }
        else if (sp == m_complex_spot)
        {
            if (NULL == m_complex_spot_border_na && m_spot_border_names[5].size())
            {
                m_complex_spot_border_na = xr_new<CMapSpot>(this);
                m_complex_spot_border_na->Load(g_uiSpotXml, m_spot_border_names[5].c_str());
            }
            return m_complex_spot_border_na;
        }
    }

    return NULL;
}

CRelationMapLocation::CRelationMapLocation(const shared_str& type, u16 object_id, u16 pInvOwnerActorID)
    : CMapLocation(*type, object_id), m_last_relation()
{
    m_curr_spot_name = type;
    m_pInvOwnerActorID = pInvOwnerActorID;
    m_b_visible = false;
    m_b_minimap_visible = true;
    m_b_levelmap_visible = true;
}

CRelationMapLocation::~CRelationMapLocation() {}
xr_vector<CMapLocation*> find_locations_res;

bool CRelationMapLocation::Update()
{
    if (false == inherited::Update())
        return false;

    bool bAlive = true;

    m_last_relation = ALife::eRelationTypeFriend;

    if (m_owner_se_object)
    {
        CSE_ALifeTraderAbstract* pEnt = NULL;
        CSE_ALifeTraderAbstract* pAct = NULL;
        pEnt = smart_cast<CSE_ALifeTraderAbstract*>(m_owner_se_object);
        pAct = smart_cast<CSE_ALifeTraderAbstract*>(ai().alife().objects().object(m_pInvOwnerActorID, true));
        if (!pEnt || !pAct)
            return false;

        m_last_relation = RELATION_REGISTRY().GetRelationType(pEnt, pAct);
        CSE_ALifeCreatureAbstract* pCreature = smart_cast<CSE_ALifeCreatureAbstract*>(m_owner_se_object);
        if (pCreature) // maybe trader ?
            bAlive = pCreature->g_Alive();
    }
    else
    {
        CInventoryOwner* pEnt = NULL;
        CInventoryOwner* pAct = NULL;

        pEnt = smart_cast<CInventoryOwner*>(Level().Objects.net_Find(m_objectID));
        pAct = smart_cast<CInventoryOwner*>(Level().Objects.net_Find(m_pInvOwnerActorID));
        if (!pEnt || !pAct)
            return false;

        m_last_relation = RELATION_REGISTRY().GetRelationType(pEnt, pAct);
        CEntityAlive* pEntAlive = smart_cast<CEntityAlive*>(pEnt);
        if (pEntAlive)
            bAlive = !!pEntAlive->g_Alive();
    }
    shared_str sname;

    if (bAlive == false)
        sname = "deadbody_location";
    else
        sname = RELATION_REGISTRY().GetSpotName(m_last_relation);

    if (m_curr_spot_name != sname)
    {
        LoadSpot(*sname);
        m_curr_spot_name = sname;
    }
    // update visibility
    bool vis_res = true;
    if (m_last_relation == ALife::eRelationTypeEnemy || m_last_relation == ALife::eRelationTypeWorstEnemy)
    {
        IGameObject* _object_ = Level().Objects.net_Find(m_objectID);
        if (_object_)
        {
            CEntityAlive* ea = smart_cast<CEntityAlive*>(_object_);
            if (ea && !ea->g_Alive())
                vis_res = true;
            else
            {
                const CGameObject* pObj = smart_cast<const CGameObject*>(_object_);
                CActor* pAct = smart_cast<CActor*>(Level().Objects.net_Find(m_pInvOwnerActorID));
                CHelmet* helm = smart_cast<CHelmet*>(pAct->inventory().ItemFromSlot(HELMET_SLOT));
                if (helm && helm->m_fShowNearestEnemiesDistance)
                {
                    if (pAct->Position().distance_to(pObj->Position()) < helm->m_fShowNearestEnemiesDistance)
                        vis_res = true;
                    else
                        vis_res = Actor()->memory().visual().visible_now(pObj);
                }
                else
                    vis_res = Actor()->memory().visual().visible_now(pObj);
            }
        }
        else
            vis_res = false;
    }

    if (bAlive == false)
    {
        IGameObject* _object_ = Level().Objects.net_Find(m_objectID);
        if (_object_)
        {
            const CGameObject* pObj = smart_cast<const CGameObject*>(_object_);
            CActor* pAct = smart_cast<CActor*>(Level().Objects.net_Find(m_pInvOwnerActorID));
            if (/*pAct->Position().distance_to_sqr(pObj->Position()) < 100.0F && */ abs(
                    pObj->Position().y - pAct->Position().y) < 3.0f)
                vis_res = true;
            else
                vis_res = false;
        }
        else
            vis_res = false;
    }

    if (m_b_visible == false && vis_res == true)
        m_minimap_spot->ResetXformAnimation();

    m_b_visible = vis_res;

    if (m_b_visible)
    {
        m_b_minimap_visible = true;
        m_b_levelmap_visible = true;

        if (Level().MapManager().GetMapLocationsForObject(m_objectID, find_locations_res))
        {
            xr_vector<CMapLocation*>::iterator it = find_locations_res.begin();
            xr_vector<CMapLocation*>::iterator it_e = find_locations_res.end();
            for (; it != it_e; ++it)
            {
                CMapLocation* ml = (*it);
                if (ml == this)
                    continue;

                m_b_minimap_visible = m_b_minimap_visible && (ml->MiniMapSpot() == NULL);
                m_b_levelmap_visible = m_b_levelmap_visible && (ml->LevelMapSpot() == NULL);
            }
        }
    }

    return true;
}

void CRelationMapLocation::UpdateMiniMap(CUICustomMap* map)
{
    if (IsVisible() && m_b_minimap_visible)
        inherited::UpdateMiniMap(map);
}

void CRelationMapLocation::UpdateLevelMap(CUICustomMap* map)
{
    if (IsVisible() && m_b_levelmap_visible)
        inherited::UpdateLevelMap(map);
}

#ifdef DEBUG
void CRelationMapLocation::Dump()
{
    inherited::Dump();
    Msg("--CRelationMapLocation m_curr_spot_name=[%s]", *m_curr_spot_name);
}
#endif
