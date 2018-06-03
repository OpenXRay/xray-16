////////////////////////////////////////////////////////////////////////////
//	Module 		: level_graph_debug.cpp
//	Created 	: 02.10.2001
//  Modified 	: 11.11.2003
//	Author		: Oles Shihkovtsov, Dmitriy Iassenev
//	Description : Level graph debug functions
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LevelGraphDebugRender.hpp"
#ifdef DEBUG
#include "xrAICore/Navigation/game_graph.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/graph_engine.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Level.h"
#include "game_base_space.h"
#include "game_sv_single.h"
#include "game_cl_base.h"
#include "xrserver_objects_alife_monsters.h"
#include "alife_simulator.h"
#include "alife_graph_registry.h"
#include "alife_object_registry.h"
#include "alife_human_brain.h"
#include "alife_monster_movement_manager.h"
#include "alife_monster_detail_path_manager.h"
#include "ai_space.h"
#include "ui_base.h"
#include "CustomMonster.h"
#include "ai/stalker/ai_stalker.h"
#include "team_base_zone.h"
#include "space_restriction_manager.h"
#include "space_restriction.h"
#include "space_restrictor.h"
#include "space_restriction_base.h"
#include "detail_path_manager.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "memory_space.h"
#include "movement_manager.h"
#include "cover_point.h"
#include "cover_manager.h"
#include "cover_evaluators.h"
#include "smart_cover_object.h"
#include "debug_renderer.h"
#include "xrEngine/GameFont.h"

LevelGraphDebugRender::LevelGraphDebugRender() : gameGraph(nullptr), levelGraph(nullptr)
{
    debugShader->create("debug\\ai_nodes", "$null");
    currentLevelId = -1;
    currentActual = false;
    currentCenter = {flt_max, flt_max, flt_max};
    currentRadius = {flt_max, flt_max, flt_max};
}

LevelGraphDebugRender::~LevelGraphDebugRender() {}
void LevelGraphDebugRender::SetupCurrentLevel(int levelId)
{
    if (currentLevelId == levelId)
        return;
    currentActual = false;
    currentLevelId = levelId;
}

void LevelGraphDebugRender::Render(CGameGraph& gameGraph, CLevelGraph& levelGraph)
{
    this->gameGraph = &gameGraph;
    this->levelGraph = &levelGraph;
    if (psAI_Flags.test(aiDrawGameGraph))
        DrawGameGraph();
    if (!bDebug && !psAI_Flags.test(aiMotion))
        return;
    if (bDebug && psAI_Flags.test(aiDebug))
        DrawNodes();
    DrawRestrictions();
    if (psAI_Flags.test(aiCover))
        DrawCovers();
    if (psAI_Flags.test(aiMotion))
        DrawObjects();
#ifdef DEBUG
    DrawDebugNode();
#endif
}

void LevelGraphDebugRender::Modify(int vid, Fbox& bbox)
{
    bbox.modify(gameGraph->vertex(vid)->game_point());
    CGameGraph::const_iterator it, end;
    gameGraph->begin(vid, it, end);
    for (; it != end; it++)
        bbox.modify(gameGraph->vertex(gameGraph->value(vid, it))->game_point());
}

void LevelGraphDebugRender::UpdateCurrentInfo()
{
    currentActual = true;
    Fbox bbox;
    bbox.invalidate();
    bool found = false;
    bool all = currentLevelId == -1;
    int vertexCount = gameGraph->header().vertex_count();
    for (int i = 0; i < vertexCount; i++)
    {
        if (!all)
        {
            if (gameGraph->vertex(i)->level_id() != currentLevelId)
            {
                if (found)
                    break;
                continue;
            }
            found = true;
        }
        Modify(i, bbox);
    }
    bbox.getcenter(currentCenter);
    bbox.getradius(currentRadius);
}

Fvector LevelGraphDebugRender::ConvertPosition(const Fvector& pos)
{
    Fvector result = pos;
    result.sub(currentCenter, pos);
    result.x *= 5 / currentRadius.x;
    result.y *= 1 / currentRadius.y;
    result.z *= 5 / currentRadius.z;
    result.mul(0.5f);
    result.add(Level().CurrentEntity()->Position());
    result.y += 4.5f;
    return result;
}

