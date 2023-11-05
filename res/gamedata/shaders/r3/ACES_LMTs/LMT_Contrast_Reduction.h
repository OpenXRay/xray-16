//=================================================================================================
//LMT - Contrast Reduction
//Match STALKER's original tonemapping by reducing the contrast
//=================================================================================================

void Contrast_Reduction( inout float3 aces)
{
	float Contrast_Amount = 0.7;
	const float mid = 0.18;
	aces = pow(aces, Contrast_Amount) * mid/pow(mid,Contrast_Amount);
	
}
