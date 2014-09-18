#include "stdafx.h"
#pragma hdrstop

#include "ESceneAIMapTools.h"

#include "scene.h"
#include "../ECore/Editor/ui_main.h"
#include "ui_leveltools.h"
#include "cl_intersect.h"
#include "MgcAppr3DPlaneFit.h"
#include "sceneobject.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Editor/EditMesh.h"
#include "leftbar.h"
#include "ETools.h"

static SPickQuery	PQ;

IC void SnapXZ	(Fvector&	V, float ps)
{
	V.x = snapto(V.x,ps);
	V.z = snapto(V.z,ps);
}
struct tri	{
	Fvector* v[3];
	Fvector	N;
};

const int	RCAST_MaxTris	= (2*1024);
const int	RCAST_Count		= 4;
const int	RCAST_Total		= (2*RCAST_Count+1)*(2*RCAST_Count+1);
const float	RCAST_Depth		= 1.f;
const float RCAST_VALID 	= 0.55f;

BOOL ESceneAIMapTool::CreateNode(Fvector& vAt, SAINode& N, bool bIC)
{
	// *** Query and cache polygons for ray-casting
	Fvector	PointUp;		PointUp.set(vAt);	PointUp.y	+= RCAST_Depth;		SnapXZ	(PointUp,m_Params.fPatchSize);
	Fvector	PointDown;		PointDown.set(vAt);	PointDown.y	-= RCAST_Depth;		SnapXZ	(PointDown,m_Params.fPatchSize);

	Fbox	BB;				BB.set	(PointUp,PointUp);		BB.grow(m_Params.fPatchSize/2);	// box 1
	Fbox	B2;				B2.set	(PointDown,PointDown);	B2.grow(m_Params.fPatchSize/2);	// box 2
	BB.merge				(B2);

    if (m_CFModel)
    {
    	/*
        for(u32 i=0; i<m_CFModel->get_tris_count(); ++i)
        {
            CDB::TRI* tri = (m_CFModel->get_tris()+i);
            if(tri->material!=0)
            	Msg("non-default material");
        }
        */
    	Scene->BoxQuery(PQ,BB,CDB::OPT_FULL_TEST,m_CFModel);
    }else
    	Scene->BoxQuery(PQ,BB,CDB::OPT_FULL_TEST,GetSnapList());

	DWORD	dwCount 		= PQ.r_count();
	if (dwCount==0){
//		Log("chasm1");
		return FALSE;			// chasm?
	}

	// *** Transfer triangles and compute sector
//	R_ASSERT(dwCount<RCAST_MaxTris);
	static xr_vector<tri> tris;	tris.reserve(RCAST_MaxTris);	tris.clear();
	for (DWORD i=0; i<dwCount; i++)
	{
    	SPickQuery::SResult* R = PQ.r_begin()+i;

        if (R->e_obj&&R->e_mesh)
        {
            CSurface* surf		= R->e_mesh->GetSurfaceByFaceID(R->tag);
//.			SGameMtl* mtl 		= GMLib.GetMaterialByID(surf->_GameMtl());
//.			if (mtl->Flags.is(SGameMtl::flPassable))continue;


            Shader_xrLC* c_sh	= EDevice.ShaderXRLC.Get(surf->_ShaderXRLCName());
            if (!c_sh->flags.bCollision) 			continue;
        }
  /*
		if(m_CFModel)
        {
            u16 mtl_id 	= R->material;

            if(std::find(m_ignored_materials.begin(), m_ignored_materials.end(), mtl_id) != m_ignored_materials.end() )
            {
//.                Msg("--ignore");
                continue;
            }
        }
*/
    	tris.push_back	(tri());
		tri&		D = tris.back();
		Fvector*	V = R->verts;   

		D.v[0]		= &V[0];
		D.v[1]		= &V[1];
		D.v[2]		= &V[2];
		D.N.mknormal(*D.v[0],*D.v[1],*D.v[2]);
		if (D.N.y<=0)	tris.pop_back	();
	}
	if (tris.size()==0){
//		Log("chasm2");
		return FALSE;			// chasm?
	}

	static xr_vector<Fvector>	points;		points.reserve(RCAST_Total); points.clear();
	static xr_vector<Fvector>	normals;	normals.reserve(RCAST_Total);normals.clear();
	Fvector P,D; D.set(0,-1,0);

	float coeff 	= 0.5f*m_Params.fPatchSize/float(RCAST_Count);

	for (int x=-RCAST_Count; x<=RCAST_Count; x++) 
	{
		P.x = vAt.x + coeff*float(x); 
		for (int z=-RCAST_Count; z<=RCAST_Count; z++) {
			P.z = vAt.z + coeff*float(z);
			P.y = vAt.y + 10.f;

			float	tri_min_range	= flt_max;
			int		tri_selected	= -1;
			float	range,u,v;
			for (i=0; i<DWORD(tris.size()); i++){
				if (ETOOLS::TestRayTriA(P,D,tris[i].v,u,v,range,false)){
					if (range<tri_min_range){
						tri_min_range	= range;
						tri_selected	= i;
					}
				}
			}
			if (tri_selected>=0) {
				P.y -= tri_min_range;
				points.push_back(P);
				normals.push_back(tris[tri_selected].N);
			}
		}
	}
	if (points.size()<3) {
//		Msg		("Failed to create node at [%f,%f,%f].",vAt.x,vAt.y,vAt.z);
		return	FALSE;
	}
//.
	float rc_lim = bIC?0.015f:0.7f;
	if (float(points.size())/float(RCAST_Total) < rc_lim) {
//		Msg		("Partial chasm at [%f,%f,%f].",vAt.x,vAt.y,vAt.z);
		return	FALSE;
	}

	// *** Calc normal
	Fvector vNorm;
	vNorm.set(0,0,0);
	for (DWORD n=0; n<normals.size(); n++)
		vNorm.add(normals[n]);
	vNorm.div(float(normals.size()));
	vNorm.normalize();
	/*
	{
		// second algorithm (Magic)
		Fvector N,O;
		N.set(vNorm);
		O.set(points[0]);
		Mgc::OrthogonalPlaneFit(
			points.size(),(Mgc::Vector3*)points.begin(),
			*((Mgc::Vector3*)&O),
			*((Mgc::Vector3*)&N)
		);
		if (N.y<0) N.invert();
		N.normalize();
		vNorm.lerp(vNorm,N,.3f);
		vNorm.normalize();
	}
	*/

 
	// *** Align plane
	Fvector vOffs;
	vOffs.set(0,-1000,0);
	Fplane PL; 	PL.build(vOffs,vNorm);
	for (DWORD p=0; p<points.size(); p++)
	{
		float dist = PL.classify(points[p]);
		if (dist>0) {
			vOffs = points[p];
			PL.build(vOffs,vNorm);
		}
	}

	// *** Create node and register it
	N.Plane.build	(vOffs,vNorm);					// build plane
	D.set			(0,1,0);
	N.Plane.intersectRayPoint(PointDown,D,N.Pos);	// "project" position

	// *** Validate results
	vNorm.set(0,1,0);
	if (vNorm.dotproduct(N.Plane.n)<_cos(deg2rad(60.f)))  return FALSE;

	float y_old = vAt.y;
	float y_new = N.Pos.y;
	if (y_old>y_new) {
		// down
		if (y_old-y_new > m_Params.fCanDOWN ) return FALSE;
	} else {
		// up
		if (y_new-y_old > m_Params.fCanUP	) return FALSE;
	}
 
	// *** Validate plane
	{
		Fvector PLP; D.set(0,-1,0);
		int num_successed_rays = 0;
		for (int x=-RCAST_Count; x<=RCAST_Count; x++) 
		{
			P.x = N.Pos.x + coeff*float(x);
			for (int z=-RCAST_Count; z<=RCAST_Count; z++) {
				P.z = N.Pos.z + coeff*float(z);
				P.y = N.Pos.y;
				N.Plane.intersectRayPoint(P,D,PLP);	// "project" position
				P.y = PLP.y+RCAST_VALID*0.01f;
				
				float	tri_min_range	= flt_max;
				int		tri_selected	= -1;
				float	range,u,v;
				for (i=0; i<tris.size(); i++){
					if (ETOOLS::TestRayTriA(P,D,tris[i].v,u,v,range,false)){
						if (range<tri_min_range){
							tri_min_range	= range;
							tri_selected	= i;
						}
					}
				}
				if (tri_selected>=0){
					if (tri_min_range<RCAST_VALID) num_successed_rays++;
				}
			}
		}
		float perc = float(num_successed_rays)/float(RCAST_Total);
//.		if (!bIC&&(perc < 0.5f)){
		float perc_lim = bIC?0.015f:0.5f;
		if (perc < perc_lim){
			//			Msg		("Floating node.");
			return	FALSE;
		}
	}

	// *** Mask check
	// ???

	return TRUE;
}

