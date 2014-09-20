/****************************************************************************************
	
    Copyright (C) NVIDIA Corporation 2003

    TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
    *AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
    OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
    BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
    WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
    BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
    ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
    BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

*****************************************************************************************/
#pragma once

#include <windows.h>
#include "tPixel.h"

typedef HRESULT (__cdecl *MIPcallback)(void * data, int miplevel, DWORD size, int width, int height, void * user_data);
                     

inline char * GetDXTCVersion() { return "Version 7.33";}


enum NVDXT_OPTIONS
{

    dBackgroundNameStatic = 3,
    dProfileDirNameStatic = 4,
    dSaveButton = 5,
    dProfileNameStatic = 6,



    dGenerateMipMaps = 30,
    dMIPMapSourceFirst = dGenerateMipMaps,
    dUseExistingMipMaps = 31,
    dNoMipMaps = 32,
    dMIPMapSourceLast = dNoMipMaps,

    dSpecifiedMipMapsCombo = 39,



    dbShowDifferences = 40,
    dbShowFiltering = 41,
    dbShowMipMapping = 42,
    dbShowAnisotropic = 43,

    dChangeClearColorButton = 50,
    dDitherColor = 53,

    dLoadBackgroundImageButton = 54,
    dUseBackgroundImage = 55,

    dbBinaryAlpha = 56,
    dAlphaBlending = 57,

    dFadeColor = 58,  //.
    dFadeAlpha = 59,

    dFadeToColorButton = 60,
    dAlphaBorder = 61,
    dBorder = 62,
    dBorderColorButton = 63,

    dNormalMapCombo = 64,

    dDitherMIP0 = 66,
    dGreyScale = 67,
    dQuickCompress = 68,
    dLightingEnable = 69,

    dbPreviewDisableMIPmaps = 71,




    dZoom = 79,


    dTextureTypeCombo = 80,

    dFadeAmount = 90,
    dFadeToAlpha = 91,
    dFadeToDelay = 92,

    dBinaryAlphaThreshold = 94,

    dFilterGamma = 100,
    dFilterBlur = 101,
    dFilterWidth = 102,
    dbOverrideFilterWidth = 103,
    dLoadProfile = 104,
    dSaveProfile = 105,
    dSharpenMethodCombo = 106,
    dProfileCombo = 107,
    dSelectProfileDirectory = 108,
    dbEnableGammaCorrection = 109,
    dbAlphaModulate = 110,
    dbDXT5NormalMap = 111,





    dViewDXT1 = 200,
    dViewDXT2 = 201,
    dViewDXT3 = 202,
    dViewDXT5 = 203,
    dViewA4R4G4B4 = 204,
    dViewA1R5G5B5 = 205,
    dViewR5G6B5 = 206,
    dViewA8R8G8B8 = 207,


    // 3d viewing options
    d3DPreviewButton = 300, 
    d2DPreviewButton = 301, 
    dPreviewRefresh = 302, 




    dAskToLoadMIPMaps = 400,
    dShowAlphaWarning = 401,
    dShowPower2Warning = 402,
    dTextureFormatBasedOnAlpha = 403,
    dSystemMessage = 404,
    dHidePreviewButtons = 405,
    dCalculateLuminance = 406,

    dSpecifyAlphaBlendingButton = 500,
    dUserSpecifiedFadingAmounts = 501,
    dSharpenSettingsButton = 502,
    dFilterSettingsButton = 503,
    dNormalMapGenerationSettingsButton = 504,
    dConfigSettingsButton = 505,
    dFadingDialogButton = 506,
    dPreviewDialogButton = 507,
    dResetDefaultsButton = 508,
    dImageDialogButton = 510,



    dSaveTextureFormatCombo = 600,
    dMIPFilterCombo = 601,


    ///////////  Normal Map


    dScaleEditText = 1003,
    dProxyItem = 1005,
    dMinZEditText = 1008,



    dALPHA = 1009,
    dFirstCnvRadio = dALPHA,
    dAVERAGE_RGB = 1010,
    dBIASED_RGB = 1011,
    dRED = 1012,
    dGREEN = 1013,
    dBLUE = 1014,
    dMAX = 1015,
    dCOLORSPACE = 1016,
    dNORMALIZE = 1017,
    dConvertToHeightMap = 1018,
    dLastCnvRadio = dConvertToHeightMap,

