#pragma once
#include "xrCore.h"

struct vertRender;
struct vertBoned1W;
struct vertBoned2W;
struct vertBoned3W;
struct vertBoned4W;
class CBoneInstance;

namespace XRay
{
namespace Math
{
void Skin1W_SSE(vertRender *D, vertBoned1W *S, u32 vCount, CBoneInstance *Bones);
void Skin2W_SSE(vertRender *D, vertBoned2W *S, u32 vCount, CBoneInstance *Bones);
void Skin3W_SSE(vertRender *D, vertBoned3W *S, u32 vCount, CBoneInstance *Bones);
void Skin4W_SSE(vertRender *D, vertBoned4W *S, u32 vCount, CBoneInstance *Bones);
} // namespace Math
} // namespace XRay
