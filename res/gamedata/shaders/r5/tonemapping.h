
#ifndef	TONEMAPPING_H
#define	TONEMAPPING_H
//
uniform float tnmp_a;
uniform float tnmp_b;
uniform float tnmp_c;
uniform float tnmp_d;
uniform float tnmp_e;
uniform float tnmp_f;	
uniform float tnmp_w;
uniform float tnmp_exposure;
uniform float tnmp_gamma;
uniform float tnmp_onoff;

float3 Uncharted2ToneMapping(float3 gammaPre)
{
	//GET RID OF THIS
	//IT IS ADDED AFTER TONEMAPPING AND CLIPS EVERYTHING
	return 0;
/*
	gammaPre *= tnmp_exposure;
	gammaPre = ((gammaPre * (tnmp_a * gammaPre + tnmp_c * tnmp_b) + tnmp_d * tnmp_e) / (gammaPre * (tnmp_a * gammaPre + tnmp_b) + tnmp_d * tnmp_f)) - tnmp_e / tnmp_f;
	float white = ((tnmp_w * (tnmp_a * tnmp_w + tnmp_c * tnmp_b) + tnmp_d * tnmp_e) / (tnmp_w * (tnmp_a * tnmp_w + tnmp_b) + tnmp_d * tnmp_f)) - tnmp_e / tnmp_f;
	gammaPre /= white;
	gammaPre = pow(gammaPre, (1.f / tnmp_gamma));
	return gammaPre;
*/
}
#endif