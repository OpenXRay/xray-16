#include "stdafx.h"
#pragma hdrstop

#include "EScenePortalTools.h"
#include "ui_leveltools.h"
#include "FramePortal.h"
#include "EScenePortalControls.h"
#include "portal.h"
#include "Scene.h"
#include "MgcAppr3DPlaneFit.h"

/* TODO 1 -oAlexMX -cTODO: Create tools as AI Map */

void EScenePortalTool::CreateControls()
{
//	inherited::CreateControls();
    AddControl		(xr_new<TUI_ControlPortalSelect>(estDefault,etaSelect,	this));
	// frame
    pFrame 			= xr_new<TfraPortal>((TComponent*)0);
    ((TfraPortal*)pFrame)->tool	= this;
}
//----------------------------------------------------

void EScenePortalTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

void EScenePortalTool::FillProp(LPCSTR pref, PropItemVec& items)
{
	PHelper().CreateFlag32(items, PrepareKey(pref,"Common\\Draw Simple Model"),&m_Flags,			flDrawSimpleModel);
	inherited::FillProp	(pref, items);
}
//----------------------------------------------------

CCustomObject* EScenePortalTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<CPortal>(data,name);
    O->ParentTool		= this;
    return O;
}
//----------------------------------------------------
float plane_dot(const Fplane& P0, const Fplane& P1)
{
	return P0.n.x*P1.n.x + P0.n.y*P1.n.y + P0.n.z*P1.n.z + P0.d*P1.d;
}

bool contains(Fbox P, const Fbox& P0)
{
	if (P.contains(P0))
    	return true;
    else
    {
    	P.grow(0.001f);
        if (P.contains(P0))
        	return true;
		else        
        	return false;
    }
}

struct co_plane_pred
{
	CPortal* 	m_portal;
    Fplane 		m_plane;
	co_plane_pred(CPortal* p):m_portal(p)
    {
        Fvector rkOffset, rkNormal;
        Mgc::OrthogonalPlaneFit(m_portal->m_Vertices.size(), (Mgc::Vector3*)m_portal->m_Vertices.begin(), (Mgc::Vector3&)rkOffset, (Mgc::Vector3&)rkNormal);
        m_plane.build(rkOffset, rkNormal);
		float plane_magn = sqrt(plane_dot(m_plane, m_plane));
        m_plane.n.div(plane_magn);
        m_plane.d /= plane_magn;
    }
    bool operator()(CCustomObject* p)
    {
		CPortal*	P = (CPortal*)p;
        if(m_portal==P)
        	return false;
            
        if(P->m_SectorFront==m_portal->m_SectorFront && P->m_SectorBack==m_portal->m_SectorBack)
        {
            Fvector rkOffset, rkNormal;
    		Fplane 	p_plane;
            Mgc::OrthogonalPlaneFit(P->m_Vertices.size(), (Mgc::Vector3*)P->m_Vertices.begin(), (Mgc::Vector3&)rkOffset, (Mgc::Vector3&)rkNormal);
            p_plane.build(rkOffset, rkNormal);
            float plane_magn = sqrt(plane_dot(p_plane, p_plane));
            p_plane.n.div(plane_magn);
            p_plane.d /= plane_magn;
            
			float dot = _abs(plane_dot(m_plane, p_plane));
            if(fsimilar(dot, 1.0f, EPS_L))
            {
            	Fbox	m_box, p_box;
            	m_portal->GetBox(m_box);
            	P->GetBox(p_box);
				return contains(m_box, p_box);
            }else
            return false;
        }else
        	return false;
    }
};

template<int I> bool int_greater (int val)
{
	return val>I;
}

void EScenePortalTool::RemoveSimilar()
{
    ObjectList& p_lst		= Scene->ListObj(OBJCLASS_PORTAL);
	ObjectList p_lst_del;
    ObjectList p_lst_dup	= p_lst;
    
    ObjectIt	it 			= p_lst_dup.begin();
    ObjectIt	it_e 		= p_lst_dup.end();
    
    while(it!=p_lst_dup.end())
    {
        CPortal* p				= (CPortal*)(*it);
    	if( std::find(p_lst_del.begin(),p_lst_del.end(),*it)!=p_lst_del.end() )
        {
        	++it;
            continue;
        }
        ObjectIt pit 		= std::find_if(p_lst.begin(), p_lst.end(), co_plane_pred(p));
        if(pit!=p_lst.end())
        {
			p_lst_del.push_back	(*pit);
            p_lst.erase			(pit);
            it 					= p_lst_dup.begin();
        }else
        	++it;
    }
    
   	for(ObjectIt p_del=p_lst_del.begin(); p_del!=p_lst_del.end(); ++p_del)
    {
        xr_delete(*p_del);
    }
	ELog.DlgMsg(mtInformation,"Removed '%d' portal(s).",p_lst_del.size());
    p_lst_del.clear();
    
 }
