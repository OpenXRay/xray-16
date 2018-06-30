#pragma once

#include "dTriCollideK.h"
#include "dTriColliderCommon.h"
#include "dTriColliderMath.h"
#include "__aabb_tri.h"
#include "xrPhysics/MathUtils.h"
#include "xrPhysics/console_vars.h"
#include "xrPhysics/PHWorld.h"
#include "xrCDB/xr_area.h"
#include "xrEngine/GameMtlLib.h"
#ifdef DEBUG
#include "xrPhysics/debug_output.h"
#endif


IC bool negative_tri_set_ignored_by_positive_tri(
    const Triangle& neg_tri, const Triangle& pos_tri, const Fvector* V_array)
{
    bool common0 = neg_tri.T->verts[0] == pos_tri.T->verts[0] || neg_tri.T->verts[0] == pos_tri.T->verts[1] ||
        neg_tri.T->verts[0] == pos_tri.T->verts[2];

    bool common1 = neg_tri.T->verts[1] == pos_tri.T->verts[0] || neg_tri.T->verts[1] == pos_tri.T->verts[1] ||
        neg_tri.T->verts[1] == pos_tri.T->verts[2];

    bool common2 = neg_tri.T->verts[2] == pos_tri.T->verts[0] || neg_tri.T->verts[2] == pos_tri.T->verts[1] ||
        neg_tri.T->verts[2] == pos_tri.T->verts[2];

    return (common0 && common1 && common2) ||
        !common0 && !(dDOT(neg_tri.norm, (dReal*)&V_array[pos_tri.T->verts[0]]) > neg_tri.pos) ||
        !common1 && !(dDOT(neg_tri.norm, (dReal*)&V_array[pos_tri.T->verts[1]]) > neg_tri.pos) ||
        !common2 && !(dDOT(neg_tri.norm, (dReal*)&V_array[pos_tri.T->verts[2]]) > neg_tri.pos);
}

int SetBackTrajectoryCnt(
    const dReal* p, const dReal* last_pos, Triangle& neg_tri, dxGeom* o1, dxGeom* o2, dContactGeom* Contacts)
{
    Contacts->g1 = const_cast<dxGeom*>(o2);
    Contacts->g2 = const_cast<dxGeom*>(o1);
    Contacts->normal[0] = -(last_pos[0] - p[0]);
    Contacts->normal[1] = -(last_pos[1] - p[1]);
    Contacts->normal[2] = -(last_pos[2] - p[2]);

    dReal sq_mag = dDOT(Contacts->normal, Contacts->normal);
    if (sq_mag < EPS_S)
    {
        Contacts->normal[0] = 0;
        Contacts->normal[1] = -1;
        Contacts->normal[2] = 0;
        Contacts->depth = 0.f;
    }
    else
    {
        Contacts->depth = dSqrt(sq_mag); // neg_tri.depth;//
        dReal r_mag = 1.f / Contacts->depth;
        Contacts->normal[0] *= r_mag;
        Contacts->normal[1] *= r_mag;
        Contacts->normal[2] *= r_mag;
    }

    Contacts->pos[0] = p[0];
    Contacts->pos[1] = p[1];
    Contacts->pos[2] = p[2];

    SURFACE(Contacts, 0)->mode = neg_tri.T->material;

    if (dGeomGetUserData(o1)->callback)
        dGeomGetUserData(o1)->callback(neg_tri.T, Contacts);
    return 1;
}

