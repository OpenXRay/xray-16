//=================================================================================================
//Pseudo PBR shading for STALKER Anomaly 
//Roughness is controlled with r2_gloss_min
//=================================================================================================
#include "pbr_settings.h" //load settings files
#define PI 3.14159265359
//=================================================================================================
//Metalness
//
float calc_metalness(float4 alb_gloss, float material_ID)
{
	//material ID experiment
	float metallerp = max(0.0, (material_ID*4)-0.5)/4; //approx material + weight? //nowhere near
 	//metallerp = saturate((metallerp - 0.5) * 2); //metal threshold
	//metallerp = saturate(((metallerp * 4) - 2) * 0.5); //metal threshold
	
	//binary metalness
	float metalness = saturate(material_ID - 0.75 - 0.001) > 0 ? 1 : 0;
	
	//float metal_thres = METALNESS_THRESHOLD;
	//float metal_soft = METALNESS_SOFTNESS;
	
	//float metal_thres = METALNESS_THRESHOLD * (1 - metallerp) * 2;
	float metal_thres = pow(METALNESS_THRESHOLD, exp2(metallerp));
	float metal_soft = metal_thres * 0.9;
	
	//lerp on gloss
	//metalness *= saturate(smoothstep(METALNESS_THRESHOLD-METALNESS_SOFTNESS, METALNESS_THRESHOLD+METALNESS_SOFTNESS, alb_gloss.a)); 
	metalness *= saturate((alb_gloss.a - (metal_thres-metal_soft)) / ((metal_thres+metal_soft) - (metal_thres-metal_soft)));
	return metalness;
}

float3 Soft_Light (float3 base, float3 blend)
{
	return (blend <= 0.5) ? base - (1-2*blend)*base*(1-base) : base + (2*blend-1)*(sqrt(base)-base);
}
//=================================================================================================
//Material
//
float3 calc_albedo_boost(float3 albedo)
{
	float3 blend = lerp(0.5, 1-dot(albedo, LUMINANCE_VECTOR), ALBEDO_BOOST); //boost albedo by inv
	return Soft_Light(albedo, blend);
}

float3 calc_albedo(float4 alb_gloss, float material_ID)
{

	float metalness = calc_metalness(alb_gloss, material_ID);
	
	float3 albedo = alb_gloss.rgb;
	//albedo = SRGBToLinear(albedo);
	
	albedo = calc_albedo_boost(albedo);
	//albedo = SRGBToLinear(albedo);

	float3 screen_contrib = albedo;
	screen_contrib = (1-(1-screen_contrib)*(1-screen_contrib))-lerp(dot(screen_contrib, LUMINANCE_VECTOR), screen_contrib, 0.5);

	albedo = SRGBToLinear(albedo);
	screen_contrib = SRGBToLinear(screen_contrib);

	float3 albedo_metal = screen_contrib; //metal albedo is screen blend contrib, it gets rid of all highlights.

	return saturate(lerp(albedo, albedo_metal, metalness)*ALBEDO_AMOUNT);
}

float3 calc_specular(float4 alb_gloss, float material_ID)
{
	float metalness = calc_metalness(alb_gloss, material_ID);

	float3 specular = float3(SPECULAR_BASE, SPECULAR_BASE, SPECULAR_BASE); //base fresnel to tweak
	
	float3 specular_metal = alb_gloss.rgb; //metal uses diffuse for specular
	specular_metal = calc_albedo_boost(specular_metal); //boost albedo
	specular_metal = SRGBToLinear(specular_metal);

	//tweaks for specular boost 
	//material_ID = sqrt(material_ID/0.75);
	material_ID = saturate(material_ID * 1.425);
	alb_gloss.a = sqrt(alb_gloss.a);
	
	/*
	//old spec boost
	float specular_boost = ((0.5+material_ID) * (0.5+alb_gloss.a))-0.25; //0.0 - 2.0 range
	specular_boost = specular_boost - 1; //scale in -1 to +1 range
	specular_boost = SPECULAR_RANGE * specular_boost;
	specular_boost = max(0, specular_boost + 1); //0 - 2
	*/
	
	float specular_boost = (material_ID*2-1) + (alb_gloss.a*2-1); //-2.0 - +2.0 range
	specular_boost = exp2(SPECULAR_RANGE * specular_boost);
	specular_boost = pow(specular_boost, SPECULAR_POW);

	specular *= specular_boost;

	return saturate(lerp(specular, specular_metal, metalness));
}

