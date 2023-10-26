#include "stdafx.h"
#include "Layers/xrRender/light.h"

void light::gi_generate()
{
    indirect.clear();
    indirect_photons = ps_r2_ls_flags.test(R2FLAG_GI) ? ps_r2_GI_photons : 0;

    CRandom random;
    random.seed(0x12071980);

    xrXRC& xrc = RImplementation.Sectors_xrc;
    const CDB::MODEL* model = g_pGameLevel->ObjectSpace.GetStaticModel();
    const CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
    const Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

    const u32 photons_count = indirect_photons * 8;

    indirect.reserve(photons_count);
    for (u32 i = 0; i < photons_count; ++i)
    {
        Fvector dir, idir;
        switch (flags.type)
        {
        case IRender_Light::POINT: dir.random_dir(random); break;
        case IRender_Light::SPOT: dir.random_dir(direction, cone, random); break;
        case IRender_Light::OMNIPART: dir.random_dir(direction, cone, random); break;
        }
        dir.normalize();

        xrc.ray_query(CDB::OPT_CULL | CDB::OPT_ONLYNEAREST, model, position, dir, range);
        if (!xrc.r_count())
            continue;

        const CDB::RESULT* R = xrc.r_begin();
        const CDB::TRI& T = tris[R->id];
        const Fvector Tv[3] = { verts[T.verts[0]], verts[T.verts[1]], verts[T.verts[2]] };
        Fvector TN;

        TN.mknormal(Tv[0], Tv[1], Tv[2]);
        const float dot = TN.dotproduct(idir.invert(dir));

        const float e = dot * (1 - R->range / range);
        if (e < ps_r2_GI_clip)
            continue;

        indirect.emplace_back(
            light_indirect
            {
                /*.P =*/ Fvector().mad(position, dir, R->range),
                /*.D =*/ Fvector().reflect(dir, TN),
                /*.E =*/ e,
                /*.S =*/ spatial.sector_id //. BUG
            }
        );
    }

    // sort & clip
    std::sort(indirect.begin(), indirect.end(), [](const light_indirect& A, const light_indirect& B)
    {
        return A.E > B.E;
    });
    if (indirect.size() > indirect_photons)
        indirect.erase(indirect.begin() + indirect_photons, indirect.end());

    // normalize
    if (!indirect.empty())
    {
        const float target_E = ps_r2_GI_refl;
        float total_E = 0;

        for (const auto& light : indirect)
            total_E += light.E;

        const float scale_E = target_E / total_E;

        for (auto& light : indirect)
            light.E *= scale_E;
    }
}
