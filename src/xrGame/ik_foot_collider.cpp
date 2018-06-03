#include "stdafx.h"
#include "ik_foot_collider.h"

#include "xrEngine/GameMtlLib.h"
#include "xrCDB/Intersect.hpp"
#include "Include/xrRender/Kinematics.h"

#include "Level.h"
#include "GameObject.h"
#include "entity_alive.h"

#include "xrPhysics/MathUtils.h"

#include "ik_collide_data.h"

#ifdef DEBUG
#include "PHDebug.h"
#endif

ik_foot_collider::ik_foot_collider() {}
static const Fplane invalide_plane = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};

struct ik_pick_result
{
    ik_pick_result(ik_foot_geom::e_collide_point _point)
        : p(invalide_plane), position(Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX)),
          point(_point), range(0)
    {
        triangle[0] = Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        triangle[1] = Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        triangle[2] = Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    Fplane p;
    Fvector triangle[3];
    Fvector position;
    ik_foot_geom::e_collide_point point;
    float range;
};

bool ignore_tri(CDB::TRI& tri)
{
    SGameMtl* material = GMLib.GetMaterialByIdx(tri.material);

    return (material->Flags.test(SGameMtl::flPassable) && !material->Flags.test(SGameMtl::flActorObstacle)) ||
        material->Flags.test(SGameMtl::flClimable); // ||
    // material->Flags.test( SGameMtl::flActorObstacle );
}

bool ignore_static_tri(int tri)
{
    VERIFY(Level().ObjectSpace.GetStaticModel()->get_tris_count() > tri);
    CDB::TRI* triangle = Level().ObjectSpace.GetStaticTris() + tri;
    return ignore_tri(*triangle);
}

IC bool ignore_object(IGameObject* O)
{
    VERIFY(O);
    if (static_cast<CGameObject*>(O)
            ->cast_entity_alive() /*&& static_cast<CGameObject*>( O )->cast_entity_alive()->g_Alive() */)
        return true;
    return false;
}

IC bool ignore_result(collide::rq_result& R)
{
    if (R.O)
        return ignore_object(R.O);
    else
        return ignore_static_tri(R.element);
}

IC void tri_plane(const Fvector& v0, const Fvector& v1, const Fvector& v2, Fplane& p)
{
    p.n.mknormal(v0, v1, v2);
    VERIFY(!fis_zero(p.n.magnitude()));
    p.n.invert();
    p.d = -p.n.dotproduct(v0);
}

IC void tri_plane(const CDB::TRI& tri, Fplane& p)
{
    Fvector* pVerts = Level().ObjectSpace.GetStaticVerts();
    tri_plane(pVerts[tri.verts[0]], pVerts[tri.verts[1]], pVerts[tri.verts[2]], p);
}

IC bool get_plane_static(ik_pick_result& r, Fvector& next_pos, float& next_range, const collide::rq_result& R,
    float pick_dist, const Fvector& pos, const Fvector& pick_v)
{
    VERIFY(Level().ObjectSpace.GetStaticModel()->get_tris_count() > R.element);
    CDB::TRI* tri = Level().ObjectSpace.GetStaticTris() + R.element;
    Fvector* pVerts = Level().ObjectSpace.GetStaticVerts();

    r.triangle[0] = pVerts[tri->verts[0]];
    r.triangle[1] = pVerts[tri->verts[1]];
    r.triangle[2] = pVerts[tri->verts[2]];

    tri_plane(r.triangle[0], r.triangle[1], r.triangle[2], r.p);

    r.position.add(pos, Fvector().mul(pick_v, R.range));
    next_pos.set(r.position);
    next_range = pick_dist - R.range;
    if (ignore_tri(*tri))
    {
        next_pos.add(Fvector().mul(pick_v, EPS_L));
        float dot = pick_v.dotproduct(r.p.n);
        if (0.f < dot)
        {
            next_pos.add(Fvector().mul(r.p.n, EPS_L));
        }
        // next_pos.add( Fvector().mul( r.p.n, EPS_L  ) );
        next_range -= EPS_L;
#ifdef DEBUG
        float u, v, d;
        VERIFY(!(CDB::TestRayTri(next_pos, pick_v, r.triangle, u, v, d, true) && d > 0.f));
#endif // DEBUG
        return false;
    }
    r.range = R.range;
    return true;
}