float calc_rough(float4 alb_gloss, float material_ID)
{
	float metalness = calc_metalness(alb_gloss, material_ID);
	
	alb_gloss.a = pow(alb_gloss.a, ROUGHNESS_POW - (metalness * METAL_BOOST)); //metal boost
	
	float roughpow = 0.5 / max(0.001, 1 - Ldynamic_color.w);
	float rough = pow(lerp(ROUGHNESS_HIGH, ROUGHNESS_LOW, alb_gloss.a), roughpow);

	//rough = pow(rough, 1 + (metalness * METAL_BOOST)); //metal boost
	
	return saturate(rough*rough);
}

//=================================================================================================
//Rain and Foliage
//
void calc_rain(inout float3 albedo, inout float3 specular, inout float rough, in float4 alb_gloss, in float material_ID, in float rainmask)
{
	//rain based on Remember Me's implementation
	//float wetness = saturate(rain_params.x*rainmask);
	// yohji - edited to clamp rain_density between 0-0.5, to prevent weird shading with high rain_density
	float wetness = saturate(smoothstep(0.1,0.9,clamp(rain_params.x, 0.0, 0.5)*rainmask));

	float porosity = 1-saturate(material_ID*1.425); //metal material at 0, concrete at 1
	//porosity = saturate((porosity-0.5)/0.4); //Remember Me rain porosity

	float factor = lerp(1,0.2, porosity); //albedo darkening factor
	
	albedo *= lerp(1, factor, wetness); 
	rough = lerp(0.001, rough, lerp(1, factor, wetness)); 
	specular = lerp(specular, 0.02, wetness);
}

void calc_foliage(inout float3 albedo, inout float3 specular, inout float rough, in float4 alb_gloss, in float mat_id)
{
	//specular = (abs(mat_id-MAT_FLORA) <= MAT_FLORA_ELIPSON) ? calc_specular(alb_gloss, 0.0) : specular;
	specular = (abs(mat_id-MAT_FLORA) <= MAT_FLORA_ELIPSON) ? alb_gloss.g * 0.02 : specular;
	//specular = (abs(mat_id-MAT_FLORA) <= MAT_FLORA_ELIPSON) ? pow(alb_gloss.g * 0.1414, 2) : specular;
}

//=================================================================================================
//Functions
//

float F_Shlick(float f0, float f90, float vDotH)
{
	return lerp(f0, f90, pow(1-vDotH, 5));
}

float3 F_Shlick(float3 f0, float3 f90, float vDotH)
{
	return lerp(f0, f90, pow(1-vDotH, 5));
}
// We have a better approximation of the off specular peak
// but due to the other approximations we found this one performs better .
// N is the normal direction
// R is the mirror vector
// This approximation works fine for G smith correlated and uncorrelated
float3 getSpecularDominantDir(float3 N, float3 R, float roughness)
{
	float smoothness = saturate(1 - roughness);
	float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
	// The result is not normalized as we fetch in a cubemap
	return lerp(N, R, lerpFactor);
}

//=================================================================================================
//Shading
//

//include BRDFs
#include "pbr_brdf_blinn.h" //brdf
#include "pbr_brdf_ggx.h" //brdf

float Lit_Burley(float nDotL, float nDotV, float vDotH, float rough)
{
	float fd90 = 0.5 + 2 * vDotH * vDotH * rough;
	float lightScatter = F_Shlick(1, fd90, nDotL);
	float viewScatter = F_Shlick(1, fd90, nDotV);
 	return (lightScatter * viewScatter) / PI;
}

