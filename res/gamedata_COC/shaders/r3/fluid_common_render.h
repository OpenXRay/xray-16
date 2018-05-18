//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------
//#define OCCLUDED_PIXEL_RAYVALUE     float4(1, 0, 0, 0)
//	Use very large value for aplha to help edge detection
#define OCCLUDED_PIXEL_RAYVALUE     float4(1, 0, 0, 100000)
#define NEARCLIPPED_PIXEL_RAYPOS    float3(0, -1, 0)

//	Z for skybox is zero, so patch this in shader
#define Z_EPSILON	0.00001
//	Value for skybox depth
#define Z_MAX		100000

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
Texture2D	sceneDepthTex;
Texture3D	colorTex;

Texture2D	rayDataTex;
Texture2D	rayDataTexSmall;
Texture2D	rayCastTex;
Texture2D	edgeTex;
Texture2D   jitterTex;

Texture2D	fireTransferFunction;

//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------

sampler	samPointClamp;
sampler	samLinearClamp;
sampler	samRepeat;

//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------
//	Set once per volume
//	Use for all rendering passes
cbuffer FluidRenderConfig
{
	float		RTWidth;
	float		RTHeight;

	float4		DiffuseLight;

	float4x4	WorldViewProjection;
	float4x4	InvWorldViewProjection; 

	float		ZNear;
	float		ZFar;

	float4		gridDim;	//	float3
	float4		recGridDim;	//	float3
	float		maxGridDim;
	float		gridScaleFactor = 1.0;
	float4		eyeOnGrid;	//	float3
}

//static	float		edgeThreshold = 0.2;
//static	float		edgeThreshold = 0.1;
static	float		edgeThreshold = 0.01;

static const bool	g_bRaycastFilterTricubic = false; // true: tricubic; false: trilinear
//static const bool	g_bRaycastFilterTricubic = true; // true: tricubic; false: trilinear

#include "fluid_common_tricubic.h"

//	Fire setup
static const float RednessFactor = 5.0f;
static const float fireAlphaMultiplier = 0.95f;
//static const float smokeAlphaMultiplier = 0.05f;
static const float smokeAlphaMultiplier = 0.5f;
//static const float smokeColorMultiplier = 2.00f;
static const float smokeColorMultiplier = 0.02f;

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
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
    // There's no textureUV11 because its weight is zero.
    float4 position      : SV_Position;   // vertex position
    float2 textureUV00   : TEXCOORD0;  // kernel tap texture coords 
    float2 textureUV01   : TEXCOORD1;  // kernel tap texture coords 
    float2 textureUV02   : TEXCOORD2;  // kernel tap texture coords 
    float2 textureUV10   : TEXCOORD3;  // kernel tap texture coords 
    float2 textureUV12   : TEXCOORD4;  // kernel tap texture coords 
    float2 textureUV20   : TEXCOORD5;  // kernel tap texture coords 
    float2 textureUV21   : TEXCOORD6;  // kernel tap texture coords 
    float2 textureUV22   : TEXCOORD7;  // kernel tap texture coords 
};

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------

float EdgeDetectScalar(float sx, float sy, float threshold)
{
    float dist = (sx*sx+sy*sy);
    float e = (dist > threshold*ZFar)? 1: 0;
    return e;
}

/*
// We can select either back=to-front or front-to-back raycasting and blending.
//  front-to-back may be slightly more expensive, but if the smoke is dense it allows
//  early-out when the opacity gets saturated (close to 1.0), making it a bit cheaper
//
// Define BACK_TO_FRONT to use back-to-front raycasting
//#define BACK_TO_FRONT 1
void DoSample(float weight, float3 O, inout float4 color )
{
    // This value can be tuned to produce denser or thinner looking smoke
    // Alternatively a transfer function could be used
    #define OPACITY_MODULATOR 0.1

    float3 texcoords;
    float4 sample;
    float t;

    texcoords = float3( O.x, 1 - O.y, O.z) ;
//    sample = weight * colorTex.SampleLevel(samLinearClamp, texcoords, 0);
//	sample = weight * abs(SampleTricubic(colorTex, texcoords));
//	sample = weight * abs(SampleTrilinear(colorTex, texcoords));
	sample = weight * abs(Sample(colorTex, texcoords));
    sample.a = (sample.r) * OPACITY_MODULATOR;

#ifdef BACK_TO_FRONT // back-to-front blending
    color.rgb = (1 - sample.a) * color.r + sample.a * sample.r;
    color.a = (1 - sample.a) * color.a + sample.a;
#else // front-to-back blending
    t = sample.a * (1.0-color.a);
    color.rgb += t * sample.r;
    color.a += t;
#endif

}

float4 Raycast( PS_INPUT_RAYCAST input )
{
    float4 color = 0;
    float4 rayData = rayDataTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));

    // Don't raycast if the starting position is negative 
    //   (see use of OCCLUDED_PIXEL_RAYVALUE in PS_RAYDATA_FRONT)
    if(rayData.x < 0)
        return color;

    // If the front face of the box was clipped here by the near plane of the camera
    //   (see use of NEARCLIPPED_PIXEL_RAYPOS in PS_RAYDATA_BACK)
    if(rayData.y < 0)
    {
       // Initialize the position of the fragment and adjust the depth
       rayData.xyz = input.posInGrid;
       rayData.w = rayData.w - ZNear;
//	   return float4 (1,0,0,saturate(rayData.w/5));
    }

    float3 rayOrigin = rayData.xyz;
    float Offset = jitterTex.Sample( samRepeat, input.pos.xy / 256.0 ).r;
    float rayLength = rayData.w;

    // Sample twice per voxel
    float fSamples = ( rayLength / gridScaleFactor * maxGridDim ) * 2.0;
    int nSamples = floor(fSamples);
    float3 stepVec = normalize( (rayOrigin - eyeOnGrid) * gridDim ) * recGridDim * 0.5;
   
    float3 O = rayOrigin + stepVec*Offset;
    
#ifdef BACK_TO_FRONT
    // In back-to-front blending we start raycasting from the surface point and step towards the eye
    O += fSamples * stepVec;
    stepVec = -stepVec;
#endif

    for( int i=0; i<nSamples ; i++ )
    {
        DoSample(1, O, color);
        O += stepVec;

#ifndef BACK_TO_FRONT
    // If doing front-to-back blending we can do early exit when opacity saturates
    if( color.a > 0.99 )
        break;
#endif
    }

    // The last sample is weighted by the fractional part of the ray length in voxel 
    //  space (fSamples), thus avoiding banding artifacts when the smoke is blended against the scene
    if( i == nSamples )
    {
        DoSample(frac(fSamples), O, color);
    }

    return color;
}
*/