IC bool get_plane_dynamic(ik_pick_result& r, Fvector& next_pos, float& next_range, const collide::rq_result R,
    float pick_dist, const Fvector& pos, const Fvector& pick_v)
{
    next_pos.add(pos, Fvector().mul(pick_v, R.range + EPS_L));
    next_range = pick_dist - R.range - EPS_L;

    if (ignore_object(R.O))
        return false;

    IRenderVisual* V = R.O->Visual();
    if (V)
    {
        IKinematics* K = V->dcast_PKinematics();
        if (K)
        {
            float dist = pick_dist;
            IKinematics::pick_result res;

            if (K->PickBone(R.O->XFORM(), res, dist, pos, pick_v, (u16)R.element))
            {
                // cld.collided = true;
                r.position.add(pos, Fvector().mul(pick_v, res.dist));
                r.p.n.invert(res.normal);
                r.p.d = -r.p.n.dotproduct(r.position);
                r.triangle[0] = res.tri[0];
                r.triangle[1] = res.tri[1];
                r.triangle[2] = res.tri[2];
                next_pos.set(r.position);
                next_range = pick_dist - res.dist;
                r.range = res.dist;
                return true;
            }
        }
    }
    return false;
}

static const float reach_dist = 1.5f;
IC bool get_plane(ik_pick_result& r, Fvector& next_pos, float& next_range, const collide::rq_result R, float pick_dist,
    const Fvector& pos, const Fvector& pick_v)
{
    if (!R.O)
        return get_plane_static(r, next_pos, next_range, R, pick_dist, pos, pick_v);
    else
        return get_plane_dynamic(r, next_pos, next_range, R, pick_dist, pos, pick_v);
}

bool Pick(ik_pick_result& r, const ik_pick_query& q, IGameObject* ignore_object)
{
    VERIFY(q.is_valid());

    float range = q.range();

    collide::rq_result R;
    bool collided = false;
    Fvector pos = q.pos();

    while (g_pGameLevel->ObjectSpace.RayPick(pos, q.dir(), range, collide::rqtBoth, R, ignore_object))
    {
        Fvector next_pos = pos;
        float next_range = range;

        collided = get_plane(r, next_pos, next_range, R, range, pos, q.dir());
        if (collided)
            break;

        range = next_range;
        pos = next_pos;
        if (range < EPS)
            break;
    }

#ifdef DEBUG
    if (ph_dbg_draw_mask1.test(phDbgDrawIKCollision) && collided && !R.O)
    {
        CDB::TRI* tri = Level().ObjectSpace.GetStaticTris() + R.element;
        Fvector p = q.pos();
        p.add(Fvector().mul(q.dir(), range));
        DBG_DrawLine(pos, p, color_xrgb(255, 0, 0));
        if (tri)
        {
            DBG_DrawTri(tri, Level().ObjectSpace.GetStaticVerts(), color_xrgb(255, 0, 0));
        }
    }
#endif
    return collided;
}

// void DBG_DrawTri( const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 ac, bool solid );

void chose_best_plane(Fplane& p, const Fvector& v, Fplane& p0, Fplane& p1, Fplane& p2)
{
    MAX_OF(p0.n.dotproduct(v), p = p0, p1.n.dotproduct(v), p = p1, p2.n.dotproduct(v), p = p2);
}

#ifdef DEBUG
void DBG_DrawTri(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 ac, bool solid);
#endif

