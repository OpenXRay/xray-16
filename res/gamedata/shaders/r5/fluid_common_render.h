#define OCCLUDED_PIXEL_RAYVALUE     float4(1, 0, 0, 100000)
#define NEARCLIPPED_PIXEL_RAYPOS    float3(0, -1, 0)

#define Z_EPSILON	0.00001
#define Z_MAX		100000

Texture2D	sceneDepthTex;
Texture3D	colorTex;

Texture2D	rayDataTex;
Texture2D	rayDataTexSmall;
Texture2D	rayCastTex;
Texture2D	edgeTex;
Texture2D   jitterTex;

Texture2D	fireTransferFunction;

SamplerState	samPointClamp;
SamplerState	samLinearClamp;
SamplerState	samRepeat;

cbuffer FluidRenderConfig
{
	float		RTWidth;
	float		RTHeight;

	float4		DiffuseLight;

	float4x4	WorldViewProjection;
	float4x4	InvWorldViewProjection; 

	float		ZNear;
	float		ZFar;

	float4		gridDim;
	float4		recGridDim;
	float		maxGridDim;
	float		gridScaleFactor = 1.0;
	float4		eyeOnGrid;
}

static	float		edgeThreshold = 0.01;

static const bool	g_bRaycastFilterTricubic = false;

#include "fluid_common_tricubic.h"

static const float RednessFactor = 5.0f;
static const float fireAlphaMultiplier = 0.95f;
static const float smokeAlphaMultiplier = 0.5f;
static const float smokeColorMultiplier = 0.02f;

struct VS_INPUT
{
    float3 pos      : POSITION;
};

struct PS_INPUT_RAYDATA_BACK
{
    float4 pos      : SV_Position;
    float  depth    : TEXCOORD0;
};

struct PS_INPUT_RAYDATA_FRONT
{
    float4 pos      : SV_Position;
    float3 posInGrid: POSITION;
    float  depth    : TEXCOORD0;
};

struct PS_INPUT_RAYCAST
{
    float4 pos      : SV_Position;
    float3 posInGrid: POSITION;
};


struct VS_OUTPUT_EDGE
{
    float4 position      : SV_Position;
    float2 textureUV00   : TEXCOORD0;
    float2 textureUV01   : TEXCOORD1;
    float2 textureUV02   : TEXCOORD2;
    float2 textureUV10   : TEXCOORD3;
    float2 textureUV12   : TEXCOORD4;
    float2 textureUV20   : TEXCOORD5;
    float2 textureUV21   : TEXCOORD6;
    float2 textureUV22   : TEXCOORD7;
};

float EdgeDetectScalar(float sx, float sy, float threshold)
{
    float dist = (sx*sx+sy*sy);
    float e = (dist > threshold*ZFar)? 1: 0;
    return e;
}

void DoSample(float weight, float3 O, inout float4 color )
{
    #define OPACITY_MODULATOR 0.1

    float3 texcoords;
	texcoords = float3( O.x, 1 - O.y, O.z) ;

#ifndef	RENDER_FIRE
	float t;
	float4 sample = weight * abs(Sample(colorTex, texcoords));
	sample.a = (sample.r) * 0.1;
	t = sample.a * (1.0-color.a);
	color.rgb += t * sample.r;
	color.a += t;
#else
        
	float threshold = 1.4;
	float maxValue = 3;
        
	float s = colorTex.SampleLevel(samLinearClamp, texcoords, 0).x;
	s = clamp(s,0,maxValue);
          
	if(s>threshold)
	{
		float lookUpVal = ( (s-threshold)/(maxValue-threshold) );
		lookUpVal = 1.0 - pow(lookUpVal,RednessFactor);
		lookUpVal = clamp(lookUpVal,0,1);
		float3 interpColor = fireTransferFunction.SampleLevel(samLinearClamp,float2(lookUpVal,0),0).rgb;
		float mult = (s-threshold);
		color += float4(weight*interpColor.rgb,weight*mult*mult*fireAlphaMultiplier); 
	}
	else    
	{
		float4 sample = weight*s;
		sample.a = sample.r*0.1*smokeAlphaMultiplier;
		float3 smokeColor = float3(0.9,0.35,0.055);
		color.rgb = (1 - sample.a) * color.rgb + sample.a * sample.rrr * smokeColor * smokeColorMultiplier * 5.0; 
		color.a = (1 - sample.a) * color.a + sample.a;
	}
#endif
}

float4 Raycast( PS_INPUT_RAYCAST input )
{
    float4 color = 0;
    float4 rayData = rayDataTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));

    if(rayData.x < 0)
        return color;

    if(rayData.y < 0)
    {
       rayData.xyz = input.posInGrid;
       rayData.w = rayData.w - ZNear;
    }

    float3 rayOrigin = rayData.xyz;
    float Offset = jitterTex.Sample( samRepeat, input.pos.xy / 256.0 ).r;
    float rayLength = rayData.w;

    float fSamples = ( rayLength / gridScaleFactor * maxGridDim ) * 2.0;
    int nSamples = floor(fSamples);
    float3 stepVec = normalize( (rayOrigin - eyeOnGrid) * gridDim ) * recGridDim * 0.5;
   
    float3 O = rayOrigin + stepVec*Offset;
    
#ifdef 	RENDER_FIRE
    O += fSamples * stepVec;
    stepVec = -stepVec;
#endif

    for( int i=0; i<nSamples ; i++ )
    {
        DoSample(1, O, color);
        O += stepVec;

#ifndef RENDER_FIRE
	    if( color.a > 0.99 )
    	    break;
#endif
    }

    if( i == nSamples )
    {
        DoSample(frac(fSamples), O, color);
    }

    return color;
}