//----------------------------------------------------
// file: StaticMesh.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "EditMesh.h"
#include "EditObject.h"
#include "cl_collector.h"
#include "ui_main.h"
#include "pick_defs.h"
#include "../ETools/ETools.h"

/*
void CEditableMesh::CHullPickFaces(PlaneVec& pl, Fmatrix& parent, U32Vec& fl){
	u32 i=0;
	Fvector p;
    vector<bool> inside(m_Points.size(),true);
    for(FvectorIt v_it=m_Points.begin();v_it!=m_Points.end();v_it++){
        parent.transform_tiny(p,*v_it);
        for(PlaneIt p_it=pl.begin(); p_it!=pl.end(); p_it++)
        	if (p_it->classify(p)>EPS_L) { inside[v_it-m_Points.begin()]=false; break; }
    }
    for(FaceIt f_it=m_Faces.begin();f_it!=m_Faces.end();f_it++,i++)
    	if (inside[f_it->pv[0].pindex]&&inside[f_it->pv[1].pindex]&&inside[f_it->pv[2].pindex]) fl.push_back(i);
}
*/
//----------------------------------------------------

static IntVec		sml_processed;
static Fvector		sml_normal;
static float		m_fSoftAngle;
//----------------------------------------------------

//----------------------------------------------------
// номер face должен соответствовать списку
//----------------------------------------------------
void CEditableMesh::GenerateCFModel()
{
	UnloadCForm		();
	// Collect faces
	CDB::Collector* CL = ETOOLS::create_collector();
	// double sided
	for (SurfFacesPairIt sp_it=m_SurfFaces.begin(); sp_it!=m_SurfFaces.end(); sp_it++){
		IntVec& face_lst = sp_it->second;
		for (IntIt it=face_lst.begin(); it!=face_lst.end(); it++){
			st_Face&	F = m_Faces[*it];
            ETOOLS::collector_add_face_d(CL,m_Vertices[F.pv[0].pindex],m_Vertices[F.pv[1].pindex],m_Vertices[F.pv[2].pindex], *it);
			if (sp_it->first->m_Flags.is(CSurface::sf2Sided))
				ETOOLS::collector_add_face_d(CL,m_Vertices[F.pv[2].pindex],m_Vertices[F.pv[1].pindex],m_Vertices[F.pv[0].pindex], *it);
		}
	}
	m_CFModel 		= ETOOLS::create_model_cl(CL);
	ETOOLS::destroy_collector(CL);
}

void CEditableMesh::RayQuery(SPickQuery& pinf)
{
    if (!m_CFModel) GenerateCFModel();
//*
	ETOOLS::ray_query	(m_CFModel, pinf.m_Start, pinf.m_Direction, pinf.m_Dist);
	for (int r=0; r<ETOOLS::r_count(); r++)
		pinf.append		(ETOOLS::r_begin()+r,m_Parent,this);
/*
	XRC.ray_query	(m_CFModel, pinf.m_Start, pinf.m_Direction, pinf.m_Dist);
    for (int r=0; r<XRC.r_count(); r++)
        pinf.append	(XRC.r_begin()+r,m_Parent,this);
//*/
}

void CEditableMesh::RayQuery(const Fmatrix& parent, const Fmatrix& inv_parent, SPickQuery& pinf)
{
    if (!m_CFModel) GenerateCFModel();
//*
	ETOOLS::ray_query_m	(inv_parent, m_CFModel, pinf.m_Start, pinf.m_Direction, pinf.m_Dist);
	for (int r=0; r<ETOOLS::r_count(); r++)
		pinf.append_mtx(parent,ETOOLS::r_begin()+r,m_Parent,this);
/*
	XRC.ray_query	(inv_parent, m_CFModel, pinf.m_Start, pinf.m_Direction, pinf.m_Dist);
    for (int r=0; r<XRC.r_count(); r++)
        pinf.append_mtx(parent,XRC.r_begin()+r,m_Parent,this);
//*/
}

void CEditableMesh::BoxQuery(const Fmatrix& parent, const Fmatrix& inv_parent, SPickQuery& pinf)
{
    if (!m_CFModel) GenerateCFModel();
    ETOOLS::box_query_m(inv_parent, m_CFModel, pinf.m_BB);
    for (int r=0; r<ETOOLS::r_count(); r++)
        pinf.append_mtx(parent,ETOOLS::r_begin()+r,m_Parent,this);
}

static const float _sqrt_flt_max = _sqrt(flt_max*0.5f);

