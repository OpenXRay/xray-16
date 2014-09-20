#include "stdafx.h"


#include "xrMU_model.h"
#include "xrMU_Model_Reference.h"

#include "build.h"
#include "../../xrcdb/xrcdb.h"
/*
int getCFormVID		(xrMU_Model::v_vertices& V,xrMU_Model::_vertex *F)
{
	xrMU_Model::v_vertices_it it	= std::std::lower_bound(V.begin(),V.end(),F);
	return it-V.begin();
}
*/
/*
extern int bCriticalErrCnt;
int getTriByEdge	(xrMU_Model::_vertex *V1, xrMU_Model::_vertex *V2, xrMU_Model::_face* parent, xrMU_Model::v_faces &ids)
{
	xrMU_Model::_face*	found	= 0;
	int		f_count				= 0;

	for (xrMU_Model::v_faces_it I=V1->m_adjacents.begin(); I!=V1->m_adjacents.end(); I++)
	{
		xrMU_Model::_face* test = *I;
		if (test == parent) continue;
		if (test->VContains(V2)) {
			f_count++;
			found = test;
		}
	}
	if (f_count>1) {
		bCriticalErrCnt	++;
		pBuild->err_multiedge.w_fvector3(V1->P);
		pBuild->err_multiedge.w_fvector3(V2->P);
	}
	if (found) {
		xrMU_Model::v_faces_it F = std::lower_bound(ids.begin(),ids.end(),found);
		if (found == *F) return (u32)(F-ids.begin());
		else return -1;
	} else {
		return -1;
	}
}

*/