void ESceneAIMapTool::hash_Initialize()
{
	for (int i=0; i<=HDIM_X; i++)
		for (int j=0; j<=HDIM_Z; j++){
//			m_HASH[i][j]			= xr_new<AINodeVec>();
			m_HASH[i][j].clear		();
			m_HASH[i][j].reserve	(64);
		}
}

void ESceneAIMapTool::hash_FillFromNodes()
{
	for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++){
    	AINodeVec* V = HashMap((*it)->Pos); R_ASSERT2(V,"AINode position out of bounds.");
        V->push_back	(*it);
    }
}

void ESceneAIMapTool::hash_Clear()
{
	for (int i=0; i<=HDIM_X; i++)
		for (int j=0; j<=HDIM_Z; j++)
//			xr_delete	(m_HASH[i][j]);
			m_HASH[i][j].clear		();
}

void ESceneAIMapTool::HashRect(const Fvector& v, float radius, Irect& result)
{
	Fvector				VMmin,	VMscale, VMeps, scale;

	Fbox&				bb = m_AIBBox;
	VMscale.set			(bb.max.x-bb.min.x, bb.max.y-bb.min.y, bb.max.z-bb.min.z);
	VMmin.set			(bb.min);
	VMeps.set			(float(VMscale.x/HDIM_X/2.f),float(0),float(VMscale.z/HDIM_Z/2.f));
	VMeps.x				= (VMeps.x<EPS_L)?VMeps.x:EPS_L;
	VMeps.y				= (VMeps.y<EPS_L)?VMeps.y:EPS_L;
	VMeps.z				= (VMeps.z<EPS_L)?VMeps.z:EPS_L;
	scale.set			(float(HDIM_X),float(0),float(HDIM_Z));
	scale.div			(VMscale);

	// Hash
	result.x1 			= iFloor((v.x-VMmin.x-radius)*scale.x);	clamp(result.x1,0,HDIM_X);
	result.y1 			= iFloor((v.z-VMmin.z-radius)*scale.z);	clamp(result.y1,0,HDIM_Z);
	result.x2 			= iFloor((v.x-VMmin.x+radius)*scale.x);	clamp(result.x2,0,HDIM_X);
	result.y2 			= iFloor((v.z-VMmin.z+radius)*scale.z);	clamp(result.y2,0,HDIM_Z);
}