void LevelGraphDebugRender::DrawEdge(int vid1, int vid2)
{
    const GameGraph::CVertex& v1 = *gameGraph->vertex(vid1);
    const GameGraph::CVertex& v2 = *gameGraph->vertex(vid2);
    float radius = 0.005f;
    if (psAI_Flags.test(aiDrawGameGraphRealPos))
        radius = 1.f;
    const u32 defaultVertexColor = color_xrgb(0, 255, 255);
    const u32 xVertexColor = color_xrgb(255, 0, 255);
    const u32 edgeColor = color_xrgb(0, 255, 0);
    u32 vertexColor1 = !v1.vertex_type()[3] ? xVertexColor : defaultVertexColor;
    u32 vertexColor2 = !v2.vertex_type()[3] ? xVertexColor : defaultVertexColor;
    Fvector pos1, pos2;
    if (psAI_Flags.test(aiDrawGameGraphRealPos))
    {
        pos1 = v1.level_point();
        pos2 = v2.level_point();
    }
    else
    {
        pos1 = ConvertPosition(v1.game_point());
        pos2 = ConvertPosition(v2.game_point());
    }
    CDebugRenderer& render = Level().debug_renderer();
    render.draw_aabb(pos1, radius, radius, radius, vertexColor1);
    render.draw_aabb(pos2, radius, radius, radius, vertexColor2);
    render.draw_line(Fidentity, pos1, pos2, edgeColor);
}

void LevelGraphDebugRender::DrawVertex(int vid)
{
    CGameGraph::const_iterator it, end;
    gameGraph->begin(vid, it, end);
    for (; it != end; it++)
    {
        int neighbourId = gameGraph->value(vid, it);
        if (neighbourId < vid)
            DrawEdge(vid, neighbourId);
    }
}

void LevelGraphDebugRender::DrawStalkers(int vid)
{
    if (!ai().get_alife())
        return;
    float radius = 0.0105f;
    if (psAI_Flags.test(aiDrawGameGraphRealPos))
        radius = 1;
    const u32 color = color_xrgb(255, 0, 0);
    IGameFont& font = *UI().Font().pFontDI;
    Fvector pos;
    if (psAI_Flags.test(aiDrawGameGraphRealPos))
        pos = gameGraph->vertex(vid)->level_point();
    else
        pos = ConvertPosition(gameGraph->vertex(vid)->game_point());
    font.SetColor(color_xrgb(255, 255, 0));
    bool showText = true;
    Fvector4 temp;
    Device.mFullTransform.transform(temp, pos);
    font.OutSetI(temp.x, -temp.y);
    font.SetHeightI(0.05f / _sqrt(temp.w));
    if (temp.z < 0 || temp.w < 0 || _abs(temp.x) > 1 || _abs(temp.y) > 1)
        showText = false;
    using ObjectRegistry = CALifeGraphRegistry::OBJECT_REGISTRY;
    using DetailPath = CALifeMonsterDetailPathManager::PATH;
    const ObjectRegistry& objects = ai().alife().graph().objects()[vid].objects();
    CDebugRenderer& render = Level().debug_renderer();
    if (showText)
    {
        bool firstTime = true;
        for (auto& pair : objects.objects())
        {
            CSE_ALifeDynamicObject* object = pair.second;
            auto* stalker = smart_cast<CSE_ALifeHumanStalker*>(object);
            if (!stalker)
                continue;
            const DetailPath& path = stalker->brain().movement().detail().path();
            const float& walked_distance = path.size() < 2 ? 0 : stalker->brain().movement().detail().walked_distance();
            // font.OutNext("%s",stalker->name_replace());
            if (path.size() >= 2 && !fis_zero(walked_distance))
                continue;
            if (!firstTime)
                continue;
            Fvector pos;
            if (psAI_Flags.test(aiDrawGameGraphRealPos))
                pos = gameGraph->vertex(stalker->m_tGraphID)->level_point();
            else
                pos = ConvertPosition(gameGraph->vertex(stalker->m_tGraphID)->game_point());
            render.draw_aabb(pos, radius, radius, radius, color);
            firstTime = false;
        }
    }
    for (auto& pair : objects.objects())
    {
        CSE_ALifeDynamicObject* object = pair.second;
        auto* stalker = smart_cast<CSE_ALifeHumanStalker*>(object);
        if (!stalker)
            continue;
        const DetailPath& path = stalker->brain().movement().detail().path();
        if (path.size() < 2)
            continue;
        u32 vid1 = stalker->m_tGraphID;
        u32 vid2 = path[path.size() - 2];
        const float& walkedDistance = stalker->brain().movement().detail().walked_distance();
        if (fis_zero(walkedDistance))
            continue;
        const CGameGraph::CVertex& v1 = *gameGraph->vertex(vid1);
        const CGameGraph::CVertex& v2 = *gameGraph->vertex(vid2);
        Fvector pos1, pos2;
        float distance;
        if (psAI_Flags.test(aiDrawGameGraphRealPos))
        {
            pos1 = v1.level_point();
            pos2 = v2.level_point();
            distance = pos1.distance_to(pos2);
        }
        else
        {
            pos1 = ConvertPosition(v1.game_point());
            pos2 = ConvertPosition(v2.game_point());
            distance = v1.game_point().distance_to(v2.game_point());
        }
        Fvector direction = Fvector().sub(pos2, pos1);
        float magnitude = direction.magnitude();
        direction.normalize();
        direction.mul(magnitude * walkedDistance / distance);
        direction.add(pos1);
        render.draw_aabb(direction, radius, radius, radius, color);
        Fvector4 temp;
        Device.mFullTransform.transform(temp, direction);
        if (temp.z < 0 || temp.w < 0 || _abs(temp.x) > 1 || _abs(temp.y) > 1)
            continue;
        font.SetHeightI(0.05f / _sqrt(temp.w));
    }
}