float Lambert_Source(float nDotL,float rough)
{
	float exponent = lerp(1.4, 0.6, rough);
	return (pow(nDotL, exponent) * ((exponent + 1.0) * 0.5)) / max(1e-5, PI * nDotL);
}

float Lit_Diffuse(float nDotL, float nDotV, float vDotH, float rough)
{
#ifdef USE_BURLEY_DIFFUSE
	return Lit_Burley(nDotL, nDotV, vDotH, rough);
#else
	return Lambert_Source(nDotL, rough);
#endif
}

float3 Lit_Specular(float nDotL, float nDotH, float nDotV, float vDotH, float3 f0, float rough)
{
#ifdef USE_GGX_SPECULAR
	return Lit_GGX(nDotL, nDotH, nDotV, vDotH, f0, rough); //GGX is much more expensive but looks nicer
#else
	return Lit_Blinn(nDotL, nDotH, nDotV, vDotH, f0, rough); //much cheaper pbr blinn 
#endif
}

float3 Lit_BRDF(float rough, float3 albedo, float3 f0, float3 V, float3 N, float3 L )
{
	float3 H = normalize(V + L );
	
	float nDotL = saturate(dot(N, L));
	float nDotH = saturate(dot(N, H));
	//float nDotV = saturate(dot(N, V));
	//float nDotV = 1e-5 + abs(dot(N, V)); //DICE
	float nDotV = max(1e-5, dot(N, V));
	float vDotH = saturate(dot(V, H));
	
	float3 diffuse_term = Lit_Diffuse(nDotL, nDotV, vDotH, rough).rrr;
	diffuse_term *= albedo;
	
	float3 specular_term = Lit_Specular(nDotL, nDotH, nDotV, vDotH, f0, rough);
	
	// horizon occlusion with falloff, should be computed for direct specular too
	//float R = reflect(V, N);
	//float R = 2 * dot(N, V) * N - V;
	//float horizon = saturate(1.0 + dot(R, N)); //needs vertex normals
	float horizon = saturate(0.95 + dot(N, V));
	horizon *= horizon;

	specular_term *= horizon; //horizon atten
	
	return (diffuse_term + specular_term) *  nDotL * PI;
}

//=================================================================================================
//Ambient
//

float EnvBurley(float roughness, float NV)
{
    //Burley (Hill's curve)
    float d0 = 0.97619 - 0.488095 * pow(1.0 - NV, 5.0);
    float d1 = 1.55754 + (-2.02221 + (2.56283 - 1.06244 * NV) * NV) * NV;
    return lerp(d0, d1, roughness);
}

float Amb_Diffuse(float3 f0, float rough, float nDotV)
{
#ifdef USE_BURLEY_DIFFUSE
    return EnvBurley(rough, nDotV);
#else
	return 1.0; 
#endif
}

float3 Amb_Specular(float3 f0, float rough, float nDotV)
{
#ifdef USE_GGX_SPECULAR
    return EnvGGX(f0, rough, nDotV);
#else
    return EnvBlops2(f0, rough, nDotV);
#endif
}

float3 Amb_BRDF(float rough, float3 albedo, float3 f0, float3 env_d, float3 env_s, float3 V, float3 N)
{
	//float nDotV = saturate(dot(N, V));
	//float nDotV = 1e-5 + abs(dot(N, V)); //DICE
	float nDotV = max(1e-5, dot(N, V));
	 
	float3 diffuse_term = Amb_Diffuse(f0, rough, nDotV);
	diffuse_term *= env_d * albedo;
	
	float3 specular_term = Amb_Specular(f0, rough, nDotV);
	specular_term *= env_s;
	
	// horizon occlusion with falloff, should be computed for direct specular too
	//float R = reflect(V, N);
	//float horizon = saturate(1.0 + dot(R, N)); //needs vertex normals
	float horizon = saturate(0.95 + dot(N, V));
	horizon *= horizon;

	specular_term *= horizon; //horizon atten
	
	return diffuse_term + specular_term;
}