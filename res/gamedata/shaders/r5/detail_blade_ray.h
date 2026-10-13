#ifndef DETAIL_BLADE_RAY_H
#define DETAIL_BLADE_RAY_H

#include "rt_common.h"
#include "detail_blade_material.h"
#include "shared/shading_class.h"

struct RTBladeShading
{
    float3 normal;
    float variance;
};

float3 RTBladeAttributeDerivative(float3 gradU, float3 gradV, float2 uvDeriv)
{
    return gradU * uvDeriv.x + gradV * uvDeriv.y;
}

RTBladeShading RTResolveBladeShading(RTTriangleVertex v0, RTTriangleVertex v1, RTTriangleVertex v2,
    RTShadingVertex vertex, float2 uvDx, float2 uvDy, float3 toViewer)
{
    float3 normal1 = vertex.edgeNormal1;
    float3 normal2 = vertex.edgeNormal2;
    float widthPercent = vertex.uv.x;
    float3 blended = BladeWidthNormal(normal1, normal2, widthPercent);
    float invLength = BladeWidthNormalScale(blended);

    RTBladeShading result;
    result.normal = FoliageViewerNormal(blended * invLength, v1.position - v0.position, v2.position - v0.position, toViewer);

    float3 grad1U, grad1V, grad2U, grad2V;
    RTUVDerivedBasis(v0.edgeNormal1, v1.edgeNormal1, v2.edgeNormal1, v0.uv, v1.uv, v2.uv, grad1U, grad1V);
    RTUVDerivedBasis(v0.edgeNormal2, v1.edgeNormal2, v2.edgeNormal2, v0.uv, v1.uv, v2.uv, grad2U, grad2V);

    float3 derivX = BladeWidthNormalDerivative(RTBladeAttributeDerivative(grad1U, grad1V, uvDx),
        RTBladeAttributeDerivative(grad2U, grad2V, uvDx), normal1, normal2, widthPercent, uvDx.x);
    float3 derivY = BladeWidthNormalDerivative(RTBladeAttributeDerivative(grad1U, grad1V, uvDy),
        RTBladeAttributeDerivative(grad2U, grad2V, uvDy), normal1, normal2, widthPercent, uvDy.x);
    result.variance = BladeNormalVariance(derivX, derivY, invLength);
    return result;
}

#endif
