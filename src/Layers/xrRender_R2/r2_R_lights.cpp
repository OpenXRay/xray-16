#include "stdafx.h"

bool check_grass_shadow(light* L, CFrustum VB)
{
    // Grass shadows are allowed?
    if (ps_ssfx_grass_shadows.x < 3 || !ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))
        return false;

    // Inside the range?
    if (L->vis.distance > ps_ssfx_grass_shadows.z)
        return false;

    // Is in view? L->vis.visible?
    u32 mask = 0xff;
    if (!VB.testSphere(L->position, L->range * 0.6f, mask))
        return false;

    return true;
}

void CRender::render_lights(light_Package& LP)
{
    //////////////////////////////////////////////////////////////////////////
    // Refactor order based on ability to pack shadow-maps
    // 1. calculate area + sort in descending order
    // const	u16		smap_unassigned		= u16(-1);
    {
        xr_vector<light*>& source = LP.v_shadowed;
        for (u32 it = 0; it < source.size(); it++)
        {
            light* L = source[it];
            L->vis_update();
            if (!L->vis.visible)
            {
                source.erase(source.begin() + it);
                it--;
            }
            else
            {
                LR.compute_xf_spot(L);
            }
        }
    }

    // 2. refactor - infact we could go from the backside and sort in ascending order
    {
        xr_vector<light*>& source = LP.v_shadowed;
        xr_vector<light*> refactored;
        refactored.reserve(source.size());
        const size_t total = source.size();

        for (u16 smap_ID = 0; refactored.size() != total; ++smap_ID)
        {
            LP_smap_pool.initialize(RImplementation.o.smapsize);
            std::sort(source.begin(), source.end(), [](light* l1, light* l2)
            {
                const u32 a0 = l1->X.S.size;
                const u32 a1 = l2->X.S.size;
                return a0 > a1; // reverse -> descending
            });
            for (size_t test = 0; test < source.size(); ++test)
            {
                light* L = source[test];
                SMAP_Rect R{};
                if (LP_smap_pool.push(R, L->X.S.size))
                {
                    // OK
                    L->X.S.posX = R.min.x;
                    L->X.S.posY = R.min.y;
                    L->vis.smap_ID = smap_ID;
                    refactored.push_back(L);
                    source.erase(source.begin() + test);
                    --test;
                }
            }
        }

        // save (lights are popped from back)
        std::reverse(refactored.begin(), refactored.end());
        LP.v_shadowed = std::move(refactored);
    }

    auto& cmd_list = get_imm_context().cmd_list;
    Target->rt_smap_depth->set_slice_read(0);

    PIX_EVENT(SHADOWED_LIGHTS);

    //////////////////////////////////////////////////////////////////////////
    // sort lights by importance???
    // while (has_any_lights_that_cast_shadows) {
    //		if (has_point_shadowed)		->	generate point shadowmap
    //		if (has_spot_shadowed)		->	generate spot shadowmap
    //		switch-to-accumulator
    //		if (has_point_unshadowed)	-> 	accum point unshadowed
    //		if (has_spot_unshadowed)	-> 	accum spot unshadowed
    //		if (was_point_shadowed)		->	accum point shadowed
    //		if (was_spot_shadowed)		->	accum spot shadowed
    //	}
    //	if (left_some_lights_that_doesn't cast shadows)
    //		accumulate them
    static xr_vector<light*> L_spot_s;

    struct task_data_t
    {
        light* L;
        Task* task;
        u32 batch_id;
    };
    static xr_vector<task_data_t> lights_queue{};
    lights_queue.reserve(R__NUM_SUN_CASCADES);

    const auto &calc_lights = [](Task &, void* data)
    {
        const auto* task_data = static_cast<task_data_t*>(data);
        auto& dsgraph = RImplementation.get_context(task_data->batch_id);
        {
            auto* L = task_data->L;

            L->svis[task_data->batch_id].begin();

            dsgraph.o.phase = PHASE_SMAP;
            dsgraph.r_pmask(true, RImplementation.o.Tshadows);
            dsgraph.o.sector_id = L->spatial.sector_id;
            dsgraph.o.view_pos = L->position;
            dsgraph.o.xform = L->X.S.combine;
            dsgraph.o.view_frustum.CreateFromMatrix(L->X.S.combine, FRUSTUM_P_ALL & (~FRUSTUM_P_NEAR));

            dsgraph.build_subspace();
        }
    };

    const auto& flush_lights = [&]()
    {
        for (const auto& [L, task, batch_id] : lights_queue)
        {
            VERIFY(task);
            TaskScheduler->Wait(*task);

            auto& dsgraph = get_context(batch_id);

            const bool bNormal = !dsgraph.mapNormalPasses[0][0].empty() || !dsgraph.mapMatrixPasses[0][0].empty();
            const bool bSpecial = !dsgraph.mapNormalPasses[1][0].empty() || !dsgraph.mapMatrixPasses[1][0].empty() ||
                !dsgraph.mapSorted.empty();
            if (bNormal || bSpecial)
            {
                PIX_EVENT_CTX(dsgraph.cmd_list, SHADOWED_LIGHT);

                Stats.s_merged++;
                L_spot_s.push_back(L);
                Target->phase_smap_spot(dsgraph.cmd_list, L);
                dsgraph.cmd_list.set_xform_world(Fidentity);
                dsgraph.cmd_list.set_xform_view(L->X.S.view);
                dsgraph.cmd_list.set_xform_project(L->X.S.project);
                dsgraph.render_graph(0);
                if (ps_r2_ls_flags.test(R_FLAGEXT_LIGHT_DETAILS))
                {
                    if (check_grass_shadow(L, ViewBase))
                    {
                        Details->fade_distance = -1; // Use light position to calc "fade"
                        Details->light_position.set(L->position);
                        Details->Render(dsgraph.cmd_list);
                    }
                }
                L->X.S.transluent = FALSE;
                if (bSpecial)
                {
                    L->X.S.transluent = TRUE;
                    Target->phase_smap_spot_tsh(dsgraph.cmd_list, L);
                    PIX_EVENT_CTX(dsgraph.cmd_list, SHADOWED_LIGHTS_RENDER_GRAPH);
                    dsgraph.render_graph(1); // normal level, secondary priority
                    PIX_EVENT_CTX(dsgraph.cmd_list, SHADOWED_LIGHTS_RENDER_SORTED);
                    dsgraph.render_sorted(); // strict-sorted geoms
                }
            }
            else
            {
                Stats.s_finalclip++;
            }

            L->svis[batch_id].end(); // NOTE(DX11): occqs are fetched here, this should be done on the imm context only
            RImplementation.release_context(batch_id);
        }

        lights_queue.clear();
    };

    while (!LP.v_shadowed.empty())
    {
        // if (has_spot_shadowed)
        Stats.s_used++;

        // generate spot shadowmap
        Target->phase_smap_spot_clear(cmd_list);
        xr_vector<light*>& source = LP.v_shadowed;
        light* L = source.back();
        const u16 sid = L->vis.smap_ID;
        while (true)
        {
            if (source.empty())
                break;
            L = source.back();
            if (L->vis.smap_ID != sid)
                break;

            const auto batch_id = alloc_context(false);
            if (batch_id == R_dsgraph_structure::INVALID_CONTEXT_ID)
            {
                VERIFY(!lights_queue.empty());
                flush_lights();
                continue;
            }

            source.pop_back();
            Lights_LastFrame.push_back(L);

            // calculate
            task_data_t data;
            data.batch_id = batch_id;
            data.L = L;
            data.task = &TaskScheduler->CreateTask("slight_calc", calc_lights, sizeof(data), (void*)&data);
            if (o.mt_calculate)
            {
                TaskScheduler->PushTask(*data.task);
            }
            else
            {
                TaskScheduler->RunTask(*data.task);
            }
            lights_queue.emplace_back(data);
        }
        flush_lights(); // in case if something left

        cmd_list.Invalidate();

        PIX_EVENT(UNSHADOWED_LIGHTS);

        //		switch-to-accumulator
        Target->phase_accumulator(cmd_list);

        PIX_EVENT(POINT_LIGHTS);

        //		if (has_point_unshadowed)	-> 	accum point unshadowed
        if (!LP.v_point.empty())
        {
            light* L2 = LP.v_point.back();
            LP.v_point.pop_back();
            L2->vis_update();
            if (L2->vis.visible)
            {
                Target->accum_point(cmd_list, L2);
                render_indirect(L2);
            }
        }

        PIX_EVENT(SPOT_LIGHTS);

        //		if (has_spot_unshadowed)	-> 	accum spot unshadowed
        if (!LP.v_spot.empty())
        {
            light* L2 = LP.v_spot.back();
            LP.v_spot.pop_back();
            L2->vis_update();
            if (L2->vis.visible)
            {
                LR.compute_xf_spot(L2);
                Target->accum_spot(cmd_list, L2);
                render_indirect(L2);
            }
        }

        PIX_EVENT(SPOT_LIGHTS_ACCUM_VOLUMETRIC);

        //		if (was_spot_shadowed)		->	accum spot shadowed
        if (!L_spot_s.empty())
        {
            PIX_EVENT(ACCUM_SPOT);
            for (light* p_light : L_spot_s)
            {
                Target->accum_spot(cmd_list, p_light);
                render_indirect(p_light);
            }

            PIX_EVENT(ACCUM_VOLUMETRIC);
            if (RImplementation.o.advancedpp && ps_r2_ls_flags.is(R2FLAG_VOLUMETRIC_LIGHTS))
                for (light* p_light : L_spot_s)
                    Target->accum_volumetric(cmd_list, p_light);

            L_spot_s.clear();
        }
    }

    PIX_EVENT(POINT_LIGHTS_ACCUM);
    // Point lighting (unshadowed, if left)
    if (!LP.v_point.empty())
    {
        xr_vector<light*>& Lvec = LP.v_point;
        for (light* p_light : Lvec)
        {
            p_light->vis_update();
            if (p_light->vis.visible)
            {
                render_indirect(p_light);
                Target->accum_point(cmd_list, p_light);
            }
        }
        Lvec.clear();
    }

    PIX_EVENT(SPOT_LIGHTS_ACCUM);
    // Spot lighting (unshadowed, if left)
    if (!LP.v_spot.empty())
    {
        xr_vector<light*>& Lvec = LP.v_spot;
        for (light* p_light : Lvec)
        {
            p_light->vis_update();
            if (p_light->vis.visible)
            {
                LR.compute_xf_spot(p_light);
                render_indirect(p_light);
                Target->accum_spot(cmd_list, p_light);
            }
        }
        Lvec.clear();
    }
}

