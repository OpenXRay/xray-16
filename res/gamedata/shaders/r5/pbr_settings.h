/**
 * @ Description: Enhanced Shaders and Color Grading 1.10
 * @ Author: https://www.moddb.com/members/kennshade
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/enhanced-shaders-and-color-grading-for-151
 */
 
//=================================================================================================
//Settings for PBR conversion
//=================================================================================================
#define USE_BURLEY_DIFFUSE //use expensive Disney/Burley diffuse
#define USE_GGX_SPECULAR //use more expensive GGX specular
//=================================================================================================
#define ALBEDO_BOOST 0.0
#define ALBEDO_AMOUNT 1.00

#define ROUGHNESS_LOW 0.5
#define ROUGHNESS_HIGH 1.0
#define ROUGHNESS_POW 1.0

#define SPECULAR_BASE 0.01
#define SPECULAR_RANGE 1.0
#define SPECULAR_POW 1.0

#define METAL_BOOST 0.25
#define METALNESS_THRESHOLD 0.125
#define METALNESS_SOFTNESS 0.125