//#define	RENDER_FIRE
void DoSample(float weight, float3 O, inout float4 color )
{
    // This value can be tuned to produce denser or thinner looking smoke
    // Alternatively a transfer function could be used
    #define OPACITY_MODULATOR 0.1

    float3 texcoords;
	texcoords = float3( O.x, 1 - O.y, O.z) ;

#ifndef	RENDER_FIRE
	//render smoke with front to back blending
	float t;
	float4 sample = weight * abs(Sample(colorTex, texcoords));
	sample.a = (sample.r) * 0.1;
	t = sample.a * (1.0-color.a);
	color.rgb += t * sample.r;
	color.a += t;
#else	//	RENDER_FIRE
	//render fire and smoke with back to front blending 
        
	//dont render the area below where the fire originates
//	if(O.z < OBSTACLE_MAX_HEIGHT/gridDim.z)
//		return;
        
	//this is the threshold at which we decide whether to render fire or smoke
	float threshold = 1.4;
	float maxValue = 3;
        
	float s = colorTex.SampleLevel(samLinearClamp, texcoords, 0).x;
	s = clamp(s,0,maxValue);
          
	if(s>threshold)   
	{   
		//render fire
		float lookUpVal = ( (s-threshold)/(maxValue-threshold) );
		lookUpVal = 1.0 - pow(lookUpVal,RednessFactor);
		lookUpVal = clamp(lookUpVal,0,1);
		float3 interpColor = fireTransferFunction.SampleLevel(samLinearClamp,float2(lookUpVal,0),0); 
		float mult = (s-threshold);
		color += float4(weight*interpColor.rgb,weight*mult*mult*fireAlphaMultiplier); 
	}
	else    
	{
		//render smoke
		float4 sample = weight*s;
		sample.a = sample.r*0.1*smokeAlphaMultiplier;
		float3 smokeColor = float3(0.9,0.35,0.055);
		color.rgb = (1 - sample.a) * color.rgb + sample.a * sample.rrr * smokeColor * smokeColorMultiplier * 5.0; 
		color.a = (1 - sample.a) * color.a + sample.a;
	}
#endif	//	RENDER_FIRE
}

float4 Raycast( PS_INPUT_RAYCAST input )
{
    float4 color = 0;
    float4 rayData = rayDataTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));

    // Don't raycast if the starting position is negative 
    //   (see use of OCCLUDED_PIXEL_RAYVALUE in PS_RAYDATA_FRONT)
    if(rayData.x < 0)
        return color;

    // If the front face of the box was clipped here by the near plane of the camera
    //   (see use of NEARCLIPPED_PIXEL_RAYPOS in PS_RAYDATA_BACK)
    if(rayData.y < 0)
    {
       // Initialize the position of the fragment and adjust the depth
       rayData.xyz = input.posInGrid;
       rayData.w = rayData.w - ZNear;
//	   return float4 (1,0,0,saturate(rayData.w/5));
    }

    float3 rayOrigin = rayData.xyz;
    float Offset = jitterTex.Sample( samRepeat, input.pos.xy / 256.0 ).r;
    float rayLength = rayData.w;

    // Sample twice per voxel
    float fSamples = ( rayLength / gridScaleFactor * maxGridDim ) * 2.0;
    int nSamples = floor(fSamples);
    float3 stepVec = normalize( (rayOrigin - eyeOnGrid) * gridDim ) * recGridDim * 0.5;
   
    float3 O = rayOrigin + stepVec*Offset;
    
#ifdef 	RENDER_FIRE
    // In back-to-front blending we start raycasting from the surface point and step towards the eye
    O += fSamples * stepVec;
    stepVec = -stepVec;
#endif	//	RENDER_FIRE

    for( int i=0; i<nSamples ; i++ )
    {
        DoSample(1, O, color);
        O += stepVec;

#ifndef RENDER_FIRE
    	// If doing front-to-back blending we can do early exit when opacity saturates
	    if( color.a > 0.99 )
    	    break;
#endif	//	RENDER_FIRE
    }

    // The last sample is weighted by the fractional part of the ray length in voxel 
    //  space (fSamples), thus avoiding banding artifacts when the smoke is blended against the scene
    if( i == nSamples )
    {
        DoSample(frac(fSamples), O, color);
    }

    return color;
}