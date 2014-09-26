#pragma once
#include "tPixel.h" 

namespace nv
{
    template<class T>
    inline T Min(const T& x, const T& y)
    {
        return x < y ? : x : y;
    }
    template<class T>
    inline T Max(const T& x, const T& y)
    {
        return x > y ? x : y;
    }
}

typedef unsigned short nvhalf;

union nv_half_data
{
    unsigned short bits;
    struct
    {
        unsigned long m : 10;
        unsigned long e : 5;
        unsigned long s : 1;
    } ieee;
};

union nv_ieee_single
{
    float f;
    struct
    {
        unsigned long m : 23;
        unsigned long e : 8;
        unsigned long s : 1;
    } ieee;
};

class nvColorConvert
{
public:
    void RGBAToFloat(const RGBAImage& srcImage, fpImage& dstImage);
    void RGBAToFloat(const RGBAMipMappedImage& srcMIPImage, fpMipMappedImage& dstMIPImage);
    void FloatToRGBA(const fpImage& srcImage, RGBAImage&  dstImage);
    void FloatToRGBA(const fpMipMappedImage& srcMIPImage, RGBAMipMappedImage& dstMIPImage);
    fpPixel RGBEtoFloat(int r, int g, int b, int e);
    // photoshop import export
    float PSSignedToFloat(int pixel, int depth, int plane);
    float PSUnsignedToFloat(int pixel, int depth);
    int FloatToUnsignedPS(float pixel, int depth);
    int FloatToSignedPS(float pixel, int depth, int plane);
    // format to float
    static float UnsignedToFloat(int channel); // unsigned, 8 bits
    static float UnsignedToFloat(int channel, int nBits); // unsigned
    static float SignedToFloat(int channel); // 8 bits
    static float SignedToFloat(int channel, int nBits);

    static void RGBAToFloat(int r, int g, int b, int a, fpPixel& fp)
    {
        fp.r = UnsignedToFloat(r);
        fp.g = UnsignedToFloat(g);
        fp.b = UnsignedToFloat(b);
        fp.a = UnsignedToFloat(a);
    }

    static void RGBAToFloat(const rgba_t& inColor, fpPixel& fp)
    {
        fp.r = UnsignedToFloat(inColor.r);
        fp.g = UnsignedToFloat(inColor.g);
        fp.b = UnsignedToFloat(inColor.b);
        fp.a = UnsignedToFloat(inColor.a);
    }

    static void RGBEToFloat(fpPixel& fp, const rgba_t& rgbe);
    static void RGBEToFloat(float &r, float& g, float& b, const rgba_t& rgbe);

    static float HalfToFloat(nvhalf val)
    {
        nv_half_data h;
        //h.bits = val.value_bits;
        h.bits = val;
        nv_ieee_single sng;
        sng.ieee.s = h.ieee.s;
        // handle special cases
        if (h.ieee.e == 0 && h.ieee.m == 0)
        {
            // zero
            sng.ieee.m = 0;
            sng.ieee.e = 0;
        }
        else if (h.ieee.e == 0 && h.ieee.m != 0)
        {
            // denorm -- denorm half will fit in non-denorm single
            const float half_denorm = 1.0f / 16384.0f; // 2^-14
            float mantissa = (float)h.ieee.m / 1024.0f;
            float sgn = (h.ieee.s) ? -1.0f : 1.0f;
            sng.f = sgn*mantissa*half_denorm;
        }
        else if (h.ieee.e == 31 && h.ieee.m == 0)
        {
            // infinity
            sng.ieee.e = 0xff;
            sng.ieee.m = 0;
        }
        else if (h.ieee.e == 31 && h.ieee.m != 0)
        {
            // NaN
            sng.ieee.e = 0xff;
            sng.ieee.m = 1;
        }
        else
        {
            sng.ieee.e = h.ieee.e + 112;
            sng.ieee.m = h.ieee.m << 13;
        }
        return sng.f;
    }

    /// float to format
    static unsigned long FloatToUnsigned(float channel); // 8 bits
    static unsigned long FloatToUnsigned(float channel, int nBits);
    static long FloatToSigned(float channel); // 8 bits
    static long FloatToSigned(float channel, int nBits);

    static unsigned long NormalToUnsigned(float inColor)
    {
        return FloatToUnsigned(inColor * 0.5f + 0.5f);
    }

    static unsigned long NormalToUnsigned(float inColor, int nBits)
    {
        return FloatToUnsigned(inColor * 0.5f + 0.5f, nBits);
    }

    static void NormalToRGBA(const fpPixel& inColor, rgba_t& outColor)
    {
        outColor.r = (unsigned char)NormalToUnsigned(inColor.r);
        outColor.g = (unsigned char)NormalToUnsigned(inColor.g);
        outColor.b = (unsigned char)NormalToUnsigned(inColor.b);
        outColor.a = (unsigned char)NormalToUnsigned(inColor.a);
    }

    static void FloatToRGBA(const fpPixel& inColor, rgba_t& p)
    {
        p.r = (unsigned char)FloatToUnsigned(inColor.r);
        p.g = (unsigned char)FloatToUnsigned(inColor.g);
        p.b = (unsigned char)FloatToUnsigned(inColor.b);
        p.a = (unsigned char)FloatToUnsigned(inColor.a);
    }

