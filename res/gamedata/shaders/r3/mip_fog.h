//=================================================================================================
//Mip Fog for STALKER Anomaly 
//Inspired by Uncharted 4
//=================================================================================================
#define HEIGHTFOGMAX 50
#define HEIGHTFOGMIN -75

#define FOGMAXSHARPNESS 0.995
#define FOGROUGHCURVE 1.5

#define MIPFOGAMOUNT 1.0
#define SUNFOGAMOUNT 0.25

#define MINFOGDENSITY 0.2
#define MAXFOGDENSITY 5.0
#define HEIGHTFOGCURVE 5
//=================================================================================================
float Calc_Exponent(float a)
{
	return 1 / (pow(2, a));
}

float Calc_Height(float3 wpos)
{
	return 1 - saturate((wpos.y - HEIGHTFOGMIN) / (HEIGHTFOGMAX-HEIGHTFOGMIN));
}

float Calc_FinalFog(float fog, float height)
{	
	float HeightLerp = pow(height, HEIGHTFOGCURVE);
/*
	float CamHeight = Calc_Height(eye_position);
	float CamLerp = pow(CamHeight, HEIGHTFOGCURVE);
	//HeightLerp = max(HeightLerp, CamLerp);
	HeightLerp = (HeightLerp * (1-CamLerp)) + CamLerp;
*/
	float Fog = Calc_Exponent(fog * lerp(MINFOGDENSITY, MAXFOGDENSITY, HeightLerp));
	return saturate(1 - Fog);
}

float3 Calc_SunFog(float3 pos, float3 fogrough)
{
	float3 SunFog = saturate(dot(normalize(Ldynamic_dir), -normalize(pos)));

	float gloss = lerp(FOGMAXSHARPNESS*0.5, 0, fogrough);
	gloss = gloss*gloss;
	gloss = pow(8192, gloss);
	SunFog = pow(SunFog, gloss) * ((gloss + 2)/(8 * PI)); //BLOPS2 blinn

	SunFog *= SRGBToLinear(Ldynamic_color.rgb); 
	
	return SunFog * SUNFOGAMOUNT;
}

float3 Calc_MipFog(float3 sky, float3 fogrough)
{
	sky = normalize(sky);
	
	//cubemap projection
	float3 skyabs = abs(sky);
	float skymax = max(skyabs.x, max(skyabs.y, skyabs.z));
	sky	  /= skymax;
	if (sky.y < 0.999) 
	sky.y = sky.y*2-1;	//fake remapping 
	
	sky = normalize(sky);
	
	float FogMip = lerp(CUBE_MIPS - (CUBE_MIPS * FOGMAXSHARPNESS), CUBE_MIPS, fogrough); //don't use base mip
	
	float3 s0 = env_s0.SampleLevel(smp_base, sky, FogMip);
	float3 s1 = env_s1.SampleLevel(smp_base, sky, FogMip);
	
	float3 MipFog = lerp(s0,s1,env_color.w);
	
	//srgb tint
	float3 FogTint = lerp(fog_color.rgb * 2.0, env_color.rgb, fogrough*fogrough); //env for close, fog for far
	
	MipFog *= FogTint; 
	
	//linear cubemap
	MipFog = SRGBToLinear(MipFog);	

	return MipFog * MIPFOGAMOUNT;
}

float3 Calc_Fog(float3 pos, float3 color)
{	
	color = SRGBToLinear(color.rgb);
	
	//view to world space
	float3 sky = mul(m_inv_V, pos );
	float3 wpos = sky + eye_position;

	float distance = length(pos);

	float fog = saturate(distance * fog_params.w + fog_params.x);

	float height = Calc_Height(wpos);

	float FinalFog = Calc_FinalFog(fog, height);

	float fogrough = fog;
	fogrough = pow(1 - fogrough , FOGROUGHCURVE);
	
	float3 MipFog = Calc_MipFog(sky, fogrough);
	float3 SunFog = Calc_SunFog(pos, fogrough);
	
	float3 FogColor = MipFog + SunFog;
	
	//fog blend
	float3 FogBlend = FinalFog; 
	
	//fog alpha
	float fogalpha = fog * fog;
	FogBlend *= 1 - fogalpha; 
	FogBlend += fogalpha; 
	
	float3 Final = lerp(color, FogColor, FogBlend);
	Final = LinearTosRGB(Final);
	
	return Final;
}