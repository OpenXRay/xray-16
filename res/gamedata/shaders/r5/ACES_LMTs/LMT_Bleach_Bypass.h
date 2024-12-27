//=================================================================================================
//LMT - Bleach Bypass by Nvidia
//Creates a highly contrasted and desaturated look
//=================================================================================================

	float Bleach_Bypass_Amount = 0.75; //strength of the effect

void Bleach_Bypass( inout float3 aces)
{
	float3 x = aces;
#ifdef USE_ACES
	x = ACEScg_to_sRGB(x);
	x = max(0, x);
#endif
	
	x = x/(1+x); //compress to LDR
	
	float4 base = float4(x.rgb, 1); //compress to LDR
	float3 lumCoeff = float3(0.25,0.65,0.1);
	float lum = dot(lumCoeff,base.rgb);
	float3 blend = lum.rrr;
	float L = min(1,max(0,10*(lum- 0.45)));
	float3 result1 = 2.0f * base.rgb * blend;
	float3 result2 = 1.0f - 2.0f*(1.0f-blend)*(1.0f-base.rgb);
	float3 newColor = lerp(result1,result2,L);
	float A2 = Bleach_Bypass_Amount * base.a;
	float3 mixRGB = A2 * newColor.rgb;
	mixRGB += ((1.0f-A2) * base.rgb);
	mixRGB = saturate(mixRGB);
	
	x = mixRGB / max(0.004, 1 - mixRGB); //expand to HDR
	
#ifdef USE_ACES
	x = sRGB_to_ACEScg(x);
#endif
	
	aces = lerp( aces, x, Technicolor_Amount );
}
