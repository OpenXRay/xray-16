#include "stdafx.h"
#include "FLOD.h"
#include "xrCommon/xr_array.h"

#ifdef _EDITOR
#include "IGame_Persistent.h"
#include "Environment.h"
#else
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#endif

extern float r_ssaLOD_A;
extern float r_ssaLOD_B;

template <class T> IC bool cmp_first_l(const T &lhs, const T &rhs) { return (lhs.first < rhs.first); }
template <class T> IC bool cmp_first_h(const T &lhs, const T &rhs) { return (lhs.first > rhs.first); }

void R_dsgraph_structure::render_lods(bool _setup_zb, bool _clear)
{
    ZoneScoped;
    PIX_EVENT_CTX(cmd_list, dsgraph_render_lods);

    if (mapLOD.empty())
        return;

    if (_setup_zb)
        mapLOD.get_left_right(lstLODs); // front-to-back
    else
        mapLOD.get_right_left(lstLODs);	// back-to-front

    if (lstLODs.empty())
        return;

    // *** Fill VB and generate groups
    u32 shid = _setup_zb ? SE_R1_LMODELS : SE_R1_NORMAL_LQ;
    FLOD* firstV = (FLOD*)lstLODs[0].pVisual;
    ref_selement cur_S = firstV->shader->E[shid];
    float ssaRange = r_ssaLOD_A - r_ssaLOD_B;
    if (ssaRange < EPS_S)
        ssaRange = EPS_S;

    const u32 uiVertexPerImposter = 4;
    const u32 uiImpostersFit = RImplementation.Vertex.GetSize() / (firstV->geom->vb_stride * uiVertexPerImposter);

    //Msg("dbg_lods: shid[%d],firstV[%X]",shid,u32((void*)firstV));
    //Msg("dbg_lods: shader[%X]",u32((void*)firstV->shader._get()));
    //Msg("dbg_lods: shader_E[%X]",u32((void*)cur_S._get()));

    for (u32 i = 0; i < lstLODs.size(); i++)
    {
        const u32 iBatchSize = std::min(lstLODs.size() - i, (size_t)uiImpostersFit);
        int cur_count = 0;
        u32 vOffset;
        FLOD::_hw* V =
            (FLOD::_hw*)RImplementation.Vertex.Lock(iBatchSize * uiVertexPerImposter, firstV->geom->vb_stride, vOffset);

        for (u32 j = 0; j < iBatchSize; ++j, ++i)
        {
            // sort out redundancy
            R_dsgraph::_LodItem& P = lstLODs[i];
            if (P.pVisual->shader->E[shid] == cur_S)
                cur_count++;
            else
            {
                lstLODgroups.push_back(cur_count);
                cur_S = P.pVisual->shader->E[shid];
                cur_count = 1;
            }

            // calculate alpha
            float ssaDiff = P.ssa - r_ssaLOD_B;
            float scale = ssaDiff / ssaRange;
            int iA = iFloor((1 - scale) * 255.f);
            u32 uA = u32(clampr(iA, 0, 255));

            // calculate direction and shift
            FLOD* lodV = (FLOD*)P.pVisual;
            Fvector Ldir, shift;
            Ldir.sub(lodV->vis.sphere.P, Device.vCameraPosition).normalize();
            shift.mul(Ldir, -.5f * lodV->vis.sphere.R);

            // gen geometry
            FLOD::_face* facets = lodV->facets;
            svector<std::pair<float, u32>, 8> selector;
            for (u32 s = 0; s < 8; s++)
                selector.push_back(std::make_pair(Ldir.dotproduct(facets[s].N), s));
            std::sort(selector.begin(), selector.end(), [](const auto& v1, const auto& v2)
            {
                return v1.first < v2.first;
            });

            const float dot_best = selector[selector.size() - 1].first;
            const float dot_next = selector[selector.size() - 2].first;
            const float dot_next_2 = selector[selector.size() - 3].first;
            size_t id_best = selector[selector.size() - 1].second;
            size_t id_next = selector[selector.size() - 2].second;

            // Now we have two "best" planes, calculate factor, and approx normal
            const float fA = dot_best, fB = dot_next, fC = dot_next_2;
            const float alpha = 0.5f + 0.5f * (1 - (fB - fC) / (fA - fC));
            const int iF = iFloor(alpha * 255.5f);
            const u32 uF = u32(clampr(iF, 0, 255));

            // Fill VB
            const FLOD::_face& FA = facets[id_best];
            const FLOD::_face& FB = facets[id_next];
            xr_array<int, 4> vid = {3, 0, 2, 1};
            for (int id : vid)
            {
                V->p0.add(FB.v[id].v, shift);
                V->p1.add(FA.v[id].v, shift);
                V->n0 = FB.N;
                V->n1 = FA.N;
                V->sun_af = color_rgba(FB.v[id].c_sun, FA.v[id].c_sun, uA, uF);
                V->t0 = FB.v[id].t;
                V->t1 = FA.v[id].t;
                V->rgbh0 = FB.v[id].c_rgb_hemi;
                V->rgbh1 = FA.v[id].c_rgb_hemi;
                V++;
            }
        }
        lstLODgroups.push_back(cur_count);
        RImplementation.Vertex.Unlock(iBatchSize * uiVertexPerImposter, firstV->geom->vb_stride);

        // *** Render
        cmd_list.set_xform_world(Fidentity);
        for (u32 uiPass = 0; uiPass < SHADER_PASSES_MAX; ++uiPass)
        {
            int current = 0;
            u32 vCurOffset = vOffset;

            for (int p_count : lstLODgroups)
            {
                u32 uiNumPasses = lstLODs[current].pVisual->shader->E[shid]->passes.size();
                if (uiPass < uiNumPasses)
                {
                    cmd_list.set_Element(lstLODs[current].pVisual->shader->E[shid], uiPass);
                    cmd_list.set_Geometry(firstV->geom);
                    cmd_list.Render(D3DPT_TRIANGLELIST, vCurOffset, 0, 4 * p_count, 0, 2 * p_count);
                }
                cmd_list.stat.r.s_flora_lods.add(4 * p_count);
                current += p_count;
                vCurOffset += 4 * p_count;
            }
        }

        lstLODgroups.clear();
    }

    lstLODs.clear();

    if (_clear)
        mapLOD.clear();
}