AINodeVec* ESceneAIMapTool::HashMap(int ix, int iz)
{
	return (ix<=HDIM_X && iz<=HDIM_Z)?&m_HASH[ix][iz]:0;
}

AINodeVec* ESceneAIMapTool::HashMap(Fvector& V)
{
	// Calculate offset,scale,epsilon
	Fvector				VMmin,	VMscale, VMeps, scale;

	Fbox&				bb = m_AIBBox;
	VMscale.set			(bb.max.x-bb.min.x, bb.max.y-bb.min.y, bb.max.z-bb.min.z);
	VMmin.set			(bb.min);
	VMeps.set			(float(VMscale.x/HDIM_X/2.f),float(0),float(VMscale.z/HDIM_Z/2.f));
	VMeps.x				= (VMeps.x<EPS_L)?VMeps.x:EPS_L;
	VMeps.y				= (VMeps.y<EPS_L)?VMeps.y:EPS_L;
	VMeps.z				= (VMeps.z<EPS_L)?VMeps.z:EPS_L;
	scale.set			(float(HDIM_X),float(0),float(HDIM_Z));
	scale.div			(VMscale);

	// Hash
	DWORD ix,iz;
	ix = iFloor((V.x-VMmin.x)*scale.x);
	iz = iFloor((V.z-VMmin.z)*scale.z);
	return (ix<=HDIM_X && iz<=HDIM_Z)?&m_HASH[ix][iz]:0;
}

SAINode* ESceneAIMapTool::FindNode(Fvector& vAt, float eps)
{
	AINodeVec* V = HashMap(vAt);
    if (!V) return 0;

	for (AINodeIt I=V->begin(); I!=V->end(); I++){
		SAINode* N = *I;
		if (vAt.similar(N->Pos,eps)) return N;
	}
	return 0;
}
 
BOOL ESceneAIMapTool::CanTravel(Fvector _from, Fvector _at)
{
	_at.y		= _from.y;
    
	float eps	= 0.1f;
	float eps_y = m_Params.fPatchSize*1.5f; // * tan(56) = 1.5
	Fvector Result; float radius = m_Params.fPatchSize/_sqrt(2.f);

	// 1
	MotionSimulate(Result,_from,_at,radius,0.7f);
	BOOL b1 = fsimilar(Result.x,_at.x,eps)&&fsimilar(Result.z,_at.z,eps)&&fsimilar(Result.y,_at.y,eps_y);
	if (b1) return TRUE;

	// 2
	MotionSimulate(Result,_from,_at,radius,2.f);
	BOOL b2 = fsimilar(Result.x,_at.x,eps)&&fsimilar(Result.z,_at.z,eps)&&fsimilar(Result.y,_at.y,eps_y);
	if (b2) return TRUE;

	return FALSE;
}

SAINode* ESceneAIMapTool::BuildNode(Fvector& vFrom, Fvector& vAt, bool bIC, bool bSuperIC)	// return node's index
{
	// *** Test if we can travel this path
	SnapXZ			(vAt,m_Params.fPatchSize);

	if (!(bIC||CanTravel(vFrom, vAt)))	return 0;

	// *** set up node
	SAINode N;

	BOOL bRes		= CreateNode(vAt,N,bIC);
    if (!bRes&&bIC&&bSuperIC){
    	Fvector D	= {0,1,0};
		N.Plane.build(vAt,D);					// build plane
		N.Plane.intersectRayPoint(vAt,D,N.Pos);	// "project" position
        bRes		= TRUE;
    }
	if (bRes) {
		//*** check if similar node exists
		SAINode* old = FindNode(N.Pos);
		if (!old){
			// register xr_new<node
            AINodeVec* V 		= HashMap(N.Pos);
            if (V){ 
	        	m_Nodes.push_back	(xr_new<SAINode>(N));
                V->push_back		(m_Nodes.back());
                return m_Nodes.back();
            }else return 0;
		}else{
			// where already was node - return it
			return old;
		}
	}else{ 
    	return 0;
    }
}

