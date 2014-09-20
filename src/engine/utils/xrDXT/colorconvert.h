#pragma once
#include "tPixel.h" 

#ifndef ULMIN
#define ULMIN

template < class T > inline T ulMin(const T &x, const T &y)
{
    if (x < y)
        return x;
    else
        return y;
}
#endif


#ifndef ULMAX
#define ULMAX
template < class T > inline T ulMax(const T &x, const T &y)
{
    if (x < y)
        return y;
    else
        return x;
}
#endif




inline void ARGBToAlphaAndVector(const rgba_t& inColor, 
                                 unsigned long& theHeight, 
                                 fpPixel& outVector)
{

                        
    outVector.x = (float)inColor.r / 127.5 - 1.0;
    outVector.y = (float)inColor.g / 127.5 - 1.0;
    outVector.z = (float)inColor.b / 127.5 - 1.0;

    outVector.a = theHeight;

}

inline void ARGBToAlphaAndVector(const rgba_t * inColor, 
                                 unsigned long& theHeight, 
                                 fpPixel& outVector)
{

                        
    outVector.x = (float)inColor->r / 127.5 - 1.0;
    outVector.y = (float)inColor->g / 127.5 - 1.0;
    outVector.z = (float)inColor->b / 127.5 - 1.0;

    outVector.a = theHeight;

}


inline void ColorToVector(rgba_t color, float & r, float & g, float & b)
{

    r = (float)(color.r) / 127.5 - 1.0;
    g = (float)(color.g) / 127.5 - 1.0;
    b = (float)(color.b) / 127.5 - 1.0;
}






inline void AlphaAndVectorToRGBA( const unsigned long & theHeight, const fpPixel & inVector, rgba_t & outColor )
{
	const unsigned int red   = ulMin( 255u, (unsigned int)( ( inVector.x + 1.0f ) * 127.5f  ) );
	const unsigned int green = ulMin( 255u, (unsigned int)( ( inVector.y + 1.0f ) * 127.5f ) );
	const unsigned int blue  = ulMin( 255u, (unsigned int)( ( inVector.z + 1.0f ) * 127.5f ) );

	//outColor = ( ( (unsigned int)theHeight << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue << 0 ) );
    outColor.a = theHeight;
    
    outColor.r = red;
    outColor.g = green;
    outColor.b = blue;


}
inline void RGBAToAlphaAndVector(const rgba_t & inColor, unsigned long& theHeight, fpPixel& outVector)
{

    outVector.x = (double)inColor.r / 127.5 - 1.0;
    outVector.y = (double)inColor.g / 127.5 - 1.0;
    outVector.z = (double)inColor.b / 127.5 - 1.0;

    theHeight = inColor.a;

}

    
/*
inline void AlphaAndVectorToARGB( const unsigned long & theHeight, 
                                 const fpPixel & inVector,rgba_t & outColor )
{
	int red   = (( inVector.x + 1.0f ) * 127.5f ) + 0.5;
	int green = (( inVector.y + 1.0f ) * 127.5f ) + 0.5 ;
	int blue  = (( inVector.z + 1.0f ) * 127.5f ) + 0.5 ;

    if ( red < 0)
        red = 0;
    if (red > 255)
        red = 255;
    if (green < 0)
        green = 0;
    if (green > 255)
        green = 255;

    if (blue < 0)
        blue = 0;
    if (blue > 255)
        blue = 255;

	//outColor = ( ( (unsigned int)theHeight << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue << 0 ) );
    outColor.a = theHeight;

    outColor.r = red;
    outColor.g = green;
    outColor.b = blue;


}   */








__forceinline
void AlphaAndVectorToQ8W8V8U8(unsigned long theHeight, 
                              float fq, float fw, float fv, 
                              q8w8v8u8_t & outColor )
{
    // Map the range [-1,1] to [0,255]
    // Does not clamp for you

    int q   = (int)((( fq ) * 127.0f ) + 0.5f);
    int w = (int)((( fw ) * 127.0f ) + 0.5f);
    int v  = (int)((( fv ) * 127.0f ) + 0.5f);

    if (q < -127)
        q = -127;
    if (q > 127)
        q = 127;
    if (w < -127)
        w = -127;
    if (w > 127)
        w = 127;

    if (v < -127)
        v = -127;
    if (v > 127)
        v = 127;


    outColor.q = q;
    outColor.w = w;
    outColor.v = v;

    outColor.u = theHeight;



}