    dbAddHeightMap = 1019,

    d3DPreview = 1021,      
    dDecalTexture = 1022,
    dbUseDecalTexture = 1023,
    dbBrighten = 1024,
    dbAnimateLight = 1025,
    dStaticDecalName = 1026,
    dbSignedOutput = 1027,
    dbNormalMapSwapRGB = 1028,


    dbWrap = 1030,
    dbMultipleLayers = 1031,
    db_16_16 = 1032,

    dAlphaNone = 1033,
    dFirstAlphaRadio = dAlphaNone,
    dAlphaHeight = 1034,
    dAlphaClear = 1035,
    dAlphaWhite = 1036,
    dLastAlphaRadio = dAlphaWhite,

    dbInvertY = 1037,
    db_12_12_8 = 1038,
    dbInvertX = 1039,

    dFilter4x = 1040,
    dFirstFilterRadio = dFilter4x,
    dFilter3x3 = 1041,
    dFilter5x5 = 1042,
    dFilterDuDv = 1043,
    dFilter7x7 = 1044,
    dFilter9x9 = 1045,
    dFilterQ8W8V8U8 = 1046,
    dLastFilterRadio = dFilterQ8W8V8U8,


    dbEnableNormalMapConversion = 1050,
    dbErrorDiffusion = 1051,


    dCustomFilterDataFirst = 2000,
    // 5x5  Filter 0- 24
    dCustomFilterDataLast = 2024,  

    dCustomDiv = 2025,
    dCustomBias = 2026,

    dUnSharpRadius = 2027,
    dUnSharpAmount = 2028,
    dUnSharpThreshold = 2029,

    dXSharpenStrength = 2030,
    dXSharpenThreshold = 2031,



    dSharpenTimesMIP0 = 2100,
    dSharpenTimesFirst = dSharpenTimesMIP0,

    dSharpenTimesMIP1 = 2101,
    dSharpenTimesMIP2 = 2102,
    dSharpenTimesMIP3 = 2103,
    dSharpenTimesMIP4 = 2104,
    dSharpenTimesMIP5 = 2105,
    dSharpenTimesMIP6 = 2106,
    dSharpenTimesMIP7 = 2107,
    dSharpenTimesMIP8 = 2108,
    dSharpenTimesMIP9 = 2109,
    dSharpenTimesMIP10 = 2110,
    dSharpenTimesMIP11 = 2111,
    dSharpenTimesMIP12 = 2112,
    dSharpenTimesMIP13 = 2113,
    dSharpenTimesLast = dSharpenTimesMIP13,

    dDeriveDiv = 2200,   // balance
    dDeriveBias = 2201,



};

#include "ddsTypes.h"




// Windows handle for our plug-in (seen as a dynamically linked library):
extern HANDLE hDllInstance;




#define SHARP_TIMES_ENTRIES 14

typedef unsigned char Boolean;  // for photoshop scripting




typedef struct CNormalMap
{
public:
        
    CNormalMap()
    {
        bEnableNormalMapConversion = false;

        minz = 0;
        scale = 1;
        filterKernel = dFilter3x3;
         
        heightConversionMethod = dAVERAGE_RGB;
        alphaResult = dAlphaNone;

        bWrap = false;
        bInvertX = false;
        bInvertY = false;
        bSignedOutput = true;
        bAddHeightMap = false;
        bNormalMapSwapRGB = false;


    }
    Boolean bEnableNormalMapConversion; // do not convert to a normal map
    int minz;       // minimum value the z value can attain in the x y z  normal
                    // keeps normal point "upwards"
    float scale;    // height multiplier

    //dFilter4x = 1040,
    //dFilter3x3 = 1041,
    //dFilter5x5 = 1042,
    //dFilterDuDv = 1043,
    //dFilter7x7 = 1044,
    //dFilter9x9 = 1045,

    int filterKernel;  // kernel used to create normal maps.  Done this way to be compatible with plugins


    //dALPHA = 1009,
    //dAVERAGE_RGB = 1010,
    //dBIASED_RGB = 1011,
    //dRED = 1012,
    //dGREEN = 1013,
    //dBLUE = 1014,
    //dMAX = 1015,
    //dCOLORSPACE = 1016,
    //dNORMALIZE = 1017,
    int heightConversionMethod;  // method to convert color to height

    //dAlphaNone = 1033,
    //dAlphaHeight = 1034,
    //dAlphaClear = 1035,
    //dAlphaWhite = 1036,
    int alphaResult;     // what to do with the alpha channel when done

    Boolean bWrap;
    Boolean bInvertX;       // flip the x direction
    Boolean bInvertY;       // flip the y direction
    Boolean bSignedOutput;  // output is -128 to 127
    Boolean bAddHeightMap;
    Boolean bNormalMapSwapRGB; // swap color channels
} CNormalMap;

