//----------------------------------------------------
// file: EditMeshModify.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "EditMesh.h"
#include "EditObject.h"
//----------------------------------------------------
void CEditableMesh::Transform(const Fmatrix& parent)
{
	// transform position
	for(u32 k=0; k<m_VertCount; ++k)
		parent.transform_tiny(m_Vertices[k]);

    // RecomputeBBox
	RecomputeBBox	();
    // update normals & cform
#ifdef _EDITOR
	UnloadRenderBuffers	();
	UnloadCForm		();
#endif
    UnloadFNormals	(true);
    UnloadVNormals	(true);
    UnloadSVertices	(true);
}
//----------------------------------------------------

int CEditableMesh::FindSimilarUV(st_VMap* vmap, Fvector2& _uv)
{
	int sz			= vmap->size();
	for (int k=0; k<sz; ++k)
	{
		const Fvector2& uv = vmap->getUV(k);
		if (uv.similar(_uv)) 
			return k;
	}
	return -1;
}

int CEditableMesh::FindSimilarWeight(st_VMap* vmap, float _w)
{
	int sz			= vmap->size();
	for (int k=0; k<sz; ++k)
	{
		float w		= vmap->getW(k);
		if (fsimilar(w,_w)) return k;
	}
	return -1;
}

void CEditableMesh::RebuildVMaps()
{
//.	Log			("Rebuilding VMaps...");
	IntVec		m_VertVMap;
	m_VertVMap.resize(m_VertCount,-1);
	VMapVec		nVMaps;
	VMRefsVec	nVMRefs;
	// refs copy to new
	{
		nVMRefs.resize(m_VMRefs.size());
		for (VMRefsIt o_it=m_VMRefs.begin(),n_it=nVMRefs.begin(); o_it!=m_VMRefs.end(); o_it++,n_it++){
			n_it->count	= o_it->count;
			n_it->pts	= xr_alloc<st_VMapPt>(n_it->count);
		}
	}

	for (u32 f_id=0; f_id<m_FaceCount; f_id++){
		st_Face& F=m_Faces[f_id];
		for (int k=0; k<3; k++){
			u32 pts_cnt			= m_VMRefs[F.pv[k].vmref].count;
			for (u32 pt_id=0; pt_id<pts_cnt; pt_id++){
                st_VMapPt* n_pt_it	= &nVMRefs[F.pv[k].vmref].pts[pt_id];
                st_VMapPt* o_pt_it	= &m_VMRefs[F.pv[k].vmref].pts[pt_id];
				st_VMap* vmap=m_VMaps[o_pt_it->vmap_index];
				switch (vmap->type){
				case vmtUV:{
					int& pm=m_VertVMap[F.pv[k].pindex];
					if (-1==pm){ // point map
						pm=F.pv[k].vmref;
						int vm_idx=FindVMapByName(nVMaps,vmap->name.c_str(),vmap->type,false);
						if (-1==vm_idx){
							nVMaps.push_back(xr_new<st_VMap>(vmap->name.c_str(),vmap->type,false));
							vm_idx=nVMaps.size()-1;
						}
						st_VMap* nVMap=nVMaps[vm_idx];

//						int uv_idx = FindSimilarUV(nVMap,vmap->getUV(pt_it->index));
//						if (uv_idx==-1){
//							uv_idx	= nVMap->size();
//							nVMap->appendUV(vmap->getUV(pt_it->index));
//							nVMap->appendVI(F.pv[k].pindex);
//						}

						nVMap->appendUV(vmap->getUV(o_pt_it->index));
						nVMap->appendVI(F.pv[k].pindex);
						n_pt_it->index = nVMap->size()-1;
						n_pt_it->vmap_index=vm_idx;
					}
					else{ // poly map
						int vm_idx=FindVMapByName(nVMaps,vmap->name.c_str(),vmap->type,true);
						if (-1==vm_idx){
							nVMaps.push_back(xr_new<st_VMap>(vmap->name.c_str(),vmap->type,true));
							vm_idx=nVMaps.size()-1;
						}
						st_VMap* nVMapPM=nVMaps[vm_idx];

//						int uv_idx = FindSimilarUV(nVMapPM,vmap->getUV(pt_it->index));
//						if (uv_idx==-1){
//							uv_idx	= nVMapPM->size();
//							nVMapPM->appendUV(vmap->getUV(pt_it->index));
//							nVMapPM->appendVI(F.pv[k].pindex);
//							nVMapPM->appendPI(f_it-m_Faces.begin());
//						}
//						n_pt_it->index = uv_idx;

						nVMapPM->appendUV(vmap->getUV(o_pt_it->index));
						nVMapPM->appendVI(F.pv[k].pindex);
						nVMapPM->appendPI(f_id);
						n_pt_it->index = nVMapPM->size()-1;
						n_pt_it->vmap_index=vm_idx;
					}
				}break;
				case vmtWeight:{
					int vm_idx=FindVMapByName(nVMaps,vmap->name.c_str(),vmap->type,false);
					if (-1==vm_idx){
						nVMaps.push_back(xr_new<st_VMap>(vmap->name.c_str(),vmap->type,false));
						vm_idx=nVMaps.size()-1;
					}
					st_VMap* nWMap=nVMaps[vm_idx];
					nWMap->appendW	(vmap->getW(o_pt_it->index));
					nWMap->appendVI	(F.pv[k].pindex);
					n_pt_it->index	= nWMap->size()-1;
					n_pt_it->vmap_index=vm_idx;
				}break;
				}
			}
		}
	}
	for (VMapIt vm_it=m_VMaps.begin(); vm_it!=m_VMaps.end(); vm_it++)
		xr_delete(*vm_it);

	m_VMaps.clear();
	m_VMaps=nVMaps;
	// clear refs
	for (VMRefsIt ref_it=m_VMRefs.begin(); ref_it!=m_VMRefs.end(); ref_it++)
		xr_free			(ref_it->pts);
	m_VMRefs.clear();
	m_VMRefs = nVMRefs;
}