__forceinline
void VectorToU16V16( float u, float v, u16v16_t & outColor )
{
	// u and v values in [-1,1] range are converted to
	// two's complement 16 bit integers and stored in output unsigned long

	short int du, dv;
	du = (int) ( u * ( 2 << 15 ) );
	dv = (int) ( v * ( 2 << 15 ) );

	//*outColor = du | (( dv << 16 ) & 0xFFFF0000); 
    outColor.r = du;
    outColor.g = dv;



}


__forceinline
void AlphaAndVectorToARGB( unsigned long theHeight, float fred, float fgreen, float fblue, 
                          rgba_t & outColor )
{
	// Map the range [-1,1] to [0,255]
	// Does not clamp for you

	/*int r= (int) (( red   + 1.0f ) * 127.5f);
	int g = (int) (( green + 1.0f ) * 127.5f);
	int b = (int) (( blue  + 1.0f ) * 127.5f);

	outColor = ( ( alpha << 24 ) | ( r << 16 ) | ( g << 8 ) | ( b << 0 ) );*/


    int red   = (int)((( fred + 1.0f ) * 127.5f ) + 0.5f);
    int green = (int)((( fgreen + 1.0f ) * 127.5f ) + 0.5f);
    int blue  = (int)((( fblue + 1.0f ) * 127.5f ) + 0.5f);

    if (red < 0)
        red = 0;
    if (red > 255)
        red = 255;
    if (green < 0)
        green = 0;
    if (green > 255)
        green = 255;

    if (blue < 0)
        blue = 0;
    if (blue > 255)
        blue = 255;

    //outColor = ( ( theHeight << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue << 0 ) );

    outColor.a = theHeight;
    outColor.r = red;
    outColor.g = green;
    outColor.b = blue;



}



__forceinline
void RGBToFloat( rgba_t inColor, 
                float * outAlpha, float * outRed, float * outGreen, float * outBlue )
{
	// Converts RGB [0,255] to float [0,1]

	*outAlpha	= (float)( inColor.a) / 255.0f;
	*outRed		= (float)( inColor.r) / 255.0f;
	*outGreen	= (float)( inColor.g) / 255.0f;
	*outBlue	= (float)( inColor.b) / 255.0f;
}


__forceinline
void RGBToFloatVector( rgba_t inColor, float * outAlpha, float * outRed, float * outGreen, float * outBlue )
{
	// Converts RGB [0,255] to float [-1,1]
	// Use if RGB is a vector from a normal map

	RGBToFloat( inColor, outAlpha, outRed, outGreen, outBlue );
	*outAlpha	= ( *outAlpha	* 2.0f ) - 1.0f;
	*outRed		= ( *outRed		* 2.0f ) - 1.0f;
	*outGreen	= ( *outGreen	* 2.0f ) - 1.0f;
	*outBlue	= ( *outBlue	* 2.0f ) - 1.0f;

}

__forceinline
void RGBToFloat( rgba_t inColor, 
                double * outAlpha, 
                double * outRed, double* outGreen, double * outBlue )
{
	// Converts RGB [0,255] to float [0,1]

	*outAlpha	= (float)( inColor.a) / 255.0f;
	*outRed		= (float)( inColor.r) / 255.0f;
	*outGreen	= (float)( inColor.g) / 255.0f;
	*outBlue	= (float)( inColor.b) / 255.0f;
}



__forceinline
void RGBToFloatVector( rgba_t inColor, double * outAlpha, 
                      double * outRed, double * outGreen, double * outBlue )
{
	// Converts RGB [0,255] to float [-1,1]
	// Use if RGB is a vector from a normal map

	RGBToFloat( inColor, outAlpha, outRed, outGreen, outBlue );
	*outAlpha	= ( *outAlpha	* 2.0f ) - 1.0f;
	*outRed		= ( *outRed		* 2.0f ) - 1.0f;
	*outGreen	= ( *outGreen	* 2.0f ) - 1.0f;
	*outBlue	= ( *outBlue	* 2.0f ) - 1.0f;

}












inline 
void VectorToColor(float r, float g, float b, rgba_t & color)
{


    int ir = ((r + 1.0) * 127.5) + 0.5;
    int ig = ((g + 1.0) * 127.5) + 0.5;
    int ib = ((b + 1.0) * 127.5) + 0.5;

    if (r < 0)
        r = 0;
    if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    if (b > 255)
        b = 255;

    color.r = r;
    color.g = g;
    color.b = b;


}





