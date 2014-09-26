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
#include <dds/tPixel.h>
#include <dds/ddsTypes.h>
                    
inline char* GetDXTCVersion() { return "Version 8.30"; }

// max mip maps
#define MAX_MIP_MAPS 17

typedef unsigned char nvBoolean; // for photoshop scripting

typedef struct nvNormalMap
{
public:
    // do not convert to a normal map
    nvBoolean bEnableNormalMapConversion;
    // minimum value the z value can attain in the x y z  normal keeps normal point "upwards"
    int minz;
    // height multiplier
    float scale;
    // kernel used to create normal maps. Done this way to be compatible with plugins
    nvNormalMapFilters filterKernel;
    // method to convert color to height
    nvHeightConversionMethods heightConversionMethod;
    nvAlphaResult alphaResult; // what to do with the alpha channel when done
    nvBoolean bWrap;
    nvBoolean bInvertX; // flip the x direction
    nvBoolean bInvertY; // flip the y direction
    nvBoolean bInvertZ; // flip the z direction
    nvBoolean bAddHeightMap;
    nvBoolean bNormalMapSwapRGB; // swap color channels

    nvNormalMap()
    {
        bEnableNormalMapConversion = false;
        minz = 0;
        scale = 1;
        filterKernel = kFilter3x3;
        heightConversionMethod = kAverageRGB;
        alphaResult = kAlphaUnchanged;
        bWrap = false;
        bInvertX = false;
        bInvertY = false;
        bInvertZ = false;
        bAddHeightMap = false;
        bNormalMapSwapRGB = false;
    }
} nvNormalMap;

class nvCompressionOptions
{
public:
    nvCompressionOptions()
    {
        SetDefaultOptions();
    }

