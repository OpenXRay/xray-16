#include "StdAfx.h"

#include "ik_calculate_data.h"

#include "ik/IKLimb.h"

SCalculateData::SCalculateData(CIKLimb& l, const Fmatrix& o)
    : m_angles(0), m_limb(&l), m_obj(&o), do_collide(false),
      state(), cl_shift(Fvector().set(0, 0, 0)), apply(false),
      l(0), a(0) {}
