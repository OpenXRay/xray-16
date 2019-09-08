#include "stdafx.h"
#include "Feel_Vision.h"
#include "Render.h"
#include "xr_object.h"
#include "xr_collide_form.h"
#include "IGame_Level.h"
#include "xrCDB/Intersect.hpp"

namespace Feel
{
Vision::Vision(IGameObject const* owner) : pure_relcase(&Vision::feel_vision_relcase), m_owner(owner) {}
Vision::~Vision() {}
struct SFeelParam
{
    Vision* parent;
    Vision::feel_visible_Item* item;
    float vis;
    float vis_threshold;
    SFeelParam(Vision* _parent, Vision::feel_visible_Item* _item, float _vis_threshold)
        : parent(_parent), item(_item), vis(1.f), vis_threshold(_vis_threshold)
    {
    }
};
IC BOOL feel_vision_callback(collide::rq_result& result, LPVOID params)
{
    SFeelParam* fp = (SFeelParam*)params;
    float vis = fp->parent->feel_vision_mtl_transp(result.O, result.element);
    fp->vis *= vis;
    if (NULL == result.O && fis_zero(vis))
    {
        CDB::TRI* T = g_pGameLevel->ObjectSpace.GetStaticTris() + result.element;
        Fvector* V = g_pGameLevel->ObjectSpace.GetStaticVerts();
        fp->item->Cache.verts[0].set(V[T->verts[0]]);
        fp->item->Cache.verts[1].set(V[T->verts[1]]);
        fp->item->Cache.verts[2].set(V[T->verts[2]]);
    }
    return (fp->vis > fp->vis_threshold);
}
void Vision::o_new(IGameObject* O)
{
    feel_visible.push_back(feel_visible_Item());
    feel_visible_Item& I = feel_visible.back();
    I.O = O;
    I.Cache_vis = 1.f;
    I.Cache.verts[0].set(0, 0, 0);
    I.Cache.verts[1].set(0, 0, 0);
    I.Cache.verts[2].set(0, 0, 0);
    I.fuzzy = -EPS_S;
    I.cp_LP = O->get_new_local_point_on_mesh(I.bone_id);
    I.cp_LAST = O->get_last_local_point_on_mesh(I.cp_LP, I.bone_id);
}
void Vision::o_delete(IGameObject* O)
{
    xr_vector<feel_visible_Item>::iterator I = feel_visible.begin(), TE = feel_visible.end();
    for (; I != TE; ++I)
        if (I->O == O)
        {
            feel_visible.erase(I);
            return;
        }
}

void Vision::feel_vision_clear()
{
    seen.clear();
    query.clear();
    diff.clear();
    feel_visible.clear();
}

void Vision::feel_vision_relcase(IGameObject* object)
{
    xr_vector<IGameObject*>::iterator Io;
    Io = std::find(seen.begin(), seen.end(), object);
    if (Io != seen.end())
        seen.erase(Io);
    Io = std::find(query.begin(), query.end(), object);
    if (Io != query.end())
        query.erase(Io);
    Io = std::find(diff.begin(), diff.end(), object);
    if (Io != diff.end())
        diff.erase(Io);
    xr_vector<feel_visible_Item>::iterator Ii = feel_visible.begin(), IiE = feel_visible.end();
    for (; Ii != IiE; ++Ii)
        if (Ii->O == object)
        {
            feel_visible.erase(Ii);
            break;
        }
}

void Vision::feel_vision_query(Fmatrix& mFull, Fvector& P)
{
    CFrustum Frustum;
    Frustum.CreateFromMatrix(mFull, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    // Traverse object database
    r_spatial.clear();
    g_SpatialSpace->q_frustum(r_spatial, 0, STYPE_VISIBLEFORAI, Frustum);

    // Determine visibility for dynamic part of scene
    seen.clear();
    for (u32 o_it = 0; o_it < r_spatial.size(); o_it++)
    {
        ISpatial* spatial = r_spatial[o_it];
        IGameObject* object = spatial->dcast_GameObject();
        if (object && feel_vision_isRelevant(object))
            seen.push_back(object);
    }
    if (seen.size() > 1)
    {
        std::sort(seen.begin(), seen.end());
        xr_vector<IGameObject*>::iterator end = std::unique(seen.begin(), seen.end());
        if (end != seen.end())
            seen.erase(end, seen.end());
    }
}

void Vision::feel_vision_update(IGameObject* parent, Fvector& P, float dt, float vis_threshold)
{
    // B-A = objects, that become visible
    if (!seen.empty())
    {
        xr_vector<IGameObject*>::iterator E = std::remove(seen.begin(), seen.end(), parent);
        seen.resize(E - seen.begin());

        {
            diff.resize(_max(seen.size(), query.size()));
            xr_vector<IGameObject*>::iterator E2 =
                std::set_difference(seen.begin(), seen.end(), query.begin(), query.end(), diff.begin());
            diff.resize(E2 - diff.begin());
            for (u32 i = 0; i < diff.size(); i++)
                o_new(diff[i]);
        }
    }

    // A-B = objects, that are invisible
    if (!query.empty())
    {
        diff.resize(_max(seen.size(), query.size()));
        xr_vector<IGameObject*>::iterator E =
            std::set_difference(query.begin(), query.end(), seen.begin(), seen.end(), diff.begin());
        diff.resize(E - diff.begin());
        for (u32 i = 0; i < diff.size(); i++)
            o_delete(diff[i]);
    }

    // Copy results and perform traces
    query = seen;
    o_trace(P, dt, vis_threshold);
}
void Vision::o_trace(Fvector& P, float dt, float vis_threshold)
{
    RQR.r_clear();
    xr_vector<feel_visible_Item>::iterator I = feel_visible.begin(), E = feel_visible.end();
    for (; I != E; ++I)
    {
        if (0 == I->O->GetCForm())
        {
            I->fuzzy = -1;
            continue;
        }

        // verify relation
        // if (positive(I->fuzzy) && I->O->Position().similar(I->cp_LR_dst,lr_granularity) &&
        // P.similar(I->cp_LR_src,lr_granularity))
        // continue;

        I->cp_LR_dst = I->O->Position();
        I->cp_LR_src = P;
        I->cp_LAST = I->O->get_last_local_point_on_mesh(I->cp_LP, I->bone_id);

        //
        Fvector D, OP = I->cp_LAST;
        D.sub(OP, P);
        if (fis_zero(D.magnitude()))
        {
            I->fuzzy = 1.f;
            continue;
        }

        float f = D.magnitude() + .2f;
        if (f > fuzzy_guaranteed)
        {
            D.div(f);
            // setup ray defs & feel params
            collide::ray_defs RD(P, D, f, CDB::OPT_CULL,
                collide::rq_target(collide::rqtStatic | collide::rqtObject | collide::rqtObstacle));
            SFeelParam feel_params(this, &*I, vis_threshold);
            // check cache
            if (I->Cache.result && I->Cache.similar(P, D, f))
            {
                // similar with previous query
                feel_params.vis = I->Cache_vis;
                // Log("cache 0");
            }
            else
            {
                float _u, _v, _range;
                if (CDB::TestRayTri(P, D, I->Cache.verts, _u, _v, _range, false) && (_range > 0 && _range < f))
                {
                    feel_params.vis = 0.f;
                    // Log("cache 1");
                }
                else
                {
                    // cache outdated. real query.
                    VERIFY(!fis_zero(RD.dir.magnitude()));

                    if (g_pGameLevel->ObjectSpace.RayQuery(RQR, RD, feel_vision_callback, &feel_params, NULL, NULL))
                    {
                        I->Cache_vis = feel_params.vis;
                        I->Cache.set(P, D, f, TRUE);
                    }
                    else
                    {
                        // feel_params.vis = 0.f;
                        // I->Cache_vis = feel_params.vis ;
                        I->Cache.set(P, D, f, FALSE);
                    }
                    // Log("query");
                }
            }
            // Log("Vis",feel_params.vis);
            r_spatial.clear();
            g_SpatialSpace->q_ray(r_spatial, 0, STYPE_VISIBLEFORAI, P, D, f);

            RD.flags = CDB::OPT_ONLYFIRST;

            bool collision_found = false;
            xr_vector<ISpatial*>::const_iterator i = r_spatial.begin();
            xr_vector<ISpatial*>::const_iterator e = r_spatial.end();
            for (; i != e; ++i)
            {
                if (*i == m_owner)
                    continue;

                if (*i == I->O)
                    continue;

                IGameObject const* object = (*i)->dcast_GameObject();
                RQR.r_clear();
                if (object && object->GetCForm() && !object->GetCForm()->_RayQuery(RD, RQR))
                    continue;

                collision_found = true;
                break;
            }

            if (collision_found)
                feel_params.vis = 0.f;

            if (feel_params.vis < feel_params.vis_threshold)
            {
                // INVISIBLE, choose next point
                I->fuzzy -= fuzzy_update_novis * dt;
                clamp(I->fuzzy, -.5f, 1.f);
                I->cp_LP = I->O->get_new_local_point_on_mesh(I->bone_id);
            }
            else
            {
                // VISIBLE
                I->fuzzy += fuzzy_update_vis * dt;
                clamp(I->fuzzy, -.5f, 1.f);
            }
        }
        else
        {
            // VISIBLE, 'cause near
            I->fuzzy += fuzzy_update_vis * dt;
            clamp(I->fuzzy, -.5f, 1.f);
        }
    }
}
};
