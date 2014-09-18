#include "stdafx.h"
#include "mmintrin.h"

void	__stdcall	xrSlerp_x86		(_quaternion<float>* D, _quaternion<float>* Q1, _quaternion<float>* Q2, float t)
{
	D->slerp(*Q1,*Q2,t);
}

// xyzw
// wxyz

// scalar constants
/*
const float DELTA_1 = -0.999f;
const float DELTA_2 = 0.999f;
const float HALF_PI = 1.570796f;

void	__stdcall	xrSlerp_3Dnow	(_quaternion* result, _quaternion* q1, _quaternion* q2, float t)
{
    __m64 omega, scale0, scale1, cosom, isinom, qwx, qyz, r0, r1, r2;

    _m_femms();

    //; DOT the quats to get the acosine of the angle between them
    cosom = _m_pfadd (_m_pfmul (_m_from_floats (&q1->w),
                                _m_from_floats (&q2->w)),
                      _m_pfmul (_m_from_floats (&q1->y),
                                _m_from_floats (&q2->y)));
    cosom = _m_pfacc (cosom, cosom);

    //; Two special cases:
    //; if (cosom <= DELTA - 1.0f) do perpendiclar
    if (_m_to_int (_m_pfcmpgt (cosom, _m_from_float (DELTA_1))) == 0)
    {
        // Perpendicular case
        //; set scale0, scale1 for slerp to a perpendicular quat
        //; scale0 = _sin ((1.0f - slerp) * HALF_PI);
        scale0 = _m_sin (_m_pfmul (_m_pfsub (_m_from_float (ONES.m64_f32[0]),
                                             _m_from_float (slerp)),
                                   _m_from_float (HALF_PI)));


        //; scale1 = _sin (slerp * HALF_PI);
        r0 = _m_sin (_m_pfmul (_m_from_float (slerp),
                               _m_from_float (HALF_PI)));
        scale1 = _m_punpckldq (r0, r0);                  // scale1 | scale1

        //; q2w = quat2[3] *  scale1
        //; q2x = quat2[2] * -scale1
        //; q2y = quat2[1] *  scale1
        //; q2z = quat2[0] * -scale1
        r1 = _m_from_floats (&q2->y);
        r2 = _m_from_floats (&q1->s);

        // swap low and high dwords
        r1 = _m_punpckhdq (_m_punpckldq (r1, r1), r1);
        r2 = _m_punpckhdq (_m_punpckldq (r2, r2), r2);

        qwx = _m_pfmul (r1, r0);
        qyz = _m_pfmul (r2, r0);
    }
    else
    {
        //; if (cosom >= 1.0f - DELTA) do linear
        if (_m_to_int (_m_pfcmpge (cosom, _m_from_float (DELTA_2))) != 0)
        {
            //; Set scale0, scale1 for linear interpolation
            //; scale0 = 1.0f - slerp;
            //; scale1 = slerp;
            r1		= _m_from_float (slerp);
            scale0	= _m_pfsub (_m_from_float (ONES.m64_f32[0]), r1);    // -slerp | 1 - slerp
            scale1	= _m_punpckldq (r1, r1);                             // slerp | slerp
        }
        else
        {
            //; otherwise, SLERP away!
            //; omega = (float)acos (cosom);
            omega	= _m_acos (cosom);

            //; isinom = 1.0f / (float)sin (omega);
            r0		= _m_sin		(omega);
            r1		= _m_pfrcp		(r0);
            isinom	= _m_pfrcpit2	(_m_pfrcpit1 (r0, r1), r1);			// isinom = 1/sin(o) | 1/sin(o)

            //; scale0 = (float)sin ((1.0f - slerp) * omega) * isinom;
            //; scale1 = (float)sin (slerp * omega) * isinom;
            //; (this code is terribly sisd, but I'm can't puzzle out a better way)
            scale0	= _m_sin		(_m_pfmul (_m_pfsub (ONES, _m_from_float (slerp)), omega));

            r0		= _m_sin		(_m_pfmul (_m_from_float (slerp), omega));
            scale1	= _m_pfmul		(_m_punpckldq (r0, r0), isinom);

        }

        //; q2w = quat2[0] * scale1;
        //; q2x = quat2[1] * scale1;
        //; q2y = quat2[2] * scale1;
        //; q2z = quat2[3] * scale1;
        qwx = _m_pfmul (scale1, _m_from_floats (&q2->w));
        qyz = _m_pfmul (scale1, _m_from_floats (&q2->y));
    }

    // Slerp away
    //; result[0] = scale0 * quat1[0] + q2x;
    //; result[1] = scale0 * quat1[1] + q2y;
    //; result[2] = scale0 * quat1[2] + q2z;
    //; result[3] = scale0 * quat1[3] + q2w;
    r0			= _m_punpckldq (scale0, scale0);
    __m64 *pres = (__m64 *)result;
    pres[0]		= _m_pfadd (
						_m_pfmul (r0,
                        _m_from_floats (&q1->w)),
                        qwx);
    pres[1]		= _m_pfadd (
						_m_pfmul (r0,
                        _m_from_floats (&q1->y)),
                        qyz);

	_mm_empty	();
}
*/