int ESceneAIMapTool::BuildNodes(const Fvector& pos, int sz, bool bIC)
{
    // Align emitter
    Fvector			Pos = pos;
    SnapXZ			(Pos,m_Params.fPatchSize);
    Pos.y			+= 1;
    Fvector			Dir; Dir.set(0,-1,0);

	int cnt			= 0;		
    if (m_CFModel)
    	cnt=Scene->RayQuery(PQ,Pos,Dir,3,CDB::OPT_ONLYNEAREST|CDB::OPT_CULL,m_CFModel);
    else
    	cnt=Scene->RayQuery(PQ,Pos,Dir,3,CDB::OPT_ONLYNEAREST|CDB::OPT_CULL,GetSnapList());

    if (0==cnt) {
        ELog.Msg	(mtInformation,"Can't align position.");
        return		0;
    } else {
        Pos.y 		= Pos.y - PQ.r_begin()->range;
    }
		
    // Build first node
    int oldcount 	= m_Nodes.size();
    SAINode* start 	= BuildNode(Pos,Pos,bIC);
    if (!start)		return 0;

    // Estimate nodes
    float estimated_nodes	= (2*sz-1)*(2*sz-1);

	SPBItem* pb 	= 0;
    if (estimated_nodes>1024) pb = UI->ProgressStart(1, "Building nodes...");
    float radius			= sz*m_Params.fPatchSize-EPS_L;
    // General cycle
    for (int k=0; k<(int)m_Nodes.size(); k++){
        SAINode* N 			= m_Nodes[k];
        // left 
        if (0==N->n1){
            Pos.set			(N->Pos);
            Pos.x			-=	m_Params.fPatchSize;
            if (Pos.distance_to(start->Pos)<=radius)
	            N->n1		=	BuildNode(N->Pos,Pos,bIC);
        }
        // fwd
        if (0==N->n2){
            Pos.set			(N->Pos);
            Pos.z			+=	m_Params.fPatchSize;
            if (Pos.distance_to(start->Pos)<=radius)
	            N->n2		=	BuildNode(N->Pos,Pos,bIC);
        }
        // right
        if (0==N->n3){
            Pos.set			(N->Pos);
            Pos.x			+=	m_Params.fPatchSize;
            if (Pos.distance_to(start->Pos)<=radius)
	            N->n3		=	BuildNode(N->Pos,Pos,bIC);
        }
        // back
        if (0==N->n4){
            Pos.set			(N->Pos);
            Pos.z			-=	m_Params.fPatchSize;
            if (Pos.distance_to(start->Pos)<=radius)
	            N->n4		=	BuildNode(N->Pos,Pos,bIC);
        }
        if (estimated_nodes>1024){
            if (k%128==0) {
                float	p1	= float(k)/float(m_Nodes.size());
                float	p2	= float(m_Nodes.size())/estimated_nodes;
                float	p	= 0.1f*p1+0.9f*p2;

                clamp	(p,0.f,1.f);
                pb->Update(p);
                // check need abort && redraw
                if (UI->NeedAbort()) break;
            }
        }
    }
	if (estimated_nodes>1024) UI->ProgressEnd(pb);
    return oldcount-m_Nodes.size();
}

void ESceneAIMapTool::BuildNodes(bool bFromSelectedOnly)
{
	// begin
	m_Nodes.reserve	(1024*1024);

	// Initialize hash
//	hash_Initialize ();

    R_ASSERT(!m_Nodes.empty());
    // Estimate nodes
    Fvector	Pos,LevelSize;
    m_AIBBox.getsize	(LevelSize);
    float estimated_nodes	= (LevelSize.x/m_Params.fPatchSize)*(LevelSize.z/m_Params.fPatchSize);

    SPBItem* pb = UI->ProgressStart(1, "Building nodes...");
    // General cycle
    for (int k=0; k<(int)m_Nodes.size(); k++){
        SAINode* N 			= m_Nodes[k];
    	if (bFromSelectedOnly && !N->flags.is(SAINode::flSelected)) continue;
        // left 
        if (0==N->n1){
            Pos.set			(N->Pos);
            Pos.x			-=	m_Params.fPatchSize;
            N->n1			=	BuildNode(N->Pos,Pos,false);
        }
        // fwd
        if (0==N->n2){
            Pos.set			(N->Pos);
            Pos.z			+=	m_Params.fPatchSize;
            N->n2			=	BuildNode(N->Pos,Pos,false);
        }
        // right
        if (0==N->n3){
            Pos.set			(N->Pos);
            Pos.x			+=	m_Params.fPatchSize;
            N->n3			=	BuildNode(N->Pos,Pos,false);
        }
        // back
        if (0==N->n4){
            Pos.set			(N->Pos);
            Pos.z			-=	m_Params.fPatchSize;
            N->n4			=	BuildNode(N->Pos,Pos,false);
        }
    	if (bFromSelectedOnly){
	        // select neighbour nodes
            if (N->n1) N->n1->flags.set(SAINode::flSelected,TRUE);
            if (N->n2) N->n2->flags.set(SAINode::flSelected,TRUE);
            if (N->n3) N->n3->flags.set(SAINode::flSelected,TRUE);
            if (N->n4) N->n4->flags.set(SAINode::flSelected,TRUE);
        }
        
        if (k%512==0) {
            float	p1	= float(k)/float(m_Nodes.size());
            float	p2	= float(m_Nodes.size())/estimated_nodes;
            float	p	= 0.1f*p1+0.9f*p2;

            clamp	(p,0.f,1.f);
            pb->Update(p);
            // check need abort && redraw
            if (k%32768==0) UI->RedrawScene(true);
            if (UI->NeedAbort()) break;
        }
    }
    UI->ProgressEnd(pb);
}

