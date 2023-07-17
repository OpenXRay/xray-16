#ifndef	TONEMAPPING_H
#define	TONEMAPPING_H

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

float3 Uncharted2ToneMapping(float3 color)
{

    color *= tnmp_exposure;
    color = ((color * (tnmp_a * color + tnmp_c * tnmp_b) + tnmp_d * tnmp_e) / (color * (tnmp_a * color + tnmp_b) + tnmp_d * tnmp_f)) - tnmp_e / tnmp_f;
    float white = ((tnmp_w * (tnmp_a * tnmp_w + tnmp_c * tnmp_b) + tnmp_d * tnmp_e) / (tnmp_w * (tnmp_a * tnmp_w + tnmp_b) + tnmp_d * tnmp_f)) - tnmp_e / tnmp_f;
    color /= white;
    color = pow(color, (1.0 / tnmp_gamma));
    return color;
}
#endif