typedef struct CompressionOptions
{
    CompressionOptions()
    {
        rescaleImageType = RESCALE_NONE; 
        rescaleImageFilter = kMIPFilterCubic; 
        scaleX = 1;
        scaleY = 1;
        bClamp = false;
        clampX = 4096;
        clampY = 4096;

        bClampScale = false;
        clampScaleX = 4096;
        clampScaleY = 4096;

        MipMapType = dGenerateMipMaps;         // dNoMipMaps, dUseExistingMipMaps, dGenerateMipMaps
        SpecifiedMipMaps = 0;   // if dSpecifyMipMaps or dUseExistingMipMaps is set (number of mipmaps to generate)

        MIPFilterType = kMIPFilterTriangle;      // for MIP maps
        
        bBinaryAlpha = false;       // zero or one alpha channel




        bRGBE = false;

        bAlphaBorder = false;       // make an alpha border
        bAlphaBorderLeft = false;    
        bAlphaBorderRight = false;   

        bAlphaBorderTop = false;     
        bAlphaBorderBottom = false;  




        bBorder = false;            // make a color border
        BorderColor.u = 0;        // color of border


        bFadeColor = false;         // fade color over MIP maps
        bFadeAlpha = false;         // fade alpha over MIP maps

        FadeToColor.u = 0;        // color to fade to
        FadeToAlpha = 0;        // alpha value to fade to (0-255)

        FadeToDelay = 0;        // start fading after 'n' MIP maps

        FadeAmount = 0;         // percentage of color to fade in

        BinaryAlphaThreshold = 0;  //. 128 When Binary Alpha is selected, below this value, alpha is zero


        bDitherColor = false;       // enable dithering during 16 bit conversion
        bDitherMIP0 = false;// enable dithering during 16 bit conversion for each MIP level (after filtering)

        bQuickCompress = false;         // Fast compression scheme
        bForceDXT1FourColors = false;  // do not let DXT1 use 3 color representation


        SharpenFilterType = kSharpenFilterNone;
        bErrorDiffusion = false;

        weightType = kLuminanceWeighting;     
        bNormalizeTexels = false;

        weight[0] = 0.3086f; // luminance conversion values   
        weight[1] = 0.6094f;    
        weight[2] = 0.0820f;    

        // gamma value for all filters
        bEnableFilterGamma = false;
        FilterGamma = 2.2f;

        // alpha value for 
        FilterBlur = 1.0f;
        // width of filter
        FilterWidth = 10.0f;
        bOverrideFilterWidth = false;

        TextureType = kTextureType2D;        // regular decal, cube or volume  
        TextureFormat = kDXT1;	             // 

        bSwapRGB = false;           // swap color positions R and G
        user_data = NULL;            // user supplied point passed down to write functions

        int i,j;

        float default_filter[5][5] = 
        { 
            0, 0, 0, 0, 0,
                0,0,-2,0,0,
                0,-2,11,-2,0,
                0, 0,-2, 0,0,
                0, 0, 0, 0, 0,  
        };


        for(i = 0; i<5; i++)
            for(j = 0; j<5; j++)
                custom_filter_data.filter[i][j] = default_filter[i][j];


        custom_filter_data.div = 3;   // div
        custom_filter_data.bias = 0;  // bias

        unsharp_data.radius = 5.0; // radius
        unsharp_data.amount = 0.5;  // amount
        unsharp_data.threshold = 0;     // threshold

        xsharp_data.strength  = 255; // xsharp strength
        xsharp_data.threshold  = 255; // xsharp threshold


        sharpening_passes_per_mip_level[0] = 0;

        for(i = 1; i<SHARP_TIMES_ENTRIES; i++)
            sharpening_passes_per_mip_level[i] = 1;
        for(i = 0; i<SHARP_TIMES_ENTRIES; i++)
            sharpening_passes_per_mip_level[i] = 0;

        bAlphaModulate = false;
        bDXT5NormalMap = false;
    }

    
    CompressionWeighting  weightType;   // weighting type for DXT compressop
    float weight[3];                    // weights used for compress



    CNormalMap normalMap;               // filled when processing normal maps
    Boolean bNormalizeTexels;


    Boolean         bClamp;             // Clamp to max size     
    float           clampX;             // clamping values
    float           clampY;

    Boolean         bClampScale;      // maximum value of h or w (retain scale)
    float           clampScaleX;             // clamping values
    float           clampScaleY;
                                        

    RescaleTypes    rescaleImageType;     // rescaling type before image before compression
    MIPFilterTypes  rescaleImageFilter;   // rescaling filter

    float           scaleX;             // scale to this if we are prescaling images before compressing
    float           scaleY;

    long            MipMapType;         // dNoMipMaps, dSpecifyMipMaps, dUseExistingMipMaps, dGenerateMipMaps

    long            SpecifiedMipMaps;   // if dSpecifyMipMaps or dUseExistingMipMaps is set (number of mipmaps to generate)

    MIPFilterTypes  MIPFilterType;      // for MIP map, select from MIPFilterTypes

    Boolean         bBinaryAlpha;       // zero or one alpha channel


    Boolean         bRGBE;                  // convert to RGBE
    Boolean         bAlphaBorder;           // make an alpha border
    Boolean         bAlphaBorderLeft;       // make an alpha border on just the left
    Boolean         bAlphaBorderRight;      // make an alpha border on justthe right

    Boolean         bAlphaBorderTop;        // make an alpha border on just the top
    Boolean         bAlphaBorderBottom;      // make an alpha 


    Boolean         bBorder;            // make a color border
    rgba_t          BorderColor;        // color of border


    Boolean         bFadeColor;         // fade color over MIP maps
    Boolean         bFadeAlpha;         // fade alpha over MIP maps

    rgba_t          FadeToColor;        // color to fade to
    long            FadeToAlpha;        // alpha value to fade to (0-255)

    long            FadeToDelay;        // start fading after 'n' MIP maps

    long            FadeAmount;         // percentage of color to fade in

    long            BinaryAlphaThreshold;  // When Binary Alpha is selected, below this value, alpha is zero

    // dithering is currently disabled
    Boolean         bDitherColor;       // enable dithering during 16 bit conversion
    Boolean         bDitherMIP0;        // enable dithering during 16 bit conversion for each MIP level (after filtering)

    Boolean         bQuickCompress;         // Fast compression scheme
    Boolean         bForceDXT1FourColors;  // do not let DXT1 use 3 color representation


    // sharpening after creating each MIP map level

    // used when custom sharping filter is used
    // 5x5 filter 
    struct 
    {
        float filter[5][5];
        float div;
        float bias;

    }  custom_filter_data; 



    // used when unsharpen sharping filter is used
    struct  
    {
        float radius; // radius
        float amount;  // amount
        float threshold;     // threshold

    } unsharp_data; 
    
    // used when xsharpen sharping filter is used
    struct 
    {
        // 0 - 255, stored as float
        float strength;
        float threshold;
    } xsharp_data;


    int sharpening_passes_per_mip_level[SHARP_TIMES_ENTRIES]; 

    SharpenFilterTypes  SharpenFilterType; // post filtering image sharpening

    Boolean bErrorDiffusion;        // diffuse error, used for helping gradient images

    // convert to gamma space before filtering
    Boolean bEnableFilterGamma;
    float FilterGamma;               // gamma value for filtering (MIP map generation)


    float FilterBlur;               // sharpness or blurriness of filtering
    Boolean bOverrideFilterWidth; // use the specified width in FilterWidth,instead of the default
    float FilterWidth;     // override fiter width with this value

	TextureTypes 		TextureType;        // what type of texture is this?
	TextureFormats 		TextureFormat;	    // format to convert to

    Boolean   bSwapRGB;             // swap color positions R and G
    Boolean   bDXT5NormalMap;             // 

    bool bAlphaModulate;            // modulate color by alpha for filtering


    void * user_data;               // user supplied values passed down to write functions

} CompressionOptions;

