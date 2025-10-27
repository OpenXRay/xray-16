//=================================================================================================
//LMT - Technicolor from http://enbseries.enbdev.com/forum/viewtopic.php?f=7&t=3552
//Saturation boost that emulates the vibrancy of 3 strip technicolor
//=================================================================================================

	float Technicolor_Amount   = .4;

void Technicolor( inout float3 aces)
{
	float3 x = aces;
#ifdef USE_ACES
	x = ACEScg_to_sRGB(x);
	x = max(0, x);
#endif
	
	float3 colStrength   = .4;
	float brightness   = 0;
	
	x = x/(1+x); //compress to LDR
	
	float3 source = saturate(x.rgb);
	float3 temp = 1-source.rgb;
	float3 target = temp.grg;
	float3 target2 = temp.bbr;
	float3 temp2 = source.rgb * target.rgb;
	temp2.rgb *= target2.rgb;
	temp.rgb = temp2.rgb * colStrength.rgb;
	temp2.rgb *= brightness;
	target.rgb = temp.grg;
	target2.rgb = temp.bbr;
	temp.rgb = source.rgb - target.rgb;
	temp.rgb += temp2.rgb;
	temp2.rgb = saturate(temp.rgb - target2.rgb);
	
	x = temp2 / max(0.004, 1 - temp2); //expand to HDR
	
#ifdef USE_ACES
	x = sRGB_to_ACEScg(x);
#endif
	
	aces = lerp( aces, x, Technicolor_Amount );
}