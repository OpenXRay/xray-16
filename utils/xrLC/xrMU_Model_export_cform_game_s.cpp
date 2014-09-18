#include "stdafx.h"
#include "xrMU_model.h"
void xrMU_Reference::export_cform_game	(_mesh& mesh, xr_vector<cform_FailFace>& failedfaces)
{
	Log	("model:",*(model->m_name));

	// verts 
	for (u32 V=0; V<model->m_vertices.size(); V++)	model->m_vertices[V]->handle	= _mesh::InvalidVertexHandle.idx();

	// faces
	std::vector <_mesh::VertexHandle>	fhandles;
	cform_mergeprops					fmergeprops;
	for (xrMU_Model::v_faces_it I=model->m_faces.begin(); I!=model->m_faces.end(); I++)
	{
		xrMU_Model::_face* F = *I;
		if (F->Shader().flags.bCollision) 
		{
			// add vertices
			fhandles.clear	();
			for (u32 v=0; v<3; v++)
			{
				_mesh::VertexHandle	h	= _mesh::VertexHandle(F->v[v]->handle);
				if (_mesh::InvalidVertexHandle == h)	{ 
					Fvector		p;		
					xform.transform_tiny(p,F->v[0]->P);
					h = mesh.add_vertex	(_mesh::Point(p.x,p.y,p.z));
					F->v[v]->handle		= h.idx();
				}
				fhandles.push_back	(h);
			}

			// add face
			fmergeprops.material		= F->dwMaterialGame;
			fmergeprops.sector			= sector;
			_mesh::FaceHandle	hface	= mesh.add_face		(fhandles);
			if (hface == _mesh::InvalidFaceHandle)	{
				failedfaces.push_back		(cform_FailFace());
				failedfaces.back().P[0]		= F->v[0]->P;
				failedfaces.back().P[1]		= F->v[1]->P;
				failedfaces.back().P[2]		= F->v[2]->P;
				failedfaces.back().props	= fmergeprops.props;
			}
			else				mesh.face(hface).set_props	(fmergeprops.props);
		}
	}
}
