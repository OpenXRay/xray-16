#include "stdafx.h"
#include "r_backend_lod.h"

R_LOD::R_LOD(CBackend& cmd_list_in) : cmd_list(cmd_list_in)
{
    unmap();
}

void R_LOD::set_LOD(float LOD)
{
    if (c_LOD)
    {
        float factor = clampr<float>(ceil(LOD * LOD * LOD * LOD * LOD * 8.0f), 1, 7);
        cmd_list.set_c(c_LOD, factor);
    }
}
