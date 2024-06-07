//=================================================================================================
//ACEScg color space and ACEScc log space transformations and Reference Rendering Transform for sRGB monitors
//Based on code by Stephen Hill (@self_shadow), Unity Technologies and others
//=================================================================================================
#define USE_ACES //Use ACES style color management
//=================================================================================================
static const float PI = 3.14159265;
static const float HALF_MIN = 6.10352e-5f;
static const float HALF_MAX = 65504;
static const float ACEScc_MAX = 1.4679964;
static const float ACEScc_MIDGRAY = 0.4135884;
//static const float3 LUMINANCE_VECTOR = float3(0.213, 0.715, 0.072);

static const float RRT_GLOW_GAIN = 0.05;
static const float RRT_GLOW_MID = 0.08;

static const float RRT_RED_SCALE = 0.82;
static const float RRT_RED_PIVOT = 0.03;
static const float RRT_RED_HUE = 0.0;
static const float RRT_RED_WIDTH = 135.0;

static const float RRT_SAT_FACTOR = 0.96;

//static const float CINEMA_WHITE = 48.0;
//static const float CINEMA_BLACK = CINEMA_WHITE / 2400.0;
static const float ODT_SAT_FACTOR = 0.93;

static const float DIM_SURROUND_GAMMA = 0.9811;

//=================================================================================================
//Color transformation matrices
//

static const float3x3 sRGB_2_AP0 = 
{
	0.4397010, 0.3829780, 0.1773350,
	0.0897923, 0.8134230, 0.0967616,
	0.0175440, 0.1115440, 0.8707040
};

static const float3x3 AP0_2_sRGB = 
{
	2.52169, -1.13413, -0.38756,
	-0.27648, 1.37272, -0.09624,
	-0.01538, -0.15298, 1.16835,
};

static const float3x3 sRGB_2_AP1 = 
{
	0.61319, 0.33951, 0.04737,
	0.07021, 0.91634, 0.01345,
	0.02062, 0.10957, 0.86961
};

static const float3x3 AP1_2_sRGB = 
{
	1.70505, -0.62179, -0.08326,
	-0.13026, 1.14080, -0.01055,
	-0.02400, -0.12897, 1.15297,
};

static const float3x3 AP0_2_AP1 = {
     1.4514393161, -0.2365107469, -0.2149285693,
    -0.0765537734,  1.1762296998, -0.0996759264,
     0.0083161484, -0.0060324498,  0.9977163014
};

static const float3x3 AP1_2_AP0 = {
     0.6954522414, 0.1406786965, 0.1638690622,
     0.0447945634, 0.8596711185, 0.0955343182,
    -0.0055258826, 0.0040252103, 1.0015006723
};

static const float3 AP1_RGB2Y = float3(0.272229, 0.674082, 0.0536895);
/*
static const float3x3 RRT_SAT_MAT = 
{
	0.9708890, 0.0269633, 0.00214758,
	0.0108892, 0.9869630, 0.00214758,
	0.0108892, 0.0269633, 0.96214800
};

static const float3x3 ODT_SAT_MAT = 
{
	0.949056, 0.0471857, 0.00375827,
	0.019056, 0.9771860, 0.00375827,
	0.019056, 0.0471857, 0.93375800
};
*/
static const float3x3 AP1_2_XYZ_MAT = {
     0.6624541811, 0.1340042065, 0.1561876870,
     0.2722287168, 0.6740817658, 0.0536895174,
    -0.0055746495, 0.0040607335, 1.0103391003
};

static const float3x3 XYZ_2_AP1_MAT = {
     1.6410233797, -0.3248032942, -0.2364246952,
    -0.6636628587,  1.6153315917,  0.0167563477,
     0.0117218943, -0.0082844420,  0.9883948585
};

//=================================================================================================
//Color space transformations
//

float3 sRGB_to_ACES(float3 x)
{
	return mul(sRGB_2_AP0, x);
}

float3 ACES_to_sRGB(float3 x)
{
	return mul(AP0_2_sRGB, x);
}

float3 sRGB_to_ACEScg(float3 x)
{
	return mul(sRGB_2_AP1, x);
}

float3 ACEScg_to_sRGB(float3 x)
{
	return mul(AP1_2_sRGB, x);
}

float3 ACEScg_to_ACES(float3 x)
{
	return mul(AP1_2_AP0, x);
}

float3 ACES_to_ACEScg(float3 x)
{
	return mul(AP0_2_AP1, x);
}

float lin_to_ACEScc(float x)
{
    return (x < 0.00003051757) ? (log2(0.00001525878 + x * 0.5) + 9.72) / 17.52 : (log2(x) + 9.72) / 17.52;
/*
if (x <= 0)
	return -0.3584474886;
else if (x < pow(2, -15))
	return (log2( pow(2, -16) + x * 0.5) + 9.72) / 17.52;
else
	return (log2(x) + 9.72) / 17.52;
*/
}

