// shared/contact_shadows.h — screen-space contact from temporal history RT
#ifndef CONTACT_SHADOWS_H
#define CONTACT_SHADOWS_H

#ifdef CSM_SHADOW_FORWARD
// History only (t28). Do not declare unused g_ContactDepth — FilterReflection
// drops it from the layout while TextureSlot(27) still binds → NVRHI fails.
Texture2D g_ContactHistory : register(t28); // 1 = lit (fullscreen contact pass)

// Sample only the precomputed contact history. Inline march against prev-frame
// depth + current InvVP caused a sticky dark disc (false hits + 0.82 temporal).
float ContactShadow(float3 worldPos, float3 lightDir)
{
	float len = contact_shadow_length;
	if (len <= 1e-4)
		return 1.0;

	float4 clip0 = mul(m_VP, float4(worldPos, 1.0));
	if (clip0.w <= 1e-4)
		return 1.0;

	float2 uv0 = float2(clip0.x / clip0.w * 0.5 + 0.5, 0.5 - clip0.y / clip0.w * 0.5);
	if (any(uv0 < 0.0) || any(uv0 > 1.0))
		return 1.0;

	// lightDir kept for call-site API; history pass is view-aligned, not light-marched.
	return saturate(g_ContactHistory.SampleLevel(smp_nofilter, uv0, 0).r)
		+ 0.0 * dot(lightDir, lightDir);
}
#else
float ContactShadow(float3 worldPos, float3 lightDir)
{
	return 1.0 + 0.0 * dot(lightDir, lightDir);
}
#endif

#endif
