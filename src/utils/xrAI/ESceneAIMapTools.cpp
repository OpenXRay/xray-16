#include "stdafx.h"
#pragma hdrstop

#include "ESceneAIMapTools.h"
#include "Common/LevelStructure.hpp"
#include "Scene.h"
#include "UI_LevelMain.h"
#include "UI_LevelTools.h"
#include "ESceneAIMapControls.h"
#include "xrPool.h"

// chunks
#define AIMAP_VERSION 0x0002
//----------------------------------------------------
#define AIMAP_CHUNK_VERSION 0x0001
#define AIMAP_CHUNK_FLAGS 0x0002
#define AIMAP_CHUNK_BOX 0x0003
#define AIMAP_CHUNK_PARAMS 0x0004
#define AIMAP_CHUNK_NODES 0x0006
#define AIMAP_CHUNK_SNAP_OBJECTS 0x0007
#define AIMAP_CHUNK_INTERNAL_DATA 0x0008
#define AIMAP_CHUNK_INTERNAL_DATA2 0x0009
//----------------------------------------------------

poolSS<SAINode, 1024> g_ainode_pool;

void* SAINode::operator new(std::size_t size) { return g_ainode_pool.create(); }
void* SAINode::operator new(std::size_t size, SAINode* src) { return src; }
void SAINode::operator delete(void* ptr) { g_ainode_pool.destroy((SAINode*)ptr); }
void SAINode::PointLF(Fvector& D, float patch_size)
{
    Fvector d;
    d.set(0, -1, 0);
    Fvector v = Pos;
    float s = patch_size / 2;
    v.x -= s;
    v.z += s;
    Plane.intersectRayPoint(v, d, D);
}

void SAINode::PointFR(Fvector& D, float patch_size)
{
    Fvector d;
    d.set(0, -1, 0);
    Fvector v = Pos;
    float s = patch_size / 2;
    v.x += s;
    v.z += s;
    Plane.intersectRayPoint(v, d, D);
}

void SAINode::PointRB(Fvector& D, float patch_size)
{
    Fvector d;
    d.set(0, -1, 0);
    Fvector v = Pos;
    float s = patch_size / 2;
    v.x += s;
    v.z -= s;
    Plane.intersectRayPoint(v, d, D);
}

void SAINode::PointBL(Fvector& D, float patch_size)
{
    Fvector d;
    d.set(0, -1, 0);
    Fvector v = Pos;
    float s = patch_size / 2;
    v.x -= s;
    v.z -= s;
    Plane.intersectRayPoint(v, d, D);
}

void SAINode::LoadLTX(CInifile& ini, LPCSTR sect_name, ESceneAIMapTool* tools) { R_ASSERT(0); }
void SAINode::SaveLTX(CInifile& ini, LPCSTR sect_name, ESceneAIMapTool* tools)
{
    R_ASSERT2(0, "dont use it !!!");
    u32 id;
    u16 pl;
    NodePosition np;

    id = n1 ? (u32)n1->idx : InvalidNode;
    ini.w_u32(sect_name, "n1", id);

    id = n2 ? (u32)n2->idx : InvalidNode;
    ini.w_u32(sect_name, "n2", id);

    id = n3 ? (u32)n3->idx : InvalidNode;
    ini.w_u32(sect_name, "n3", id);

    id = n4 ? (u32)n4->idx : InvalidNode;
    ini.w_u32(sect_name, "n4", id);

    pl = pvCompress(Plane.n);
    ini.w_u16(sect_name, "plane", pl);

    tools->PackPosition(np, Pos, tools->m_AIBBox, tools->m_Params);
    string256 buff;

    s16 x;
    u16 y;
    s16 z;

    sprintf(buff, "%i,%u,%i", np.x, np.y, np.z);
    ini.w_string(sect_name, "np", buff);
    ini.w_u8(sect_name, "flag", flags.get());
}

void SAINode::LoadStream(IReader& F, ESceneAIMapTool* tools)
{
    u32 id;
    u16 pl;
    NodePosition np;
    F.r(&id, 3);
    n1 = (SAINode*)tools->UnpackLink(id);
    F.r(&id, 3);
    n2 = (SAINode*)tools->UnpackLink(id);
    F.r(&id, 3);
    n3 = (SAINode*)tools->UnpackLink(id);
    F.r(&id, 3);
    n4 = (SAINode*)tools->UnpackLink(id);
    pl = F.r_u16();
    pvDecompress(Plane.n, pl);
    F.r(&np, sizeof(np));
    tools->UnpackPosition(Pos, np, tools->m_AIBBox, tools->m_Params);
    Plane.build(Pos, Plane.n);
    flags.assign(F.r_u8());
}

