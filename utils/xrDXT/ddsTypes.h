#pragma once
typedef enum RescaleTypes
{
    RESCALE_NONE,               // no rescale
    RESCALE_NEAREST_POWER2,     // rescale to nearest power of two
    RESCALE_BIGGEST_POWER2,   // rescale to next bigger power of 2
    RESCALE_SMALLEST_POWER2,  // rescale to smaller power of 2 
    RESCALE_NEXT_SMALLEST_POWER2,  // rescale to next smaller power of 2
    RESCALE_PRESCALE,           // rescale to this size
    RESCALE_RELSCALE,           // relative rescale
    RESCALE_LAST,              //


} RescaleTypes;


typedef enum SharpenFilterTypes
{
    kSharpenFilterNone,
    kSharpenFilterNegative,
    kSharpenFilterLighter,
    kSharpenFilterDarker,
    kSharpenFilterContrastMore,
    kSharpenFilterContrastLess,
    kSharpenFilterSmoothen,
    kSharpenFilterSharpenSoft,
    kSharpenFilterSharpenMedium,
    kSharpenFilterSharpenStrong,
    kSharpenFilterFindEdges,
    kSharpenFilterContour,
    kSharpenFilterEdgeDetect,
    kSharpenFilterEdgeDetectSoft,
    kSharpenFilterEmboss,
    kSharpenFilterMeanRemoval,
    kSharpenFilterUnSharp,
    kSharpenFilterXSharpen,
    kSharpenFilterWarpSharp,
    kSharpenFilterCustom,
    kSharpenFilterLast,
};



typedef enum MIPFilterTypes
{
    kMIPFilterPoint ,    
    kMIPFilterBox ,      
    kMIPFilterTriangle , 
    kMIPFilterQuadratic ,
    kMIPFilterCubic ,    

    kMIPFilterCatrom ,   
    kMIPFilterMitchell , 

    kMIPFilterGaussian , 
    kMIPFilterSinc ,     
    kMIPFilterBessel ,   

    kMIPFilterHanning ,  
    kMIPFilterHamming ,  
    kMIPFilterBlackman , 
    kMIPFilterKaiser,
    kMIPFilterLast,
};


enum TextureFormats
{
    kDXT1 ,
    kDXT1a ,  // DXT1 with one bit alpha
    kDXT3 ,   // explicit alpha
    kDXT5 ,   // interpolated alpha
    k4444 ,   // a4 r4 g4 b4
    k1555 ,   // a1 r5 g5 b5
    k565 ,    // a0 r5 g6 b5
    k8888 ,   // a8 r8 g8 b8
    k888 ,    // a0 r8 g8 b8
    k555 ,    // a0 r5 g5 b5
    k8   ,   // paletted
    kV8U8 ,   // DuDv 
    kCxV8U8 ,   // normal map
    kA8 ,            // alpha only
    k4  ,            // 16 bit color      
    kQ8W8V8U8,
    kA8L8,
    kR32F,
    kA32B32G32R32F,
    kA16B16G16R16F,
    kL8,       // luminance
    kTextureFormatLast
};


enum TextureTypes
{
    kTextureType2D,
    kTextureTypeCube,
    kTextureTypeVolume,  
    kTextureTypeLast,  
};
      /*
enum NormalMapTypes
{
    kColorTextureMap,
    kTangentSpaceNormalMap,
    kObjectSpaceNormalMap,
};
        */


enum CompressionWeighting
{
    kLuminanceWeighting,
    kGreyScaleWeighting,
    kTangentSpaceNormalMapWeighting,
    kObjectSpaceNormalMapWeighting,
    kUserDefinedWeighting, // used values stored in 'weight'
};
    