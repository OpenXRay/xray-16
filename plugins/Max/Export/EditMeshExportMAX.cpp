//----------------------------------------------------
// file: EditMesh.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

//#include "..\..\Shared\EditMesh.h"
//#include "..\..\Shared\EditObject.h"

#include "..\..\..\editors\ECore\Editor\EditMesh.h"
#include "..\..\..\editors\ECore\Editor\EditObject.h"

#include "MeshExpUtility.h"

#include "Exporter.h"
#include "..\..\Shared\GameMaterial.h"
//----------------------------------------------------
void CEditableMesh::FlipFaces(){
	VERIFY(m_Faces);
	for(u32 f = 0; f<GetFCount(); f++){
		st_FaceVert v = m_Faces[f].pv[0];
		m_Faces[f].pv[0] = m_Faces[f].pv[2];
		m_Faces[f].pv[2] = v;
	}
}
//----------------------------------------------------------------------------

TriObject *CEditableMesh::ExtractTriObject( INode *node, int &deleteIt )
{
	deleteIt = FALSE;
	Object *obj = node->EvalWorldState(0).obj;
	if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) { 
		TriObject *tri = (TriObject *) obj->ConvertToType(0, 
			Class_ID(TRIOBJ_CLASS_ID, 0));
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != tri) deleteIt = TRUE;
		return tri;
	}else{
		return NULL;
	}
}
//----------------------------------------------------------------------------

bool CEditableMesh::Convert( INode *node )
{
	// prepares & checks
	BOOL bDeleteObj;
	bool bResult = true;
	TriObject *obj = ExtractTriObject( node, bDeleteObj );

	if( !obj ){
		ELog.Msg(mtError,"%s -> Can't convert to TriObject", node->GetName() );
		return false; }

	if( obj->mesh.getNumFaces() <=0 ){
		ELog.Msg(mtError,"%s -> There are no faces ?", node->GetName() );
		if (bDeleteObj) delete (obj);
		return false; }

	Mtl *pMtlMain = node->GetMtl();
	DWORD cSubMaterials=0;

	if (pMtlMain){
		// There is at least one material. We're in case 1) or 2)
		cSubMaterials = pMtlMain->NumSubMtls();
		if (cSubMaterials < 1){
			// Count the material itself as a submaterial.
			cSubMaterials = 1;
		}
	}

	// build normals
	obj->mesh.buildRenderNormals();

	// vertices
	m_VertCount = obj->mesh.getNumVerts();
	m_Vertices = xr_alloc<Fvector>(m_VertCount);
	for (int v_i=0; v_i<m_VertCount; v_i++){
		Point3* p = obj->mesh.verts+v_i;
		m_Vertices[v_i].set(p->x,p->y,p->z);
	}

	// set smooth group MAX type
	m_Flags.set(flSGMask,TRUE);

	// faces
	m_FaceCount		= obj->mesh.getNumFaces();
	m_Faces			= xr_alloc<st_Face>	(m_FaceCount);
	m_SmoothGroups	= xr_alloc<u32>		(m_FaceCount);

	m_VMRefs.reserve(m_FaceCount*3);
	if (0==obj->mesh.mapFaces(1))
	{
		bResult = false;
		ELog.Msg(mtError,"'%s' hasn't UV mapping!", node->GetName());
	}
	if (bResult)
	{
		CSurface* surf=0;
		for (int f_i=0; f_i<m_FaceCount; ++f_i)
		{
			Face*	vf = obj->mesh.faces+f_i;
			TVFace* tf = obj->mesh.mapFaces(1) + f_i;
			if (!tf)
			{
				bResult = false;
				ELog.Msg(mtError,"'%s' hasn't UV mapping!", node->GetName());
				break;
			}
			m_SmoothGroups[f_i]					= vf->getSmGroup();
			for (int k=0; k<3; ++k)
			{
				m_Faces[f_i].pv[k].pindex = vf->v[k];
				m_VMRefs.push_back(st_VMapPtLst());
				st_VMapPtLst&	vm_lst = m_VMRefs.back();
				vm_lst.count	= 1;
				vm_lst.pts		= xr_alloc<st_VMapPt>(vm_lst.count);
				for (DWORD vm_i=0; vm_i<vm_lst.count; ++vm_i)
				{
					vm_lst.pts[vm_i].vmap_index	= 0;
					vm_lst.pts[vm_i].index 		= tf->t[k];
				}
				m_Faces[f_i].pv[k].vmref	= m_VMRefs.size()-1;
				if (!bResult) break;
			}
			if (pMtlMain)
			{
				int m_id = obj->mesh.getFaceMtlIndex(f_i);
				if (cSubMaterials == 1)
				{
					m_id = 0;
				}else
				{
					// SDK recommends mod'ing the material ID by the valid # of materials, 
					// as sometimes a material number that's too high is returned.
					m_id %= cSubMaterials;
				}
				surf = m_Parent->CreateSurface(pMtlMain,m_id);
				if (!surf) bResult = false;
			}
			if (!bResult) break;
			m_SurfFaces[surf].push_back(f_i);
		}
	}

	// vmaps
	if( bResult ){
		int vm_cnt = obj->mesh.getNumTVerts();
		m_VMaps.resize(1);
		st_VMap*& VM = m_VMaps.back();
		VM = xr_new<st_VMap>("Texture",vmtUV,false);
		for (int tx_i=0; tx_i<vm_cnt; tx_i++){
			UVVert* tv = obj->mesh.tVerts + tx_i;
			VM->appendUV(tv->x,1-tv->y);
		}
	}

	if ((GetVertexCount()<4)||(GetFaceCount()<2))
	{
		ELog.Msg(mtError,"Invalid mesh: '%s'. Faces<2 or Verts<4");
		bResult = false;
	}

	if (bResult ){
		ELog.Msg(mtInformation,"Model '%s' contains: %d points, %d faces",
			node->GetName(), m_VertCount, m_FaceCount);
	}

	if (bResult)
	{
		RecomputeBBox	();
		OptimizeMesh	(false);
		RebuildVMaps	();
		ELog.Msg(mtInformation,"Model '%s' converted: %d points, %d faces",
			node->GetName(), GetVertexCount(), GetFaceCount());
	}

	if (bDeleteObj) delete (obj);
	return bResult;
}
//----------------------------------------------------------------------------

