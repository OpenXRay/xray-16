#include "StdAfx.h"
#include "ik_object_shift.h"
#include "pose_extrapolation.h"
#include "xrPhysics/MathUtils.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
#ifdef DEBUG

void object_shift::dbg_draw(
    const Fmatrix& current_pos, const extrapolation::points& predict, const Fvector& start) const
{
    Fvector p0;
    current_pos.transform_tiny(p0, start);
    Fmatrix predicted_pos;
    predict.extrapolate(predicted_pos, taget_time);
    Fvector p1;
    predicted_pos.transform_tiny(p1, start);
    const u16 nb_points = 200;

    float time_global = Device.fTimeGlobal;
    if (time_global > taget_time)
        time_global = taget_time;

    float time = taget_time - time_global;
    float time_passed = time_global - current_time;
    if (fis_zero(time))
        return;

    float time_quant = (time) / nb_points;

    const Fvector vadd = Fvector().sub(p1, p0).mul(1.f / nb_points);
    for (u16 i = 0; i < nb_points; ++i)
    {
        float fshift0 = current + delta_shift(time_passed + time_quant * i);
        float fshift1 = current + delta_shift(time_passed + time_quant * (i + 1));
        Fvector v0 = Fvector().add(p0, Fvector().mul(vadd, float(i))).add(Fvector().set(0, fshift0, 0));
        Fvector v1 = Fvector().add(p0, Fvector().mul(vadd, float(i + 1))).add(Fvector().set(0, fshift1, 0));

        DBG_DrawLine(v0, v1, color_xrgb(0, 255, 0));
    }

    float start_shift = current + delta_shift(time_passed);
    float end_shift = current + delta_shift(time_passed + time);
    DBG_DrawLine(Fvector().add(p0, Fvector().set(0, start_shift, 0)), Fvector().add(p1, Fvector().set(0, end_shift, 0)),
        color_xrgb(255, 0, 0));
}
#endif

static const float global_max_shift = 1.0f;
float object_shift::shift() const
{
    float time_global = Device.fTimeGlobal;
    if (time_global > taget_time)
        time_global = taget_time;
    return shift(time_global);
}

float object_shift::shift(float time_global) const
{
    float time_passed = time_global - current_time;
    float lshift = current + delta_shift(time_passed);
    clamp(lshift, -global_max_shift, global_max_shift);
    return lshift;
}

float object_shift::delta_shift(float delta_time) const
{
    if (b_freeze)
        return 0.f;
    float sq_time = delta_time * delta_time;
    return speed * (delta_time) + accel * sq_time / 2.f + aaccel * sq_time * delta_time / 6.f;
}

bool square_equation(float a, float b, float c, float& x0, float& x1) // returns true if has real roots
{
    float d = b * b - 4.f * a * c;
    if (d < 0.f)
        return false;
    float srt_d_2a = 0.5f * _sqrt(d) / a;
    float b_2a = 0.5f * b / a;
    x0 = -b_2a - srt_d_2a;
    x1 = -b_2a + srt_d_2a;
    return true;
}

// static const float max_possible_shift_speed_up = 4.5f; //
// static const float max_shift_avr_accel_up = 3.5f; //
// static const float max_possible_shift_speed_down = 4.5f; //
// static const float max_shift_avr_accel_down= 3.5f; //
// float clamp_taget_to_max_possible_shift_speed_return_shift_taget(float &taget, float current, float speed, float time
// )
//{
//	float x			= taget - current ;
//	return x;
//	float taget_speed = x/time;
//
//		//clamp(taget_speed,-max_possible_shift_speed, max_possible_shift_speed );
//	save_min( taget_speed, max_possible_shift_speed_up );
//	save_max( taget_speed, -max_possible_shift_speed_down );
//
//	float change_speed = taget_speed - speed;
//	float avr_accel = change_speed/time;
//
//	//clamp( avr_accel,-max_shift_avr_accel, max_shift_avr_accel );
//	save_min( avr_accel, max_shift_avr_accel_up );
//	save_max( avr_accel, -max_shift_avr_accel_down );
//	taget_speed = speed + avr_accel * time;
//
//	//clamp(taget_speed,-max_possible_shift_speed, max_possible_shift_speed );
//	save_min( taget_speed, max_possible_shift_speed_up );
//	save_max( taget_speed, -max_possible_shift_speed_down );
//	x = taget_speed * time;
//
//	taget = x + current;
//
//	return x;
//
//
//}
float half_shift_restrict_up = 0.1f;
float half_shift_restrict_down = 0.15f;

void object_shift::set_taget(float taget_, float time)
{
    if (b_freeze)
        return;
    clamp(taget_, -global_max_shift, global_max_shift);
    // if( fsimilar(taget, taget_) )
    //	return;
    taget = taget_;

    float time_global = Device.fTimeGlobal;

    if (fis_zero(taget_ - shift(time_global + time)))
    {
        taget_time = time_global + time;
        return;
    }

    if (time < EPS_S)
        time = Device.fTimeDelta;

    current = shift();

    float time_pased = 0.f;

    if (time_global > taget_time)
        time_pased = taget_time - current_time;
    else
        time_pased = time_global - current_time;

    speed = speed + accel * time_pased + aaccel * time_pased * time_pased / 2.f;

    // accel += aaccel * time_pased;
    float x = taget - current;

    // float x = clamp_taget_to_max_possible_shift_speed_return_shift_taget( taget, current, speed, time );

    float sq_time = time * time;

    accel = 2.f * (3.f * x / sq_time - 2.f * speed / time);
    aaccel = 6.f * (speed / sq_time - 2.f * x / sq_time / time);

    // aaccel = 3.f*( speed/sq_time - x/sq_time/time );
    // accel = - aaccel * time;

    float x0, x1;
    if ((x > half_shift_restrict_up || x < -half_shift_restrict_down) &&
        square_equation(aaccel / 2.f, accel, speed, x0, x1))
    {
        float max_shift0 = _abs(delta_shift(x0));
        float max_shift1 = _abs(delta_shift(x1));
        float ax = _abs(x);
        bool bx0 = max_shift0 > 2.f * ax;
        bool bx1 = max_shift1 > 2.f * ax;
        if (((x0 > 0.f && x0 < time - EPS_S) && bx0) || ((x1 > 0.f && x1 < time - EPS_S) && bx1))
        {
            aaccel = 0.f;
            speed = 2 * x / time;
            accel = -speed / time;
        }
    }

    VERIFY(_valid(accel));

    current_time = time_global;

    taget_time = time_global + time;
}