__forceinline
void AlphaAndVectorToR12G12B8( float red, float green, float blue, r12g12b8_t & outColor )
{
	// Map the range [-1,1] to [0,255]
	// Does not clamp for you


    unsigned int maxval_12 = (1 << 12) - 1;
    unsigned int maxval_8  = (1 << 8) - 1;

    float scale_12 = (float)maxval_12 / 2.0;
    float scale_8 = (float)maxval_8 / 2.0;

	int r = (int) (( red   + 1.0f ) * scale_12);
	int g = (int) (( green + 1.0f ) * scale_12);
	int b = (int) (( blue  + 1.0f ) * scale_8);


    if (r < 0)
        r = 0;
    if (r > maxval_12)
        r = maxval_12;
    if (g < 0)
        g = 0;
    if (g > maxval_12)
        g = maxval_12;
    if (b < 0)
        b = 0;
    if (b > maxval_8)
        b = maxval_8;

    outColor.r = r;
    outColor.g = g;
    outColor.b = b;

}





inline void AlphaAndVectorToARGB( const unsigned long & theHeight, 
                                 const fpPixel & inVector, rgba_t& outColor )
{
	/*const unsigned int red   = min( 255u, (unsigned int)( ( inVector.x + 1.0f ) * 127.5f  ) );
	const unsigned int green = min( 255u, (unsigned int)( ( inVector.y + 1.0f ) * 127.5f ) );
	const unsigned int blue  = min( 255u, (unsigned int)( ( inVector.z + 1.0f ) * 127.5f ) );

	outColor = ( ( (unsigned int)theHeight << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue << 0 ) );
      */

    int red   = (int)((( inVector.x + 1.0f ) * 127.5f ) + 0.5f);
    int green = (int)((( inVector.y + 1.0f ) * 127.5f ) + 0.5f);
    int blue  = (int)((( inVector.z + 1.0f ) * 127.5f ) + 0.5f);

    if (red < 0)
        red = 0;
    if (red > 255)
        red = 255;
    if (green < 0)
        green = 0;
    if (green > 255)
        green = 255;

    if (blue < 0)
        blue = 0;
    if (blue > 255)
        blue = 255;

    //outColor = ( ( theHeight << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue << 0 ) );
    outColor.a = theHeight;

    outColor.r = red;
    outColor.b = green;
    outColor.b = blue;




}

inline void AlphaAndVectorToARGB( const unsigned long & theHeight, 
                                 const fpPixel & inVector,rgba_t * outColor )
{
	int red   = (( inVector.x + 1.0f ) * 127.5f ) + 0.5;
	int green = (( inVector.y + 1.0f ) * 127.5f ) + 0.5 ;
	int blue  = (( inVector.z + 1.0f ) * 127.5f ) + 0.5 ;

    if ( red < 0)
        red = 0;
    if (red > 255)
        red = 255;
    if (green < 0)
        green = 0;
    if (green > 255)
        green = 255;

    if (blue < 0)
        blue = 0;
    if (blue > 255)
        blue = 255;

    outColor->a = theHeight;

    outColor->r = red;
    outColor->g = green;
    outColor->b = blue;


}

inline void AlphaAndVectorToARGB( const unsigned long & theHeight, 
                                 const fpPixel * inVector,rgba_t * outColor )
{
	int red   = (( inVector->x + 1.0f ) * 127.5f ) + 0.5;
	int green = (( inVector->y + 1.0f ) * 127.5f ) + 0.5 ;
	int blue  = (( inVector->z + 1.0f ) * 127.5f ) + 0.5 ;

    if ( red < 0)
        red = 0;
    if (red > 255)
        red = 255;
    if (green < 0)
        green = 0;
    if (green > 255)
        green = 255;

    if (blue < 0)
        blue = 0;
    if (blue > 255)
        blue = 255;

    outColor->a = theHeight;

    outColor->r = red;
    outColor->g = green;
    outColor->b = blue;


}


inline void AlphaAndVectorToARGB( const unsigned long & theHeight, 
                                 const fpPixel * inVector,rgba_t & outColor )
{
	int red   = (( inVector->x + 1.0f ) * 127.5f ) + 0.5;
	int green = (( inVector->y + 1.0f ) * 127.5f ) + 0.5 ;
	int blue  = (( inVector->z + 1.0f ) * 127.5f ) + 0.5 ;

    if ( red < 0)
        red = 0;
    if (red > 255)
        red = 255;
    if (green < 0)
        green = 0;
    if (green > 255)
        green = 255;

    if (blue < 0)
        blue = 0;
    if (blue > 255)
        blue = 255;

    outColor.a = theHeight;

    outColor.r = red;
    outColor.g = green;
    outColor.b = blue;


}
