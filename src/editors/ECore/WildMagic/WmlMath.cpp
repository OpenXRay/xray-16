// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.
#include "stdafx.h"
#pragma hdrstop

#include "WmlMath.h"
using namespace Wml;

template<> const float Math<float>::EPSILON = FLT_EPSILON;
template<> const float Math<float>::ZERO_TOLERANCE = 1e-06f;
template<> const float Math<float>::MAX_REAL = flt_max;//.FLT_MAX;
template<> const float Math<float>::_PI = (float)(4.0*atan(1.0));
template<> const float Math<float>::TWO_PI = 2.0f*Math<float>::_PI;
template<> const float Math<float>::HALF_PI = 0.5f*Math<float>::_PI;
template<> const float Math<float>::INV_PI = 1.0f/Math<float>::_PI;
template<> const float Math<float>::INV_TWO_PI = 1.0f/Math<float>::TWO_PI;
template<> const float Math<float>::DEG_TO_RAD = Math<float>::_PI/180.0f;
template<> const float Math<float>::RAD_TO_DEG = 180.0f/Math<float>::_PI;

template<> const double Math<double>::EPSILON = DBL_EPSILON;
template<> const double Math<double>::ZERO_TOLERANCE = 1e-08;
template<> const double Math<double>::MAX_REAL = DBL_MAX;
template<> const double Math<double>::_PI = 4.0*atan(1.0);
template<> const double Math<double>::TWO_PI = 2.0*Math<double>::_PI;
template<> const double Math<double>::HALF_PI = 0.5*Math<double>::_PI;
template<> const double Math<double>::INV_PI = 1.0/Math<double>::_PI;
template<> const double Math<double>::INV_TWO_PI = 1.0/Math<double>::TWO_PI;
template<> const double Math<double>::DEG_TO_RAD = Math<double>::_PI/180.0;
template<> const double Math<double>::RAD_TO_DEG = 180.0/Math<double>::_PI;

namespace Wml
{
template WML_ITEM class Math<float>;
template WML_ITEM class Math<double>;
};

