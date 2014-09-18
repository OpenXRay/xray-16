// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#include "WmlVector4.h"
using namespace Wml;

template<> const Vector4<float> Vector4<float>::ZERO(0.0f,0.0f,0.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_X(1.0f,0.0f,0.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_Y(0.0f,1.0f,0.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_Z(0.0f,0.0f,1.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_W(0.0f,0.0f,0.0f,1.0f);

template<> const Vector4<double> Vector4<double>::ZERO(0.0,0.0,0.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_X(1.0,0.0,0.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_Y(0.0,1.0,0.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_Z(0.0,0.0,1.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_W(0.0,0.0,0.0,1.0);
