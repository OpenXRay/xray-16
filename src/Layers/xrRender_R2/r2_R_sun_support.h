#pragma once

const u32 LIGHT_CUBOIDSIDEPOLYS_COUNT = 4;
const u32 LIGHT_CUBOIDVERTICES_COUNT = 2 * LIGHT_CUBOIDSIDEPOLYS_COUNT;

template <bool _debug>
class FixedConvexVolume
{
public:
    struct _poly
    {
        int points[4];
        Fplane plane;
    };

    xr_vector<sun::ray> view_frustum_rays;
    sun::ray view_ray;
    sun::ray light_ray;
    Fvector3 light_cuboid_points[LIGHT_CUBOIDVERTICES_COUNT];
    _poly light_cuboid_polys[LIGHT_CUBOIDSIDEPOLYS_COUNT];

public:
    void compute_planes()
    {
        for (u32 it = 0; it < LIGHT_CUBOIDSIDEPOLYS_COUNT; it++)
        {
            _poly& P = light_cuboid_polys[it];

            P.plane.build(
                light_cuboid_points[P.points[0]], light_cuboid_points[P.points[2]], light_cuboid_points[P.points[1]]);

            // verify
            if (_debug)
            {
                Fvector& p0 = light_cuboid_points[P.points[0]];
                Fvector& p1 = light_cuboid_points[P.points[1]];
                Fvector& p2 = light_cuboid_points[P.points[2]];
                Fvector& p3 = light_cuboid_points[P.points[3]];
                Fplane p012;
                p012.build(p0, p1, p2);
                Fplane p123;
                p123.build(p1, p2, p3);
                Fplane p230;
                p230.build(p2, p3, p0);
                Fplane p301;
                p301.build(p3, p0, p1);
                VERIFY(p012.n.similar(p123.n) && p012.n.similar(p230.n) && p012.n.similar(p301.n));
            }
        }
    }

    void compute_caster_model_fixed(
        xr_vector<Fplane>& dest, Fvector3& translation, float map_size, bool clip_by_view_near)
    {
        translation.set(0.f, 0.f, 0.f);

        if (fis_zero(1 - abs(view_ray.D.dotproduct(light_ray.D)), EPS_S))
            return;

        // compute planes for each polygon.
        compute_planes();

        for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
            VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);

        int align_planes[2];
        int align_planes_count = 0;

        // find one or two planes that align to view frustum from behind.
        for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
        {
            float tmp_dot = view_ray.D.dotproduct(light_cuboid_polys[i].plane.n);
            if (tmp_dot <= EPS_L)
                continue;

            align_planes[align_planes_count] = i;
            ++align_planes_count;

            if (align_planes_count == 2)
                break;
        }

        Fvector align_vector;
        align_vector.set(0.f, 0.f, 0.f);

        // Align ray points to the align planes.
        for (int p = 0; p < align_planes_count; ++p)
        {
            // Hack !
            float min_dist = 10000;
            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
            {
                float tmp_dist = 0;
                Fvector tmp_point = view_frustum_rays[i].P;

                tmp_dist = light_cuboid_polys[align_planes[p]].plane.classify(tmp_point);
                min_dist = std::min(tmp_dist, min_dist);
            }

            Fvector shift = light_cuboid_polys[align_planes[p]].plane.n;
            shift.mul(min_dist);
            align_vector.add(shift);
        }

        translation.add(align_vector);

        // Move light ray by the alignment shift.
        light_ray.P.add(align_vector);

        // Here we can skip this stage us in the next pass we need only normals of planes.
        // in the next translate_light_model call will contain this shift as well.
        // translate_light_model	( align_vector );

        // Reset to reuse.
        align_vector.set(0.f, 0.f, 0.f);

        // Check if view edges intersect, and push planes................
        for (int p = 0; p < align_planes_count; ++p)
        {
            float max_mag = 0;
            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
            {
                float plane_dot_ray = view_frustum_rays[i].D.dotproduct(light_cuboid_polys[align_planes[p]].plane.n);
                if (plane_dot_ray < 0)
                {
                    Fvector per_plane_view;
                    per_plane_view.crossproduct(light_cuboid_polys[align_planes[p]].plane.n, view_ray.D);
                    Fvector per_view_to_plane;
                    per_view_to_plane.crossproduct(per_plane_view, view_ray.D);

                    float tmp_mag = -plane_dot_ray / view_frustum_rays[i].D.dotproduct(per_view_to_plane);

                    max_mag = (max_mag < tmp_mag) ? tmp_mag : max_mag;
                }
            }

            if (fis_zero(max_mag))
                continue;

            VERIFY(max_mag <= 1.f);

            float dist = -light_cuboid_polys[align_planes[p]].plane.n.dotproduct(translation);
            align_vector.mad(light_cuboid_polys[align_planes[p]].plane.n, dist * max_mag);
        }

