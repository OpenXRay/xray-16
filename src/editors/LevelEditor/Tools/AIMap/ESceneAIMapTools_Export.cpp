#include "stdafx.h"

const u32 MAX_NODE_BIT_COUNT = 23; // original, todo: replace

void ESceneAIMapTool::UnpackPosition(Fvector &Pdest, const NodePosition &Psrc, Fbox &bb, SAIParams &params)
{
    Pdest.x = float(Psrc.x) * params.fPatchSize;
    Pdest.y = (float(Psrc.y) / 65535) * (bb.vMax.y - bb.vMin.y) + bb.vMin.y;
    Pdest.z = float(Psrc.z) * params.fPatchSize;
}

u32 ESceneAIMapTool::UnpackLink(u32 &L)
{
    return L & 0x00ffffff;
}

void ESceneAIMapTool::PackPosition(NodePosition &Dest, Fvector &Src, Fbox &bb, SAIParams &params)
{
    float sp = 1 / params.fPatchSize;
    int px, py, pz;
    px = iFloor(Src.x * sp + EPS_L);
    py = iFloor(65535.f * (Src.y - bb.vMin.y) / (bb.vMax.y - bb.vMin.y) + EPS_L);
    pz = iFloor(Src.z * sp + EPS_L);

    clamp(px, -32767, 32767);
    Dest.x = s16(px);
    clamp(py, 0, 65535);
    Dest.y = u16(py);
    clamp(pz, -32767, 32767);
    Dest.z = s16(pz);
}

bool ESceneAIMapTool::Export(LPCSTR path)
{
    //.?	if (!RealUpdateSnapList()) return false;
    if (!Valid())
        return false;

    // calculate bbox
    Fbox bb;
    CalculateNodesBBox(bb);

    xr_string fn = xr_string(path) + "build.aimap";

    // export
    IWriter *F = FS.w_open(fn.c_str());

    if (F)
    {
        F->open_chunk(E_AIMAP_CHUNK_VERSION);
        F->w_u16(E_AIMAP_VERSION);
        F->close_chunk();

        F->open_chunk(E_AIMAP_CHUNK_BOX);
        F->w(&bb, sizeof(bb));
        F->close_chunk();

        F->open_chunk(E_AIMAP_CHUNK_PARAMS);
        F->w(&m_Params, sizeof(m_Params));
        F->close_chunk();

        EnumerateNodes();
        F->open_chunk(E_AIMAP_CHUNK_NODES);

        u32 nodesCount = m_Nodes.size();
        u32 defaultMaxNodesCount = (u32(1) << u32(MAX_NODE_BIT_COUNT)) - 2;

        if (nodesCount > defaultMaxNodesCount)
        {
            ELog.DlgMsg(mtInformation, "Warning. AI-Map contains %u nodes. Original compiler/game can handle only %u nodes. Extra: %u nodes",
                        nodesCount,
                        defaultMaxNodesCount,
                        nodesCount - defaultMaxNodesCount);
        }

        F->w_u32(nodesCount);

        for (AINodeIt it = m_Nodes.begin(); it != m_Nodes.end(); it++)
        {
            u32 id;
            u16 pl;
            NodePosition np;

            id = (*it)->n1 ? (u32)(*it)->n1->idx : InvalidNode;
            F->w(&id, 3);
            id = (*it)->n2 ? (u32)(*it)->n2->idx : InvalidNode;
            F->w(&id, 3);
            id = (*it)->n3 ? (u32)(*it)->n3->idx : InvalidNode;
            F->w(&id, 3);
            id = (*it)->n4 ? (u32)(*it)->n4->idx : InvalidNode;
            F->w(&id, 3);
            pl = pvCompress((*it)->Plane.n);
            F->w_u16(pl);
            PackPosition(np, (*it)->Pos, bb, m_Params);
            F->w(&np, sizeof(np));
        }
        F->close_chunk();

        FS.w_close(F);
        return true;
    }
    return false;
}
/*
    u32 			id;
    u16 			pl;
    NodePosition 	np;
    F.r				(&id,3); 			n1 = (SAINode*)tools->UnpackLink(id);
    F.r				(&id,3); 			n2 = (SAINode*)tools->UnpackLink(id);
    F.r				(&id,3); 			n3 = (SAINode*)tools->UnpackLink(id);
    F.r				(&id,3); 			n4 = (SAINode*)tools->UnpackLink(id);
    pl				= F.r_u16(); 		pvDecompress(Plane.n,pl);
    F.r				(&np,sizeof(np)); 	tools->UnpackPosition(Pos,np,tools->m_BBox,tools->m_Params);
    Plane.build		(Pos,Plane.n);
*/
