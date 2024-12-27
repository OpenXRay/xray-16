//BLOPS 2 blinn phong PBR
//credit to Treyarch

float CalcGlossmap(float rough)
{
	return 1.0 - rough;
}

float RoughToGlossExp(float rough)
{
	//return = 2 / ((rough*rough) - 2); UE4 blinn
	return pow(8192, CalcGlossmap(rough));//blops2
	//return pow(2, 11 * CalcGlossmap(rough)) - 1;
}

float D_Blinn(float a, float nDotH)
{
	return pow(nDotH, a) * ((a+2)/(8*PI)); //BLOPS2
}

float G_Smith(float a, float nDotX)
{
	float k = 2 / sqrt(PI * (a + 2)); //remapping for BLOPS
	return nDotX / (nDotX * (1 - k) + k);
}


float G_SmithBLOPS(float a, float nDotl, float nDotv)
{
	float k = 2 / sqrt(PI * (a + 2)); //remapping for BLOPS
	float A = nDotl * (1 - k) + k; //remapping for BLOPS
	float B = nDotv * (1 - k) + k; //remapping for BLOPS
	
	return 1 / (A*B);
}

float3 Lit_Blinn(float nDotL, float nDotH, float nDotV, float vDotH, float3 f0, float rough)
{
	float a = RoughToGlossExp(rough);

	float d = D_Blinn(a, nDotH);
	
	//float v = G_Smith(a, nDotL) * G_Smith(a, nDotV);
	float v = G_SmithBLOPS(a, nDotL, nDotV);
	
	float3 f90Atten = saturate(50*f0); //UE4 specular shadowing
	float3 f = F_Shlick(f0, f90Atten, vDotH);

	return d * f * v;
}

float3 EnvBlops2(float3 f0, float rough, float NoV )
{
	float g = CalcGlossmap(rough);
	float3 f90Atten = saturate(50*f0); //UE4 specular shadowing
	
	float4 t = float4(1/0.96, 0.475, (0.0275-0.25*0.04)/0.96, 0.25);
	t *= float4(g, g, g, g);
	t += float4(0, 0, (0.015-0.75*0.04)/0.96, 0.75);
	
	float a0 = t.x * min(t.y, exp2(-9.28*NoV)) + t.z;
	float a1 = t.w;
	
	return saturate(f90Atten * a0 + f0 * (a1 - a0));
}