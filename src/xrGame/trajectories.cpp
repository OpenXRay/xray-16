#include "StdAfx.h"
#include "trajectories.h"
#include "Level.h"
#include "xrPhysics/IPHWorld.h"
#include "xrGame/ai_debug_variables.h"
#include "xrEngine/xr_object.h"
#include "xrCore/_vector3d_ext.h"

static void trajectory_get_position(
    Fvector& result, const Fvector& start_position, const Fvector& velocity, const Fvector& gravity, const float& time)
{
    // result = start_position + velocity*t + gravity*t^2/2
    result.mad(start_position, velocity, time).mad(gravity, _sqr(time) * .5f);
}

inline static float trajectory_max_error_time(float t0, float t1) { return ((t1 + t0) * .5f); }
static float trajectory_pick_error(
    float low, float high, const Fvector& position, const Fvector& velocity, const Fvector& gravity)
{
    float max_error_time = trajectory_max_error_time(low, high);

    Fvector start;
    trajectory_get_position(start, position, velocity, gravity, low);

    Fvector target;
    trajectory_get_position(target, position, velocity, gravity, high);

    Fvector max_error;
    trajectory_get_position(max_error, position, velocity, gravity, max_error_time);

    Fvector start_to_max_error = Fvector().sub(max_error, start);
    float magnitude = start_to_max_error.magnitude();
    start_to_max_error.mul(1.f / magnitude);
    Fvector start_to_target = Fvector().sub(target, start).normalize();
    float cosine_alpha = start_to_max_error.dotproduct(start_to_target);
    float sine_alpha = _sqrt(1.f - _sqr(cosine_alpha));
    return (magnitude * sine_alpha);
}

static float trajectory_select_pick_time(const float& start_low, float high, const Fvector& start,
    const Fvector& velocity, const Fvector& gravity, const float& epsilon)
{
    float low = start_low;
    float check_time = high;
    float time_epsilon = .1f / velocity.magnitude();
    while (!fsimilar(low, high, time_epsilon))
    {
        float distance = trajectory_pick_error(start_low, check_time, start, velocity, gravity);

        if (distance < epsilon)
            low = check_time;
        else
            high = check_time;

        check_time = (low + high) * .5f;
    }

    return (low);
}

IC BOOL trajectory_query_callback(collide::rq_result& result, LPVOID params)
{
    *(float*)params = result.range;
    return (false);
}

static bool trajectory_check_collision(float low, float high, Fvector const& position, Fvector const& velocity,
    Fvector const& gravity, IGameObject* const self_object, IGameObject* const ignored_object,
    Fvector& collide_position, collide::rq_results& temp_rq_results, Fvector box_size,
    xr_vector<trajectory_pick>* const out_trajectory_picks, xr_vector<Fvector>* const out_collide_tris)
{
    Fvector start;
    trajectory_get_position(start, position, velocity, gravity, low);

    Fvector target;
    trajectory_get_position(target, position, velocity, gravity, high);

    Fvector start_to_target = Fvector().sub(target, start);
    float distance = start_to_target.magnitude();

    if (distance < .01f)
        return (false);

    start_to_target.mul(1.f / distance);
    collide::ray_defs ray_defs(start, start_to_target, distance, CDB::OPT_CULL, collide::rqtBoth);

    float range = distance;

    BOOL previous_enabled = self_object->getEnabled();
    self_object->setEnabled(FALSE);

    BOOL throw_ignore_object_enabled = FALSE;
    if (ignored_object)
    {
        throw_ignore_object_enabled = ignored_object->getEnabled();
        ignored_object->setEnabled(FALSE);
    }

    float const epsilon = 0.0001f;
    bool box_result = false;

    if (box_size.magnitude() <= epsilon)
        Level().ObjectSpace.RayQuery(temp_rq_results, ray_defs, trajectory_query_callback, &range, NULL, self_object);
    else
    {
        Fvector box_center;
        box_center.add(start, target);
        box_center.mul(0.5f);

        Fvector const box_z_axis = start_to_target;
        Fvector box_y_axis;
        Fvector box_x_axis;
        if (_abs(box_z_axis.x) > epsilon || _abs(box_z_axis.z) > epsilon)
        {
            Fvector const down = {0, -1, 0};
            Fvector box_x_axis;
            box_x_axis.crossproduct(box_z_axis, down);
            box_y_axis.crossproduct(box_z_axis, box_x_axis);
        }
        else
        {
            box_y_axis = Fvector().set(0, 0, 1.f);
            box_x_axis.crossproduct(box_y_axis, box_z_axis).normalize();
        }

        box_size.z = distance;

        if (out_trajectory_picks)
        {
            trajectory_pick pick;
            pick.center = box_center;
            pick.sizes = box_size;
            pick.x_axis = normalize(box_x_axis);
            pick.y_axis = normalize(box_y_axis);
            pick.z_axis = normalize(box_z_axis);
            out_trajectory_picks->push_back(pick);
        }

        box_result = !Level().ObjectSpace.BoxQuery(box_center, box_z_axis, box_y_axis, box_size, out_collide_tris);
    }

    if (ignored_object)
        ignored_object->setEnabled(throw_ignore_object_enabled);

    self_object->setEnabled(previous_enabled);

    if (box_size.magnitude() > epsilon)
        return box_result;

    if (range < distance)
        collide_position.mad(start, start_to_target, range);

    return (range == distance);
}

bool trajectory_intersects_geometry(float trajectory_time, Fvector const& trajectory_start,
    Fvector const& trajectory_end, Fvector const& trajectory_velocity, Fvector& collide_position,
    IGameObject* const self_object, IGameObject* const ignored_object, collide::rq_results& temp_rq_results,
    xr_vector<trajectory_pick>* const out_trajectory_picks, xr_vector<Fvector>* const out_collide_tris,
    Fvector const& box_size)
{
    out_trajectory_picks;
    out_collide_tris;

#ifdef DEBUG
    if (out_trajectory_picks)
        out_trajectory_picks->resize(0);
    if (out_collide_tris)
        out_collide_tris->resize(0);
#endif // #ifdef DEBUG

    const Fvector gravity = Fvector().set(0.f, -physics_world()->Gravity(), 0.f);
    const float epsilon = .1f;

    float low = 0.f;
    const float high = trajectory_time;
    for (;;)
    {
        float time = trajectory_select_pick_time(low, high, trajectory_start, trajectory_start, gravity, epsilon);

        if (!trajectory_check_collision(low, time, trajectory_start, trajectory_velocity, gravity, self_object,
                ignored_object, collide_position, temp_rq_results, box_size, out_trajectory_picks, out_collide_tris))
        {
            if (fsimilar(time, high) && collide_position.similar(trajectory_end, .2f))
                break;

            return true;
        }

        if (fsimilar(time, high))
            break;

        low = time;
    }

    return false;
}