    static void FloatToBGRA(const fpPixel& inColor, unsigned long& outColor)
    {
        unsigned int r = FloatToUnsigned(inColor.r);
        unsigned int g = FloatToUnsigned(inColor.g);
        unsigned int b = FloatToUnsigned(inColor.b);
        unsigned int a = FloatToUnsigned(inColor.a);
        outColor = a << 24 | r << 16 | g << 8 | b;
    }

    static void FloatToBGRA(float r, float g, float b, float a, unsigned long& outColor)
    {
        unsigned int ri = FloatToUnsigned(r);
        unsigned int gi = FloatToUnsigned(g);
        unsigned int bi = FloatToUnsigned(b);
        unsigned int ai = FloatToUnsigned(a);
        outColor = ai << 24 | ri << 16 | gi << 8 | bi;
    }

    static void FloatToQ8W8V8U8(const fpPixel& inColor, q8w8v8u8_t & outColor)
    {
        outColor.q = (char)FloatToSigned(inColor.r);
        outColor.v = (char)FloatToSigned(inColor.g);
        outColor.w = (char)FloatToSigned(inColor.b);
        outColor.u = (char)FloatToSigned(inColor.a);
    }

    static void FloatToU16V16(float u, float v, v16u16_t& outColor)
    {
        outColor.u = (short)FloatToSigned(u, 16);
        outColor.v = (short)FloatToSigned(v, 16);
    }

    static void FloatToR12G12B8(const fpPixel& inColor, r12g12b8_t& outColor)
    {
        outColor.r = FloatToUnsigned(inColor.r, 12);
        outColor.g = FloatToUnsigned(inColor.g, 12);
        outColor.b = FloatToUnsigned(inColor.b, 8);
    }

    static void NormalToR12G12B8(const fpPixel& inColor, r12g12b8_t& outColor)
    {
        outColor.r = FloatToUnsigned(inColor.r * 0.5f + 0.5f, 12);
        outColor.g = FloatToUnsigned(inColor.g * 0.5f + 0.5f, 12);
        outColor.b = FloatToUnsigned(inColor.b * 0.5f + 0.5f, 8);
    }

    static float FloatToRGBE_Alpha(const fpPixel& fp);
    static void FloatToRGBE(rgba_t& rgbe, const float &r, const float& g, const float& b);
    static void FloatToRGBE(rgba_t * rgbe, const float &r, const float& g, const float& b);
    static void FloatToRGBE(fpPixel& rgbe, const fpPixel& fp);
    static void FloatToRGBE(rgba_t& rgbe, const fpPixel& fp);
    static unsigned char FloatToRGBE_DXT3Alpha(const fpPixel& fp);

    static nvhalf FloatToHalf(float val)
    {
        nv_ieee_single f;
        f.f = val;
        nv_half_data h;
        h.ieee.s = f.ieee.s;
        // handle special cases
        //const float half_denorm = (1.0f/16384.0f);
        if (f.ieee.e == 0 && f.ieee.m == 0)
        {
            // zero
            h.ieee.m = 0;
            h.ieee.e = 0;
        }
        else if (f.ieee.e == 0 && f.ieee.m != 0)
        {
            // denorm -- denorm float maps to 0 half
            h.ieee.m = 0;
            h.ieee.e = 0;
        }
        else if (f.ieee.e == 0xff && f.ieee.m == 0)
        {
            // infinity
            h.ieee.m = 0;
            h.ieee.e = 31;
        }
        else if (f.ieee.e == 0xff && f.ieee.m != 0)
        {
            // NaN
            h.ieee.m = 1;
            h.ieee.e = 31;
        }
        else
        {
            // regular number
            int new_exp = f.ieee.e - 127;
            if (new_exp < -24)
            { // this maps to 0
                h.ieee.m = 0;
                h.ieee.e = 0;
            }
            if (new_exp < -14)
            {
                // this maps to a denorm
                h.ieee.e = 0;
                unsigned int exp_val = (unsigned int)(-14 - new_exp); // 2^-exp_val
                switch (exp_val)
                {
                case 0:
                    //fprintf(stderr, "ftoh: logical error in denorm creation!\n"); 
                    h.ieee.m = 0;
                    break;
                case 1: h.ieee.m = 512 + (f.ieee.m >> 14); break;
                case 2: h.ieee.m = 256 + (f.ieee.m >> 15); break;
                case 3: h.ieee.m = 128 + (f.ieee.m >> 16); break;
                case 4: h.ieee.m = 64 + (f.ieee.m >> 17); break;
                case 5: h.ieee.m = 32 + (f.ieee.m >> 18); break;
                case 6: h.ieee.m = 16 + (f.ieee.m >> 19); break;
                case 7: h.ieee.m = 8 + (f.ieee.m >> 20); break;
                case 8: h.ieee.m = 4 + (f.ieee.m >> 21); break;
                case 9: h.ieee.m = 2 + (f.ieee.m >> 22); break;
                case 10: h.ieee.m = 1; break;
                }
            }
            else if (new_exp > 15)
            { // map this value to infinity
                h.ieee.m = 0;
                h.ieee.e = 31;
            }
            else
            {
                h.ieee.e = new_exp + 15;
                h.ieee.m = f.ieee.m >> 13;
            }
        }
        return *(nvhalf*)&h.bits;
    }
};
