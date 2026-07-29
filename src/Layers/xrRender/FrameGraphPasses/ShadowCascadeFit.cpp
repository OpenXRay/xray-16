#include "stdafx.h"
#include "ShadowCascadeFit.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/device.h"

namespace xray::render::fg::passes
{

namespace
{

void FrustumSliceCorners(float zNear, float zFar, Fvector outCorners[8])
{
    const Fvector& C = Device.vCameraPosition;
    const Fvector& D = Device.vCameraDirection;
    const Fvector& R = Device.vCameraRight;
    const Fvector& U = Device.vCameraTop;

    const float t = tanf(deg2rad(Device.fFOV * 0.5f));
    const float a = Device.fASPECT;

    auto corner = [&](float z, float sx, float sy) {
        const float h = t * z;
        const float w = h * a;
        Fvector p;
        p.mad(C, D, z);
        p.mad(p, R, sx * w);
        p.mad(p, U, sy * h);
        return p;
    };

    outCorners[0] = corner(zNear, -1.f, -1.f);
    outCorners[1] = corner(zNear, +1.f, -1.f);
    outCorners[2] = corner(zNear, +1.f, +1.f);
    outCorners[3] = corner(zNear, -1.f, +1.f);
    outCorners[4] = corner(zFar, -1.f, -1.f);
    outCorners[5] = corner(zFar, +1.f, -1.f);
    outCorners[6] = corner(zFar, +1.f, +1.f);
    outCorners[7] = corner(zFar, -1.f, +1.f);
}

} // namespace

void ComputeFrustumFitCascadeMatrices(
    const Fvector& sunDirTowardLight,
    float mapSize,
    float viewZNear,
    float viewZFar,
    u32 cascadeIndex,
    u32 smapResolution,
    Fmatrix& outClipVP,
    Fmatrix& outSampleVP)
{
    Fvector L_dir;
    L_dir.set(-sunDirTowardLight.x, -sunDirTowardLight.y, -sunDirTowardLight.z);
    L_dir.normalize_safe();

    Fvector L_right, L_up;
    L_right.set(1.f, 0.f, 0.f);
    if (_abs(L_right.dotproduct(L_dir)) > 0.99f)
        L_right.set(0.f, 0.f, 1.f);
    L_up.crossproduct(L_dir, L_right);
    L_up.normalize_safe();
    L_right.crossproduct(L_up, L_dir);
    L_right.normalize_safe();

    const float z0 = std::max(viewZNear, VIEWPORT_NEAR);
    const float z1 = std::max(viewZFar, z0 + 1.f);

    Fvector corners[8];
    FrustumSliceCorners(z0, z1, corners);

    Fvector L_pos;
    L_pos.mad(Device.vCameraPosition, L_dir, -(mapSize * 3.f + 400.f));

    Fmatrix lightView0;
    lightView0.build_camera_dir(L_pos, L_dir, L_up);

    float minZ = FLT_MAX, maxZ = -FLT_MAX;
    for (int i = 0; i < 8; ++i)
    {
        Fvector ls;
        lightView0.transform_tiny(ls, corners[i]);
        minZ = std::min(minZ, ls.z);
        maxZ = std::max(maxZ, ls.z);
    }

    // View-independent fit: fixed ortho size per cascade, centered slightly ahead
    // of the camera using ONLY the horizontal facing (pitch ignored). Frustum-slice
    // fitting shrank/shifted the ortho when looking down, so off-screen tree casters
    // dropped out and shadows popped with no smooth fade. A stable, pitch-independent
    // footprint keeps casters in the map regardless of where you look.
    float orthoSize = mapSize;

    Fvector fwdH = Device.vCameraDirection;
    fwdH.y = 0.f;
    if (fwdH.magnitude() > 1e-3f)
        fwdH.normalize();
    else
        fwdH.set(0.f, 0.f, 1.f);
    Fvector centerW;
    centerW.mad(Device.vCameraPosition, fwdH, orthoSize * 0.25f);

    Fvector centerLS;
    lightView0.transform_tiny(centerLS, centerW);

    const float texelWorld = orthoSize / float(std::max(smapResolution, 1u));
    const float cxSnap = floorf(centerLS.x / texelWorld) * texelWorld;
    const float cySnap = floorf(centerLS.y / texelWorld) * texelWorld;

    Fvector shiftWorld;
    shiftWorld.mul(L_right, cxSnap);
    shiftWorld.mad(shiftWorld, L_up, cySnap);
    Fvector L_posFit;
    L_posFit.add(L_pos, shiftWorld);

    Fmatrix lightView;
    lightView.build_camera_dir(L_posFit, L_dir, L_up);

    minZ = FLT_MAX;
    maxZ = -FLT_MAX;
    for (int i = 0; i < 8; ++i)
    {
        Fvector ls;
        lightView.transform_tiny(ls, corners[i]);
        minZ = std::min(minZ, ls.z);
        maxZ = std::max(maxZ, ls.z);
    }

    const float zn = 0.1f;
    const float zPad = 1.41421f * orthoSize;
    float zf = (maxZ - minZ) + zPad + 50.f;
    const float zPull = std::max(0.f, -minZ) + orthoSize * 0.5f + 50.f;
    zf += zPull;
    zf = std::max(zf, orthoSize + 100.f);

    Fmatrix zOff;
    zOff.identity();
    zOff.translate(0.f, 0.f, -(minZ - zn - zPull * 0.5f));

    Fmatrix viewOff;
    viewOff.mul(zOff, lightView);

    Fmatrix lightProj;
    lightProj.build_projection_ortho_stdz(orthoSize, orthoSize, zn, zn + zf);
    outClipVP.mul(lightProj, viewOff);

    const float fRange = (cascadeIndex == 0) ? ps_r2_sun_depth_near_scale : ps_r2_sun_depth_far_scale;
    // Small constant depth bias. Acne is handled mainly by world normal-offset
    // (shadow_sampling.h NormalOffsetWorld), so keep this low to avoid detaching
    // object shadows (peter-panning) that made cast shadows vanish.
    const float fBias = -(0.0004f + orthoSize * 0.000006f);
    Fmatrix texScaleBias;
    texScaleBias.identity();
    texScaleBias._11 = 0.5f;
    texScaleBias._22 = -0.5f;
    texScaleBias._33 = fRange;
    texScaleBias._41 = 0.5f;
    texScaleBias._42 = 0.5f;
    texScaleBias._43 = fBias;
    outSampleVP.mul(texScaleBias, outClipVP);
}

} // namespace xray::render::fg::passes