#define MX 25
#define MY 15
#define MZ 25
static Fvector		VMmin, VMscale;
static U32Vec		VM[MX+1][MY+1][MZ+1];
static Fvector		VMeps;

static FvectorVec	m_NewPoints;
bool CEditableMesh::OptimizeFace(st_Face& face){
	Fvector points[3];
	int mface[3];
	int k;

	for (k=0; k<3; k++){
    	points[k].set(m_Vertices[face.pv[k].pindex]);
		mface[k] = -1;
    }

	// get similar vert idx list
	for (k=0; k<3; k++){
		U32Vec* vl;
		int ix,iy,iz;
		ix = iFloor(float(points[k].x-VMmin.x)/VMscale.x*MX);
		iy = iFloor(float(points[k].y-VMmin.y)/VMscale.y*MY);
		iz = iFloor(float(points[k].z-VMmin.z)/VMscale.z*MZ);
		vl = &(VM[ix][iy][iz]);
		for(U32It it=vl->begin();it!=vl->end(); it++){
			FvectorIt v = m_NewPoints.begin()+(*it);
            if( v->similar(points[k],EPS) )
                mface[k] = *it;
		}
	}
	for(k=0; k<3; k++ ){
		if( mface[k] == -1 ){
			mface[k] = m_NewPoints.size();
			m_NewPoints.push_back( points[k] );
			int ix,iy,iz;
			ix = iFloor(float(points[k].x-VMmin.x)/VMscale.x*MX);
			iy = iFloor(float(points[k].y-VMmin.y)/VMscale.y*MY);
			iz = iFloor(float(points[k].z-VMmin.z)/VMscale.z*MZ);
			VM[ix][iy][iz].push_back(mface[k]);
			int ixE,iyE,izE;
			ixE = iFloor(float(points[k].x+VMeps.x-VMmin.x)/VMscale.x*MX);
			iyE = iFloor(float(points[k].y+VMeps.y-VMmin.y)/VMscale.y*MY);
			izE = iFloor(float(points[k].z+VMeps.z-VMmin.z)/VMscale.z*MZ);
			if (ixE!=ix)
				VM[ixE][iy][iz].push_back(mface[k]);
			if (iyE!=iy)
				VM[ix][iyE][iz].push_back(mface[k]);
			if (izE!=iz)
				VM[ix][iy][izE].push_back(mface[k]);
			if ((ixE!=ix)&&(iyE!=iy))
				VM[ixE][iyE][iz].push_back(mface[k]);
			if ((ixE!=ix)&&(izE!=iz))
				VM[ixE][iy][izE].push_back(mface[k]);
			if ((iyE!=iy)&&(izE!=iz))
				VM[ix][iyE][izE].push_back(mface[k]);
			if ((ixE!=ix)&&(iyE!=iy)&&(izE!=iz))
				VM[ixE][iyE][izE].push_back(mface[k]);
		}
	}

	if ((mface[0]==mface[1])||(mface[1]==mface[2])||(mface[0]==mface[2])){
		Msg("!Optimize: Invalid face found. Removed.");
        return false;
	}else{
    	face.pv[0].pindex = mface[0];
    	face.pv[1].pindex = mface[1];
    	face.pv[2].pindex = mface[2];
        return true;
	}
}