void CRender::render_indirect(light* L) const
{
    if (!ps_r2_ls_flags.test(R2FLAG_GI))
        return;

    auto& cmd_list = RImplementation.get_imm_context().cmd_list;

    light LIGEN;
    LIGEN.set_type(IRender_Light::REFLECTED);
    LIGEN.set_shadow(false);
    LIGEN.set_cone(PI_DIV_2 * 2.f);

    const xr_vector<light_indirect>& Lvec = L->indirect;
    if (Lvec.empty())
        return;
    const float LE = L->color.intensity();
    for (auto& LI : Lvec)
    {
        // energy and color
        const float LIE = LE * LI.E;
        if (LIE < ps_r2_GI_clip)
            continue;

        Fvector T{ L->color.r, L->color.g, L->color.b };
        T.mul(LI.E);
        LIGEN.set_color(T.x, T.y, T.z);

        // geometric
        Fvector L_up{ 0, 1, 0 }, L_right;
        if (_abs(L_up.dotproduct(LI.D)) > .99f)
            L_up = { 0, 0, 1 };

        L_right.crossproduct(L_up, LI.D).normalize();
        LIGEN.spatial.sector_id = LI.S;
        LIGEN.set_position(LI.P);
        LIGEN.set_rotation(LI.D, L_right);

        // range
        // dist^2 / range^2 = A - has infinity number of solutions
        // approximate energy by linear fallof Emax / (1 + x) = Emin
        const float Emax = LIE;
        const float Emin = 1.f / 255.f;
        const float x = (Emax - Emin) / Emin;
        if (x < 0.1f)
            continue;
        LIGEN.set_range(x);

        Target->accum_reflected(cmd_list, &LIGEN);
    }
}