void SAINode::SaveStream(IWriter& F, ESceneAIMapTool* tools)
{
    u32 id;
    u16 pl;
    NodePosition np;

    id = n1 ? (u32)n1->idx : InvalidNode;
    F.w(&id, 3);
    id = n2 ? (u32)n2->idx : InvalidNode;
    F.w(&id, 3);
    id = n3 ? (u32)n3->idx : InvalidNode;
    F.w(&id, 3);
    id = n4 ? (u32)n4->idx : InvalidNode;
    F.w(&id, 3);
    pl = pvCompress(Plane.n);
    F.w_u16(pl);
    tools->PackPosition(np, Pos, tools->m_AIBBox, tools->m_Params);
    F.w(&np, sizeof(np));
    F.w_u8(flags.get());
}

ESceneAIMapTool::ESceneAIMapTool() : ESceneToolBase(OBJCLASS_AIMAP)
{
    m_Shader = 0;
    m_Flags.zero();

    m_AIBBox.invalidate();
    //    m_Header.size_y				= m_Header.aabb.max.y-m_Header.aabb.min.y+EPS_L;
    hash_Initialize();
    m_VisRadius = 30.f;
    m_SmoothHeight = 0.5f;
    m_BrushSize = 1;
    m_CFModel = 0;
}

//----------------------------------------------------

ESceneAIMapTool::~ESceneAIMapTool() {}
//----------------------------------------------------

void ESceneAIMapTool::Clear(bool bOnlyNodes)
{
    inherited::Clear();
    hash_Clear();
    for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
        xr_delete(*it);
    m_Nodes.clear();
    if (!bOnlyNodes)
    {
        m_SnapObjects.clear();
        m_AIBBox.invalidate();
        ExecCommand(COMMAND_REFRESH_SNAP_OBJECTS);
        g_ainode_pool.clear();
    }
}

//----------------------------------------------------

void ESceneAIMapTool::CalculateNodesBBox(Fbox& bb)
{
    bb.invalidate();
    for (AINodeIt b_it = m_Nodes.begin(); b_it != m_Nodes.end(); ++b_it)
    {
        VERIFY(_valid((*b_it)->Pos));
        bb.modify((*b_it)->Pos);
    }
}

//----------------------------------------------------
extern BOOL ai_map_shown;

void ESceneAIMapTool::OnActivate()
{
    inherited::OnActivate();
    ai_map_shown = TRUE;
}

void ESceneAIMapTool::OnFrame()
{
    if (m_Flags.is(flUpdateHL))
    {
        m_Flags.set(flUpdateHL, FALSE);
        for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
            (*it)->flags.set(SAINode::flHLSelected, FALSE);
        for (it = m_Nodes.begin(); it != m_Nodes.end(); it++)
        {
            SAINode& N = **it;
            if (N.flags.is(SAINode::flSelected))
                for (int k = 0; k < 4; k++)
                    if (N.n[k])
                        N.n[k]->flags.set(SAINode::flHLSelected, TRUE);
        }
    }
    if (m_Flags.is(flUpdateSnapList))
        RealUpdateSnapList();
}

//----------------------------------------------------

void ESceneAIMapTool::EnumerateNodes()
{
    u32 idx = 0;
    for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++, idx++)
        (*it)->idx = idx;
}