void LevelGraphDebugRender::DrawObjects(int vid)
{
    if (!ai().get_alife())
        return;
    const GameGraph::CVertex& vertex = *gameGraph->vertex(vid);
    float radius = 0.0105f;
    if (psAI_Flags.test(aiDrawGameGraphRealPos))
        radius = 1.0f;
    const u32 color = color_xrgb(255, 0, 0);
    IGameFont& font = *UI().Font().pFontDI;
    Fvector position;
    if (psAI_Flags.test(aiDrawGameGraphRealPos))
        position = vertex.level_point();
    else
        position = ConvertPosition(vertex.game_point());
    font.SetColor(color_xrgb(255, 255, 0));
    Fvector4 temp;
    Device.mFullTransform.transform(temp, position);
    font.OutSetI(temp.x, -temp.y);
    font.SetHeightI(0.05f / _sqrt(temp.w));
    bool showText = true;
    if (temp.z < 0 || temp.w < 0 || _abs(temp.x) > 1 || _abs(temp.y) > 1)
        showText = false;
    using ObjectRegistry = CALifeGraphRegistry::OBJECT_REGISTRY;
    using DetailPath = CALifeMonsterDetailPathManager::PATH;
    const ObjectRegistry& objects = ai().alife().graph().objects()[vid].objects();
    CDebugRenderer& render = Level().debug_renderer();
    if (showText)
    {
        bool firstTime = true;
        for (auto& pair : objects.objects())
        {
            CSE_ALifeDynamicObject* object = pair.second;
            CSE_ALifeMonsterAbstract* monster = smart_cast<CSE_ALifeMonsterAbstract*>(object);
            if (!monster)
                continue;
            const DetailPath& path = monster->brain().movement().detail().path();
            const float walkedDistance = path.size() < 2 ? 0 : monster->brain().movement().detail().walked_distance();
            // font.OutNext("%s", monster->name_replace());
            if (path.size() >= 2 && !fis_zero(walkedDistance))
                continue;
            if (!firstTime)
                continue;
            Fvector position;
            if (psAI_Flags.test(aiDrawGameGraphRealPos))
                position = gameGraph->vertex(monster->m_tGraphID)->level_point();
            else
                position = ConvertPosition(gameGraph->vertex(monster->m_tGraphID)->game_point());
            render.draw_aabb(position, radius, radius, radius, color);
            firstTime = false;
        }
    }
    for (auto& pair : objects.objects())
    {
        CSE_ALifeDynamicObject* object = pair.second;
        auto* monster = smart_cast<CSE_ALifeMonsterAbstract*>(object);
        if (!monster)
            continue;
        const DetailPath& path = monster->brain().movement().detail().path();
        if (path.size() < 2)
            continue;
        u32 vid1 = monster->m_tGraphID;
        u32 vid2 = path[path.size() - 2];
        const float walkedDistance = monster->brain().movement().detail().walked_distance();
        if (fis_zero(walkedDistance))
            continue;
        Fvector pos1, pos2;
        float distance;
        const CGameGraph::CVertex& v1 = *gameGraph->vertex(vid1);
        const CGameGraph::CVertex& v2 = *gameGraph->vertex(vid2);
        if (psAI_Flags.test(aiDrawGameGraphRealPos))
        {
            pos1 = v1.level_point();
            pos2 = v2.level_point();
            distance = pos1.distance_to(pos2);
        }
        else
        {
            pos1 = ConvertPosition(v1.game_point());
            pos2 = ConvertPosition(v2.game_point());
            distance = v1.game_point().distance_to(v2.game_point());
        }
        Fvector direction = Fvector().sub(pos1, pos2);
        float magnitude = direction.magnitude();
        direction.normalize();
        direction.mul(magnitude * walkedDistance / distance);
        direction.add(pos1);
        render.draw_aabb(direction, radius, radius, radius, color);
        Fvector4 temp;
        Device.mFullTransform.transform(temp, direction);
        if (temp.z < 0 || temp.w < 0 || _abs(temp.x) > 1 || _abs(temp.y) > 1)
            continue;
        font.SetHeightI(0.05f / _sqrt(temp.w));
    }
}