SAINode* ESceneAIMapTool::GetNode(Fvector vAt, bool bIC)	// return node's index
{
	// *** Test if we can travel this path
	SnapXZ			(vAt,m_Params.fPatchSize);

	// *** set up xr_new<node
	SAINode* N 		= xr_new<SAINode>();
    SAINode* R		= 0;
	if (CreateNode(vAt,*N,bIC)){
		R 			= FindNode(N->Pos);
    	xr_delete	(N);
    }
	xr_delete(N);
    return R;
}

void ESceneAIMapTool::UpdateLinks(SAINode* N, bool bIC)
{
	Fvector 	Pos;
    SAINode* 	D;
    // left 
    {
        Pos.set			(N->Pos);
        Pos.x			-=	m_Params.fPatchSize;
        D				= GetNode(Pos,bIC);
        if (bIC||CanTravel(N->Pos, Pos)) 			N->n1 = D;
        if (D&&(bIC||CanTravel(D->Pos, N->Pos))) 	D->n3 = N;
    }
    // fwd
    {
        Pos.set			(N->Pos);
        Pos.z			+=	m_Params.fPatchSize;
        D				= GetNode(Pos,bIC);
        if (bIC||CanTravel(N->Pos, Pos)) 			N->n2 = D;
        if (D&&(bIC||CanTravel(D->Pos, N->Pos))) 	D->n4 = N;
    }
    // right
    {
        Pos.set			(N->Pos);
        Pos.x			+=	m_Params.fPatchSize;
        D				= GetNode(Pos,bIC);
        if (bIC||CanTravel(N->Pos, Pos)) 			N->n3 = D;
        if (D&&(bIC||CanTravel(D->Pos, N->Pos))) 	D->n1 = N;
    }
    // back
    {
        Pos.set			(N->Pos);
        Pos.z			-=	m_Params.fPatchSize;
        D				= GetNode(Pos,bIC);
        if (bIC||CanTravel(N->Pos, Pos)) 			N->n4 = D;
        if (D&&(bIC||CanTravel(D->Pos, N->Pos))) 	D->n2 = N;
    }
}

bool ESceneAIMapTool::GenerateMap(bool bFromSelectedOnly)
{
	std::sort(m_ignored_materials.begin(),m_ignored_materials.end());
	bool bRes = false;
	if (!GetSnapList()->empty()){
	    if (!RealUpdateSnapList()) return false;
	    if (m_Nodes.empty()){
			ELog.DlgMsg(mtError,"Append at least one node.");
            return false;
        }

        if (!m_Flags.is(flSlowCalculate)){
            // evict resources
            ExecCommand				(COMMAND_EVICT_OBJECTS);
            ExecCommand				(COMMAND_EVICT_TEXTURES);
        
            // prepare collision model
            u32 avg_face_cnt 		= 0;
            u32 avg_vert_cnt 		= 0;
            u32 mesh_cnt		 	= 0;
            Fbox snap_bb;			
            {
                snap_bb.invalidate	();
                for (ObjectIt o_it=m_SnapObjects.begin(); o_it!=m_SnapObjects.end(); o_it++){
                    CSceneObject* 	S = dynamic_cast<CSceneObject*>(*o_it); VERIFY(S);
                    avg_face_cnt	+= S->GetFaceCount();
                    avg_vert_cnt	+= S->GetVertexCount();
                    mesh_cnt	   	+= S->Meshes()->size();
                    Fbox 			bb;
                    S->GetBox		(bb);
                    snap_bb.merge	(bb);
                }
            }

            SPBItem* pb = UI->ProgressStart(mesh_cnt,"Prepare collision model...");

            CDB::Collector* CL		= ETOOLS::create_collector();
            Fvector verts[3];
            for (ObjectIt o_it=m_SnapObjects.begin(); o_it!=m_SnapObjects.end(); o_it++)
            {
                CSceneObject* 		S = dynamic_cast<CSceneObject*>(*o_it); VERIFY(S);
                CEditableObject*    E = S->GetReference(); VERIFY(E);
                EditMeshVec& 		_meshes = E->Meshes();
                for (EditMeshIt m_it=_meshes.begin(); m_it!=_meshes.end(); m_it++)
                {
                    pb->Inc(AnsiString().sprintf("%s [%s]",S->Name,(*m_it)->Name().c_str()).c_str());
                    const SurfFaces&	_sfaces = (*m_it)->GetSurfFaces();
                    for (SurfFaces::const_iterator sp_it=_sfaces.begin(); sp_it!=_sfaces.end(); sp_it++)
                    {
                        CSurface* surf		= sp_it->first;
                        // test passable
    //.			        SGameMtl* mtl 		= GMLib.GetMaterialByID(surf->_GameMtl());
    //.					if (mtl->Flags.is(SGameMtl::flPassable))continue;

                        Shader_xrLC* c_sh	= EDevice.ShaderXRLC.Get(surf->_ShaderXRLCName());
                        if (!c_sh->flags.bCollision) 			continue;
                        // collect tris
                        const IntVec& face_lst 	= sp_it->second;
                        for (IntVec::const_iterator it=face_lst.begin(); it!=face_lst.end(); it++)
                        {
                            E->GetFaceWorld	(S->_Transform(),*m_it,*it,verts);

                            ETOOLS::collector_add_face_d(CL,verts[0],verts[1],verts[2], surf->_GameMtl() /* *it */);
                            if (surf->m_Flags.is(CSurface::sf2Sided))
                                ETOOLS::collector_add_face_d(CL,verts[2],verts[1],verts[0], surf->_GameMtl() /* *it */);
                        }
                    }
                }
            }

            UI->ProgressEnd(pb);

            UI->SetStatus		("Building collision model...");
            // create CFModel
            m_CFModel 			= ETOOLS::create_model_cl(CL);
            ETOOLS::destroy_collector(CL);
    	}

        // building
        Scene->lock			();
CTimer tm;
tm.Start();
        BuildNodes			(bFromSelectedOnly);
tm.GetElapsed_sec();
        Scene->unlock		();
//.        Log("-test time: ",	g_tm.GetElapsed_sec());
		Log("-building time: ",tm.GetElapsed_sec());
//.        Msg("-Rate: %3.2f Count: %d",(g_tm.GetElapsed_sec()/tm.GetElapsed_sec())*100.f,g_tm.count);

        // unload CFModel
		ETOOLS::destroy_model(m_CFModel);

        Scene->UndoSave		();
        bRes = true;

        UI->SetStatus		("");
    }else{
    	ELog.DlgMsg(mtError,"Fill snap list before generating slots!");
    }
    return bRes;
}