        translation.add(align_vector);
        light_ray.P.add(align_vector);
        translate_light_model(translation);

        // compute culling planes by rays as edges
        for (u32 i = 0; i < view_frustum_rays.size(); ++i)
        {
            Fvector tmp_vector;
            tmp_vector.crossproduct(view_frustum_rays[i].D, light_ray.D);

            // check if the vectors are parallel
            if (fis_zero(tmp_vector.square_magnitude(), EPS))
                continue;

            Fplane tmp_plane;
            tmp_plane.build(view_frustum_rays[i].P, tmp_vector);

            float sign = 0;
            if (check_cull_plane_valid(tmp_plane, sign, 5))
            {
                tmp_plane.n.mul(-sign);
                tmp_plane.d *= -sign;
                dest.push_back(tmp_plane);
            }
        }

        // compute culling planes by ray points pairs as edges
        if (clip_by_view_near && abs(view_ray.D.dotproduct(light_ray.D)) < 0.8)
        {
            Fvector perp_light_view, perp_light_to_view;
            perp_light_view.crossproduct(view_ray.D, light_ray.D);
            perp_light_to_view.crossproduct(perp_light_view, light_ray.D);

            Fplane plane;
            plane.build(view_ray.P, perp_light_to_view);

            float max_dist = -1000;
            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
                max_dist = _max(plane.classify(view_frustum_rays[i].P), max_dist);

            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
            {
                Fvector P = view_frustum_rays[i].P;
                P.mad(view_frustum_rays[i].D, 5);

                if (plane.classify(P) > max_dist)
                {
                    max_dist = 0.f;
                    break;
                }
            }

            if (max_dist > -1000)
            {
                plane.d += max_dist;
                dest.push_back(plane);
            }
        }

        for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
        {
            dest.push_back(light_cuboid_polys[i].plane);
            dest.back().n.mul(-1);
            dest.back().d *= -1;
            VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);
        }

        // Compute ray intersection with light model, this is needed to next cascade to start it's placement.
        for (u32 i = 0; i < view_frustum_rays.size(); ++i)
        {
            float min_dist = 2 * map_size;
            for (int p = 0; p < 4; ++p)
            {
                float dist;
                if ((light_cuboid_polys[p].plane.n.dotproduct(view_frustum_rays[i].D)) > -0.1)
                    dist = map_size;
                else
                    light_cuboid_polys[p].plane.intersectRayDist(view_frustum_rays[i].P, view_frustum_rays[i].D, dist);

                if (dist > EPS_L && dist < min_dist)
                    min_dist = dist;
            }

            view_frustum_rays[i].P.mad(view_frustum_rays[i].D, min_dist);
        }
    }

    bool check_cull_plane_valid(Fplane const& plane, float& sign, float mad_factor = 0.f)
    {
        bool valid = false;
        bool oriented = false;
        float orient = 0;
        for (u32 j = 0; j < view_frustum_rays.size(); ++j)
        {
            float tmp_dist = 0.f;
            Fvector tmp_pt = view_frustum_rays[j].P;
            tmp_pt.mad(view_frustum_rays[j].D, mad_factor);
            tmp_dist = plane.classify(tmp_pt);

            if (fis_zero(tmp_dist, EPS_L))
                continue;

            if (!oriented)
            {
                orient = tmp_dist > 0.f ? 1.f : -1.f;
                valid = true;
                oriented = true;
                continue;
            }

            if (tmp_dist < 0 && orient < 0 || tmp_dist > 0 && orient > 0)
                continue;

            valid = false;
            break;
        }
        sign = orient;
        return valid;
    }

    void translate_light_model(Fvector translate)
    {
        Fmatrix trans_mat;
        trans_mat.translate(translate);
        for (int i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; ++i)
            light_cuboid_polys[i].plane.d -= translate.dotproduct(light_cuboid_polys[i].plane.n);
    }
};