void LevelGraphDebugRender::DrawGameGraph()
{
    IGameObject* entity = Level().CurrentEntity();
    if (!entity)
        return;
    const Fmatrix& xform = entity->XFORM();
    Fvector center = {0.f, 5.f, 0.f};
    Fvector bounds = {3.f, 0.f, 3.f};
// draw back plane
#if 0 // XXX: disabled in original, reenable?
    Fvector vertices[4];
	xform.transform_tiny(vertices[0], {center.x-bounds.x, center.y+bounds.y, center.z+bounds.z});
	xform.transform_tiny(vertices[1], {center.x+bounds.x, center.y+bounds.y, center.z+bounds.z});
	xform.transform_tiny(vertices[2], {center.x-bounds.x, center.y-bounds.y, center.z-bounds.z});
	xform.transform_tiny(vertices[3], {center.x+bounds.x, center.y-bounds.y, center.z-bounds.z});
    u32 backColor = color_xrgb(0, 0, 0);
    GlobalEnv.DRender->dbg_DrawTRI(Fidentity, vertices[0], vertices[2], vertices[1], backColor);
    GlobalEnv.DRender->dbg_DrawTRI(Fidentity, vertices[1], vertices[2], vertices[3], backColor);
#endif
    // draw vertices
    UpdateCurrentInfo();
    bool found = false;
    bool all = currentLevelId == -1;
    int vertexCount = gameGraph->header().vertex_count();
    for (int i = 0; i < vertexCount; i++)
    {
        if (!all)
        {
            if (gameGraph->vertex(i)->level_id() != currentLevelId)
            {
                if (found)
                    break;
                continue;
            }
            found = true;
        }
        DrawVertex(i);
        if (psAI_Flags.test(aiDrawGameGraphStalkers))
            DrawStalkers(i);
        if (psAI_Flags.test(aiDrawGameGraphObjects))
            DrawObjects(i);
    }
#if 0 // XXX: update/delete
    for (int i = 0; i<(int)ai().game_graph().header().vertex_count(); i++)
    {
        Fvector t1 = ai().game_graph().vertex(i)->game_point();
        t1.y += .6f;
        NORMALIZE_VECTOR(t1);
        Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(0, 0, 255));
        CGameGraph::const_iterator	I, E;
        ai().game_graph().begin(i, I, E);
        for (; I != E; ++I)
        {
            Fvector t2 = ai().game_graph().vertex((*I).vertex_id())->game_point();
            t2.y += .6f;
            NORMALIZE_VECTOR(t2);
            Level().debug_renderer().draw_line(Fidentity, t1, t2, color_xrgb(0, 255, 0));
        }
        Fvector T;
        Fvector4 S;
        T.set(t1);
        //T.y+= 1.5f;
        T.y += 1.5f / 10.f;
        Device.mFullTransform.transform(S, T);
        //out of screen
        if (S.z < 0 || S.w < 0)
            continue;
        if (S.x < -1.f || S.x > 1.f || S.y<-1.f || S.x>1.f)
            continue;
        F->SetSizeI(0.05f / _sqrt(_abs(S.w)));
        F->SetColor(0xffffffff);
        F->OutI(S.x, -S.y, "%d", i);
    }
    {
        const xr_vector<u32> &path = map_point_path;
        if (path.size())
        {
            Fvector t1 = ai().game_graph().vertex(path.back())->game_point();
            t1.y += .6f;
            NORMALIZE_VECTOR(t1);
            Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(0, 0, 255));
            for (int i = (int)path.size() - 2; i >= 0; --i)
            {
                Fvector t2 = ai().game_graph().vertex(path[i])->game_point();
                t2.y += .6f;
                NORMALIZE_VECTOR(t2);
                Level().debug_renderer().draw_aabb(t2, .05f, .05f, .05f, color_xrgb(0, 0, 255));
                Level().debug_renderer().draw_line(Fidentity, t1, t2, color_xrgb(0, 0, 255));
                t1 = t2;
            }
        }
    }
    if (GameID() == eGameIDSingle && ai().get_alife())
    {
        {
            GameGraph::_LEVEL_ID J = ai().game_graph().vertex(ai().alife().graph().actor()->m_tGraphID)->level_id();
            for (int i = 0, n = (int)ai().game_graph().header().vertex_count(); i<n; ++i)
            {
                if (ai().game_graph().vertex(i)->level_id() != J)
                    continue;
                Fvector t1 = ai().game_graph().vertex(i)->level_point(), t2 = ai().game_graph().vertex(i)->game_point();
                t1.y += .6f;
                t2.y += .6f;
                NORMALIZE_VECTOR(t2);
                Level().debug_renderer().draw_aabb(t1, .5f, .5f, .5f, color_xrgb(255, 255, 255));
                //Level().debug_renderer().draw_line(Fidentity,t1,t2,color_xrgb(255,255,255));
                Fvector T;
                Fvector4 S;
                T.set(t1);
                //T.y+= 1.5f;
                T.y += 1.5f;
                Device.mFullTransform.transform(S, T);
                //out of screen
                if (S.z < 0 || S.w < 0)
                    continue;
                if (S.x < -1.f || S.x > 1.f || S.y<-1.f || S.x>1.f)
                    continue;
                F->SetSizeI(0.1f / _sqrt(_abs(S.w)));
                F->SetColor(0xffffffff);
                F->OutI(S.x, -S.y, "%d", i);
            }
        }

        ALife::D_OBJECT_P_MAP::const_iterator	I = ai().alife().objects().objects().begin();
        ALife::D_OBJECT_P_MAP::const_iterator	E = ai().alife().objects().objects().end();
        for (; I != E; ++I)
        {
            {
                CSE_ALifeMonsterAbstract *tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract *>((*I).second);
                if (tpALifeMonsterAbstract && tpALifeMonsterAbstract->m_bDirectControl && !tpALifeMonsterAbstract->m_bOnline)
                {
                    CSE_ALifeHumanAbstract *tpALifeHuman = smart_cast<CSE_ALifeHumanAbstract *>(tpALifeMonsterAbstract);
                    if (tpALifeHuman && tpALifeHuman->brain().movement().detail().path().size())
                    {
                        Fvector t1 = ai().game_graph().vertex(tpALifeHuman->brain().movement().detail().path().back())->game_point();
                        t1.y += .6f;
                        NORMALIZE_VECTOR(t1);
                        Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(0, 0, 255));
                        for (int i = (int)tpALifeHuman->brain().movement().detail().path().size() - 2; i >= 0; --i)
                        {
                            Fvector t2 = ai().game_graph().vertex(tpALifeHuman->brain().movement().detail().path()[i])->game_point();
                            t2.y += .6f;
                            NORMALIZE_VECTOR(t2);
                            Level().debug_renderer().draw_aabb(t2, .05f, .05f, .05f, color_xrgb(0, 0, 255));
                            Level().debug_renderer().draw_line(Fidentity, t1, t2, color_xrgb(0, 0, 255));
                            t1 = t2;
                        }
                    }
                    if (tpALifeMonsterAbstract->m_fDistanceToPoint > EPS_L)
                    {
                        Fvector t1 = ai().game_graph().vertex(tpALifeMonsterAbstract->m_tGraphID)->game_point();
                        Fvector t2 = ai().game_graph().vertex(tpALifeMonsterAbstract->m_tNextGraphID)->game_point();
                        t2.sub(t1);
                        t2.mul(tpALifeMonsterAbstract->m_fDistanceFromPoint / tpALifeMonsterAbstract->m_fDistanceToPoint);
                        t1.add(t2);
                        t1.y += .6f;
                        NORMALIZE_VECTOR(t1);
                        Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(255, 0, 0));
                    }
                    else
                    {
                        Fvector t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
                        t1.y += .6f;
                        NORMALIZE_VECTOR(t1);
                        Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(255, 0, 0));
                    }
                }
                else
                {
                    CSE_ALifeInventoryItem *l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>((*I).second);
                    if (l_tpALifeInventoryItem && !l_tpALifeInventoryItem->attached())
                    {
                        Fvector t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
                        t1.y += .6f;
                        NORMALIZE_VECTOR(t1);
                        Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(255, 255, 0));
                    }
                    else
                    {
                        CSE_ALifeCreatureActor *tpALifeCreatureActor = smart_cast<CSE_ALifeCreatureActor*>((*I).second);
                        if (tpALifeCreatureActor)
                        {
                            Fvector t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
                            t1.y += .6f;
                            NORMALIZE_VECTOR(t1);
                            Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(255, 255, 255));
                        }
                        else
                        {
                            CSE_ALifeTrader *tpALifeTrader = smart_cast<CSE_ALifeTrader*>((*I).second);
                            if (tpALifeTrader)
                            {
                                Fvector t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
                                t1.y += .6f;
                                NORMALIZE_VECTOR(t1);
                                Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(0, 0, 0));
                            }
                            else
                            {
                                CSE_ALifeSmartZone *smart_zone = smart_cast<CSE_ALifeSmartZone*>((*I).second);
                                if (smart_zone)
                                {
                                    Fvector t1 = ai().game_graph().vertex((*I).second->m_tGraphID)->game_point();
                                    t1.y += .6f;
                                    NORMALIZE_VECTOR(t1);
                                    Level().debug_renderer().draw_aabb(t1, .05f, .05f, .05f, color_xrgb(255, 0, 0));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#endif
}

void LevelGraphDebugRender::DrawNodes()
{
    IGameObject* obj = Level().CurrentEntity();
    Fvector objPos = obj->Position();
    objPos.y += 0.5f;
    // display
    u32 vid = obj->ai_location().level_vertex_id();
    IGameFont* font = UI().Font().pFontDI;
    font->SetHeightI(0.02f);
    font->OutI(0.0f, 0.5f, "%f,%f,%f", VPUSH(objPos));
    svector<u32, 128> linked;
    {
        CLevelGraph::const_iterator it, end;
        levelGraph->begin(vid, it, end);
        for (; it != end; it++)
            linked.push_back(levelGraph->value(vid, it));
    }
    // render
    GEnv.DRender->SetShader(debugShader);
    font->SetColor(color_rgba(255, 255, 255, 255));
    Fvector minPos = Device.vCameraPosition;
    Fvector maxPos = Device.vCameraPosition;
    minPos.sub(30.0f);
    maxPos.add(30.0f);
    CLevelGraph::const_vertex_iterator it, end;
    if (levelGraph->valid_vertex_position(minPos))
    {
        it = std::lower_bound(levelGraph->begin(), levelGraph->end(), levelGraph->vertex_position(minPos).xz(),
            CLevelGraph::vertex::predicate2);
    }
    else
        it = levelGraph->begin();
    if (levelGraph->valid_vertex_position(maxPos))
    {
        end = std::upper_bound(levelGraph->begin(), levelGraph->end(), levelGraph->vertex_position(maxPos).xz(),
            CLevelGraph::vertex::predicate);
        if (end != levelGraph->end())
            end++;
    }
    else
        end = levelGraph->end();
    const float sc = levelGraph->header().cell_size() / 16;
    const float st = 0.98f * levelGraph->header().cell_size() / 2;
    const float tt = 0.01f;
    for (; it != end; it++)
    {
        Fvector vertexPos = levelGraph->vertex_position(*it);
        u32 Nid = levelGraph->vertex_id(it);
        if (Device.vCameraPosition.distance_to(vertexPos) > 30)
            continue;
        float sr = levelGraph->header().cell_size();
        if (GEnv.Render->ViewBase.testSphere_dirty(vertexPos, sr))
        {
            u32 colorC = color_xrgb(0, 0, 255);
            u32 colorT = color_xrgb(255, 255, 255);
            u32 colorH = color_xrgb(0, 128, 0);
            bool hl = false;
            if (Nid == vid)
            {
                hl = true;
                colorT = color_xrgb(0, 255, 0);
            }
            else
            {
                for (u32 t = 0; t < linked.size(); t++)
                {
                    if (linked[t] == Nid)
                    {
                        hl = true;
                        colorT = colorH;
                        break;
                    }
                }
            }
            // unpack plane
            Fplane PL;
            Fvector vNorm;
            pvDecompress(vNorm, it->plane());
            PL.build(vertexPos, vNorm);
            // create vertices
            auto createVertex = [&](Fplane& pl, const Fvector& v) {
                const Fvector up = {0, 1, 0};
                Fvector result;
                pl.intersectRayPoint(v, up, result);
                result.mad(result, pl.n, tt);
                return result;
            };
            Fvector v1 = createVertex(PL, {vertexPos.x - st, vertexPos.y, vertexPos.z - st}); // minX, minZ
            Fvector v2 = createVertex(PL, {vertexPos.x + st, vertexPos.y, vertexPos.z - st}); // maxX, minZ
            Fvector v3 = createVertex(PL, {vertexPos.x + st, vertexPos.y, vertexPos.z + st}); // maxX, maxZ
            Fvector v4 = createVertex(PL, {vertexPos.x - st, vertexPos.y, vertexPos.z + st}); // minX, maxZ
            // render quad
            GEnv.DRender->dbg_DrawTRI(Fidentity, v3, v2, v1, colorT);
            GEnv.DRender->dbg_DrawTRI(Fidentity, v1, v4, v3, colorT);
            // render center
            Level().debug_renderer().draw_aabb(vertexPos, sc, sc, sc, colorC);
            // render id
            if (hl)
            {
                Fvector offsetPos = vertexPos;
                offsetPos.y += 0.3f;
                Fvector4 tsmPos;
                Device.mFullTransform.transform(tsmPos, offsetPos);
                if (tsmPos.z < 0 || tsmPos.w < 0 || _abs(tsmPos.x) > 1 || _abs(tsmPos.y) > 1)
                    continue;
                font->SetHeightI(0.05f / _sqrt(_abs(tsmPos.w)));
                font->SetColor(0xffffffff);
                font->OutI(tsmPos.x, -tsmPos.y, "~%d", Nid);
            }
        }
    }
}

void LevelGraphDebugRender::DrawRestrictions()
{
    CSpaceRestrictionManager& spaceRestrictionMgr = Level().space_restriction_manager();
    CDebugRenderer& debugRenderer = Level().debug_renderer();
    CRandom random;
    const float halfR = 0.05f;
    const float yOffset = 0.1;
    for (auto& pair : spaceRestrictionMgr.restrictions())
    {
        if (pair.second->released())
            continue;
        if (!pair.second->initialized())
            continue;
        u8 b = u8(random.randI(255));
        u8 g = u8(random.randI(255));
        u8 r = u8(random.randI(255));
        for (u32 vid : pair.second->border())
        {
            Fvector temp = levelGraph->vertex_position(vid);
            temp.y += yOffset;
            debugRenderer.draw_aabb(temp, halfR, halfR, halfR, color_xrgb(r, g, b));
        }
#ifdef USE_FREE_IN_RESTRICTIONS
        for (auto& fir : pair.second->m_free_in_restrictions)
        {
            for (u32 vid : fir.m_restriction->border())
            {
                Fvector temp = levelGraph->vertex_position(vid);
                temp.y += yOffset;
                debugRenderer.draw_aabb(temp, halfR, halfR, halfR, color_xrgb(0, 255, 0));
            }
        }
#endif
    }
}

void LevelGraphDebugRender::DrawCovers()
{
    CDebugRenderer& debugRenderer = Level().debug_renderer();
    const float cellSize = levelGraph->header().cell_size();
    const float halfSize = cellSize / 2;
    auto& nearest = coverPointCache;
    nearest.reserve(1000);
    ai().cover_manager().covers().nearest(Device.vCameraPosition, 5.0f, nearest);
    for (auto coverPoint : nearest)
    {
        // high cover
        Fvector pos = coverPoint->position();
        pos.y += 1.5f;
        debugRenderer.draw_aabb(pos, halfSize - 0.01f, 1.f, halfSize - 0.01f, color_xrgb(0, 255, 0));
        CLevelGraph::CVertex* v = levelGraph->vertex(coverPoint->level_vertex_id());
        Fvector dir;
        float bestValue = -1;
        u32 j = 0;
        for (u32 i = 0; i < 36; ++i)
        {
            float value = levelGraph->high_cover_in_direction(float(10 * i) / 180 * PI, v);
            dir.setHP(float(10 * i) / 180 * PI, 0);
            dir.normalize();
            dir.mul(value * halfSize);
            dir.add(pos);
            dir.y = pos.y;
            debugRenderer.draw_line(Fidentity, pos, dir, color_xrgb(0, 0, 255));
            value = levelGraph->compute_high_square(float(10 * i) / 180 * PI, PI / 2, v);
            if (value > bestValue)
            {
                bestValue = value;
                j = i;
            }
        }
        Fvector highCoverDirs[] = {pos, pos, pos, pos};
        highCoverDirs[0].x -= v->high_cover(0) * halfSize / 15;
        highCoverDirs[1].z += v->high_cover(1) * halfSize / 15;
        highCoverDirs[2].x += v->high_cover(2) * halfSize / 15;
        highCoverDirs[3].z -= v->high_cover(3) * halfSize / 15;
        for (u32 i = 0; i < 4; i++)
            debugRenderer.draw_line(Fidentity, pos, highCoverDirs[i], color_xrgb(255, 0, 0));
        float value = levelGraph->high_cover_in_direction(float(10 * j) / 180 * PI, v);
        dir.setHP(float(10 * j) / 180 * PI, 0);
        dir.normalize();
        dir.mul(value * halfSize);
        dir.add(pos);
        dir.y = pos.y;
        debugRenderer.draw_line(Fidentity, pos, dir, color_xrgb(0, 0, 0));
        // low cover
        pos = coverPoint->position();
        pos.y += 0.6f;
        debugRenderer.draw_aabb(pos, halfSize - 0.01f, 1.f, halfSize - 0.01f, color_xrgb(0, 255, 0));
        v = levelGraph->vertex(coverPoint->level_vertex_id());
        bestValue = -1;
        j = 0;
        for (u32 i = 0; i < 36; ++i)
        {
            float value = levelGraph->low_cover_in_direction(float(10 * i) / 180 * PI, v);
            dir.setHP(float(10 * i) / 180.f * PI, 0);
            dir.normalize();
            dir.mul(value * halfSize);
            dir.add(pos);
            dir.y = pos.y;
            debugRenderer.draw_line(Fidentity, pos, dir, color_xrgb(0, 0, 255));
            value = levelGraph->compute_low_square(float(10 * i) / 180 * PI, PI / 2, v);
            if (value > bestValue)
            {
                bestValue = value;
                j = i;
            }
        }
        Fvector lowCoverDirs[] = {pos, pos, pos, pos};
        lowCoverDirs[0].x -= v->low_cover(0) * halfSize / 15;
        lowCoverDirs[1].z += v->low_cover(1) * halfSize / 15;
        lowCoverDirs[2].x += v->low_cover(2) * halfSize / 15;
        lowCoverDirs[3].z -= v->low_cover(3) * halfSize / 15;
        for (u32 i = 0; i < 4; i++)
            debugRenderer.draw_line(Fidentity, pos, lowCoverDirs[i], color_xrgb(255, 0, 0));
        value = levelGraph->low_cover_in_direction(float(10 * j) / 180 * PI, v);
        dir.setHP(float(10 * j) / 180 * PI, 0);
        dir.normalize();
        dir.mul(value * halfSize);
        dir.add(pos);
        dir.y = pos.y;
        debugRenderer.draw_line(Fidentity, pos, dir, color_xrgb(0, 0, 0));
    }
}

void LevelGraphDebugRender::DrawObjects()
{
    const float halfR = 1.0f;
    CDebugRenderer& debugRenderer = Level().debug_renderer();
    CObjectList& levelObjects = Level().Objects;
    u32 objectCount = levelObjects.o_count();
    for (u32 i = 0; i < objectCount; i++)
    {
        IGameObject* obj = levelObjects.o_get_by_iterator(i);
        auto teamBaseZone = smart_cast<CTeamBaseZone*>(obj);
        if (teamBaseZone)
        {
            teamBaseZone->OnRender();
            continue;
        }
        auto monster = smart_cast<CCustomMonster*>(obj);
        if (monster)
        {
            monster->OnRender();
            auto& detailPath = monster->movement().detail().path();
            if (!detailPath.empty())
            {
                Fvector temp = detailPath[detailPath.size() - 1].position;
                debugRenderer.draw_aabb(temp, halfR, halfR, halfR, color_xrgb(0, 0, 255));
            }
            continue;
        }
        auto smartCover = smart_cast<smart_cover::object*>(obj);
        if (smartCover)
        {
            smartCover->OnRender();
            continue;
        }
    }
}

void LevelGraphDebugRender::DrawDebugNode()
{
    if (!g_bDebugNode)
        return;
    CDebugRenderer& debugRenderer = Level().debug_renderer();
    Fvector srcPos, dstPos;
    const float halfR = 0.35f;
    const float yOffset = 10.0f;
    if (levelGraph->valid_vertex_id(g_dwDebugNodeSource))
    {
        srcPos = levelGraph->vertex_position(g_dwDebugNodeSource);
        dstPos = srcPos;
        dstPos.y += yOffset;
        debugRenderer.draw_aabb(srcPos, halfR, halfR, halfR, color_xrgb(0, 0, 255));
        debugRenderer.draw_line(Fidentity, srcPos, dstPos, color_xrgb(0, 0, 255));
    }
    if (levelGraph->valid_vertex_id(g_dwDebugNodeDest))
    {
        srcPos = levelGraph->vertex_position(g_dwDebugNodeDest);
        dstPos = srcPos;
        dstPos.y += yOffset;
        debugRenderer.draw_aabb(srcPos, halfR, halfR, halfR, color_xrgb(255, 0, 0));
        debugRenderer.draw_line(Fidentity, srcPos, dstPos, color_xrgb(255, 0, 0));
    }
}

#endif // DEBUG
