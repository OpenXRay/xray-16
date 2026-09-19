#include "stdafx.h"

#include "LegacyOgfSkeleton.h"

#include "xrCore/FMesh.hpp"
#include "xrCore/FS.h"
#include "xrCore/LocatorAPI.h"
#include "xrCore/xr_trims.h"
#include "xrCore/Animation/Bone.hpp"

#include <cstring>

namespace XRay
{
namespace Animation
{
namespace
{
u16 FindBone(const xr_vector<OzzBoneDesc>& bones, pcstr name)
{
    for (size_t i = 0; i < bones.size(); ++i)
        if (0 == xr_strcmp(bones[i].name.c_str(), name))
            return u16(i);
    return BI_NONE;
}

bool ReadBones(IReader* ogf, xr_vector<OzzBoneDesc>& bones)
{
    if (!ogf->find_chunk(OGF_S_BONE_NAMES))
        return false;

    const u32 count = ogf->r_u32();
    if (count == 0)
        return false;

    bones.resize(count);

    xr_vector<shared_str> parents(count);
    for (u32 i = 0; i < count; ++i)
    {
        string256 buf;

        ogf->r_stringZ(buf, sizeof(buf));
        xr_strlwr(buf);
        bones[i].name = shared_str(buf);

        ogf->r_stringZ(buf, sizeof(buf));
        xr_strlwr(buf);
        parents[i] = shared_str(buf);

        Fobb obb;
        ogf->r(&obb, sizeof(Fobb));

        bones[i].parent = BI_NONE;
        bones[i].bind_local.identity();
    }

    for (u32 i = 0; i < count; ++i)
    {
        pcstr parent = parents[i].c_str();
        if (!parent || !parent[0])
            continue;

        const u16 id = FindBone(bones, parent);
        if (id == BI_NONE)
            return false;
        bones[i].parent = id;
    }

    IReader* ikd = ogf->open_chunk(OGF_S_IKDATA);
    if (ikd)
    {
        for (u32 i = 0; i < count; ++i)
        {
            const u16 vers = u16(ikd->r_u32());
            shared_str game_mtl_name;
            ikd->r_stringZ(game_mtl_name);
            SBoneShape shape;
            ikd->r(&shape, sizeof(SBoneShape));
            SJointIKData ik_data;
            ik_data.Import(*ikd, vers);
            Fvector vXYZ, vT;
            ikd->r_fvector3(vXYZ);
            ikd->r_fvector3(vT);
            bones[i].bind_local.setXYZi(vXYZ);
            bones[i].bind_local.translate_over(vT);
            ikd->r_float();
            Fvector center_of_mass;
            ikd->r_fvector3(center_of_mass);
        }
        ikd->close();
    }

    return true;
}

void AppendGlob(pcstr mask, xr_vector<shared_str>& motion_refs)
{
    FS_FileSet fset;
    FS.file_list(fset, "$game_meshes$", FS_ListFiles, mask);
    FS.file_list(fset, "$level$", FS_ListFiles, mask);

    for (const auto& file : fset)
        motion_refs.emplace_back(file.name.c_str());
}

void ReadMotionRefs(IReader* ogf, xr_vector<shared_str>& motion_refs, bool& has_embedded_motions)
{
    has_embedded_motions = false;

    if (ogf->find_chunk(OGF_S_MOTION_REFS))
    {
        string_path items_nm;
        ogf->r_stringZ(items_nm, sizeof(items_nm));
        const int set_cnt = _GetItemCount(items_nm);
        motion_refs.reserve(size_t(set_cnt));
        for (int k = 0; k < set_cnt; ++k)
        {
            string_path nm;
            _GetItem(items_nm, k, nm);
            if (strstr(nm, "\\*.omf"))
            {
                AppendGlob(nm, motion_refs);
                continue;
            }
            xr_strcat(nm, ".omf");
            motion_refs.emplace_back(nm);
        }
        return;
    }

    if (ogf->find_chunk(OGF_S_MOTION_REFS2))
    {
        const u32 set_cnt = ogf->r_u32();
        motion_refs.reserve(set_cnt);
        for (u32 k = 0; k < set_cnt; ++k)
        {
            string_path nm;
            ogf->r_stringZ(nm, sizeof(nm));
            if (strstr(nm, "\\*.omf"))
            {
                AppendGlob(nm, motion_refs);
                continue;
            }
            xr_strcat(nm, ".omf");
            motion_refs.emplace_back(nm);
        }
        return;
    }

    has_embedded_motions = true;
}
}

bool ReadOgfSkeleton(
    IReader* ogf, xr_vector<OzzBoneDesc>& bones, xr_vector<shared_str>& motion_refs, bool& has_embedded_motions)
{
    bones.clear();
    motion_refs.clear();
    has_embedded_motions = false;

    if (!ogf)
        return false;

    if (!ReadBones(ogf, bones))
    {
        bones.clear();
        return false;
    }

    ReadMotionRefs(ogf, motion_refs, has_embedded_motions);
    return true;
}
}
}
