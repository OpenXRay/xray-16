#include "stdafx.h"
#pragma hdrstop

#include "FMesh.hpp"

void ogf_desc::Load(IReader& F)
{
    F.r_stringZ(source_file);
    F.r_stringZ(build_name);
    F.r(&build_time, sizeof(build_time));
    F.r_stringZ(create_name);
    F.r(&create_time, sizeof(create_time));
    F.r_stringZ(modif_name);
    F.r(&modif_time, sizeof(modif_time));
}
void ogf_desc::Save(IWriter& F)
{
    F.w_stringZ(source_file);
    F.w_stringZ(build_name);
    F.w(&build_time, sizeof(build_time));
    F.w_stringZ(create_name);
    F.w(&create_time, sizeof(create_time));
    F.w_stringZ(modif_name);
    F.w(&modif_time, sizeof(modif_time));
}
