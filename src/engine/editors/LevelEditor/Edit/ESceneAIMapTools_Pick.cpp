#include "stdafx.h"
#pragma hdrstop

#include "ESceneAIMapTools.h"                            
#include "scene.h"
#include "../ECore/Editor/ui_main.h"
#include "ui_leveltools.h"
#include "ESceneAIMapControls.h"   
#include "ui_levelmain.h"

SAINode* ESceneAIMapTool::PickNode(const Fvector& start, const Fvector& dir, float& dist)
{
	SAINode* R 	= 0;
    CFrustum frustum;
    LUI->m_CurrentCp.add(1); 			// fake add min vaalue for calculate frustum 
    float psz	= (m_Params.fPatchSize/2)*(m_Params.fPatchSize/2);
    if (LUI->SelectionFrustum(frustum)){
        for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++){
            SAINode& N 	= **it;
            u32 mask 	= 0xffff;
            Fbox bb; bb.set(N.Pos,N.Pos); bb.min.sub(m_Params.fPatchSize*0.35f); bb.max.add(m_Params.fPatchSize*0.35f);
            if (frustum.testSAABB(N.Pos,m_Params.fPatchSize,bb.data(),mask)){
                Fvector dest;
                if (N.Plane.intersectRayPoint(start,dir,dest)){
                    if (N.Pos.distance_to_sqr(dest)<psz){
                        float d = start.distance_to(dest);
                        if (d<dist){
                            R 	= &N;
                            dist= d;
                        }
                    }
                }
            }
        }
    }
    return R;
}
bool ESceneAIMapTool::PickObjects(Fvector& dest, const Fvector& start, const Fvector& dir, float dist)
{         	         
	SPickQuery	PQ;
	if (!GetSnapList()->empty()){
        if (Scene->RayQuery(PQ,start,dir,dist,CDB::OPT_ONLYNEAREST|CDB::OPT_CULL,GetSnapList())){
            dest.mad(start,dir,PQ.r_begin()->range);
            return true;
        }
    }else{
    	ELog.DlgMsg(mtInformation,"Fill object list and try again.");
    }
    return false;
}
                             
int ESceneAIMapTool::RaySelect(int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly)
{
	int count=0;
	if (!m_Flags.is(flHideNodes)){
        switch (LTools->GetSubTarget()){
        case estAIMapNode:{
            SAINode * N = PickNode(start, direction, distance);
            if (N&&!bDistanceOnly){ 
                if (flag==-1) 	N->flags.invert(SAINode::flSelected); 
                N->flags.set	(SAINode::flSelected,flag); 
                count++;
            }
        }break;
        }
        UpdateHLSelected	();
        UI->RedrawScene		();
    }
    return count;
}

int ESceneAIMapTool::FrustumSelect(int flag, const CFrustum& frustum)
{
    int count = 0;
	if (!m_Flags.is(flHideNodes)){
        switch (LTools->GetSubTarget()){
        case estAIMapNode:{
            for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++){
                SAINode& N 	= **it;
                u32 mask 	= 0xffff;
                Fbox bb; bb.set(N.Pos,N.Pos); bb.min.sub(m_Params.fPatchSize*0.35f); bb.max.add(m_Params.fPatchSize*0.35f);
                if (frustum.testSAABB(N.Pos,m_Params.fPatchSize,bb.data(),mask)){
                    if (-1==flag)	(*it)->flags.invert(SAINode::flSelected);
                    else			(*it)->flags.set(SAINode::flSelected,flag);
                    count++;
                }
            }
        }break;
        }
    
        UpdateHLSelected	();
        UI->RedrawScene		();
 	}
    return count;
}