    void SetDefaultOptions()
    {
        quality = kQualityProduction;
        // in kQualityHighest mode, rms error above which will cause a long search for a better answer
        rmsErrorSearchThreshold = 400;
        rescaleImageType = kRescaleNone; 
        rescaleImageFilter = kMipFilterCubic; 
        scaleX = 1;
        scaleY = 1;
        bClamp = false;
        clampX = 4096;
        clampY = 4096;
        bClampScale = false;
        clampScaleX = 4096;
        clampScaleY = 4096;
        mipMapGeneration = kGenerateMipMaps; // dNoMipMaps, dUseExistingMipMaps, dGenerateMipMaps
        numMipMapsToWrite = 0; // (number of mipmaps to write out)
        mipFilterType = kMipFilterTriangle; // for MIP maps        
        bBinaryAlpha = false; // zero or one alpha channel
        bRGBE = false;
        bAlphaBorder = false; // make an alpha border
        bAlphaBorderLeft = false;
        bAlphaBorderRight = false;
        bAlphaBorderTop = false;
        bAlphaBorderBottom = false;        
        bBorder = false; // make a color border
        borderColor32F.r = 0.0f; // color of border
        borderColor32F.g = 0.0f; // color of border
        borderColor32F.b = 0.0f; // color of border
        borderColor32F.a = 0.0f; // alpha of border
        bFadeColor = false; // fade color over MIP maps
        bFadeAlpha = false; // fade alpha over MIP maps
        fadeToColor32F.r = 0.0f; // color to fade to
        fadeToColor32F.g = 0.0f; // color to fade to
        fadeToColor32F.b = 0.0f; // color to fade to
        fadeToColor32F.a = 0.0f; // alpha to fade to
        fadeToDelay = 0; // start fading after 'n' MIP maps
        fadeAmount32F = 0.15f; // percentage of color to fade in %15
        alphaThreshold32F = 0.5; // When Binary Alpha is selected, below this value, alpha is zero
        bDitherColor = false; // enable dithering during 16 bit conversion
        bDitherMip0 = false; // enable dithering during 16 bit conversion for each MIP level (after filtering)
        bForceDXT1FourColors = false; // do not let DXT1 use 3 color representation
        sharpenFilterType = kSharpenFilterNone;
        bErrorDiffusion = false;
        errorDiffusionWidth = 1;
        weightType = kLuminanceWeighting;
        bNormalizeTexels = false;
        weight[0] = 0.3086f; // luminance conversion values   
        weight[1] = 0.6094f;
        weight[2] = 0.0820f;
        // gamma value for all filters
        bEnableFilterGamma = false;
        filterGamma = 2.2f;
        // alpha value for
        filterBlur = 1.0f;
        // width of filter
        filterWidth = 10.0f;
        bOverrideFilterWidth = false;
        textureType = kTextureTypeTexture2D; // regular decal, cube or volume
        textureFormat = kDXT1;
        bSwapRG = false; // swap color positions R and G
		bSwapRB = false; // swap color positions R and G
        user_data = NULL; // user supplied point passed down to write functions
        float default_filter[5][5] = 
        { 
            0, 0,  0, 0, 0,
            0, 0, -2, 0, 0,
            0,-2, 11,-2, 0,
            0, 0, -2, 0, 0,
            0, 0,  0, 0, 0,  
        };        
        for (int i = 0; i < 5; i++)
        {    
            for (int j = 0; j < 5; j++)
            {
                custom_filter_data.filter[i][j] = default_filter[i][j];
            }
        }
        custom_filter_data.div = 3; // div
        custom_filter_data.bias = 0; // bias
        unsharp_data.radius32F = 5.0; // radius
        unsharp_data.amount32F = 0.5; // amount
        unsharp_data.threshold32F = 0; // threshold
        xsharp_data.strength32F  = 1.0f; // xsharp strength
        xsharp_data.threshold32F  = 1.0f; // xsharp threshold
        sharpening_passes_per_mip_level[0] = 0;
        for (int i = 1; i < MAX_MIP_MAPS; i++)
            sharpening_passes_per_mip_level[i] = 1;
        bAlphaFilterModulate = false;
        bPreModulateColorWithAlpha = false;
        bUserSpecifiedFadingAmounts = false;
        for (int i = 0; i < 256; i++)
        {
            color_palette[i].r = 0;
            color_palette[i].g = 0;
            color_palette[i].b = 0;
            color_palette[i].a = 0;
        }
        paletteSize = 0; // this will be set by the format read or write
        autoGeneratePalette = false;
        outputScale.r = 1.0f; // scale and bias when writing output values
        outputScale.g = 1.0f; // scale and bias when writing output values
        outputScale.b = 1.0f; // scale and bias when writing output values
        outputScale.a = 1.0f; // scale and bias when writing output values
        outputBias.r = 0.0f;
        outputBias.g = 0.0f;
        outputBias.b = 0.0f;
        outputBias.a = 0.0f;
        inputScale.r = 1.0f; // scale and bias after loading data
        inputScale.g = 1.0f; // scale and bias
        inputScale.b = 1.0f; // scale and bias
        inputScale.a = 1.0f; // scale and bias
        inputBias.r = 0.0f;
        inputBias.g = 0.0f;
        inputBias.b = 0.0f;
        inputBias.a = 0.0f;
        bConvertToGreyScale = false;
        greyScaleWeight.r = 0.3086f; // scale and bias after loading data
        greyScaleWeight.g = 0.6094f; // scale and bias 
        greyScaleWeight.b = 0.0820f; // scale and bias 
        greyScaleWeight.a = 0.0f; // scale and bias
        brightness.r = 0.0; // adjust brightness = 0 none
        brightness.g = 0.0; // 
        brightness.b = 0.0; // 
        brightness.a = 0.0; // 
        contrast.r = 1; // contrast 1 == none
        contrast.g = 1;
        contrast.b = 1;
        contrast.a = 1;
        bCalcLuminance = false; // do not convert to luminance by default
        bOutputWrap = false; // wrap the values when outputting to the desired format
        bCreateOnePalette = false;
    }

    /////////////////////////// COMPRESSION QUALITY //////////////////////////////////
    nvQualitySetting quality;
    float rmsErrorSearchThreshold;
    
    void SetQuality(nvQualitySetting setting,  float threshold)
    {
        quality = setting;
        // if setting == kQualityHighest, if the RMS error for a 4x4 block is bigger than
        // this, an extended search is performed.  In practice this has been equivalent to and 
        // exhaustive search in the entire domain. aka it doesn't get any better than this.
        rmsErrorSearchThreshold = threshold;
    }

