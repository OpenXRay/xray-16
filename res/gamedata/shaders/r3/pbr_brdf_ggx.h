float D_GGX(float NdotH, float a2)
{
    float denominator = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (PI * denominator * denominator);
}

float Lambda_Smith(float NdotX, float a)
{    
    float a2 = a * a;
    float NdotX_2 = NdotX * NdotX;
    return (-1.0 + sqrt(a2 * (1.0 - NdotX_2) / NdotX_2 + 1.0)) * 0.5;
}

//Height Correlated Masking-shadowing function
float G2_Smith_Correlated(float NdotL, float NdotV, float a)
{
	float lambdaV = Lambda_Smith(NdotV, a);
	float lambdaL = Lambda_Smith(NdotL, a);

	return 1.0 / (1.0 + lambdaV + lambdaL);
}

float G2_SmithJointApprox(float NdotL, float NdotV, float a)
{
	float Vis_V = NdotL * ( NdotV * ( 1 - a ) + a );
	float Vis_L = NdotV * ( NdotL * ( 1 - a ) + a );
	return 0.5 * rcp( Vis_V + Vis_L );
}

float3 Lit_GGX(float NdotL, float NdotH, float NdotV, float VdotH, float3 F0, float rough)
{
	//Alpha
	float a = rough * rough;
	float a2 = a * a;

    //Normal distribution function
    float D = D_GGX(NdotH, a2);
    
    //Masking-shadowing
    //float V = G2_Smith_Correlated(NdotL, NdotV, a);
    float V = G2_SmithJointApprox(NdotL, NdotV, a); //denom included?
    
    //Fresnel
    float3 f90Atten = saturate(50*F0); //UE4 specular shadowing
    float3 F = F_Shlick(F0, f90Atten, VdotH);
    
    //Numerator
    float3 numerator = (D * V) * F;
    
    //Denominator
    float denominator = 4.0 * NdotV;
    
    return numerator; //UE4 has no denom
    //return numerator / denominator;
}

//UE4 mobile approx
float2 EnvBRDFApprox(float Roughness, float NoV )
{
	const float4 c0 = { -1, -0.0275, -0.572, 0.022 };
	const float4 c1 = { 1, 0.0425, 1.04, -0.04 };
	float4 r = Roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV ) ) * r.x + r.y;
	float2 AB = float2(-1.04, 1.04 ) * a004 + r.zw;
	return AB;
}

//-------------------------------------------------------------------------------------------------
// Returns scale and bias values for environment specular reflections that represents the
// integral of the geometry/visibility + fresnel terms for a GGX BRDF given a particular
// viewing angle and roughness value. The final value is computed using polynomials that were
// fitted to tabulated data generated via monte carlo integration.
//-------------------------------------------------------------------------------------------------
float2 GGXEnvironmentBRDFScaleBias(float roughness, float nDotV)
{
    const float sqrtRoughness = sqrt(roughness);
    const float nDotV2 = nDotV * nDotV;
    const float sqrtRoughness2 = sqrtRoughness * sqrtRoughness;
    const float sqrtRoughness3 = sqrtRoughness2 * sqrtRoughness;

    const float delta = 0.991086418474895f + (0.412367709802119f * sqrtRoughness * nDotV2) -
                        (0.363848256078895f * sqrtRoughness2) -
                        (0.758634385642633f * nDotV * sqrtRoughness2);
    const float bias = saturate((0.0306613448029984f * sqrtRoughness) + 0.0238299731830387f /
                                (0.0272458171384516f + sqrtRoughness3 + nDotV2) -
                                0.0454747751719356f);

    const float scale = saturate(delta - bias);
    return float2(scale, bias);
}

//LVutner ambient BRDF
float2 integrate_brdf(float roughness, float NV)
{
    //Schlick's approximation
    float F_partial = pow(1.0 - NV, 5.0);

    //GGX
    const float4 c0 = float4(-1.0, -0.0275, -0.26, 0.0109);    
    const float4 c1 = float4(1.0, 0.0455, 1.0417, -0.0417);

    float4 r = roughness * c0 + c1;
    float a004 = min(0.9 - 0.75 * roughness, F_partial) * r.x + r.y;
    float2 AB = float2(-1.0417, 1.0417) * a004 + r.zw;        

    //Output (Scale, Bias, Burley)
    return float2(AB.x, AB.y);        
}

float3 EnvGGX(float3 f0, float rough, float nDotV )
{
	//UE4 GGX
	float3 f90Atten = saturate(50*f0); //UE4 specular shadowing
    float2 AB = EnvBRDFApprox(rough, nDotV);
	/*
	//Matt Pettineo GGX
    float2 AB = GGXEnvironmentBRDFScaleBias(rough, nDotV);  
	
	//LVutner GGX
    float2 AB = integrate_brdf(rough, nDotV);  
	*/
	
	return (f0 * AB.x + AB.y * f90Atten);
}