#include "stdafx.h"
#include "build.h"
#include "utils/xrLC_Light/xrface.h"
#include "utils/xrLC_Light/calculate_normals.h"
#include "utils/xrLC_Light/xrLC_GlobalData.h"



//void 
// Performs simple cross-smooth

void CBuild::CalcNormals()
{

	calculate_normals<Vertex>::calc_normals( lc_global_data()->g_vertices(), lc_global_data()->g_faces() );
	// Models
    Logger.Status("Models...");
	MU_ModelsCalculateNormals();
}

