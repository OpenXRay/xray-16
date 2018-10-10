////////////////////////////////////////////////////////////////////////////
//	Module 		: steering_behaviour.cpp
//	Created 	: 25.04.2008
//  Modified 	: 25.04.2008
//	Author		: Lain
//	Description : steering behaviour classes
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "steering_behaviour.h"

//----------------------------------------------------------
// Steering Behaviour Namespace
//----------------------------------------------------------

namespace steering_behaviour
{
#define STEER_ASSERT VERIFY

namespace detail
{
const float near_zero = 0.0001f;
static vec zero_vec = {0.f, 0.f, 0.f};

vec random_vec()
{
    struct local
    {
        static float random_component() { return -1.f + 2.f * (rand() / (float)RAND_MAX); }
    };

    return normalize(cr_fvector3(local::random_component(), local::random_component(), local::random_component()));
}

ICF float min(float a, float b) { return a < b ? a : b; }
ICF float max(float a, float b) { return a < b ? a : b; }
} // namespace detail

//----------------------------------------------------------
// base
//----------------------------------------------------------

float base::calc_dist_factor(float dist) const { return calc_dist_factor(m_p_params->factor, dist); }
float base::calc_dist_factor(vec_arg factor, float dist) const
{
    STEER_ASSERT(m_p_params->min_factor_dist >= s_min_factor_dist);
    const float r = detail::min(dist, m_p_params->min_factor_dist);
    const float r2 = r * r;

    return factor.x + factor.y / r + factor.z / r2;
}

//----------------------------------------------------------
// evade
//----------------------------------------------------------

vec evade::calc_acceleration()
{
    const vec dest2pos = m_p_params->dest - m_p_params->pos;
    const float dest2pos_mag = magnitude(dest2pos);

    const float dist = detail::max(dest2pos_mag, detail::near_zero);

    if (dist > m_p_params->max_evade_range)
    {
        return detail::zero_vec;
    }

    STEER_ASSERT(m_p_params->pf_random_dir != NULL);

    const vec pos2dest_norm = (dest2pos_mag > detail::near_zero) ? (dest2pos * (1.f / dest2pos_mag)) :
                                                                   normalize((*m_p_params->pf_random_dir)());

    return pos2dest_norm * calc_dist_factor(dist);
}

//----------------------------------------------------------
// pursue
//----------------------------------------------------------

vec pursue::calc_acceleration()
{
    const vec pos2dest = m_p_params->dest - m_p_params->pos;
    const float dist = magnitude(pos2dest);

    if (dist < m_p_params->arrive_range)
    {
        return detail::zero_vec;
    }

    const vec pos2dest_norm = pos2dest * (1.f / dist);

    if (dist <= m_p_params->change_vel_range)
    {
        const float vel = m_p_params->vel;
        const float arrive_vel = m_p_params->arrive_vel;
        const float sum_vel = vel + arrive_vel;

        if (sum_vel < detail::near_zero)
        {
            // fix it ??
            return detail::zero_vec;
        }

        const float path_time = 2 * dist / sum_vel;
        return pos2dest_norm * ((arrive_vel - vel) / path_time);
    }
    else
    {
        return pos2dest_norm * calc_dist_factor(dist);
    }
}

//----------------------------------------------------------
// restrictor
//----------------------------------------------------------

vec restrictor::calc_acceleration()
{
    const vec pos2rest = m_p_params->restrictor_pos - m_p_params->pos;
    const float dist = magnitude(pos2rest);

    if (dist <= m_p_params->max_allowed_range)
    {
        return detail::zero_vec;
    }
    else
    {
        const vec pos2rest_norm = pos2rest * (1.f / dist);
        return pos2rest_norm * calc_dist_factor(dist);
    }
}

//----------------------------------------------------------
// wander
//----------------------------------------------------------

vec wander::calc_acceleration()
{
    m_wander_angle += (rand() % 2 ? +1 : -1) * m_p_params->angle_change;

    const vec dir = m_p_params->dir;

    const vec proj_dir = cr_fvector3(proj_x(dir), proj_y(dir), 0);

    const float proj_dir_mag = magnitude(proj_dir);

    if (proj_dir_mag < detail::near_zero)
    {
        return detail::zero_vec;
    }

    const float cosa = _cos(m_wander_angle);
    const float sina = _sin(m_wander_angle);

    vec res = detail::zero_vec;
    proj_x(res) = proj_dir.x * cosa - proj_dir.y * sina;
    proj_y(res) = proj_dir.x * sina + proj_dir.y * cosa;

    res = normalize(res);
    res = res + dir * m_p_params->conservativeness;

    return normalize(res) * m_p_params->factor.x;
}

float& wander::proj_x(vec& v) { return m_p_params->plane == params::yz_plane ? v.y : v.x; }
float& wander::proj_y(vec& v) { return m_p_params->plane == params::xy_plane ? v.y : v.z; }
const float& wander::proj_x(const vec& v) { return m_p_params->plane == params::yz_plane ? v.y : v.x; }
const float& wander::proj_y(const vec& v) { return m_p_params->plane == params::xy_plane ? v.y : v.z; }
//----------------------------------------------------------
// containment
//----------------------------------------------------------

vec containment::calc_acceleration()
{
    STEER_ASSERT(magnitude(m_p_params->dir) > detail::near_zero);
    STEER_ASSERT(magnitude(m_p_params->up) > detail::near_zero);

    const vec dir = normalize(m_p_params->dir);
    const vec up = normalize(m_p_params->up);
    const vec right = normalize(crossproduct(dir, up));

    vec steer = detail::zero_vec;
    for (params::Probes::iterator i = m_p_params->probes.begin(), e = m_p_params->probes.end(); i != e; ++i)
    {
        const vec local_probe = *i;
        const vec probe =
            cr_fvector3(dotproduct(local_probe, right), dotproduct(local_probe, up), dotproduct(local_probe, dir));

        vec point_on_obstacle, normal;
        if (m_p_params->test_obstacle(probe, point_on_obstacle, normal))
        {
            const float dist = magnitude(point_on_obstacle - m_p_params->pos);
            const float dist_factor = calc_dist_factor(dist);

            const vec thrust = dir * (-dist_factor);
            const vec turn = right * dotproduct(normalize(normal), right) * m_p_params->turn_factor * dist_factor;

            steer = steer + thrust + turn;
        }
    }

    return steer;
}

//----------------------------------------------------------
// grouping: cohesion + separation
//----------------------------------------------------------

vec grouping::calc_acceleration()
{
    vec steer = detail::zero_vec;
    vec sum_nearest = detail::zero_vec;

    int num_nearest = 0;
    vec cur_nearest;
    for (m_p_params->first_nearest(cur_nearest); !m_p_params->nomore_nearest(); m_p_params->next_nearest(cur_nearest))
    {
        const vec point2pos = m_p_params->pos - cur_nearest;
        const float point2pos_mag = magnitude(point2pos);

        if (point2pos_mag < m_p_params->max_separate_range)
        {
            const vec pos2dest_norm = (point2pos_mag > detail::near_zero) ? (point2pos * (1.f / point2pos_mag)) :
                                                                            normalize((*m_p_params->pf_random_dir)());

            steer = steer + pos2dest_norm * calc_dist_factor(m_p_params->separation_factor, point2pos_mag);
        }

        sum_nearest = sum_nearest + cur_nearest;
        ++num_nearest;
    }

    if (!num_nearest)
    {
        return detail::zero_vec;
    }

    const vec center = sum_nearest * (1.f / num_nearest);
    const vec pos2center = center - m_p_params->pos;
    const float pos2center_mag = magnitude(pos2center);

    // add cohesion force
    if (pos2center_mag > detail::near_zero)
    {
        steer = steer + normalize(pos2center) * calc_dist_factor(m_p_params->cohesion_factor, pos2center_mag);
    }

    return steer;
}

//----------------------------------------------------------
// manager
//----------------------------------------------------------

vec manager::calc_acceleration()
{
    remove_scheduled();

    vec v = detail::zero_vec;
    for (Behaviours::iterator i = m_behaviours.begin(), e = m_behaviours.end(); i != e; ++i)
    {
        base* p_base = (*i);
        base::params* p_params = p_base->get_supplier();
        STEER_ASSERT(p_params != NULL);

        if (!p_params->update())
        {
            schedule_remove(p_base);
        }

        if (p_params->enabled)
        {
            v = v + p_base->calc_acceleration();
        }
    }

    return v;
}

void manager::add(base* behaviour) { m_behaviours.insert(behaviour); }
void manager::schedule_remove(base* behaviour) { m_schedule_remove.insert(behaviour); }
void manager::clear()
{
    for_each(m_behaviours.begin(), m_behaviours.end(), &deleter);
    m_behaviours.clear();
    m_schedule_remove.clear();
}

void manager::remove_scheduled()
{
    for (Behaviours::iterator i = m_schedule_remove.begin(), e = m_schedule_remove.end(); i != e; ++i)
    {
        Behaviours::iterator it = m_behaviours.find(*i);
        if (it != m_behaviours.end())
        {
            deleter(*it);
            m_behaviours.erase(it);
        }
    }

    m_schedule_remove.clear();
}

void manager::deleter(base* p) { delete (p); }
} // namespace steering_behaviour