void ik_foot_collider::collide(SIKCollideData& cld, const ik_foot_geom& foot_geom, CGameObject* O, bool foot_step)
{
    VERIFY(foot_geom.is_valid());
    cld.collided = false;

    float pick_dist = collide_dist;
    // if( foot_step )
    pick_dist += reach_dist;

    //////////////////////////////////////////////////////////////////////////////////////
    const Fvector& toe_pick_v = cld.m_pick_dir;
    const Fvector pos_toe = Fvector().sub(foot_geom.pos_toe(), Fvector().mul(toe_pick_v, collide_dist));
    ik_pick_query q_toe(ik_foot_geom::toe, pos_toe, toe_pick_v, pick_dist);

    ////////////////////////////////////////////////////////////////////////////////////////
    const Fvector hill_pick_v = cld.m_pick_dir;
    const Fvector pos_heel = Fvector().sub(foot_geom.pos_heel(), Fvector().mul(hill_pick_v, collide_dist));
    ik_pick_query q_heel(ik_foot_geom::heel, pos_heel, hill_pick_v, pick_dist);

    //////////////////////////////////////////////////////////////////////////////////////////
    const Fvector side_pick_v = cld.m_pick_dir;
    const Fvector pos_side = Fvector().sub(foot_geom.pos_side(), Fvector().mul(side_pick_v, collide_dist));
    ik_pick_query q_side(ik_foot_geom::side, pos_side, side_pick_v, pick_dist);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (previous_toe_query.is_equal(q_toe) && previous_heel_query.is_equal(q_heel) &&
        previous_side_query.is_equal(q_side))
    {
        cld = previous_data;
        return;
    }

    const float foot_length = Fvector().sub(pos_toe, pos_heel).magnitude() * 1.5f;

    previous_heel_query = q_heel;
    previous_side_query = q_side;
    previous_toe_query = q_toe;

    ik_pick_result r_toe(ik_foot_geom::toe);
    cld.collided = Pick(r_toe, q_toe, O);
    cld.m_plane = r_toe.p;
    cld.m_collide_point = ik_foot_geom::toe;
//////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
    if (ph_dbg_draw_mask1.test(phDbgDrawIKCollision))
    {
        DBG_DrawPoint(pos_toe, 0.01, color_xrgb(255, 0, 0));
        if (cld.collided)
            DBG_DrawPoint(r_toe.position, 0.01, color_xrgb(0, 0, 255));
    }
#endif

    ik_pick_result r_heel(ik_foot_geom::heel);
    bool heel_collided = Pick(r_heel, q_heel, O);

    ik_pick_result r_side(ik_foot_geom::side);
    bool side_collided = Pick(r_side, q_side, O);

    bool toe_heel_compatible =
        cld.collided && heel_collided && Fvector().sub(r_heel.position, r_toe.position).magnitude() < foot_length;
    bool toe_side_compatible =
        cld.collided && side_collided && Fvector().sub(r_side.position, r_toe.position).magnitude() < foot_length;

    /*
        if( hill_collided )
        {
            if( !cld.collided || (r_hill.position.y - r_toe.position.y) > EPS )
            {
                cld.m_plane = r_heel.p;
                cld.m_collide_point = ik_foot_geom::heel;
                cld.collided = true;
                cld.m_pick_dir = heel_pick_v;

            }
        }
    */

    if (toe_heel_compatible && toe_side_compatible)
    {
        Fplane plane;
        tri_plane(r_toe.position, r_heel.position, r_side.position, plane);
        if (plane.n.dotproduct(r_toe.p.n) < 0.f)
        {
            plane.n.invert();
            plane.d = -plane.d;
        }

        cld.m_plane = plane;
#ifdef DEBUG
        if (ph_dbg_draw_mask1.test(phDbgDrawIKCollision))
        {
            DBG_DrawPoint(pos_toe, 0.01, color_xrgb(255, 0, 0));
            if (cld.collided)
            {
                DBG_DrawTri(r_toe.position, r_heel.position, r_side.position, color_xrgb(0, 0, 255), false);
            }
        }
#endif
        previous_data = cld;
        return;
    }

    float hight = -FLT_MAX;
    ik_pick_result r = r_toe;

    if (cld.collided)
    {
        hight = r_toe.position.y;
    }

    if (heel_collided && r_heel.position.y > hight)
    {
        r = r_heel;
        hight = r_heel.position.y;
        cld.collided = true;
    }

    if (side_collided && r_side.position.y > hight)
    {
        r = r_side;
        hight = r_side.position.y;
        cld.collided = true;
    }

    if (cld.collided)
    {
        cld.m_plane = r.p;
        cld.m_collide_point = r.point;
        previous_data = cld;
        return;
    }

    // chose_best_plane( cld.m_plane, pick_v, plane, r_hill.p, r_toe.p   );

    previous_data = cld;

    /*
        float u,v,d;
        if( ( !cld.collided ||
              !( CDB::TestRayTri(pos_hill, pick_v, r_toe.triangle, u, v, d, true ) && d > 0.f ) ) &&
                Pick( r_hill, pos_hill, pick_v, pick_dist, O ) )
        {

            if( !cld.collided || r_hill.position.y > r_toe.position.y )
            {
                cld.m_plane = r_hill.p;
                cld.m_collide_point = SIKCollideData::heel;
            }

            //else
            //{
            //		ik_pick_result r_foot;
            //		Fvector pos_foot = Fvector().sub( foot.c, Fvector( ).mul( pick_v, collide_dist ) );
            //		if( cld.collided && Pick( r_foot, pos_foot, pick_v, pick_dist, O ) )
            //		{
            //			Fplane plane;
            //			tri_plane( r_toe.position, r_hill.position , r_foot.position, plane );

            //			DBG_DrawTri(r_toe.position, r_hill.position, r_foot.position , color_xrgb( 255, 255, 255 ),
       false  );
            //
            //			if( plane.n.dotproduct( r_hill.p.n ) < 0.f )
            //			{
            //				plane.n.invert();
            //				plane.d = -cld.m_plane.d;
            //
            //			}
            //			chose_best_plane( cld.m_plane, pick_v, plane, r_hill.p, r_toe.p   );
            //		}
            //}

            cld.collided = true;
        }
    */
}