    /////////////////////////////// COMPRESSION  WEIGHTING //////////////////////////////
    nvCompressionWeighting weightType; // weighting type for DXT compressop
    float weight[3]; // weights used for compress

    void SetCompressionWeighting(nvCompressionWeighting type, float new_weight[3])
    {
        weightType = type;
        // if type == kUserDefinedWeighting, then use these weights
        weight[0] = new_weight[0];
        weight[1] = new_weight[1];
        weight[2] = new_weight[2];        
    }

    nvNormalMap normalMap; // filled when processing normal maps    
    nvBoolean bNormalizeTexels; // normalize the texels

    ///////////////////////////  SCALING IMAGE /////////////////////////////////
    nvRescaleTypes rescaleImageType; // rescaling type before image before compression
    nvMipFilterTypes rescaleImageFilter; // rescaling filter
    float scaleX; // scale to this if we are prescaling images before compressing
    float scaleY;
    
    // scale the image to this size first
    void PreScaleImage(float x, float y, nvMipFilterTypes filter)
    {
        rescaleImageType = kRescalePreScale;
        scaleX = x;
        scaleY = y;
        rescaleImageFilter = filter;
    }

    // relative scaling.  0.5 is half the image size
    void RelativeScaling(float x, float y, nvMipFilterTypes filter)
    {
        rescaleImageType = kRescaleRelScale;
        scaleX = x;
        scaleY = y;
        rescaleImageFilter = filter;
    }

    void RescaleToNearestPOW2(nvMipFilterTypes filter)
    {
        rescaleImageType = kRescaleNearestPower2;
        rescaleImageFilter = filter;
    }

    void RescaleToNearestBiggestPOW2(nvMipFilterTypes filter)
    {
        rescaleImageType = kRescaleBiggestPower2;
        rescaleImageFilter = filter;
    }

    void RescaleToNearestSmallestPOW2(nvMipFilterTypes filter)
    {
        rescaleImageFilter = filter;
        rescaleImageType = kRescaleSmallestPower2;
    }

    void RescaleToNearestNextSmallestPOW2(nvMipFilterTypes filter)
    {
        rescaleImageType = kRescaleNextSmallestPower2;
        rescaleImageFilter = filter;
    }

    ///////////////////   CLAMPING IMAGE SIZE ///////////////////////////////////
    nvBoolean bClamp; // Clamp to max size     
    float clampX; // clamping values
    float clampY;

    // image no bigger than...
    void ClampMaxImageSize(float maxX, float maxY)
    {    
        bClamp = true;
        clampX = maxX;
        clampY = maxY;
    }

    nvBoolean bClampScale; // maximum value of h or w (retain scale)
    float clampScaleX; // clamping values
    float clampScaleY;

    // clamp max image size and maintain image proportions.
    // Evenly scale down in both directions so that the given image size is not exceeded
    void ClampMaxImageSizeContrained(float maxX, float maxY)
    {    
        bClampScale = true;
        clampScaleX = maxX;
        clampScaleY = maxY;
    }

    ///////////////////////////// MIP MAPS ///////////////////////////////////
    nvMipMapGeneration mipMapGeneration; // changed MIPMaptype to an enum
    long numMipMapsToWrite; // max number of Mip maps to generate

    // 0 = all
    void GenerateMIPMaps(int n)
    {
        mipMapGeneration = kGenerateMipMaps;
        numMipMapsToWrite = n;
    }

    void DoNotGenerateMIPMaps()
    {
        mipMapGeneration = kNoMipMaps;
    }

    void UseExisitingMIPMaps()
    {
        // what ever is in the image
        mipMapGeneration = kUseExistingMipMaps;
        //numMipMapsToWrite is ignored
    }

    void CompleteMIPMapChain(int n)
    {
        mipMapGeneration = kCompleteMipMapChain;
        numMipMapsToWrite = n;
    }

    nvMipFilterTypes mipFilterType; // for MIP map, select from MIPFilterTypes

    /////////////////// ALPHA //////////////////////////////////////////
    nvBoolean bBinaryAlpha; // zero or one alpha channel
    // [0,1]
    // threshold for alpha transparency DXT1
    // or when Binary Alpha is selected, below this value, alpha is zero
    float alphaThreshold32F;

