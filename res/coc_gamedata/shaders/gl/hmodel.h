#ifndef        HMODEL_H
#define HMODEL_H

#include "common.h"

//uniform samplerCUBE         env_s0                ;
//uniform samplerCUBE         env_s1                ;
//uniform samplerCUBE         sky_s0                ;
//uniform samplerCUBE         sky_s1                ;

TextureCube		env_s0;
TextureCube		env_s1;
TextureCube		sky_s0;
TextureCube		sky_s1;

uniform float4	env_color;        // color.w  = lerp factor
uniform float3x4	m_v2w;

void hmodel
(
	out float3 hdiffuse, out float3 hspecular, 
	float m, float h, float s, float3 Pnt, float3 normal
)
{
        // hscale - something like diffuse reflection
	float3	nw		= mul( m_v2w, normal );
	float	hscale	= h;	//. *        (0.5 + 0.5*nw.y);

#ifdef         USE_GAMMA_22
			hscale	= (hscale*hscale);        // make it more linear
#endif

	// reflection vector
	float3	v2PntL	= normalize( Pnt );
	float3	v2Pnt	= mul( m_v2w, v2PntL );
	float3	vreflect= reflect( v2Pnt, nw );
	float	hspec	= 0.5 + 0.5 * dot( vreflect, v2Pnt );

	// material	// sample material
	//float4	light	= tex3D( s_material, float3(hscale, hspec, m) );
//	float4	light	= s_material.Sample( smp_material, float3( hscale, hspec, m ) ).xxxy;
	float4	light	= textureLod(s_material, float3( hscale, hspec, m ), 0 ).xxxy;
//	float4	light	= float4(1,1,1,1);

	// diffuse color
//	float3	e0d		= texCUBE( env_s0, nw );
//	float3	e1d		= texCUBE( env_s1, nw );
//	float3	e0d		= env_s0.Sample( smp_rtlinear, nw );
//	float3	e1d		= env_s1.Sample( smp_rtlinear, nw );
	float3	e0d		= textureLod( env_s0, nw, 0 ).rgb;
	float3	e1d		= textureLod( env_s1, nw, 0 ).rgb;
	float3	env_d	= env_color.xyz * lerp( e0d, e1d, env_color.w );
			env_d	*=env_d;	// contrast
			hdiffuse= env_d * light.xyz + L_ambient.rgb;

	// specular color
	vreflect.y      = vreflect.y*2-1;	// fake remapping
//	float3	e0s		= texCUBE( env_s0, vreflect );
//	float3	e1s		= texCUBE( env_s1, vreflect );
//	float3	e0s		= env_s0.Sample( smp_rtlinear, vreflect );
//	float3	e1s		= env_s1.Sample( smp_rtlinear, vreflect );
	float3	e0s		= textureLod( env_s0, vreflect, 0 ).rgb;
	float3	e1s		= textureLod( env_s1, vreflect, 0 ).rgb;
	float3	env_s	= env_color.xyz * lerp( e0s, e1s, env_color.w);
			env_s	*=env_s;	// contrast
		hspecular	= env_s*light.w*s;                //*h*m*s        ;        //env_s        *light.w         * s;
}

/*
void         hmodel_table        (out float3 hdiffuse, out float3 hspecular, float m, float h, float s, float3 point, float3 normal)
{
        // hscale - something like diffuse reflection
        float         hscale         = h;

        // reflection vector
        float3         v2point        = normalize        (Pnt);
        float3        vreflect= reflect         (v2point,normal);
        float         hspec         = 0.5+0.5*dot        (vreflect,v2point);

        // material
          float4         light        = tex3D                (s_material, float3(hscale, hspec, m) );                // sample material

        // diffuse color
        float3         env_d         = texCUBE         (env_s0,normal);

        // specular color
        float3         env_s          = texCUBE         (env_s0,vreflect);

        //
        hdiffuse        = env_d        *light.xyz         + L_ambient.rgb        ;
        hspecular        = env_s        *light.w         * s                ;
}
*/
#endif