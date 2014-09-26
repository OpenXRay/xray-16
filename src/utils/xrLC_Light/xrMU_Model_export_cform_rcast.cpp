#include "stdafx.h"
#include "xrMU_Model.h"
#include "xrMU_Model_Reference.h"

#include "../../xrcdb/xrcdb.h"
#include "../shader_xrlc.h"
void xrMU_Model::export_cform_rcast	(CDB::CollectorPacked& CL, Fmatrix& xform)
{
	for		(u32 fit=0; fit<m_faces.size(); fit++)	m_faces[fit]->flags.bProcessed = false;

	v_faces			adjacent;	adjacent.reserve(6*2*3);

	for (v_faces_it it = m_faces.begin(); it!=m_faces.end(); it++)
	{
		_face*	F				= (*it);
		const Shader_xrLC&	SH		= F->Shader();
		if (!SH.flags.bLIGHT_CastShadow)		continue;

		// Collect
		adjacent.clear	();
		for (int vit=0; vit<3; vit++)
		{
			_vertex* V	= F->v[vit];
			for (u32 adj=0; adj<V->m_adjacents.size(); adj++)
				adjacent.push_back(V->m_adjacents[adj]);
		}

		// Unique
		std::sort		(adjacent.begin(),adjacent.end());
		adjacent.erase	(std::unique(adjacent.begin(),adjacent.end()),adjacent.end());
		BOOL			bAlready	= FALSE;
		for (u32 ait=0; ait<adjacent.size(); ait++)
		{
			_face*	Test				= adjacent[ait];
			if (Test==F)				continue;
			if (!Test->flags.bProcessed)continue;
			if (F->isEqual(*Test))
			{
				bAlready			= TRUE;
				break;
			}
		}

		//
		if (!bAlready) 
		{
			F->flags.bProcessed		= true;
			Fvector					P[3];
			xform.transform_tiny	(P[0],F->v[0]->P);
			xform.transform_tiny	(P[1],F->v[1]->P);
			xform.transform_tiny	(P[2],F->v[2]->P);
			CL.add_face_D			(P[0],P[1],P[2],*((u32*)&F), F->sm_group );//
		}
	}
}

