// XXX tamlin: This header is a HOG! It includes tons of headers, while it's the ONLY
// place defining f.ex. fsimilar, deg2rad, clampr. Split it up!
#pragma once
#ifndef _vector_included
#define _vector_included

#include "math_constants.h"
// Define types and namespaces (CPU & FPU)
#include "xr_types.h"
#include "_math.h"
#include "_bitwise.h"
#include "_std_extensions.h"
#include "xrCommon/math_funcs_inline.h"

template <class T>
struct _quaternion;

#pragma pack(push)
#pragma pack(1)

#include "_random.h"

#include "_color.h"
#include "_vector3d.h"
#include "_vector2.h"
#include "_vector4.h"
#include "_matrix.h"
#include "_matrix33.h"
#include "_quaternion.h"
#include "_rect.h"
#include "_fbox.h"
#include "_fbox2.h"
#include "_obb.h"
#include "_sphere.h"
#include "_cylinder.h"
#include "_random.h"
#include "_compressed_normal.h"
#include "_plane.h"
#include "_plane2.h"
#include "_flags.h"
#include "xrCommon/math_funcs.h"
#ifdef DEBUG
#include "dump_string.h"
#endif
#pragma pack(pop)

#endif // include guard