//////////////////////////////////////////////////////////////////////////
// OLES: naive builder of infinite volume expanded from base frustum towards
//		 light source. really slow, but it works for our simple usage :)
// note: normals points to 'outside'
//////////////////////////////////////////////////////////////////////////
template <bool _debug>
class DumbConvexVolume
{
public:
    struct _poly
    {
        xr_vector<int> points;
        Fvector3 planeN;
        float planeD;
        float classify(Fvector3& p) { return planeN.dotproduct(p) + planeD; }
    };
    struct _edge
    {
        int p0, p1;
        int counter;
        _edge(int _p0, int _p1, int m) : p0(_p0), p1(_p1), counter(m)
        {
            if (p0 > p1)
                std::swap(p0, p1);
        }
        bool equal(_edge& E) { return p0 == E.p0 && p1 == E.p1; }
    };

public:
    xr_vector<Fvector3> points;
    xr_vector<_poly> polys;
    xr_vector<_edge> edges;

public:
    void compute_planes()
    {
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& P = polys[it];
            Fvector3 t1, t2;
            t1.sub(points[P.points[0]], points[P.points[1]]);
            t2.sub(points[P.points[0]], points[P.points[2]]);
#ifdef USE_DX9
            P.planeN.crossproduct(t1, t2).normalize();
#else
            P.planeN.crossproduct(t1, t2);

            float len = P.planeN.magnitude();

            if (len > std::numeric_limits<float>::min())
            {
                P.planeN.mul(1 / len);
            }
            else
            {
                t2.sub(points[P.points[0]], points[P.points[3]]);
                P.planeN.crossproduct(t1, t2);
                if (len > std::numeric_limits<float>::min())
                {
                    P.planeN.mul(1 / len);
                }
                else
                {
                    //	HACK:	Remove plane.
                    // VERIFY(!"Can't build normal to plane!");
                    polys.erase(polys.begin() + it);
                    --it;
                    continue;
                }
            }
#endif
            P.planeD = -P.planeN.dotproduct(points[P.points[0]]);

            // verify
            if (_debug)
            {
                Fvector& p0 = points[P.points[0]];
                Fvector& p1 = points[P.points[1]];
                Fvector& p2 = points[P.points[2]];
                Fvector& p3 = points[P.points[3]];
                Fplane p012;
                p012.build(p0, p1, p2);
                Fplane p123;
                p123.build(p1, p2, p3);
                Fplane p230;
                p230.build(p2, p3, p0);
                Fplane p301;
                p301.build(p3, p0, p1);
                VERIFY(p012.n.similar(p123.n) && p012.n.similar(p230.n) && p012.n.similar(p301.n));
            }
        }
    }

    void compute_caster_model(xr_vector<Fplane>& dest, Fvector3 direction)
    {
        CRenderTarget& T = *RImplementation.Target;

        // COG
        Fvector3 cog = { 0, 0, 0 };
        for (int it = 0; it < int(points.size()); it++)
            cog.add(points[it]);
        cog.div(float(points.size()));

        // planes
        compute_planes();
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& base = polys[it];
            if (base.classify(cog) > 0)
                std::reverse(base.points.begin(), base.points.end());
        }

        // remove faceforward polys, build list of edges -> find open ones
        compute_planes();
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& base = polys[it];
            VERIFY(base.classify(cog) < 0); // debug

            int marker = (base.planeN.dotproduct(direction) <= 0) ? -1 : 1;

            // register edges
            xr_vector<int>& plist = polys[it].points;
            for (int p = 0; p < int(plist.size()); p++)
            {
                _edge E(plist[p], plist[(p + 1) % plist.size()], marker);
                bool found = false;
                for (int e = 0; e < int(edges.size()); e++)
                    if (edges[e].equal(E))
                    {
                        edges[e].counter += marker;
                        found = true;
                        break;
                    }
                if (!found)
                {
                    edges.push_back(E);
                    if (_debug)
                        T.dbg_addline(points[E.p0], points[E.p1], color_rgba(255, 0, 0, 255));
                }
            }

            // remove if unused
            if (marker < 0)
            {
                polys.erase(polys.begin() + it);
                it--;
            }
        }

        // Extend model to infinity, the volume is not capped, so this is indeed up to infinity
        for (int e = 0; e < int(edges.size()); e++)
        {
            if (edges[e].counter != 0)
                continue;
            _edge& E = edges[e];
            if (_debug)
                T.dbg_addline(points[E.p0], points[E.p1], color_rgba(255, 255, 255, 255));
            Fvector3 point;
            points.push_back(point.sub(points[E.p0], direction));
            points.push_back(point.sub(points[E.p1], direction));
            polys.push_back(_poly());
            _poly& P = polys.back();
            int pend = int(points.size());
            P.points.push_back(E.p0);
            P.points.push_back(E.p1);
            P.points.push_back(pend - 1); // p1 mod
            P.points.push_back(pend - 2); // p0 mod
            if (_debug)
                T.dbg_addline(points[E.p0], point.mad(points[E.p0], direction, -1000), color_rgba(0, 255, 0, 255));
            if (_debug)
                T.dbg_addline(points[E.p1], point.mad(points[E.p1], direction, -1000), color_rgba(0, 255, 0, 255));
        }

        // Reorient planes (try to write more inefficient code :)
        compute_planes();
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& base = polys[it];
            if (base.classify(cog) > 0)
                std::reverse(base.points.begin(), base.points.end());
        }

        // Export
        compute_planes();
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& P = polys[it];
            Fplane pp = { P.planeN, P.planeD };
            dest.push_back(pp);
        }
    }
};
