
#ifndef	common_iostructs_h_included
#define	common_iostructs_h_included

#include "shared/common.h"

struct	v_TL0uv_positiont
{
    float4	P : POSITIONT;
    float4	Color : COLOR;
};

struct	v_TL0uv
{
    float4	P : POSITION;
    float4	Color : COLOR;
};

struct	v2p_TL0uv
{
    float4	Color : COLOR;
    float4 	HPos : SV_Position;
};

struct	p_TL0uv
{
    float4	Color : COLOR;
};

struct	v_TL_positiont
{
    float4	P : POSITIONT;
    float2	Tex0 : TEXCOORD0;
    float4	Color : COLOR;
    uint	TexIdx : TEXINDEX;
};

struct	v_TL
{
    float4	P : POSITION;
    float2	Tex0 : TEXCOORD0;
    float4	Color : COLOR;
    uint	TexIdx : TEXINDEX;
};

struct	v2p_TL
{
    float2 	Tex0 : TEXCOORD0;
    float4	Color : COLOR;
    nointerpolation uint TexIdx : TEXINDEX;
    float4 	HPos : SV_Position;
};

struct	p_TL
{
    float2 	Tex0 : TEXCOORD0;
    float4	Color : COLOR;
    nointerpolation uint TexIdx : TEXINDEX;
};

struct	v_TL2uv
{
    float4	P : POSITIONT;
    float2	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float4	Color : COLOR;
};

struct	v2p_TL2uv
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float4	Color : COLOR;
    float4 	HPos : SV_Position;
};

struct	p_TL2uv
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float4	Color : COLOR;
};
struct	v_postpr
{
    float4	P : POSITIONT;
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2	Tex2 : TEXCOORD2;
    float4	Color : COLOR0;
    float4	Gray : COLOR1;
};

struct	v2p_postpr
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2	Tex2 : TEXCOORD2;
    float4	Color : COLOR0;
    float4	Gray : COLOR1;
    float4 	HPos : SV_Position;
};

struct	p_postpr
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2	Tex2 : TEXCOORD2;
    float4	Color : COLOR0;
    float4	Gray : COLOR1;
};
struct	v_build
{
    float4	P : POSITIONT;
    float2	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2 	Tex2 : TEXCOORD2;
    float2	Tex3 : TEXCOORD3;
};

struct	v2p_build
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2 	Tex2 : TEXCOORD2;
    float2	Tex3 : TEXCOORD3;
    float4 	HPos : SV_Position;
};

struct	p_build
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2 	Tex2 : TEXCOORD2;
    float2	Tex3 : TEXCOORD3;
};
struct	v_filter
{
    float4	P : POSITIONT;
    float4 	Tex0 : TEXCOORD0;
    float4	Tex1 : TEXCOORD1;
    float4 	Tex2 : TEXCOORD2;
    float4	Tex3 : TEXCOORD3;
    float4 	Tex4 : TEXCOORD4;
    float4	Tex5 : TEXCOORD5;
    float4 	Tex6 : TEXCOORD6;
    float4	Tex7 : TEXCOORD7;
};

struct	v2p_filter
{
    float4 	Tex0 : TEXCOORD0;
    float4	Tex1 : TEXCOORD1;
    float4 	Tex2 : TEXCOORD2;
    float4	Tex3 : TEXCOORD3;
    float4 	Tex4 : TEXCOORD4;
    float4	Tex5 : TEXCOORD5;
    float4 	Tex6 : TEXCOORD6;
    float4	Tex7 : TEXCOORD7;
    float4 	HPos : SV_Position;
};

struct	p_filter
{
    float4 	Tex0 : TEXCOORD0;
    float4	Tex1 : TEXCOORD1;
    float4 	Tex2 : TEXCOORD2;
    float4	Tex3 : TEXCOORD3;
    float4 	Tex4 : TEXCOORD4;
    float4	Tex5 : TEXCOORD5;
    float4 	Tex6 : TEXCOORD6;
    float4	Tex7 : TEXCOORD7;
};

struct	v_aa_AA
{
    float4 P : POSITIONT;
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2 	Tex2 : TEXCOORD2;
    float2	Tex3 : TEXCOORD3;
    float2	Tex4 : TEXCOORD4;
    float4	Tex5 : TEXCOORD5;
    float4	Tex6 : TEXCOORD6;
};

struct	v2p_aa_AA
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2 	Tex2 : TEXCOORD2;
    float2	Tex3 : TEXCOORD3;
    float2	Tex4 : TEXCOORD4;
    float4	Tex5 : TEXCOORD5;
    float4	Tex6 : TEXCOORD6;
    float4 	HPos : SV_Position;
};

struct	p_aa_AA
{
    float2 	Tex0 : TEXCOORD0;
    float2	Tex1 : TEXCOORD1;
    float2 	Tex2 : TEXCOORD2;
    float2	Tex3 : TEXCOORD3;
    float2	Tex4 : TEXCOORD4;
    float4	Tex5 : TEXCOORD5;
    float4	Tex6 : TEXCOORD6;
};

struct	p_aa_AA_sun
{
    float2 	tc : TEXCOORD0;
    float2	unused : TEXCOORD1;
    float2 	LT : TEXCOORD2;
    float2	RT : TEXCOORD3;
    float2	LB : TEXCOORD4;
    float2	RB : TEXCOORD5;
};

struct 	v_dumb
{
    float4	P : POSITION;
};

struct 	v2p_dumb
{
    float4	HPos : SV_Position;
};