    void SetBinaryAlpha(float threshold)
    {
        bBinaryAlpha = true;
        alphaThreshold32F = threshold;
    }

    ////////////////////////// BORDERS /////////////////////////////////////////
    // set any of these to generate an alpha border
    nvBoolean bAlphaBorder; // make an alpha border
    nvBoolean bAlphaBorderLeft; // make an alpha border on just the left
    nvBoolean bAlphaBorderRight; // make an alpha border on just the right
    nvBoolean bAlphaBorderTop; // make an alpha border on just the top
    nvBoolean bAlphaBorderBottom; // make an alpha 
    nvBoolean bBorder; // make a color border
    fpPixel borderColor32F; // color of border [0,1]

    void SetBorderColor(fpPixel& color)
    {
        bBorder = true;
        borderColor32F = color;
    }

    void NoBorderColor()
    {
        bBorder = false;
    }

    /////////////////////// FADING MIP LEVELS ////////////////////////////
    nvBoolean bFadeColor; // fade color over MIP maps
    nvBoolean bFadeAlpha; // fade alpha over MIP maps
    fpPixel fadeToColor32F; // color and alpha to fade to
    long fadeToDelay; // start fading after 'n' MIP maps
    float fadeAmount32F; // percentage of color to fade in
    nvBoolean bUserSpecifiedFadingAmounts;
    float userFadingAmounts[MAX_MIP_MAPS];      

    // [0,1]
    void FadeAlphaInMIPMaps(float alpha)
    {
        bFadeAlpha = true;
        fadeToColor32F.a = alpha;
    }
    
    // 0 - 255
    void FadeColorInMIPMaps(float r, float g, float b)
    {
        fadeToColor32F.r = r;
        fadeToColor32F.g = g;
        fadeToColor32F.b = b;
        bFadeColor = true;
    } 

    void SetFadingAsPercentage(float percentPerMIP, int mipLevelToStartFading)
    {
        bUserSpecifiedFadingAmounts = false;
        fadeAmount32F = percentPerMIP;
        fadeToDelay = mipLevelToStartFading;
    }

    // or 
    void SpecifyFadingPerMIPLevel(float fadeAmounts[MAX_MIP_MAPS])
    {
        bUserSpecifiedFadingAmounts = true;
        for (int i = 0; i < MAX_MIP_MAPS; i++)
        {
            userFadingAmounts[i] = fadeAmounts[i];
        }
    }
    
    /////////////////////////// SHARPENING /////////////////////////////////////
    // sharpening after creating each MIP map level
    // used when custom sharping filter is used
    // 5x5 filter 
    struct 
    {
        float filter[5][5];
        float div;
        float bias;

    } custom_filter_data; 
    
    // used when unsharpen sharping filter is used
    struct  
    {
        float radius32F; // radius
        float amount32F; // amount
        float threshold32F; // threshold [0,1]

    } unsharp_data; 
    
    // used when xsharpen sharping filter is used
    struct 
    {
        // 0 - 1
        float strength32F;
        float threshold32F;
    } xsharp_data;

    int sharpening_passes_per_mip_level[MAX_MIP_MAPS];
    nvSharpenFilterTypes sharpenFilterType; // post filtering image sharpening

    void SetNumberOfSharpeningPassesPerMIPLevel(int passes[MAX_MIP_MAPS])
    {
        for (int i = 0; i < MAX_MIP_MAPS; i++)
        {
            sharpening_passes_per_mip_level[i] = passes[i];
        }
    }

    // [0,1]
    void XSharpenImage(float strength, float threshold)
    {
        sharpenFilterType = kSharpenFilterXSharpen;
        xsharp_data.strength32F = strength;
        xsharp_data.threshold32F = threshold;
    }

    void UnSharpenImage(float radius, float amount, float threshold)
    {
        sharpenFilterType = kSharpenFilterUnSharp;
        unsharp_data.radius32F = radius;
        unsharp_data.amount32F = amount;
        unsharp_data.threshold32F = threshold;
    }

