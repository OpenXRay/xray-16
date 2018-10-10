#include "stdafx.h"
#pragma hdrstop

#include "R_Backend_hemi.h"

R_hemi::R_hemi() { unmap(); }
void R_hemi::unmap()
{
    c_pos_faces = nullptr;
    c_neg_faces = nullptr;
    c_material = nullptr;
    c_camo_data = nullptr; //--#SM+#--
    c_custom_data = nullptr; //--#SM+#--
    c_entity_data = nullptr; //--#SM+#--
}

void R_hemi::set_pos_faces(float posx, float posy, float posz)
{
    if (c_pos_faces)
        RCache.set_c(c_pos_faces, posx, posy, posz, 0);
}
void R_hemi::set_neg_faces(float negx, float negy, float negz)
{
    if (c_neg_faces)
        RCache.set_c(c_neg_faces, negx, negy, negz, 0);
}

void R_hemi::set_material(float x, float y, float z, float w)
{
    if (c_material)
        RCache.set_c(c_material, x, y, z, w);
}

// Обновляем значения в шейдерных константах //--#SM+#-- //SM_TODO Переместить
void R_hemi::c_update(IRenderVisual* pVisual)
{
    vis_object_data* _data = pVisual->getVisData().obj_data;

    if (c_camo_data)
        RCache.set_c(c_camo_data, _data->sh_camo_data);

    if (c_custom_data)
        RCache.set_c(c_custom_data, _data->sh_custom_data);

    if (c_entity_data)
        RCache.set_c(c_entity_data, _data->sh_entity_data);
}