void ESceneAIMapTool::DenumerateNodes()
{
    u32 cnt = m_Nodes.size();
    for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
    {
        if (!((((u32)(*it)->n1 < cnt) || ((u32)(*it)->n1 == InvalidNode)) &&
                (((u32)(*it)->n2 < cnt) || ((u32)(*it)->n2 == InvalidNode)) &&
                (((u32)(*it)->n3 < cnt) || ((u32)(*it)->n3 == InvalidNode)) &&
                (((u32)(*it)->n4 < cnt) || ((u32)(*it)->n4 == InvalidNode))))
        {
            ELog.Msg(mtError, "Node: has wrong link [%3.2f, %3.2f, %3.2f], {%d,%d,%d,%d}", VPUSH((*it)->Pos), (*it)->n1,
                (*it)->n2, (*it)->n3, (*it)->n4);
            (*it)->n1 = 0;
            (*it)->n2 = 0;
            (*it)->n3 = 0;
            (*it)->n4 = 0;
            continue;
        }
        //                     ,"AINode: Wrong link found.");
        (*it)->n1 = ((u32)(*it)->n1 == InvalidNode) ? 0 : m_Nodes[(u32)(*it)->n1];
        (*it)->n2 = ((u32)(*it)->n2 == InvalidNode) ? 0 : m_Nodes[(u32)(*it)->n2];
        (*it)->n3 = ((u32)(*it)->n3 == InvalidNode) ? 0 : m_Nodes[(u32)(*it)->n3];
        (*it)->n4 = ((u32)(*it)->n4 == InvalidNode) ? 0 : m_Nodes[(u32)(*it)->n4];
        /*
                if (((u32)(*it)->n1<cnt)||((u32)(*it)->n1==InvalidNode)) (*it)->n1	=
           ((u32)(*it)->n1==InvalidNode)?0:m_Nodes[(u32)(*it)->n1];
                else (*it)->n1=0;
                if (((u32)(*it)->n2<cnt)||((u32)(*it)->n2==InvalidNode)) (*it)->n2	=
           ((u32)(*it)->n2==InvalidNode)?0:m_Nodes[(u32)(*it)->n2];
                else (*it)->n2=0;
                if (((u32)(*it)->n3<cnt)||((u32)(*it)->n3==InvalidNode)) (*it)->n3	=
           ((u32)(*it)->n3==InvalidNode)?0:m_Nodes[(u32)(*it)->n3];
                else (*it)->n3=0;
                if (((u32)(*it)->n4<cnt)||((u32)(*it)->n4==InvalidNode)) (*it)->n4	=
           ((u32)(*it)->n4==InvalidNode)?0:m_Nodes[(u32)(*it)->n4];
                else (*it)->n4=0;
        */
    }
}

bool ESceneAIMapTool::LoadLTX(CInifile& ini)
{
    R_ASSERT(0);
    return true;
}

void ESceneAIMapTool::SaveLTX(CInifile& ini, int id)
{
    inherited::SaveLTX(ini, id);

    ini.w_u32("main", "version", AIMAP_VERSION);
    ini.w_u32("main", "flags", m_Flags.get());

    ini.w_fvector3("main", "bbox_min", m_AIBBox.min);
    ini.w_fvector3("main", "bbox_max", m_AIBBox.max);

    ini.w_float("params", "patch_size", m_Params.fPatchSize);
    ini.w_float("params", "test_height", m_Params.fTestHeight);
    ini.w_float("params", "can_up", m_Params.fCanUP);
    ini.w_float("params", "can_down", m_Params.fCanDOWN);

    EnumerateNodes();
    ini.w_u32("main", "nodes_count", m_Nodes.size());

    u32 i = 0;
    string128 buff;
    for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); ++it, ++i)
    {
        sprintf(buff, "n_%d", i);
        (*it)->SaveLTX(ini, buff, this);
    }

    ini.w_float("main", "vis_radius", m_VisRadius);
    ini.w_u32("main", "brush_size", m_BrushSize);

    ini.w_float("main", "smooth_height", m_SmoothHeight);

    for (ObjectIt o_it = m_SnapObjects.begin(); o_it != m_SnapObjects.end(); ++o_it)
        ini.w_string("snap_objects", (*o_it)->Name, NULL);
}

bool ESceneAIMapTool::LoadStream(IReader& F)
{
    inherited::LoadStream(F);

    u16 version = 0;

    R_ASSERT(F.r_chunk(AIMAP_CHUNK_VERSION, &version));
    if (version != AIMAP_VERSION)
    {
        ELog.DlgMsg(mtError, "AIMap: Unsupported version.");
        return false;
    }

    R_ASSERT(F.find_chunk(AIMAP_CHUNK_FLAGS));
    F.r(&m_Flags, sizeof(m_Flags));

    R_ASSERT(F.find_chunk(AIMAP_CHUNK_BOX));
    F.r(&m_AIBBox, sizeof(m_AIBBox));

    R_ASSERT(F.find_chunk(AIMAP_CHUNK_PARAMS));
    F.r(&m_Params, sizeof(m_Params));

    R_ASSERT(F.find_chunk(AIMAP_CHUNK_NODES));
    m_Nodes.resize(F.r_u32());
    for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
    {
        *it = new SAINode();
        (*it)->LoadStream(F, this);
    }
    DenumerateNodes();

    if (F.find_chunk(AIMAP_CHUNK_INTERNAL_DATA))
    {
        m_VisRadius = F.r_float();
        m_BrushSize = F.r_u32();
    }
    if (F.find_chunk(AIMAP_CHUNK_INTERNAL_DATA2))
    {
        m_SmoothHeight = F.r_float();
    }

    // snap objects
    if (F.find_chunk(AIMAP_CHUNK_SNAP_OBJECTS))
    {
        shared_str buf;
        int cnt = F.r_u32();
        if (cnt)
        {
            for (int i = 0; i < cnt; i++)
            {
                F.r_stringZ(buf);
                CCustomObject* O = Scene->FindObjectByName(buf.c_str(), OBJCLASS_SCENEOBJECT);
                if (!O)
                    ELog.Msg(mtError, "AIMap: Can't find snap object '%s'.", buf.c_str());
                else
                    m_SnapObjects.push_back(O);
            }
        }
    }

    hash_FillFromNodes();

    return true;
}