void CEditableMesh::OptimizeMesh(BOOL NoOpt)
{
	if (!NoOpt){
#ifdef _EDITOR
    	UnloadRenderBuffers	();
		UnloadCForm     	();
#endif
        UnloadFNormals   	(true);
        UnloadVNormals   	(true);
       	UnloadSVertices  	(true);
       	UnloadAdjacency		(true);
    	
		// clear static data
		for (int x=0; x<MX+1; x++)
			for (int y=0; y<MY+1; y++)
    			for (int z=0; z<MZ+1; z++)
            		VM[x][y][z].clear();
		VMscale.set(m_Box.max.x-m_Box.min.x+EPS_S, m_Box.max.y-m_Box.min.y+EPS_S, m_Box.max.z-m_Box.min.z+EPS_S);
		VMmin.set(m_Box.min.x, m_Box.min.y, m_Box.min.z);

		VMeps.set(VMscale.x/MX/2,VMscale.y/MY/2,VMscale.z/MZ/2);
		VMeps.x = (VMeps.x<EPS_L)?VMeps.x:EPS_L;
		VMeps.y = (VMeps.y<EPS_L)?VMeps.y:EPS_L;
		VMeps.z = (VMeps.z<EPS_L)?VMeps.z:EPS_L;

		m_NewPoints.clear();
		m_NewPoints.reserve(m_VertCount);
                                                
		boolVec 	faces_mark;
		faces_mark.resize(m_FaceCount,false);
        int			i_del_face 		= 0;
		for (u32 k=0; k<m_FaceCount; k++){
    		if (!OptimizeFace(m_Faces[k])){
				faces_mark[k]		= true;

//. -----in plugin
//.              i_del_face			= 0;

//. -----in editor
                i_del_face			++;
            }
		}

        m_VertCount		= m_NewPoints.size();
        xr_free			(m_Vertices);
        m_Vertices		= xr_alloc<Fvector>(m_VertCount);
		Memory.mem_copy	(m_Vertices,&*m_NewPoints.begin(),m_NewPoints.size()*sizeof(Fvector));

		if (i_del_face){
	        st_Face* 	old_faces 	= m_Faces;
	        u32* 		old_sg 		= m_SmoothGroups;

            m_Faces			= xr_alloc<st_Face>	(m_FaceCount-i_del_face);
            m_SmoothGroups	= xr_alloc<u32>		(m_FaceCount-i_del_face);
            
            u32 new_dk	= 0;
            for (u32 dk=0; dk<m_FaceCount; ++dk)
			{
            	if (faces_mark[dk])
				{
                    for (SurfFacesPairIt plp_it=m_SurfFaces.begin(); plp_it!=m_SurfFaces.end(); ++plp_it)
					{
                        IntVec& 	pol_lst = plp_it->second;
                        for (int k=0; k<int(pol_lst.size()); ++k)
						{
                            int& f = pol_lst[k];
                            if (f>(int)dk)
							{ 
								--f;
                            }else if (f==(int)dk)
							{
                                pol_lst.erase(pol_lst.begin()+k);
                                --k;
                            }
                        }
                    }
                	continue;
                } 
//. -----in plugin
//.				new_dk++;

            	m_Faces[new_dk]				= old_faces[dk];
            	m_SmoothGroups[new_dk]		= old_sg[dk];

//. -----in editors
				++new_dk;
            }
            m_FaceCount	= m_FaceCount-i_del_face;
            xr_free		(old_faces);
            xr_free		(old_sg);
		}
	}
}