int ESceneAIMapTool::RemoveOutOfBoundsNodes()
{
	int count = 0;
	for (int k=0; k<(int)m_Nodes.size(); k++){
    	SAINode* N 		= m_Nodes[k];
    	AINodeVec* V 	= HashMap(N->Pos);
        if (!V){
        	m_Nodes.erase(m_Nodes.begin()+k);
            k--;
            count++;
        }
    }
    return count;
}

bool ESceneAIMapTool::RealUpdateSnapList()
{
	m_Flags.set					(flUpdateSnapList,FALSE);
	fraLeftBar->UpdateSnapList	();
    Fbox nodes_bb;				CalculateNodesBBox(nodes_bb);
	if (!GetSnapList()->empty()){
        Fbox bb,snap_bb;		Scene->GetBox(snap_bb,*GetSnapList());
        if (nodes_bb.is_valid()) bb.merge(snap_bb,nodes_bb); else bb.set(snap_bb);
        if (!m_AIBBox.similar(bb)){
            m_AIBBox.set		(bb);
            hash_Clear		   	();
		    hash_FillFromNodes 	();
        }
	    return true;
    }else{
    	m_AIBBox.set			(nodes_bb);
        hash_Clear		   		();
        hash_FillFromNodes 		();
        return false;
    }
}

void ESceneAIMapTool::RemoveLinks()
{
	for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++)
    	if ((*it)->flags.is(SAINode::flSelected)){
            for (int k=0; k<4; k++) 
		    	if ((*it)->n[k]&&(*it)->n[k]->flags.is(SAINode::flSelected))
        	    	(*it)->n[k] = 0;
        }
    UpdateHLSelected	();
}

static const int opposite[4]={2,3,0,1};
static const u8 fl[4]		={SAINode::flN1,SAINode::flN2,SAINode::flN3,SAINode::flN4};
void ESceneAIMapTool::InvertLinks()
{
	for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++)
    	if ((*it)->flags.is(SAINode::flSelected))
        	for (int k=0; k<4; k++)
                if ((*it)->n[k]&&(*it)->n[k]->flags.is(SAINode::flSelected)&&!(*it)->flags.is(fl[k])){
                    if (0==(*it)->n[k]->n[opposite[k]]){ 
                        (*it)->n[k]->n[opposite[k]] 		= (*it);
                        (*it)->n[k]->flags.set(fl[opposite[k]],TRUE);
                        (*it)->n[k]	= 0;
                    }
                }
    // reset processing flag
	for (AINodeIt a_it=m_Nodes.begin(); a_it!=m_Nodes.end(); a_it++)
		(*a_it)->flags.set(SAINode::flN1|SAINode::flN2|SAINode::flN3|SAINode::flN4,FALSE);
    UpdateHLSelected	();
}