bool CEditableMesh::Convert(CExporter* E)
{
	bool bResult		= true;

	m_Name				= E->m_MeshNode->GetName();

	// maps
	// Weight maps 
	m_VMaps.resize(E->m_Bones.size()+1);
	for (DWORD b_i=0; b_i<E->m_Bones.size(); b_i++)
		m_VMaps[b_i]	= xr_new<st_VMap>(E->m_Bones[b_i]->name.c_str(),vmtWeight,false);;
	// UV map
	int VM_UV_idx		= m_VMaps.size()-1;
	st_VMap*& VM_UV		= m_VMaps[VM_UV_idx];
	VM_UV				= xr_new<st_VMap>("texture",vmtUV,false);

	// points
	{
		m_VertCount		= E->m_ExpVertices.size();
		m_Vertices		= xr_alloc<Fvector>(m_VertCount);
		Fvector* p_it	= m_Vertices;

		for (ExpVertIt ev_it=E->m_ExpVertices.begin(); ev_it!=E->m_ExpVertices.end(); ev_it++,p_it++){
			p_it->set		((*ev_it)->P);
			VM_UV->appendUV	((*ev_it)->uv.x,(*ev_it)->uv.y);
		}
	}
	// faces 
	{
		// set smooth group MAX type
		m_Flags.set(flSGMask,TRUE);
		// reserve space for faces and references
		m_FaceCount		= E->m_ExpFaces.size();
		m_Faces			= xr_alloc<st_Face>	(m_FaceCount);
		m_SmoothGroups	= xr_alloc<u32>		(m_FaceCount);
		m_VMRefs.resize	(m_VertCount);

		int f_id=0;
		for (ExpFaceIt ef_it=E->m_ExpFaces.begin(); ef_it!=E->m_ExpFaces.end(); ef_it++,f_id++){
			// FACES
			m_SmoothGroups[f_id]	= (*ef_it)->sm_group;
			st_Face& F				= m_Faces[f_id];
			for (int k=0; k<3; ++k)
			{
				int v_idx			= (*ef_it)->v[k];
				st_FaceVert& vt		= F.pv[k];
				st_VERT* V			= E->m_ExpVertices[v_idx];
				vt.pindex			= v_idx;
				st_VMapPtLst& vm_lst= m_VMRefs[vt.pindex];
				vm_lst.count		= V->data.size()+1;
				vm_lst.pts			= xr_alloc<st_VMapPt>(vm_lst.count);
				vm_lst.pts[0].vmap_index= VM_UV_idx;
				vm_lst.pts[0].index 	= vt.pindex;
				for (VDIt vd_it=V->data.begin(); vd_it!=V->data.end(); vd_it++){
					DWORD idx		= vd_it-V->data.begin()+1;
					st_VMap* vm		= m_VMaps[vd_it->bone];
					vm->appendW		(vd_it->weight);
					vm_lst.pts[idx].vmap_index	= vd_it->bone;
					vm_lst.pts[idx].index 		= vm->size()-1;
				}
				vt.vmref			= vt.pindex;
			}
			CSurface* surf = m_Parent->CreateSurface(E->m_MtlMain,(*ef_it)->m_id);
			if (!surf){
				bResult = FALSE;
				break;
			}
			m_SurfFaces[surf].push_back(f_id);
		}
	}
	if ((GetVertexCount()<4)||(GetFaceCount()<2))
	{
		Log("!Invalid mesh: '%s'. Faces<2 or Verts<4",*Name());
		bResult = false;
	}
	if (bResult)
	{
		RecomputeBBox	();
		OptimizeMesh	(true);//false);
		RebuildVMaps	();
	}
	return bResult;
}
