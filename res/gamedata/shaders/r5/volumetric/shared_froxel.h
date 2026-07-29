// shared_froxel.h — froxel grid helpers for volumetric lighting MVP
#ifndef SHARED_FROXEL_H
#define SHARED_FROXEL_H

cbuffer FroxelParams : register(b5)
{
	uint3  cb_Dims;          // froxel grid dimensions (X,Y,Z)
	float  cb_Near;          // camera near
	float  cb_Far;           // camera far
	float  cb_InvLogFarNear; // 1 / log2(far/near)
	float2 cb_Pad0;

	float4x4 cb_InvVP;       // clip -> world
	float3   cb_CameraPos;
	float    cb_Pad1;

	// Fog / lighting (filled by WorldFog + sun)
	float  cb_FogDensity;
	float  cb_FogHeightFalloff;
	float  cb_FogBaseHeight;
	float  cb_Pad2;
	float3 cb_FogAlbedo;
	float  cb_Pad3;
	float3 cb_SunDir;        // world-space direction toward sun
	float  cb_Pad4;
	float3 cb_SunColor;
	float  cb_SunIntensity;
	// Classic distance fog (combine_1): fog = saturate(d * w + x)
	float4 cb_ClassicFogParams; // x=-n/(f-n), yzw=1/(f-n)
};

float FroxelSliceToViewZ(uint slice, uint slices)
{
	float t = (float)(slice + 0.5) / (float)slices;
	return cb_Near * exp2(t / max(cb_InvLogFarNear, 1e-5));
}

float ViewZToFroxelZ(float viewZ, uint slices)
{
	float z = max(viewZ, cb_Near);
	float t = log2(z / cb_Near) * cb_InvLogFarNear;
	return saturate(t) * (float)(slices - 1);
}

float3 FroxelToWorld(uint3 coord)
{
	float2 uv = (float2(coord.xy) + 0.5) / float2(cb_Dims.xy);
	float viewZ = FroxelSliceToViewZ(coord.z, cb_Dims.z);

	// Reconstruct world pos: Vulkan/D3D clip.z in [0,1] (same as restir_motion_vectors)
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	float depth01 = (viewZ - cb_Near) / max(cb_Far - cb_Near, 1e-3);
	float4 clipPos = float4(ndc, saturate(depth01), 1.0);
	float4 worldH = mul(cb_InvVP, clipPos);
	return worldH.xyz / max(worldH.w, 1e-5);
}

#endif