SAINode* ESceneAIMapTool::FindNeighbor(SAINode* N, int side, bool bIgnoreConstraints)
{
	Fvector Pos;
	Pos.set			(N->Pos);
    SnapXZ			(Pos,m_Params.fPatchSize);
    switch (side){
	case 0: Pos.x -= m_Params.fPatchSize; break;
	case 1: Pos.z += m_Params.fPatchSize; break;
	case 2: Pos.x += m_Params.fPatchSize; break;
	case 3: Pos.z -= m_Params.fPatchSize; break;
    }
	AINodeVec* nodes = HashMap(Pos);
    SAINode* R		= 0;
    if (nodes)
		if (bIgnoreConstraints){
		    float dy= flt_max;
            for (AINodeIt I=nodes->begin(); I!=nodes->end(); I++)
                if (fsimilar((*I)->Pos.x,Pos.x,EPS_L)&&fsimilar((*I)->Pos.z,Pos.z,EPS_L)){ 
                    float _dy = _abs((*I)->Pos.y-Pos.y);
                    if (_dy<dy){dy=_dy; R=*I;}
                }
        }else{
		    SAINode* R_up	= 0;
		    SAINode* R_down	= 0;
		    float dy_up 	= flt_max;
		    float dy_down 	= flt_max;
            for (AINodeIt I=nodes->begin(); I!=nodes->end(); I++){
                if (fsimilar((*I)->Pos.x,Pos.x,EPS_L)&&fsimilar((*I)->Pos.z,Pos.z,EPS_L)){ 
                    float _dy = (*I)->Pos.y-Pos.y;
                    float _ady= _abs(_dy);
                    if (_dy>=0.f){
                    	if ((_ady<m_Params.fCanUP)&&(_ady<dy_up))		{dy_up=_ady; R_up=*I;}
                    }else{
                    	if ((_ady<m_Params.fCanDOWN)&&(_ady<dy_down))	{dy_down=_ady; R_down=*I;}
                    }
                }
            }
            if (dy_down<=dy_up)	R = R_down;
            else			  	R = R_up;
        }
    return R;
}

void ESceneAIMapTool::MakeLinks(u8 side_flag, EMode mode, bool bIgnoreConstraints)
{
	if (!side_flag) return;
	for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++){
    	SAINode* T 						= *it;
    	if ((*it)->flags.is(SAINode::flSelected)){
        	for (int k=0; k<4; k++){
            	if (!(side_flag&fl[k])) continue;
            	switch (mode){
                case mdAppend:{ 
                    SAINode* S 				= FindNeighbor(T,k,bIgnoreConstraints);
                    if (S&&S->flags.is(SAINode::flSelected)) T->n[k] = S;
                }break;
                case mdRemove:{ 
                    SAINode* S 				= FindNeighbor(T,k,bIgnoreConstraints);
                    if (S&&S->flags.is(SAINode::flSelected)) T->n[k] = 0;
                }break;
                case mdInvert:{ 
                    SAINode* S 				= FindNeighbor(T,k,bIgnoreConstraints);
                    if (S){
                    	if (!T->flags.is(fl[k])){ 
                            if (T->n[k]&&S->n[opposite[k]]) continue;
                            SAINode* a			= T->n[k];
                            T->n[k] 			= S->n[opposite[k]];
                            S->n[opposite[k]]	= a;
                        }
	                    S->flags.set(fl[opposite[k]],TRUE);
                    }
                }break;
                }
            }
        }
    }
    // reset processing flag
	for (AINodeIt a_it=m_Nodes.begin(); a_it!=m_Nodes.end(); a_it++)
		(*a_it)->flags.set(SAINode::flN1|SAINode::flN2|SAINode::flN3|SAINode::flN4,FALSE);
    UpdateHLSelected	();
}

void ESceneAIMapTool::ResetNodes()
{
    SPBItem* pb = UI->ProgressStart(m_Nodes.size(), "Smoothing nodes...");

    int	n_cnt	= 0;
    
	for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++){
		SAINode& 	N 		= **it;
		if (N.flags.is(SAINode::flSelected)){
        	n_cnt++;
            N.Plane.build(N.Pos,Fvector().set(0,1,0));
        }
    }
    UI->ProgressEnd(pb);
	if (n_cnt) 		Scene->UndoSave();
}