struct 	v2p_volume
{
    float4 	tc : TEXCOORD0;
#ifdef 	USE_SJITTER
    float4 	tcJ : TEXCOORD1;
#endif
    float4 	hpos : SV_Position;
};
struct 	p_volume
{
    float4 	tc : TEXCOORD0;
#ifdef 	USE_SJITTER
    float4 	tcJ : TEXCOORD1;
#endif
};
struct         v_static
{
    float4	Nh : NORMAL;
    float4	T : TANGENT;
    float4	B : BINORMAL;
    int2	tc : TEXCOORD0;
#ifdef	USE_LM_HEMI
    int2	lmh : TEXCOORD1;
#endif
    float4	P : POSITION;
};

struct	v_static_color
{
    float4	Nh : NORMAL;
    float4	T : TANGENT;
    float4	B : BINORMAL;
    int2	tc : TEXCOORD0;
#ifdef	USE_LM_HEMI
    int2	lmh : TEXCOORD1;
#endif
    float4	color : COLOR0;
    float4	P : POSITION;
};

struct f_forward
{
    float4	color : SV_Target0;
    float4	normal : SV_Target1;
    float4	baseColor : SV_Target2;
};

struct					gbuffer_data
{
    float3  P;
    float   mtl;
    float3  N;
    float   hemi;
    float3  C;
    float   gloss;
};

struct v2p_bumped
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : BLENDINDICES;
    float3	M1 : TEXCOORD2;
    float3	M2 : TEXCOORD3;
    float3	M3 : TEXCOORD4;
#ifdef USE_TDETAIL
    float2	tcdbump : TEXCOORD5;
#endif
#ifdef USE_LM_HEMI
    float2	lmh : TEXCOORD6;
#endif
    float4	hpos : SV_Position;
};

struct p_bumped
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : BLENDINDICES;
    float3	M1 : TEXCOORD2;
    float3	M2 : TEXCOORD3;
    float3	M3 : TEXCOORD4;
#ifdef USE_TDETAIL
    float2	tcdbump : TEXCOORD5;
#endif
#ifdef USE_LM_HEMI
    float2	lmh : TEXCOORD6;
#endif
};
struct	v2p_flat
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : TEXCOORD1;
    float3	N : TEXCOORD2;
#ifdef USE_TDETAIL
    float2	tcdbump : TEXCOORD3;
#endif
#ifdef USE_LM_HEMI
    float2	lmh : TEXCOORD4;
#endif
    float	heightParam : TEXCOORD5;
    float2	interaction_uv : TEXCOORD6;
    float3	rotatedNormal1 : TEXCOORD7;
    float3	rotatedNormal2 : TEXCOORD8;
    nointerpolation uint objectId : TEXCOORD9;
    float	bladeHash : TEXCOORD10;
    float4	hpos : SV_Position;
};

struct	p_flat
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : TEXCOORD1;
    float3	N : TEXCOORD2;
#ifdef USE_TDETAIL
    float2	tcdbump : TEXCOORD3;
#endif
#ifdef USE_LM_HEMI
    float2	lmh : TEXCOORD4;
#endif
    float	heightParam : TEXCOORD5;
    float2	interaction_uv : TEXCOORD6;
    float3	rotatedNormal1 : TEXCOORD7;
    float3	rotatedNormal2 : TEXCOORD8;
    nointerpolation uint objectId : TEXCOORD9;
    float	bladeHash : TEXCOORD10;
    float4	hpos : SV_Position;
};

struct	v2p_decal
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : TEXCOORD1;
    float3	N : TEXCOORD2;
    float4	hpos : SV_Position;
};

struct	p_decal
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : TEXCOORD1;
    float3	N : TEXCOORD2;
    float4	hpos : SV_Position;
};

struct	v2p_billboard
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : TEXCOORD1;
    float3	N : TEXCOORD2;
    float	heightParam : TEXCOORD3;
    float	bladeHash : TEXCOORD4;
    float4	hpos : SV_Position;
};

struct	p_billboard
{
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4	tcdh : TEXCOORD0;
#else
    float2	tcdh : TEXCOORD0;
#endif
    float4	position : TEXCOORD1;
    float3	N : TEXCOORD2;
    float	heightParam : TEXCOORD3;
    float	bladeHash : TEXCOORD4;
    float4	hpos : SV_Position;
};

struct	v_shadow_direct_aref
{
    float4	P : POSITION;
    int4	tc : TEXCOORD0;
};

struct	v_shadow_direct
{
    float4	P : POSITION;
};


struct	v2p_shadow_direct_aref
{
    float2	tc0 : TEXCOORD1;
    float4	hpos : SV_Position;
#ifndef USE_HWSMAP
    float	depth : TEXCOORD0;
#endif
};

struct	v2p_shadow_direct
{
    float4	hpos : SV_Position;
#ifndef USE_HWSMAP
    float	depth : TEXCOORD0;
#endif
};

struct	p_shadow_direct_aref
{
    float2	tc0 : TEXCOORD1;
#ifndef USE_HWSMAP
    float	depth : TEXCOORD0;
#endif
};

struct	v_model
{
    float4	P : POSITION;
    float3	N : NORMAL;
    float3	T : TANGENT;
    float3	B : BINORMAL;
    float2	tc : TEXCOORD0;
};

struct	v_tree
{
    float4	P : POSITION;
    float4	Nh : NORMAL;
    float3	T : TANGENT;
    float3	B : BINORMAL;
    int4	tc : TEXCOORD0;
};

struct        v_detail
{
    float4      pos : POSITION;
    int4        misc : TEXCOORD0;
};

#endif