    // roll your own post filter sharpen
    void SetCustomFilter(float filter[5][5], float div, float bias)
    {
        sharpenFilterType = kSharpenFilterCustom;
        custom_filter_data.div = div;
        custom_filter_data.bias = bias;
        for (int i = 0; i < 5; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                custom_filter_data.filter[i][j] = filter[i][j];
            }
        }
    }

    nvBoolean bErrorDiffusion; // diffuse error, used for helping gradient images
    int errorDiffusionWidth; // number of texel to include

    void EnableErrorDiffusion(int width)
    {
        bErrorDiffusion = true;
        errorDiffusionWidth = width;
    }

    ///////////////////// FILTERING ////////////////////////////////////////
    // convert to gamma space before filtering
    nvBoolean bEnableFilterGamma;
    float filterGamma; // gamma value for filtering (MIP map generation)
    float filterBlur; // sharpness or blurriness of filtering
    nvBoolean bOverrideFilterWidth; // use the specified width in FilterWidth,instead of the default
    float filterWidth; // override fiter width with this value

    // 0 is no gamma correction
    void EnableGammaFiltering(float gamma)
    {
        bEnableFilterGamma = true;
        filterGamma = gamma;
    }

    void OverrideFilterWidth(float w)
    {  
        bOverrideFilterWidth = true;
        filterWidth = w;
    }

    void SetFilterSharpness(float sharp)
    {
        filterBlur = sharp;
    }

	nvTextureTypes textureType; // what type of texture is this?    
    nvTextureFormats textureFormat; // format to convert to

    void SetTextureFormat(nvTextureTypes type, nvTextureFormats format )
    {
        textureType = type;
        textureFormat = format;
    }

    size_t paletteSize;
    rgba_t color_palette[256];
    nvBoolean autoGeneratePalette; // generate palette for p8 and p4 formats
    
    // for P4 and P8 formats    
    // set 16 for P4 format and 256 for P8 format
    void SetPalette(int n, rgba_t user_palette[256])
    {  
        paletteSize = n;
        for(int i = 0; i < n; i++)
            color_palette[i] = user_palette[i];

        autoGeneratePalette = false;

    }

    ////////////////// DATA SCALING //////////////////////
    fpPixel outputScale; // scale and bias when writing output values
    fpPixel outputBias;

    void ScaleBiasOutput(fpPixel& scale, fpPixel& bias)
    {
        outputScale = scale;
        outputBias = bias;
    }
    
    fpPixel inputScale; // scale and bias on input to compressor
    fpPixel inputBias;

    void ScaleBiasInput(fpPixel& scale, fpPixel& bias)
    {
        inputScale = scale;
        inputBias = bias;
    }
    
    bool bConvertToGreyScale;
    fpPixel greyScaleWeight;

    void SetGreyScale(fpPixel& w)
    {
        bConvertToGreyScale = true;
        greyScaleWeight = w;
    }

    fpPixel brightness;
    fpPixel contrast;

    void SetBrightnessAndContrast(fpPixel& _brightness, fpPixel& _contrast)
    {
        brightness = _brightness;
        contrast = _contrast;
    }

    /////////// general enables ////////////////////////////////
    nvBoolean bOutputWrap; // wrap the values (before clamping to the format range) 
                           // when outputting to the desired format
    nvBoolean bCalcLuminance; // convert color to luminance for 'L' formats
    nvBoolean bSwapRB; // swap color positions R and G
    nvBoolean bSwapRG; // swap color positions R and G 
    nvBoolean bForceDXT1FourColors; // do not let DXT1 use 3 color representation    
    nvBoolean bRGBE; // rgba_t is in RGBE format    
    nvBoolean bCreateOnePalette; // All 4x4 compression blocks share the same palette

    /////////////////// DISABLED ////////////////////////////////
    nvBoolean bDitherColor; // enable dithering during 16 bit conversion
    nvBoolean bDitherMip0; // enable dithering during 16 bit conversion for each MIP level (after filtering)
    nvBoolean bPreModulateColorWithAlpha; // modulate color by alpha 
    nvBoolean bAlphaFilterModulate; // modulate color by alpha for filtering only

    ///////////////////////// USER DATA /////////////////////////
    void* user_data; // user supplied values passed down to write functions
};