#define		merge(pt)	if (fsimilar(P.y,REF.y,m_SmoothHeight)) { c++; pt.add(P); }
void ESceneAIMapTool::SmoothNodes()
{
    SPBItem* pb = UI->ProgressStart(m_Nodes.size(), "Smoothing nodes...");

	AINodeVec	smoothed;	smoothed.reserve(m_Nodes.size());
	U8Vec		mark;		mark.assign		(m_Nodes.size(),0);

    int	sm_nodes=0;
    
    EnumerateNodes			();
	for (AINodeIt it=m_Nodes.begin(); it!=m_Nodes.end(); it++){
		SAINode& 	N 		= **it;
        Fvector		P1,P2,P3,P4,P,REF;
        int			c;

		if (N.flags.is(SAINode::flSelected)){
        	sm_nodes++;
        
            // smooth point LF
            {
                bool	bCorner	= false;

                c=1;	N.PointLF(REF,m_Params.fPatchSize);	P1.set(REF);
                if (N.nLeft()) {
                    SAINode& L = *N.nLeft();

                    L.PointFR(P,m_Params.fPatchSize);	merge(P1);
                    if (L.nForward()) {
                        bCorner = true;
                        SAINode& C = *L.nForward();

                        C.PointRB(P,m_Params.fPatchSize);	merge(P1);
                    }
                }
                if (N.nForward()) {
                    SAINode& F = *N.nForward();

                    F.PointBL(P,m_Params.fPatchSize);	merge(P1);
                    if ((!bCorner) && F.nLeft()) {
                        bCorner = true;

                        SAINode& C = *F.nLeft();
                        C.PointRB(P,m_Params.fPatchSize);	merge(P1);
                    }
                }
                R_ASSERT(c<=4);
                P1.div(float(c));
            }

            // smooth point FR
            {
                bool	bCorner = false;

                c=1;	N.PointFR(REF,m_Params.fPatchSize); P2.set(REF);
                if (N.nForward()) {
                    SAINode& F = *N.nForward();

                    F.PointRB(P,m_Params.fPatchSize);	merge(P2);
                    if (F.nRight()) {
                        bCorner = true;
                        SAINode& C = *F.nRight();

                        C.PointBL(P,m_Params.fPatchSize);	merge(P2);
                    }
                }
                if (N.nRight()) {
                    SAINode& R = *N.nRight();

                    R.PointLF(P,m_Params.fPatchSize);	merge(P2);
                    if ((!bCorner) && R.nForward()) {
                        bCorner = true;

                        SAINode& C = *R.nForward();
                        C.PointBL(P,m_Params.fPatchSize);	merge(P2);
                    }
                }
                R_ASSERT(c<=4);
                P2.div(float(c));
            }

            // smooth point RB
            {
                bool	bCorner = false;

                c=1;	N.PointRB(REF,m_Params.fPatchSize); P3.set(REF);
                if (N.nRight()) {
                    SAINode& R = *N.nRight();

                    R.PointBL(P,m_Params.fPatchSize);	merge(P3);
                    if (R.nBack()) {
                        bCorner = true;
                        SAINode& C = *R.nBack();

                        C.PointLF(P,m_Params.fPatchSize);	merge(P3);
                    }
                }
                if (N.nBack()) {
                    SAINode& B = *N.nBack();

                    B.PointFR(P,m_Params.fPatchSize);	merge(P3);
                    if ((!bCorner) && B.nRight()) {
                        bCorner = true;

                        SAINode& C = *B.nRight();
                        C.PointLF(P,m_Params.fPatchSize);	merge(P3);
                    }
                }
                R_ASSERT(c<=4);
                P3.div(float(c));
            }

            // smooth point BL
            {
                bool	bCorner = false;

                c=1;	N.PointBL(REF,m_Params.fPatchSize); P4.set(REF);
                if (N.nBack()) {
                    SAINode& B = *N.nBack();

                    B.PointLF(P,m_Params.fPatchSize);	merge(P4);
                    if (B.nLeft()) {
                        bCorner = true;
                        SAINode& C = *B.nLeft();

                        C.PointFR(P,m_Params.fPatchSize);	merge(P4);
                    }
                }
                if (N.nLeft()) {
                    SAINode& L = *N.nLeft();

                    L.PointRB(P,m_Params.fPatchSize);	merge(P4);
                    if ((!bCorner) && L.nBack()) {
                        bCorner = true;

                        SAINode& C = *L.nBack();
                        C.PointFR(P,m_Params.fPatchSize);	merge(P4);
                    }
                }
                R_ASSERT(c<=4);
                P4.div(float(c));
            }

            // align plane
            Fvector data[4]; data[0]=P1; data[1]=P2; data[2]=P3; data[3]=P4;
            Fvector vOffs,vNorm,D;
            vNorm.set(N.Plane.n);
            vOffs.set(N.Pos);
            Mgc::OrthogonalPlaneFit(
                4,(Mgc::Vector3*)data,
                *((Mgc::Vector3*)&vOffs),
                *((Mgc::Vector3*)&vNorm)
            );
            if (vNorm.y<0) vNorm.invert();
            // create _new node
            SAINode* NEW 	= xr_new<SAINode>(N);
            NEW->n1 		= (SAINode*)(N.n1?N.n1->idx:InvalidNode);
            NEW->n2 		= (SAINode*)(N.n2?N.n2->idx:InvalidNode);
            NEW->n3 		= (SAINode*)(N.n3?N.n3->idx:InvalidNode);
            NEW->n4 		= (SAINode*)(N.n4?N.n4->idx:InvalidNode);
            NEW->Plane.build(vOffs,vNorm);
            D.set			(0,1,0);
            N.Plane.intersectRayPoint(N.Pos,D,NEW->Pos);	// "project" position
            smoothed.push_back	(NEW);
        }else{
            // create _new node
            SAINode* NEW 	= xr_new<SAINode>(N);
            NEW->n1 		= (SAINode*)(N.n1?N.n1->idx:InvalidNode);
            NEW->n2 		= (SAINode*)(N.n2?N.n2->idx:InvalidNode);
            NEW->n3 		= (SAINode*)(N.n3?N.n3->idx:InvalidNode);
            NEW->n4 		= (SAINode*)(N.n4?N.n4->idx:InvalidNode);
            smoothed.push_back	(NEW);
        }

        int k = it-m_Nodes.begin();
        if (k%128==0) {
            pb->Update(k);
            if (UI->NeedAbort()) break;
        }
    }
    UI->ProgressEnd(pb);
    Clear				(true);
    m_Nodes 			= smoothed;
	DenumerateNodes		();
    hash_FillFromNodes	();

    UpdateHLSelected	();
    
	if (sm_nodes) 		Scene->UndoSave();
}

