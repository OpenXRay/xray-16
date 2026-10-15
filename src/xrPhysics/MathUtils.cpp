#include "StdAfx.h"
#include "MathUtils.h"
#include "xrCore/_cylinder.h"

IC bool RAYvsCYLINDER(const Fcylinder& c_cylinder, const Fvector& S, const Fvector& D, float& R, BOOL bCull)
{
    const float& r = c_cylinder.m_radius;
    float h = c_cylinder.m_height / 2.f;
    const Fvector& p = S;
    const Fvector& dir = D;

    const Fvector& c = c_cylinder.m_center;
    const Fvector& ax = c_cylinder.m_direction;

    Fvector v;
    v.sub(c, p);
    float cs = dir.dotproduct(ax);
    float Lc = v.dotproduct(ax);
    float Lr = v.dotproduct(dir);

    float sq_cos = cs * cs;
    float sq_sin = 1.0f - sq_cos;
    float v_smag = v.square_magnitude();
    const float sq_r = r * r;

    if (sq_sin < EPS)
    {
        float tr1, tr2;
        float sq_dist = v_smag - Lr * Lr;
        if (sq_dist > sq_r)
            return false;
        float r_dist = _sqrt(sq_r - sq_dist) + h;
        tr1 = Lr - r_dist;

        if (tr1 > R)
            return false;
        if (tr1 < 0.f)
        {
            if (bCull)
                return false;
            else
            {
                tr2 = Lr + r_dist;
                if (tr2 < 0.f)
                    return false;
                if (tr2 < R)
                {
                    R = tr2;
                    return true;
                }
                return false;
            }
        }
        R = tr1;
        return true;
    }

    if (sq_cos < EPS)
    {
        float tr1, tr2;
        float abs_c_dist = _abs(Lc);
        if (abs_c_dist > h + r)
            return false;
        float sq_dist = v_smag - Lr * Lr - Lc * Lc;
        if (sq_dist > sq_r)
            return false;
        float lc_h = abs_c_dist - h;
        if (lc_h > 0.f)
        {
            float sq_sphere_dist = lc_h * lc_h + sq_dist * sq_dist;
            if (sq_sphere_dist > sq_r)
                return false;
            float diff = _sqrt(sq_r - sq_sphere_dist);
            tr1 = Lr - diff;
            if (tr1 > R)
                return false;
            if (tr1 < 0.f)
            {
                if (bCull)
                    return false;
                else
                {
                    tr2 = Lr + diff;
                    if (tr2 < 0.f)
                        return false;
                    if (tr2 < R)
                    {
                        R = tr2;
                        return true;
                    }
                    return false;
                }
            }
        }
        float diff = _sqrt(sq_r - sq_dist);
        tr1 = Lr - diff;

        if (tr1 > R)
            return false;
        if (tr1 < 0.f)
        {
            if (bCull)
                return false;
            else
            {
                tr2 = Lr + diff;
                if (tr2 < 0.f)
                    return false;
                if (tr2 < R)
                {
                    R = tr2;
                    return true;
                }
                return false;
            }
        }
        R = tr1;
        return true;
    }

    float tr1, tr2;
    float r_sq_sin = 1.f / sq_sin;
    float tr = (Lr - cs * Lc) * r_sq_sin;
    float tc = (cs * Lr - Lc) * r_sq_sin;

    float sq_nearest_dist = v_smag + tr * tr + tc * tc - 2 * (cs * tc * tr - Lc * tc + Lr * tr);

    if (sq_nearest_dist > sq_r)
        return false;

    float sq_horde = (sq_r - sq_nearest_dist);
    float sq_c_diff = sq_horde * sq_cos * r_sq_sin;
    float c_diff = _sqrt(sq_c_diff);
    float cp1 = tc - c_diff;
    float cp2 = tc + c_diff;

    if (cp1 > h)
    {
        float tc_h = tc - h;
        float sq_sphere_dist = sq_sin * tc_h * tc_h;
        if (sq_sphere_dist > sq_horde)
            return false;
        float tr_c = tr - tc_h * cs;
        float diff = _sqrt(sq_horde - sq_sphere_dist);
        tr1 = tr_c - diff;
        if (tr1 > R)
            return false;
        if (tr1 < 0.f)
        {
            if (bCull)
                return false;
            else
            {
                tr2 = tr_c + diff;
                if (tr2 < 0.f)
                    return false;
                if (tr2 < R)
                {
                    R = tr2;
                    return true;
                }
            }
        }
        R = tr1;
        return true;
    }

    if (cp2 < -h)
    {
        float tc_h = tc + h;
        float sq_sphere_dist = sq_sin * tc_h * tc_h;
        if (sq_sphere_dist > sq_horde)
            return false;
        float tr_c = tr - tc_h * cs;
        float diff = _sqrt(sq_horde - sq_sphere_dist);
        tr1 = tr_c - diff;
        if (tr1 > R)
            return false;
        if (tr1 < 0.f)
        {
            if (bCull)
                return false;
            else
            {
                tr2 = tr_c + diff;
                if (tr2 < 0.f)
                    return false;
                if (tr2 < R)
                {
                    R = tr2;
                    return true;
                }
            }
        }
        R = tr1;
        return true;
    }

    if (cs > 0.f)
    {
        if (cp1 > -h)
        {
            if (cp2 < h)
            {
                float diff = c_diff / cs;
                tr1 = tr - diff;
                if (tr1 > R)
                    return false;
                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        tr2 = tr + diff;
                        if (tr2 < 0.f)
                            return false;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
            else
            {
                float diff = c_diff / cs;
                tr1 = tr - diff;
                if (tr1 > R)
                    return false;
                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        float tc_h = tc - h;
                        float sq_sphere_dist = sq_sin * tc_h * tc_h;
                        float tr_c = tr - tc_h * cs;
                        float diff2 = _sqrt(sq_horde - sq_sphere_dist);
                        tr2 = tr_c + diff2;
                        if (tr2 < 0.f)
                            return false;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
        }
        else
        {
            if (cp2 < h)
            {
                float tc_h = tc + h;
                float sq_sphere_dist = sq_sin * tc_h * tc_h;
                float diff = _sqrt(sq_horde - sq_sphere_dist);
                float tr_c = tr - tc_h * cs;
                tr1 = tr_c - diff;
                if (tr1 > R)
                    return false;
                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        float diff2 = c_diff / cs;
                        tr2 = tr + diff2;
                        if (tr2 < 0.f)
                            return false;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
            else
            {
                float tc_h = tc + h;
                float tr_c = tr - tc_h * cs;
                float diff = _sqrt(sq_horde - sq_sin * tc_h * tc_h);
                tr1 = tr_c - diff;
                if (tr1 > R)
                    return false;
                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        float tc_h2 = tc - h;
                        float tr_c2 = tr - tc_h2 * cs;
                        float diff2 = _sqrt(sq_horde - sq_sin * tc_h2 * tc_h2);
                        tr2 = tr_c2 + diff2;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
        }
    }
    else
    {
        if (cp1 > -h)
        {
            if (cp2 < h)
            {
                float diff = -c_diff / cs;
                tr1 = tr - diff;
                if (tr1 > R)
                    return false;
                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        tr2 = tr + diff;
                        if (tr2 < 0.f)
                            return false;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
            else
            {
                float tc_h = tc - h;
                float tr_c = tr - tc_h * cs;
                float diff = _sqrt(sq_horde - sq_sin * tc_h * tc_h);
                tr1 = tr_c - diff;
                if (tr1 > R)
                    return false;
                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        diff = -c_diff / cs;
                        tr2 = tr + diff;
                        if (tr2 < 0.f)
                            return false;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
        }
        else
        {
            if (cp2 < h)
            {
                float diff = -c_diff / cs;
                tr1 = tr - diff;
                if (tr1 > R)
                    return false;
                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        float tc_h = tc + h;
                        float tr_c = tr - tc_h * cs;
                        diff = _sqrt(sq_horde - sq_sin * tc_h * tc_h);
                        tr2 = tr_c + diff;
                        if (tr2 < 0.f)
                            return false;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
            else
            {
                float tc_h = tc - h;
                float tr_c = tr - tc_h * cs;
                float diff = _sqrt(sq_horde - sq_sin * tc_h * tc_h);
                tr1 = tr_c - diff;
                if (tr1 > R)
                    return false;

                if (tr1 < 0.f)
                {
                    if (bCull)
                        return false;
                    else
                    {
                        tc_h = tc + h;
                        tr_c = tr - tc_h * cs;
                        diff = _sqrt(sq_horde - sq_sin * tc_h * tc_h);
                        tr2 = tr_c + diff;
                        if (tr2 < 0.f)
                            return false;
                        if (tr2 < R)
                        {
                            R = tr2;
                            return true;
                        }
                    }
                }
                R = tr1;
                return true;
            }
        }
    }
}

void capped_cylinder_ray_collision_test()
{
    Fcylinder c;
    c.m_center.set(0, 0, 0);
    c.m_direction.set(0, 0, 1);
    c.m_height = 2;
    c.m_radius = 1;

    Fvector dir, pos;
    float R;
    dir.set(1, 0, 0);
    pos.set(0, 0, 0);
    R = 3;

    RAYvsCYLINDER(c, pos, dir, R, FALSE);
    RAYvsCYLINDER(c, pos, dir, R, TRUE);
    dir.set(0, 0, 1);
    RAYvsCYLINDER(c, pos, dir, R, FALSE);
    RAYvsCYLINDER(c, pos, dir, R, TRUE);

    pos.set(-3, 0, 0);
    dir.set(1, 0, 0);
    R = 4;
    RAYvsCYLINDER(c, pos, dir, R, FALSE);
    RAYvsCYLINDER(c, pos, dir, R, TRUE);
    R = 1;
    RAYvsCYLINDER(c, pos, dir, R, FALSE);
    pos.set(0, 0, -3);
    dir.set(0, 0, 1);
    R = 4;
    RAYvsCYLINDER(c, pos, dir, R, FALSE);
    RAYvsCYLINDER(c, pos, dir, R, TRUE);

    pos.set(-3, -3, -3);
    dir.set(1, 1, 1);
    dir.normalize();
    R = 10;
    RAYvsCYLINDER(c, pos, dir, R, TRUE);
    float ir[2];
    Fcylinder::ecode code[2];
    c.intersect(pos, dir, ir, code);

    pos.set(0, 0, 0);
    RAYvsCYLINDER(c, pos, dir, R, FALSE);
    c.intersect(pos, dir, ir, code);
    RAYvsCYLINDER(c, pos, dir, R, TRUE);

    CTimer t;
    t.Start();
    for (int i = 0; i < 1000000; i++)
    {
        Fcylinder c2;
        c2.m_center.random_point(Fvector().set(2, 2, 2));
        c2.m_direction.random_dir();
        c2.m_height = Random.randF(0.2f, 2.f);
        c2.m_radius = Random.randF(0.1f, 2.f);

        Fvector dir2, pos2;
        float R = Random.randF(0.1f, 2.f);
        dir2.random_dir();
        pos2.random_point(Fvector().set(2, 2, 2));
        RAYvsCYLINDER(c, pos2, dir2, R, TRUE);
    }
    Msg("my RAYvsCYLINDE time %f ms", t.GetElapsed_sec() * 1000.f);

    t.Start();
    for (int i = 0; i < 1000000; i++)
    {
        Fcylinder c2;
        c2.m_center.random_point(Fvector().set(2, 2, 2));
        c2.m_direction.random_dir();
        c2.m_height = Random.randF(0.2f, 2.f);
        c2.m_radius = Random.randF(0.1f, 2.f);

        Fvector dir2, pos2;
        dir2.random_dir();
        pos2.random_point(Fvector().set(2, 2, 2));
        c.intersect(pos2, dir2, ir, code);
    }
    Msg("current intersect time %f ms", t.GetElapsed_sec() * 1000.f);
}