float ACEScc_to_lin(float x)
{
return (x < -0.3013698630) ? (exp2(x * 17.52 - 9.72) - 0.00001525878) * 2 : exp2(x * 17.52 - 9.72);
/*
if (x < -0.3013698630)
	return (pow( 2, x * 17.52 - 9.72) - pow( 2, -16)) * 2;
else
	return pow( 2, x * 17.52 - 9.72);
*/
}

float3 lin_to_ACEScc(float3 x)
{
	x = clamp(x, 0, HALF_MAX);
	x = float3(
	lin_to_ACEScc(x.r),
	lin_to_ACEScc(x.g),
	lin_to_ACEScc(x.b));
	return x;
}

float3 ACEScc_to_lin(float3 x)
{
	x = float3(
	ACEScc_to_lin(x.r),
	ACEScc_to_lin(x.g),
	ACEScc_to_lin(x.b));
	x = clamp(x, 0, HALF_MAX);
	return x;
}

float3 ACES_to_ACEScc(float3 x)
{
	x = ACES_to_ACEScg(x);
	x = lin_to_ACEScc(x);
	return x;
}

float3 ACEScc_to_ACES(float3 x)
{
	x = ACEScc_to_lin(x);
	x = ACEScg_to_ACES(x);
	return x;
}

float3 ACEScg_to_ACEScc(float3 x)
{
	x = lin_to_ACEScc(x);
	return x;
}

float3 ACEScc_to_ACEScg(float3 x)
{
	x = ACEScc_to_lin(x);
	return x;
}

//=================================================================================================
//Various RRT approximations
//
float3 RRTAndODTFit(float3 x)
{
	x *= 1.8;
	float3 a = x * (x + 0.0245786) - 0.000090537;
	float3 b = x * (0.983729 * x + 0.4329510) + 0.238081;
	return (a / b);
}

float3 ACESFilm(float3 x)
{
	x *= 0.7;
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return ((x*(a*x+b))/(x*(c*x+d)+e));
}