bool CEditableMesh::RayPick(float& distance, const Fvector& start, const Fvector& direction, const Fmatrix& inv_parent, SRayPickInfo* pinf)
{
	if (!m_Flags.is(flVisible)) return false;

    if (!m_CFModel) GenerateCFModel();
//.	float m_r 		= pinf?pinf->inf.range+EPS_L:UI->ZFar();// (bugs: не всегда выбирает) //S ????

	ETOOLS::ray_options	(CDB::OPT_ONLYNEAREST | CDB::OPT_CULL);
	ETOOLS::ray_query_m	(inv_parent, m_CFModel, start, direction, _sqrt_flt_max);

    if (ETOOLS::r_count()){
		CDB::RESULT* I	= ETOOLS::r_begin	();
		if (I->range<distance) {
	        if (pinf){
            	pinf->SetRESULT	(m_CFModel,I);
    	        pinf->e_obj 	= m_Parent;
        	    pinf->e_mesh	= this;
	            pinf->pt.mul	(direction,pinf->inf.range);
    	        pinf->pt.add	(start);
            }
            distance = I->range;
            return true;
		}
    }
	return false;
}
//----------------------------------------------------
#ifdef _EDITOR
bool CEditableMesh::CHullPickMesh(PlaneVec& pl, const Fmatrix& parent)
{
	Fvector p;
    boolVec inside(m_VertCount,true);
    for(u32 v_id=0;v_id<m_VertCount;v_id++){
        parent.transform_tiny(p,m_Vertices[v_id]);
        for(PlaneIt p_it=pl.begin(); p_it!=pl.end(); p_it++)
        	if (p_it->classify(p)>EPS_L) { inside[v_id]=false; break; }
    }
    for(u32 f_id=0;f_id<m_FaceCount;f_id++)
    	if (inside[m_Faces[f_id].pv[0].pindex]&&inside[m_Faces[f_id].pv[1].pindex]&&inside[m_Faces[f_id].pv[2].pindex]) return true;
    return false;
}
//----------------------------------------------------

void CEditableMesh::RecurseTri(int id)
{
	// Check if triangle already processed
	if (std::find(sml_processed.begin(),sml_processed.end(),id)!=sml_processed.end())
		return;

	sml_processed.push_back(id);

    // recurse
    for (int k=0; k<3; k++){
	    IntVec& PL = (*m_Adjs)[m_Faces[id].pv[k].pindex];
		for (IntIt pl_it=PL.begin(); pl_it!=PL.end(); pl_it++){
           	Fvector &test_normal = m_FaceNormals[*pl_it];
           	float cosa = test_normal.dotproduct(sml_normal);
           	if (cosa<m_fSoftAngle) continue;
     		RecurseTri(*pl_it);
        }
    }
}
//----------------------------------------------------

void CEditableMesh::GetTiesFaces(int start_id, U32Vec& fl, float fSoftAngle, bool bRecursive)
{
    R_ASSERT(start_id<int(m_FaceCount));
    m_fSoftAngle=cosf(deg2rad(fSoftAngle));
    GenerateFNormals	();
	GenerateAdjacency	();
    VERIFY				(m_FaceNormals);
    if (bRecursive){
    	sml_processed.clear();
        sml_normal.set(m_FaceNormals[start_id]);
    	RecurseTri(start_id);
        fl.insert(fl.begin(),sml_processed.begin(),sml_processed.end());
    }else{
    	for (int k=0; k<3; k++)
        	fl.insert(fl.end(),(*m_Adjs)[m_Faces[start_id].pv[k].pindex].begin(),(*m_Adjs)[m_Faces[start_id].pv[k].pindex].end());
        std::sort(fl.begin(),fl.end());
        fl.erase(std::unique(fl.begin(),fl.end()),fl.end());
    }
    UnloadFNormals		();
	UnloadAdjacency		();
}
//----------------------------------------------------

bool CEditableMesh::BoxPick(const Fbox& box, const Fmatrix& inv_parent, SBoxPickInfoVec& pinf)
{
    if (!m_CFModel) GenerateCFModel();

    ETOOLS::box_query_m(inv_parent, m_CFModel, box);
    if (ETOOLS::r_count()){
    	pinf.push_back(SBoxPickInfo());
		pinf.back().e_obj 	= m_Parent;
	    pinf.back().e_mesh	= this;
	    for (CDB::RESULT* I=ETOOLS::r_begin(); I!=ETOOLS::r_end(); I++) pinf.back().AddRESULT(m_CFModel,I);
        return true;
    }
    return false;
}
//----------------------------------------------------

bool CEditableMesh::FrustumPick(const CFrustum& frustum, const Fmatrix& parent)
{
	if (!m_Flags.is(flVisible)) return false;

	Fvector p[3];
	for(u32 i=0;i<m_FaceCount;i++){
		for( int k=0;k<3;k++)
            parent.transform_tiny(p[k],m_Vertices[m_Faces[i].pv[k].pindex]);
		if (frustum.testPolyInside(p,3)) return true;
	}
	return false;
}
//---------------------------------------------------------------------------

void CEditableMesh::FrustumPickFaces(const CFrustum& frustum, const Fmatrix& parent, U32Vec& fl)
{
	if (!m_Flags.is(flVisible)) return;

	Fvector p[3];
    bool bCulling=EPrefs->bp_cull;
	for(u32 p_id=0;p_id<m_FaceCount;p_id++){
        for( int k=0;k<3;++k)
        	parent.transform_tiny(p[k],m_Vertices[m_Faces[p_id].pv[k].pindex]);

        if (bCulling){
	        Fplane P; P.build(p[0],p[1],p[2]);
    	    if (P.classify(EDevice.m_Camera.GetPosition())<0) continue;
        }
        if (frustum.testPolyInside(p,3))
            fl.push_back(p_id);
    }
}
#endif //