//----------------------------------------------------

bool ESceneAIMapTool::LoadSelection(IReader& F)
{
    Clear();
    return LoadStream(F);
}

void ESceneAIMapTool::OnSynchronize() { RealUpdateSnapList(); }
//----------------------------------------------------

void ESceneAIMapTool::SaveStream(IWriter& F)
{
    inherited::SaveStream(F);

    F.open_chunk(AIMAP_CHUNK_VERSION);
    F.w_u16(AIMAP_VERSION);
    F.close_chunk();

    F.open_chunk(AIMAP_CHUNK_FLAGS);
    F.w(&m_Flags, sizeof(m_Flags));
    F.close_chunk();

    F.open_chunk(AIMAP_CHUNK_BOX);
    F.w(&m_AIBBox, sizeof(m_AIBBox));
    F.close_chunk();

    F.open_chunk(AIMAP_CHUNK_PARAMS);
    F.w(&m_Params, sizeof(m_Params));
    F.close_chunk();

    EnumerateNodes();
    F.open_chunk(AIMAP_CHUNK_NODES);
    F.w_u32(m_Nodes.size());
    for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
        (*it)->SaveStream(F, this);
    F.close_chunk();

    F.open_chunk(AIMAP_CHUNK_INTERNAL_DATA);
    F.w_float(m_VisRadius);
    F.w_u32(m_BrushSize);
    F.close_chunk();

    F.open_chunk(AIMAP_CHUNK_INTERNAL_DATA2);
    F.w_float(m_SmoothHeight);
    F.close_chunk();

    F.open_chunk(AIMAP_CHUNK_SNAP_OBJECTS);
    F.w_u32(m_SnapObjects.size());
    for (ObjectIt o_it = m_SnapObjects.begin(); o_it != m_SnapObjects.end(); o_it++)
        F.w_stringZ((*o_it)->Name);
    F.close_chunk();
}

//----------------------------------------------------

void ESceneAIMapTool::SaveSelection(IWriter& F) { SaveStream(F); }
bool ESceneAIMapTool::Valid() { return !m_Nodes.empty(); }
bool ESceneAIMapTool::IsNeedSave() { return (!m_Nodes.empty() || !m_SnapObjects.empty()); }
void ESceneAIMapTool::OnObjectRemove(CCustomObject* O, bool bDeleting)
{
    if (OBJCLASS_SCENEOBJECT == O->ClassID)
    {
        if (find(m_SnapObjects.begin(), m_SnapObjects.end(), O) != m_SnapObjects.end())
        {
            m_SnapObjects.remove(O);
            RealUpdateSnapList();
        }
    }
}

int ESceneAIMapTool::AddNode(const Fvector& pos, bool bIgnoreConstraints, bool bAutoLink, int sz)
{
    Fvector Pos = pos;
    if (1 == sz)
    {
        SAINode* N = BuildNode(Pos, Pos, bIgnoreConstraints, true);
        if (N)
        {
            N->flags.set(SAINode::flSelected, TRUE);
            if (bAutoLink)
                UpdateLinks(N, bIgnoreConstraints);
            return 1;
        }
        else
        {
            ELog.Msg(mtError, "Can't create node.");
            return 0;
        }
    }
    else
    {
        return BuildNodes(Pos, sz, bIgnoreConstraints);
    }
}

struct invalid_node_pred : public std::unary_function<SAINode*, bool>
{
    int link;

    invalid_node_pred(int _link) : link(_link) { ; }
    bool operator()(const SAINode*& x) { return x->Links() == link; }
};