template <class T>
IC int dcTriListCollider::dSortTriPrimitiveCollide(
    T primitive, dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip, const Fvector& AABB)
{
    dxGeomUserData* data = dGeomGetUserData(o1);
    dReal* last_pos = data->last_pos;
    bool no_last_pos = last_pos[0] == -dInfinity;
    const dReal* p = dGeomGetPosition(o1);

    Fbox last_box;
    last_box.setb(data->last_aabb_pos, data->last_aabb_size);
    Fbox box;
    box.setb(cast_fv(p), AABB);

    // VERIFY( g_pGameLevel );
    CDB::TRI* T_array = inl_ph_world().ObjectSpace().GetStaticTris();
    const Fvector* V_array = inl_ph_world().ObjectSpace().GetStaticVerts();
    if (no_last_pos || !last_box.contains(box))
    {
        Fvector aabb;
        aabb.set(AABB);
        aabb.mul(ph_console::ph_tri_query_ex_aabb_rate);
        ///////////////////////////////////////////////////////////////////////////////////////////////
        XRC.box_options(0);
        // VERIFY( g_pGameLevel );
        XRC.box_query(inl_ph_world().ObjectSpace().GetStaticModel(), cast_fv(p), aabb);

#ifdef DEBUG
        debug_output().dbg_total_saved_tries() -= data->cashed_tries.size();
        debug_output().dbg_new_queries_per_step()++;
#endif
        data->cashed_tries.clear();

        for (auto &Res : *XRC.r_get())
            data->cashed_tries.push_back(Res.id);

#ifdef DEBUG
        debug_output().dbg_total_saved_tries() += data->cashed_tries.size();
#endif
        data->last_aabb_pos.set(cast_fv(p));
        data->last_aabb_size.set(aabb);
    }
#ifdef DEBUG
    else
        debug_output().dbg_reused_queries_per_step()++;
#endif
    ///////////////////////////////////////////////////////////////////////////////////////////////
    int ret = 0;

    pos_tries.clear();
    dReal neg_depth = dInfinity, b_neg_depth = dInfinity;
    UINT b_count = 0;
    bool intersect = false;

#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phDbgDrawTriTestAABB))
        debug_output().DBG_DrawAABB(cast_fv(p), AABB, color_xrgb(0, 0, 255));
#endif

    bool* pushing_neg = &data->pushing_neg;
    bool* pushing_b_neg = &data->pushing_b_neg;
    bool spushing_neg = *pushing_neg;
    bool spushing_b_neg = *pushing_b_neg;
    Triangle neg_tri; //=&(data->neg_tri);
    Triangle b_neg_tri; //=&(data->b_neg_tri);
    bool neg_tri_contains_point = true;
    if (*pushing_neg)
    {
        CalculateTri(data->neg_tri, p, neg_tri, V_array);
        const dReal* neg_vertices[3] = {cast_fp(V_array[neg_tri.T->verts[0]]), cast_fp(V_array[neg_tri.T->verts[1]]),
            cast_fp(V_array[neg_tri.T->verts[2]])};
        neg_tri_contains_point = TriContainPoint(
            neg_vertices[0], neg_vertices[1], neg_vertices[2], neg_tri.norm, neg_tri.side0, neg_tri.side1, p);
        bool b_neg_dist = neg_tri.dist < 0.f;
        if (b_neg_dist || (!neg_tri_contains_point && !no_last_pos))
        {
            dReal sidePr = primitive.Proj(o1, neg_tri.norm);
            neg_tri.depth = sidePr - neg_tri.dist;
            neg_depth = neg_tri.depth;
            intersect = true;
        }
        else
        {
            *pushing_neg = false;
        }

#ifdef DEBUG
        if (debug_output().ph_dbg_draw_mask().test(phDbgDrawSavedTries))
            debug_output().DBG_DrawTri(neg_tri.T, V_array, color_xrgb(255, 0, 0));
#endif
    }

    if (*pushing_b_neg)
    {
        CalculateTri(data->b_neg_tri, p, b_neg_tri, V_array);
        if (b_neg_tri.dist < 0.f)
        {
            dReal sidePr = primitive.Proj(o1, b_neg_tri.norm);
            b_neg_tri.depth = sidePr - b_neg_tri.dist;
            b_neg_depth = b_neg_tri.depth;
        }
        else
        {
            *pushing_b_neg = false;
        }

#ifdef DEBUG
        if (debug_output().ph_dbg_draw_mask().test(phDbgDrawSavedTries))
            debug_output().DBG_DrawTri(b_neg_tri.T, V_array, color_xrgb(0, 0, 255));
#endif
    }

    bool b_pushing = *pushing_neg; //||*pushing_b_neg;
    gl_cl_tries_state.resize(data->cashed_tries.size(), Flags8().assign(0));
    B = data->cashed_tries.begin(), E = data->cashed_tries.end();
    bool gb_pased = false;
    for (I = B; I != E; ++I)
    {
#ifdef DEBUG
        debug_output().dbg_saved_tries_for_active_objects()++;
#endif
        // if(ignored_tries[I-B])continue;
		CDB::TRI* Tr = T_array + *I;
		const Point vertices[3] = {Point((dReal*)&V_array[Tr->verts[0]]), Point((dReal*)&V_array[Tr->verts[1]]),
			Point((dReal*)&V_array[Tr->verts[2]])};
        if (!aabb_tri_aabb(Point(p), Point((float*)&AABB), vertices))
            continue;
#ifdef DEBUG
        if (debug_output().ph_dbg_draw_mask().test(phDBgDrawIntersectedTries))
			debug_output().DBG_DrawTri(Tr, V_array, color_xrgb(0, 255, 0));
        debug_output().dbg_tries_num()++;
#endif
        Triangle tri;
		CalculateTri(Tr, p, tri, vertices);
        if (tri.dist < 0.f)
        {
#ifdef DEBUG
            if (debug_output().ph_dbg_draw_mask().test(phDBgDrawNegativeTries))
                debug_output().DBG_DrawTri(Tr, V_array, color_xrgb(0, 0, 255));
#endif
            float last_pos_dist = dDOT(last_pos, tri.norm) - tri.pos;
            if ((!(last_pos_dist < 0.f)) || b_pushing)
                if (__aabb_tri(Point(p), Point((float*)&AABB), vertices))
                {
#ifdef DEBUG
                    if (debug_output().ph_dbg_draw_mask().test(phDBgDrawTriesChangesSign))
						debug_output().DBG_DrawTri(Tr, V_array, color_xrgb(0, 255, 0));
#endif
					SGameMtl* material = GMLibrary().GetMaterialByIdx(Tr->material);
                    VERIFY(material);
                    bool b_passable = !!material->Flags.test(SGameMtl::flPassable);
                    bool contain_pos =
                        TriContainPoint(vertices[0], vertices[1], vertices[2], tri.norm, tri.side0, tri.side1, p);
                    bool b_pased = false;
                    if (!b_pushing && !gb_pased)
                    {
                        if (!no_last_pos && !b_passable)
                        {
#ifdef DEBUG
                            if (debug_output().ph_dbg_draw_mask().test(phDbgDrawTriTrace))
                                debug_output().DBG_DrawLine(cast_fv(last_pos), cast_fv(p), color_xrgb(255, 0, 255));
#endif
                            dVector3 tri_point;
                            PlanePoint(tri, last_pos, p, last_pos_dist, tri_point);
#ifdef DEBUG
                            if (debug_output().ph_dbg_draw_mask().test(phDbgDrawTriPoint))
                                debug_output().DBG_DrawPoint(cast_fv(tri_point), 0.01f, color_xrgb(255, 0, 255));
#endif
                            bool was_intersect = intersect;
                            intersect = intersect || TriContainPoint(vertices[0], vertices[1], vertices[2], tri.norm,
                                                         tri.side0, tri.side1, tri_point);
                            b_pased = intersect && !was_intersect;
                            gb_pased = b_pased || gb_pased;
#ifdef DEBUG
                            if (b_pased && debug_output().ph_dbg_draw_mask().test(phDbgDrawTriPoint))
                            {
                                dVectorSet(last_pos, tri_point);
                                debug_output().DBG_OpenCashedDraw();
                                debug_output().DBG_DrawPoint(cast_fv(tri_point), 0.01f, color_xrgb(255, 0, 255));
                                debug_output().DBG_ClosedCashedDraw(1000000);
                            }
#endif
                        }
                        else
                        {
                            if (contain_pos && primitive.Proj(o1, tri.norm) > -tri.dist)
                                intersect = true;
                        }
                    }
                    else
                    {
                        intersect = true;
                    }

                    if (!b_passable && (b_pased || contain_pos && no_last_pos))
                    {
                        dReal sidePr = primitive.Proj(o1, tri.norm);
                        tri.depth = sidePr - tri.dist;
                        if (neg_depth > tri.depth &&
                            (!(*pushing_neg || spushing_neg) || dDOT(neg_tri.norm, tri.norm) > -M_SQRT1_2) &&
                            (!(*pushing_b_neg || spushing_b_neg) ||
                                dDOT(b_neg_tri.norm, tri.norm) > -M_SQRT1_2)) // exclude switching on opposite side
                        // &&(!*pushing_b_neg||dDOT(b_neg_tri->norm,tri.norm)>-M_SQRT1_2)
                        {
                            neg_depth = tri.depth;
                            neg_tri = tri;
                            data->neg_tri = tri.T;
                            // ret=0;
                            // if(intersect)*pushing_neg=true;
                        }
                    }

                    if (!b_pased && b_passable)
                    { //! contain_pos&&
                        ++b_count;
                        dReal sidePr = primitive.Proj(o1, tri.norm);
                        tri.depth = sidePr - tri.dist;
                        if (b_neg_depth > tri.depth &&
                            (!(*pushing_b_neg || spushing_b_neg) || dDOT(b_neg_tri.norm, tri.norm) > -M_SQRT1_2) &&
                            ((!*pushing_neg || !spushing_neg) || dDOT(neg_tri.norm, tri.norm) > -M_SQRT1_2))
                        { // exclude switching on opposite side
                            // &&(!*pushing_neg||dDOT(neg_tri->norm,tri.norm)>-M_SQRT1_2)
                            b_neg_depth = tri.depth;
                            b_neg_tri = tri;
                            data->b_neg_tri = tri.T;
                            // ret=0;
                            // if(intersect)*pushing_b_neg=true;
                        }
                    }
                }
        }
        else
        {
#ifdef DEBUG
            if (debug_output().ph_dbg_draw_mask().test(phDBgDrawPositiveTries))
				debug_output().DBG_DrawTri(Tr, V_array, color_xrgb(255, 0, 0));
#endif
            if (ret > flags - 10)
                continue;
            if (!b_pushing && (!intersect || no_last_pos))
                ret += primitive.Collide(
                    vertices[0], vertices[1], vertices[2], &tri, o1, o2, 3, CONTACT(contact, ret * skip), skip);
            if (no_last_pos)
                pos_tries.push_back(tri);
        }
	}

    // if(intersect) ret=0;
    //xr_vector<Triangle>::iterator i;

    if (intersect)
    {
        if (neg_depth < dInfinity)
        {
            bool include = true;
            if (no_last_pos)
            {
                for (auto& it : pos_tries)
                {
                    VERIFY(neg_tri.T);
                    if (TriContainPoint((dReal*)&V_array[it.T->verts[0]], (dReal*)&V_array[it.T->verts[1]],
                        (dReal*)&V_array[it.T->verts[2]], it.norm, it.side0, it.side1, p))
                        if (negative_tri_set_ignored_by_positive_tri(neg_tri, it, V_array))
                        {
                            include = false;
                            break;
                        }
                }
            }

            if (include)
            {
                VERIFY(neg_tri.T && neg_tri.dist != -dInfinity);
                // const dReal*
                // neg_vertices[3]={cast_fp(V_array[neg_tri.T->verts[0]]),cast_fp(V_array[neg_tri.T->verts[1]]),cast_fp(V_array[neg_tri.T->verts[2]])};
                // neg_tri_contains_point = TriContainPoint(neg_vertices[0],neg_vertices[1],neg_vertices[2],
                // neg_tri.norm,neg_tri.side0,
                // neg_tri.side1,p);
                if (neg_tri_contains_point)
                {
                    int bret = primitive.CollidePlain(neg_tri.side0, neg_tri.side1, neg_tri.norm, neg_tri.T,
                        neg_tri.dist, o1, o2, flags, CONTACT(contact, 0), skip);
                    *pushing_neg = !!bret;
                    if (*pushing_neg)
                        ret = bret;
                }
                else
                    ret = SetBackTrajectoryCnt(p, last_pos, neg_tri, o1, o2, CONTACT(contact, 0));
            }
        }

        // for(i=pos_tries.begin();pos_tries.end() != i;++i){
        //	CDB::TRI* T=i->T;
        //	ret+=dTriPrimitive(
        //		(const dReal*)&V_array[T->verts[0]],
        //		(const dReal*)&V_array[T->verts[1]],
        //		(const dReal*)&V_array[T->verts[2]],
        //		T,
        //		o1,
        //		o2,
        //		3,
        //		CONTACT(contact, ret * skip),   skip);

        //}
    }

    if (b_neg_depth < dInfinity)
    {
        bool include = true;
        if (no_last_pos)
        {
            for (auto& it : pos_tries)
            {
                VERIFY(b_neg_tri.T && b_neg_tri.dist != -dInfinity);
                if (negative_tri_set_ignored_by_positive_tri(b_neg_tri, it, V_array))
                {
                    include = false;
                    break;
                }
            }
        }

        if (include)
        {
            VERIFY(b_neg_tri.T);
            int bret = 0;
            if (ret < flags - 10)
                bret = primitive.CollidePlain(b_neg_tri.side0, b_neg_tri.side1, b_neg_tri.norm, b_neg_tri.T,
                    b_neg_tri.dist, o1, o2, flags, CONTACT(contact, *pushing_neg ? ret * skip : 0), skip);
            *pushing_b_neg = !!bret;
            if (*pushing_neg)
                ret += bret;
            else if (*pushing_b_neg)
                ret = bret;
        }
    }
    if (!*pushing_neg) // no_last_pos|| && !*pushing_b_neg
        dVectorSet(last_pos, p);
    return ret;
}