float3 ACESUnity(float3 x)
{
	x *= 1.4;
	// Luminance fitting of *RRT.a1.0.3 + ODT.Academy.RGBmonitor_100nits_dim.a1.0.3*.
    // https://github.com/colour-science/colour-unity/blob/master/Assets/Colour/Notebooks/CIECAM02_Unity.ipynb
    // RMSE: 0.0012846272106
    const float a = 278.5085;
    const float b = 10.7772;
    const float c = 293.6045;
    const float d = 88.7122;
    const float e = 80.6889;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

//=================================================================================================
//Display tranformations
//
float3 ACES_IDT(float3 color)
{
	float3 aces = SRGBToLinear(color);
	aces = sRGB_to_ACES(aces);
	return aces;
}

float3 XYZ_2_xyY(float3 XYZ)
{
    float divisor = max(dot(XYZ, (1.0).xxx), 1e-4);
    return float3(XYZ.xy / divisor, XYZ.y);
}

float3 xyY_2_XYZ(float3 xyY)
{
    float m = xyY.z / max(xyY.y, 1e-4);
    float3 XYZ = float3(xyY.xz, (1.0 - xyY.x - xyY.y));
    XYZ.xz *= m;
    return XYZ;
}

float3 darkSurround_to_dimSurround(float3 linearCV)
{
    float3 XYZ = mul(AP1_2_XYZ_MAT, linearCV);

    float3 xyY = XYZ_2_xyY(XYZ);
    xyY.z = clamp(xyY.z, 0.0, HALF_MAX);
    xyY.z = pow(xyY.z, DIM_SURROUND_GAMMA);
    XYZ = xyY_2_XYZ(xyY);

    return mul(XYZ_2_AP1_MAT, XYZ);
}

float3 ACES_ODT(float3 oces)
{
	 // Apply gamma adjustment to compensate for dim surround
    oces = darkSurround_to_dimSurround(oces);
    
    //martix for ODT
	//oces = mul(ODT_SAT_MAT, oces);
    oces = lerp(dot(oces, AP1_RGB2Y).xxx, oces, ODT_SAT_FACTOR.xxx);
	
	oces = ACEScg_to_sRGB(oces);
	
	oces = LinearTosRGB(oces);
	return saturate(oces);
}

//=================================================================================================
//Reference Render Transform and Output Display Transform
//features glow module

float rgb_2_saturation(float3 x)
{
    const float TINY = 1e-4;
    float mi = min(min(x.r, x.g), x.b);
    float ma = max(max(x.r, x.g), x.b);
    return (max(ma, TINY) - max(mi, TINY)) / max(ma, 1e-2);
}

float rgb_2_yc(float3 x)
{
    const float ycRadiusWeight = 1.75;

    // Converts RGB to a luminance proxy, here called YC
    // YC is ~ Y + K * Chroma
    // Constant YC is a cone-shaped surface in RGB space, with the tip on the
    // neutral axis, towards white.
    // YC is normalized: RGB 1 1 1 maps to YC = 1
    //
    // ycRadiusWeight defaults to 1.75, although can be overridden in function
    // call to rgb_2_yc
    // ycRadiusWeight = 1 -> YC for pure cyan, magenta, yellow == YC for neutral
    // of same value
    // ycRadiusWeight = 2 -> YC for pure red, green, blue  == YC for  neutral of
    // same value.

    float r = x.x;
    float g = x.y;
    float b = x.z;
    float chroma = sqrt(b * (b - g) + g * (g - r) + r * (r - b));
    return (b + g + r + ycRadiusWeight * chroma) / 3.0;
}

float sigmoid_shaper(float x)
{
    // Sigmoid function in the range 0 to 1 spanning -2 to +2.

    float t = max(1.0 - abs(x / 2.0), 0.0);
    float y = 1.0 + sign(x) * (1.0 - t * t);

    return y / 2.0;
}

float glow_fwd(float ycIn, float glowGainIn, float glowMid)
{
    float glowGainOut;

    if (ycIn <= 2.0 / 3.0 * glowMid)
        glowGainOut = glowGainIn;
    else if (ycIn >= 2.0 * glowMid)
        glowGainOut = 0.0;
    else
        glowGainOut = glowGainIn * (glowMid / ycIn - 1.0 / 2.0);

    return glowGainOut;
}

float rgb_2_hue(float3 x)
{
    // Returns a geometric hue angle in degrees (0-360) based on RGB values.
    // For neutral colors, hue is undefined and the function will return a quiet NaN value.
    float hue;
    if (x.x == x.y && x.y == x.z)
        hue = 0.0; // RGB triplets where RGB are equal have an undefined hue
    else
        hue = (180.0 / PI) * atan2(sqrt(3.0) * (x.y - x.z), 2.0 * x.x - x.y - x.z);

    if (hue < 0.0) hue = hue + 360.0;

    return hue;
}

float center_hue(float hue, float centerH)
{
    float hueCentered = hue - centerH;
    if (hueCentered < -180.0) hueCentered = hueCentered + 360.0;
    else if (hueCentered > 180.0) hueCentered = hueCentered - 360.0;
    return hueCentered;
}

float3 ACES_RRT(float3 aces)
{
	
	// --- Glow module --- //
    float saturation = rgb_2_saturation(aces);
    float ycIn = rgb_2_yc(aces);
    float s = sigmoid_shaper((saturation - 0.4) / 0.2);
    float addedGlow = 1.0 + glow_fwd(ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);
    //addedGlow = (addedGlow > 1) ? 100 : 1; //check glow region
    aces *= addedGlow;
	
    // --- Red modifier --- //
    float hue = rgb_2_hue(aces);
    float centeredHue = center_hue(hue, RRT_RED_HUE);
    float hueWeight;
    {
        //hueWeight = cubic_basis_shaper(centeredHue, RRT_RED_WIDTH);
        hueWeight = smoothstep(0.0, 1.0, 1.0 - abs(2.0 * centeredHue / RRT_RED_WIDTH));
        hueWeight *= hueWeight;
    }
    aces.r += hueWeight * saturation * (RRT_RED_PIVOT - aces.r) * (1.0 - RRT_RED_SCALE);
	

	// --- ACES to RGB rendering space --- //
	aces = clamp(aces, 0.0, HALF_MAX);
	aces = ACES_to_ACEScg(aces);
	
	//clamp to float range and within gamut
	float3 rgbPre = clamp(aces, 0.0, HALF_MAX);
	
	// --- Global desaturation --- //
	//rgbPre = mul(RRT_SAT_MAT, rgbPre);
    rgbPre = lerp(dot(rgbPre, AP1_RGB2Y).xxx, rgbPre, RRT_SAT_FACTOR.xxx);

	float3 rgbPost;
	
	//various ACES RRT approximations
	
	//Stephen Hill - clips blacks?
	//rgbPost = RRTAndODTFit(rgbPre);
	
	//Krzysztof Narkowicz
	//rgbPost = ACESFilm(rgbPre);
	
	//Unity
	rgbPost = ACESUnity(rgbPre);
	
	float3 oces = rgbPost;
	
	return oces;
}

//=================================================================================================
//Look Modification Transform
//global modifiers to the image before tonemapping
//blue highlight fix is included, however it is for AP0, so using it requires 3 matrix tranforms
//
//load custom behaviours from another file
#include "ACES_LMT.h"

//=================================================================================================
//ACES implementation
//ACEScg is the main color space, similar to the implementation in UE4
//
float3 ACES(float3 color)
{
	//Input Display Transform into ACEScg (linear gamma space with AP1 primaries)
	float3 aces = ACES_IDT(color);

	//Look Modification Transform (preset color modifications)
	ACES_LMT(aces);

	//Reference Render Transform (tonemapping including the global desaturation but without the 'glow module')
	float3 oces = ACES_RRT(aces);
	
	//Output Display Tranform into sRGB (linear gamma space with sRGB primaries)
	oces = ACES_ODT(oces);

	return oces;
}