void ESceneAIMapTool::SelectNodesByLink(int link)
{
    SelectObjects(false);
    // remove link to sel nodes
    for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
        if ((*it)->Links() == link)
            //			if (!(*it)->flags.is(SAINode::flHide))
            (*it)->flags.set(SAINode::flSelected, TRUE);
    UI->RedrawScene();
}

void ESceneAIMapTool::SelectObjects(bool flag)
{
    switch (LTools->GetSubTarget())
    {
    case estAIMapNode:
    {
        for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
            //			if (!(*it)->flags.is(SAINode::flHide))
            (*it)->flags.set(SAINode::flSelected, flag);
    }
    break;
    }
    UpdateHLSelected();
    UI->RedrawScene();
}

struct delete_sel_node_pred : public std::unary_function<SAINode*, bool>
{
    bool operator()(SAINode*& x)
    {
        // breaking links
        for (int k = 0; k < 4; k++)
            if (x->n[k] && x->n[k]->flags.is(SAINode::flSelected))
                x->n[k] = 0;
        // free memory
        bool res = x->flags.is(SAINode::flSelected);
        if (res)
            xr_delete(x);
        return res;
    }
};

void ESceneAIMapTool::RemoveSelection()
{
    switch (LTools->GetSubTarget())
    {
    case estAIMapNode:
    {
        if (m_Nodes.size() == (u32)SelectionCount(true))
        {
            Clear(true);
        }
        else
        {
            SPBItem* pb = UI->ProgressStart(3, "Removing nodes...");
            // remove link to sel nodes
            pb->Inc("erasing nodes");
            // remove sel nodes
            AINodeIt result = std::remove_if(m_Nodes.begin(), m_Nodes.end(), delete_sel_node_pred());
            m_Nodes.erase(result, m_Nodes.end());
            pb->Inc("updating hash");
            hash_Clear();
            hash_FillFromNodes();
            pb->Inc("end");
            UI->ProgressEnd(pb);
        }
    }
    break;
    }
    UpdateHLSelected();
    UI->RedrawScene();
}

void ESceneAIMapTool::InvertSelection()
{
    switch (LTools->GetSubTarget())
    {
    case estAIMapNode:
    {
        for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
            //			if (!(*it)->flags.is(SAINode::flHide))
            (*it)->flags.invert(SAINode::flSelected);
    }
    break;
    }
    UpdateHLSelected();
    UI->RedrawScene();
}

int ESceneAIMapTool::SelectionCount(bool testflag)
{
    int count = 0;
    switch (LTools->GetSubTarget())
    {
    case estAIMapNode:
    {
        for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
            if ((*it)->flags.is(SAINode::flSelected) == testflag)
                count++;
    }
    break;
    }
    return count;
}

void ESceneAIMapTool::FillProp(LPCSTR pref, PropItemVec& items)
{
    PHelper().CreateFlag32(
        items, PrepareKey(pref, "Common\\Draw Nodes"), &m_Flags, flHideNodes, 0, 0, FlagValueCustom::flInvertedDraw);
    PHelper().CreateFlag32(items, PrepareKey(pref, "Common\\Slow Calculate Mode"), &m_Flags, flSlowCalculate);
    PHelper().CreateFloat(items, PrepareKey(pref, "Common\\Visible Radius"), &m_VisRadius, 10.f, 250.f);
    PHelper().CreateFloat(items, PrepareKey(pref, "Common\\Smooth Height"), &m_SmoothHeight, 0.1f, 100.f);

    PHelper().CreateU32(items, PrepareKey(pref, "Params\\Brush Size"), &m_BrushSize, 1, 100);
    PHelper().CreateFloat(items, PrepareKey(pref, "Params\\Can Up"), &m_Params.fCanUP, 0.f, 10.f);
    PHelper().CreateFloat(items, PrepareKey(pref, "Params\\Can Down"), &m_Params.fCanDOWN, 0.f, 10.f);
}

void ESceneAIMapTool::GetBBox(Fbox& bb, bool bSelOnly)
{
    switch (LTools->GetSubTarget())
    {
    case estAIMapNode:
    {
        if (bSelOnly)
        {
            for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
                if ((*it)->flags.is(SAINode::flSelected))
                {
                    bb.modify(Fvector().add((*it)->Pos, -m_Params.fPatchSize * 0.5f));
                    bb.modify(Fvector().add((*it)->Pos, m_Params.fPatchSize * 0.5f));
                }
        }
        else
        {
            bb.merge(m_AIBBox);
        }
    }
    break;
    }
}
