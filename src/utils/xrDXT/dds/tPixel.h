#pragma once

#include <math.h>
#include <memory.h>
#include <dds/tVector.h>
#include <dds/nvErrorCodes.h>

namespace nv
{
// modulo value x between [lo,hi]
// allows value 'hi'
template <class _Type>
inline _Type Clamp(const _Type& x, const _Type& lo, const _Type& hi)
{
    if (x < lo)
        return lo;
    else if (x > hi)
        return hi;
    else
        return x;
}

inline int iClamp(int x, int lo, int hi)
{
    if (x < lo)
        return lo;
    if (x > hi)
        return hi;
    return x;
}

inline float fClamp(float x, float lo, float hi)
{
    if (x < lo)
        return lo;
    if (x > hi)
        return hi;
    return x;
}

inline int fmod(int x, int size) { return x % size; }
inline __int64 fmod(__int64 x, __int64 size) { return x % size; }
inline unsigned __int64 fmod(unsigned __int64 x, unsigned __int64 size) { return x % size; }
inline float __cdecl fmod(float _X, float _Y) { return fmodf(_X, _Y); }
// calcMaxMipmap
//  calculates max # of mipmap levels for given texture size
inline size_t calcMaxMipmap(size_t w, size_t h)
{
    size_t n = 0;
    size_t count = 0;
    count = w > h ? w : h;
    while (count)
    {
        n++;
        count >>= 1;
    }
    return n;
}

inline size_t calcMaxMipmap(size_t w, size_t h, size_t d)
{
    size_t n = 0;
    size_t count = 0;
    count = w > h ? w : h;
    if (d > count)
        count = d;
    while (count)
    {
        n++;
        count >>= 1;
    }
    return n;
}

// get next mip level size
inline size_t NextMip(size_t m)
{
    size_t next = m / 2; // round down
    if (next == 0)
        return 1;
    else
        return next;
}

// lo = 0;
// allow hi value
template <class _Type>
inline _Type Modulo(const _Type& x, const _Type& hi)
{
    if (x >= 0 && x <= hi)
        return x;
    _Type f = fmod(x, hi);
    if (f < 0)
        f += hi;
    return f;
}

// does not allow x == size
inline int iModulo(int x, int size)
{
    if (x < 0)
    {
        int n = x / size;
        x += size * (n + 1);
    }
    return x % size;
}

template <class _Type>
inline _Type Modulo(const _Type& x, const _Type& lo, const _Type& hi)
{
    if (x >= lo && x <= hi)
        return x;
    _Type dw = hi - lo;
    _Type t = x - lo;
    _Type f = fmod(t, dw);
    if (f < 0)
        f += dw;
    f += lo;
    return f;
}
}

#pragma pack(push, 4)

// red and green
class v16u16_t
{
public:
    union
    {
        short uv[4];
        struct
        {
            short u;
            short v;
        };
    };

    v16u16_t& operator+=(const v16u16_t& v); // incrementation by a Vec4f

    void set(unsigned short _u, unsigned short _v)
    {
        u = _u;
        v = _v;
    }
};

class r12g12b8_t
{
public:
    union
    {
        struct
        {
            unsigned long r : 12;
            unsigned long g : 12;
            unsigned long b : 8;
        };
    };

    r12g12b8_t& operator+=(const r12g12b8_t& v); // incrementation by a Vec4f

    void set(unsigned long _r, unsigned long _g, unsigned long _b)
    {
        r = _r;
        g = _g;
        b = _b;
    }
};

class rgba_t
{
public:
    union
    {
        unsigned long u;
        unsigned char p[4];
        struct
        {
            unsigned char r;
            unsigned char g;
            unsigned char b;
            unsigned char a;
        };
    };

    rgba_t() {}
    rgba_t(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
    {
        a = _a;
        r = _r;
        g = _g;
        b = _b;
    }

    unsigned long bgra()
    {
        return (unsigned long)a << 24 | (unsigned long)r << 16 | (unsigned long)g << 8 | (unsigned long)b;
    }

    rgba_t& operator+=(const rgba_t& v) // incrementation by a rgba_t
    {
        r = (unsigned char)nv::Clamp((int)((int)r + (int)v.r), 0, 255);
        g = (unsigned char)nv::Clamp((int)g + (int)v.g, 0, 255);
        b = (unsigned char)nv::Clamp((int)b + (int)v.b, 0, 255);
        a = (unsigned char)nv::Clamp((int)a + (int)v.a, 0, 255);
        return *this;
    }

    rgba_t& operator-=(const rgba_t& v); // decrementation by a rgba_t
    rgba_t& operator*=(const float d); // multiplication by a constant
    rgba_t& operator/=(const float d); // division by a constant

    rgba_t& operator=(const rgba_t& v)
    {
        r = v.r;
        g = v.g;
        b = v.b;
        a = v.a;
        return *this;
    }

    friend rgba_t operator+(const rgba_t& v1, const rgba_t& v2)
    {
        int r = nv::Clamp((int)v1.r + (int)v2.r, 0, 255);
        int g = nv::Clamp((int)v1.g + (int)v2.g, 0, 255);
        int b = nv::Clamp((int)v1.b + (int)v2.b, 0, 255);
        int a = nv::Clamp((int)v1.a + (int)v2.a, 0, 255);
        return rgba_t((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
    }

    friend rgba_t operator/(const rgba_t& v, float s)
    {
        return rgba_t(
            (unsigned char)(v.r / s), (unsigned char)(v.g / s), (unsigned char)(v.b / s), (unsigned char)(v.a / s));
    }

    friend rgba_t operator/(const rgba_t& v, int s)
    {
        return rgba_t(
            (unsigned char)(v.r / s), (unsigned char)(v.g / s), (unsigned char)(v.b / s), (unsigned char)(v.a / s));
    }

    void set(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }

    void SetToZero() { r = g = b = a = 0; }
};

class rgba16_t
{
public:
    union
    {
        unsigned short rgba[4];
        struct
        {
            unsigned short r;
            unsigned short g;
            unsigned short b;
            unsigned short a;
        };
    };

    rgba16_t() {}
    rgba16_t(unsigned short _r, unsigned short _g, unsigned short _b, unsigned short _a)
    {
        a = _a;
        r = _r;
        g = _g;
        b = _b;
    }

    rgba16_t& operator+=(const rgba16_t& v) // incrementation by a rgba_t
    {
        r = (unsigned char)nv::Clamp((int)r + (int)v.r, 0, 65535);
        g = (unsigned char)nv::Clamp((int)g + (int)v.g, 0, 65535);
        b = (unsigned char)nv::Clamp((int)b + (int)v.b, 0, 65535);
        a = (unsigned char)nv::Clamp((int)a + (int)v.a, 0, 65535);
        return *this;
    }

    rgba16_t& operator-=(const rgba16_t& v); // decrementation by a rgba_t
    rgba16_t& operator*=(const float d); // multiplication by a constant
    rgba16_t& operator/=(const float d); // division by a constant

    rgba16_t& operator=(const rgba16_t& v)
    {
        r = v.r;
        g = v.g;
        b = v.b;
        a = v.a;
        return *this;
    }

    friend rgba16_t operator+(const rgba16_t& v1, const rgba16_t& v2)
    {
        int r = nv::Clamp((int)v1.r + (int)v2.r, 0, 65535);
        int g = nv::Clamp((int)v1.g + (int)v2.g, 0, 65535);
        int b = nv::Clamp((int)v1.b + (int)v2.b, 0, 65535);
        int a = nv::Clamp((int)v1.a + (int)v2.a, 0, 65535);
        return rgba16_t((unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
    }

    friend rgba16_t operator/(const rgba16_t& v, float s)
    {
        return rgba16_t(
            (unsigned short)(v.r / s), (unsigned short)(v.g / s), (unsigned short)(v.b / s), (unsigned short)(v.a / s));
    }

    friend rgba16_t operator/(const rgba16_t& v, int s)
    {
        return rgba16_t(
            (unsigned short)(v.r / s), (unsigned short)(v.g / s), (unsigned short)(v.b / s), (unsigned short)(v.a / s));
    }

    void set(unsigned short _r, unsigned short _g, unsigned short _b, unsigned short _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
};

class urgba_t
{
public:
    union
    {
        unsigned long u;
        char rgba[4];
        struct
        {
            char r;
            char g;
            char b;
            char a;
        };
    };

    urgba_t& operator+=(const urgba_t& v); // incrementation by a Vec4f

    void set(char _r, char _g, char _b, char _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
};

class q8w8v8u8_t
{
public:
    union
    {
        char qwvu[4];
        struct
        {
            char q;
            char w;
            char v;
            char u;
        };
    };

    q8w8v8u8_t& operator+=(const q8w8v8u8_t& v); // incrementation by a Vec4f

    void set(char _r, char _g, char _b, char _a)
    {
        q = _r;
        w = _g;
        v = _b;
        u = _a;
    }
};

#define _R 0
#define _G 1
#define _B 2
#define _A 3

class fpPixel
{
public:
    union
    {
        float p[4];
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };
    };

    void SetToZero()
    {
        r = 0;
        g = 0;
        b = 0;
        a = 0;
    }

    void Clamp(fpPixel& lo, fpPixel& hi)
    {
        r = nv::Clamp(r, lo.r, hi.r);
        g = nv::Clamp(g, lo.g, hi.g);
        b = nv::Clamp(b, lo.b, hi.b);
        a = nv::Clamp(a, lo.a, hi.a);
    }

    void Wrap(fpPixel& lo, fpPixel& hi)
    {
        r = nv::Modulo(r, lo.r, hi.r);
        g = nv::Modulo(g, lo.g, hi.g);
        b = nv::Modulo(b, lo.b, hi.b);
        a = nv::Modulo(a, lo.a, hi.a);
    }

    void dot(fpPixel& w)
    {
        float grey = r * w.r + g * w.g + b * w.b + a * w.a;
        r = grey;
        g = grey;
        b = grey;
    }

    fpPixel() {}
    fpPixel(const float _r, const float _g, const float _b, const float _a)
    {
        a = _a;
        r = _r;
        g = _g;
        b = _b;
    }

    fpPixel(const fpPixel& v)
    {
        a = v.a;
        r = v.r;
        g = v.g;
        b = v.b;
    }

    void set(const float _r, const float _g, const float _b, const float _a)
    {
        a = _a;
        r = _r;
        g = _g;
        b = _b;
    }

    void set(const fpPixel& v)
    {
        a = v.a;
        r = v.r;
        g = v.g;
        b = v.b;
    }

    fpPixel& operator+=(const fpPixel& v) // incrementation by a rgba_t
    {
        r += v.r;
        g += v.g;
        b += v.b;
        a += v.a;
        return *this;
    }

    fpPixel& operator-=(const fpPixel& v) // incrementation by a rgba_t
    {
        r -= v.r;
        g -= v.g;
        b -= v.b;
        a -= v.a;
        return *this;
    }

    fpPixel& operator*=(const fpPixel& v) // incrementation by a rgba_t
    {
        r *= v.r;
        g *= v.g;
        b *= v.b;
        a *= v.a;
        return *this;
    }

    fpPixel& operator/=(const fpPixel& v) // incrementation by a rgba_t
    {
        r /= v.r;
        g /= v.g;
        b /= v.b;
        a /= v.a;
        return *this;
    }

    fpPixel& operator/=(const float& s) // incrementation by a rgba_t
    {
        r /= s;
        g /= s;
        b /= s;
        a /= s;
        return *this;
    }

    fpPixel& operator=(const fpPixel& v); // assignment of a Vec3f

    friend fpPixel operator+(const fpPixel& v1, const fpPixel& v2)
    {
        return fpPixel(v1.r + v2.r, v1.g + v2.g, v1.b + v2.b, v1.a + v2.a);
    }

    friend fpPixel operator/(const fpPixel& v, float s) { return fpPixel(v.r / s, v.g / s, v.b / s, v.a / s); }
    friend int operator==(const fpPixel& v1, const fpPixel& v2);

    NV_ERROR_CODE normalize()
    {
        double u = x * x + y * y + z * z;
        if (fabs(u - 1.0) < 1e-12)
            return NV_OK; // already normalized
        if (fabs((double)u) < 1e-12)
        {
            x = y = z = 0.0f;
            return NV_CANT_NORMALIZE;
        }
        u = 1.0 / sqrt(u);
        x = (float)(x * u);
        y = (float)(y * u);
        z = (float)(z * u);
        return NV_OK;
    }
};

class fpPixel3
{
public:
    union
    {
        float p[3];
        struct
        {
            float r;
            float g;
            float b;
        };
        struct
        {
            float x;
            float y;
            float z;
        };
    };

    void SetToZero()
    {
        r = 0;
        g = 0;
        b = 0;
    }

    fpPixel3() {}
    fpPixel3(const float _r, const float _g, const float _b)
    {
        r = _r;
        g = _g;
        b = _b;
    }

    fpPixel3(const fpPixel3& v)
    {
        r = v.r;
        g = v.g;
        b = v.b;
    }

    void set(const float _r, const float _g, const float _b)
    {
        r = _r;
        g = _g;
        b = _b;
    }

    void set(const fpPixel3& v)
    {
        r = v.r;
        g = v.g;
        b = v.b;
    }

    fpPixel3& operator+=(const fpPixel3& v); // incrementation by a Vec4f
    fpPixel3& operator=(const fpPixel3& v); // assignment of a Vec3f
    fpPixel3& operator-=(const fpPixel3& v); // decrementation by a Vec3f
    fpPixel3& operator*=(const float d); // multiplication by a constant
    fpPixel3& operator/=(const float d); // division by a constant

    friend fpPixel3 operator+(const fpPixel3& v1, const fpPixel3& v2)
    {
        return fpPixel3(v1.r + v2.r, v1.g + v2.g, v1.b + v2.b);
    }

    friend fpPixel3 operator/(const fpPixel3& v, float s) { return fpPixel3(v.r / s, v.g / s, v.b / s); }
    friend int operator==(const fpPixel3& v1, const fpPixel3& v2);

    NV_ERROR_CODE normalize()
    {
        double u = x * x + y * y + z * z;
        if (fabs(u - 1.0) < 1e-12)
            return NV_OK; // already normalized
        if (fabs((double)u) < 1e-12)
        {
            x = y = z = 0.0f;
            return NV_CANT_NORMALIZE;
        }
        u = 1.0 / sqrt(u);
        x = (float)(x * u);
        y = (float)(y * u);
        z = (float)(z * u);
        return NV_OK;
    }
};

typedef fpPixel* fp_i;

inline int operator==(const fpPixel& v1, const fpPixel& v2)
{
    return v1.a == v2.a && v1.r == v2.r && v1.b == v2.b && v1.g == v2.g;
}

inline fpPixel& fpPixel::operator=(const fpPixel& v)
{
    a = v.a;
    r = v.r;
    g = v.g;
    b = v.b;
    return *this;
}

template <class _Type>
class nvImage
{
private:
    size_t m_width;
    size_t m_height;
    nvVector<_Type> m_pixels;
    bool m_RGBE;

public:
    void SetRGBE(bool b) { m_RGBE = b; }
    bool isRGBE() const { return m_RGBE; }
    size_t size() { return m_width * m_height; }
    nvImage<_Type>& operator=(const nvImage<_Type>& v)
    {
        // resize and copy over
        resize(v.width(), v.height());
        m_pixels = v.m_pixels;
        m_RGBE = v.m_RGBE;
        return *this;
    }

    _Type& operator[](size_t i)
    {
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i];
    }

    const _Type& operator[](size_t i) const
    {
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i];
    }

    _Type& operator()(const size_t& y, const size_t& x)
    {
#if _DEBUG
        assert(y < m_height);
        assert(x < m_width);
#endif
        return m_pixels[y * m_width + x];
    }
    const _Type& operator()(const size_t& y, const size_t& x) const
    {
#if _DEBUG
        assert(y < m_height);
        assert(x < m_width);
#endif
        return m_pixels[y * m_width + x];
    }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    _Type* pixels(size_t n = 0) { return &m_pixels[n]; }
    _Type* pixelsXY(size_t x, size_t y) { return &m_pixels[y * width() + x]; }
    _Type* pixelsXY_Safe(size_t x, size_t y)
    {
        if (m_pixels.size() == 0)
            return 0;
        else
            return &m_pixels[y * width() + x];
    }

    _Type* pixelsYX(size_t y, size_t x) { return &m_pixels[y * width() + x]; }
    // row / column
    _Type* pixelsRC(size_t y, size_t x) { return &m_pixels[y * width() + x]; }
    _Type& pixel_ref(size_t n = 0) { return m_pixels[n]; }
    _Type& pixelsXY_ref(size_t x, size_t y) { return m_pixels[y * width() + x]; }
    _Type& pixelsYX_ref(size_t y, size_t x) { return m_pixels[y * width() + x]; }
    // row / column
    _Type& pixelsRC_ref(size_t y, size_t x) { return m_pixels[y * width() + x]; }
    _Type* pixelsXY_wrapped(int x, int y)
    {
        y = mod(y, m_height);
        x = mod(x, m_width);
        return &m_pixels[y * m_width + x];
    }

    nvImage(const nvImage<_Type>& other)
    {
        m_width = other.m_width;
        m_height = other.m_height;
        m_pixels = other.m_pixels;
        m_RGBE = other.m_RGBE;
    }

    nvImage()
    {
        m_width = 0;
        m_height = 0;
        m_RGBE = false;
        m_pixels.clear();
    }

    void clear()
    {
        m_width = 0;
        m_height = 0;
        m_pixels.clear();
    }

    void resize(size_t width, size_t height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;
    }
    void realloc(size_t width, size_t height)
    {
        m_pixels.realloc(width * height);
        m_width = width;
        m_height = height;
    }

    nvImage<_Type>(size_t width, size_t height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;
        m_RGBE = false;
    }

    void SwapRB()
    {
        _Type* p = &m_pixels[0];
        _Type tmp;
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            tmp.r = p->r;
            p->r = p->b;
            p->b = tmp.r;
            ++p;
        }
    }

    void Scale(_Type s)
    {
        _Type* p = &m_pixels[0];
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            *p++ *= s;
        }
    }

    void Bias(_Type b)
    {
        _Type* p = &m_pixels[0];
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            *p++ += b;
        }
    }

    void dot(_Type w)
    {
        _Type* p = &m_pixels[0];
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            p->dot(w);
            ++p;
        }
    }

    void Clamp(_Type low, _Type hi)
    {
        _Type* p = &m_pixels[0];
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            p->Clamp(low, hi);
            ++p;
        }
    }

    void Wrap(_Type low, _Type hi)
    {
        _Type* p = &m_pixels[0];
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            p->Wrap(low, hi);
            ++p;
        }
    }

    void FlipTopToBottom()
    {
        _Type* swap = new _Type[m_width];
        size_t row;
        _Type* end_row;
        _Type* start_row;
        size_t len = sizeof(_Type) * m_width;
        for (row = 0; row < m_height / 2; row++)
        {
            end_row = &m_pixels[m_width * (m_height - row - 1)];
            start_row = &m_pixels[m_width * row];
            // copy row toward end of image into temporary swap buffer
            memcpy(swap, end_row, len);
            // copy row at beginning to row at end
            memcpy(end_row, start_row, len);
            // copy old bytes from row at end (in swap) to row at beginning
            memcpy(start_row, swap, len);
        }
        delete[] swap;
    }

    void SetToZero()
    {
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            pixel_ref(i).SetToZero();
        }
    }
    void SetToZeroDirect()
    {
        for (size_t i = 0; i < m_width * m_height; i++)
        {
            m_pixels[i] = 0;
        }
    }
};

typedef nvImage<rgba_t> RGBAImage;
typedef nvVector<RGBAImage> RGBAImageArray;

class RGBAMipMappedImage
{
private:
    RGBAImageArray mipArray; // array of images, one for each MIP map RGBA

public:
    void resize(size_t width, size_t height, size_t nMIPMaps)
    {
        if (nMIPMaps == 0)
            nMIPMaps = nv::calcMaxMipmap(width, height);
        mipArray.resize(nMIPMaps);
        for (size_t mipLevel = 0; mipLevel < nMIPMaps; mipLevel++)
        {
            RGBAImage& mip = mipArray[mipLevel];
            mip.resize(width, height);
            width = nv::NextMip(width);
            height = nv::NextMip(height);
        }
    }

    RGBAMipMappedImage() {}
    RGBAMipMappedImage(int width, int height, int nMIPMaps) { resize(width, height, nMIPMaps); }
    RGBAImage& operator[](size_t i) { return mipArray[i]; }
    const RGBAImage& operator[](size_t i) const { return mipArray[i]; }
    size_t numMIPMaps() const { return mipArray.size(); }
    void resize(size_t size) { mipArray.resize(size); }
    void realloc(size_t size) { mipArray.realloc(size); }
    size_t width() const
    {
        if (mipArray.size() == 0)
            return 0;
        return mipArray[0].width();
    }

    size_t height() const
    {
        if (mipArray.size() == 0)
            return 0;
        return mipArray[0].height();
    }

    void clear() { mipArray.clear(); }
};

class RGBAMipMappedCubeMap
{
private:
    RGBAMipMappedImage cubeFaces[6]; // array of images, one for each MIP map RGBA

public:
    void resize(size_t width, size_t height, size_t nMIPMaps)
    {
        if (nMIPMaps == 0)
            nMIPMaps = nv::calcMaxMipmap(width, height);
        for (int f = 0; f < 6; f++)
        {
            RGBAMipMappedImage& mipFace = cubeFaces[f];
            mipFace.resize(width, height, nMIPMaps);
        }
    }

    RGBAMipMappedCubeMap() {}
    RGBAMipMappedCubeMap(size_t width, size_t height, size_t nMIPMaps) { resize(width, height, nMIPMaps); }
    RGBAMipMappedImage& operator[](size_t i) { return cubeFaces[i]; }
    const RGBAMipMappedImage& operator[](size_t i) const { return cubeFaces[i]; }
    size_t numMIPMaps() const { return cubeFaces[0].numMIPMaps(); }
    size_t height() const { return cubeFaces[0].height(); }
    size_t width() const { return cubeFaces[0].width(); }
    void clear()
    {
        for (size_t f = 0; f < 6; f++)
        {
            RGBAMipMappedImage& mipFace = cubeFaces[f];
            mipFace.clear();
        }
    }
};

typedef nvVector<RGBAImageArray> RGBAVolume;

class RGBAMipMappedVolumeMap
{
private:
    RGBAVolume volumeArray; // array of MIP mapped images

public:
    void resize(size_t width, size_t height, size_t depth, size_t nMIPMaps)
    {
        if (nMIPMaps == 0)
            nMIPMaps = nv::calcMaxMipmap(width, height, depth);
        volumeArray.resize(nMIPMaps);
        size_t w = width;
        size_t h = height;
        size_t d = depth;
        for (size_t mipLevel = 0; mipLevel < nMIPMaps; mipLevel++)
        {
            RGBAImageArray& volImage = volumeArray[mipLevel];
            volImage.resize(d);
            for (size_t slice = 0; slice < d; slice++)
            {
                RGBAImage& mipFace = volImage[slice];
                mipFace.resize(w, h);
            }
            w = nv::NextMip(w);
            h = nv::NextMip(h);
            d = nv::NextMip(d);
        }
    }

    RGBAMipMappedVolumeMap() {}
    RGBAMipMappedVolumeMap(size_t width, size_t height, size_t depth, size_t nMIPMaps)
    {
        resize(width, height, depth, nMIPMaps);
    }

    // mip level
    RGBAImageArray& operator[](size_t i) { return volumeArray[i]; }
    const RGBAImageArray& operator[](size_t i) const { return volumeArray[i]; }
    size_t numMIPMaps() const { return volumeArray.size(); }
    const RGBAImageArray* vol0() const
    {
        if (numMIPMaps() == 0)
            return 0;
        return &volumeArray[0];
    }

    const RGBAImage* slice0() const
    {
        const RGBAImageArray* v0 = vol0();
        if (v0 == 0)
            return 0;
        if (v0->size() == 0)
            return 0;
        const RGBAImageArray& array = *v0;
        return &array[0];
    }

    size_t width() const
    {
        const RGBAImage* image0 = slice0();
        if (image0 == 0)
            return 0;
        else
            return image0->width();
    }

    size_t height() const
    {
        const RGBAImage* image0 = slice0();
        if (image0 == 0)
            return 0;
        else
            return image0->height();
    }

    size_t depth() const
    {
        const RGBAImageArray* v0 = vol0();
        if (v0 == 0)
            return 0;
        return v0->size();
    }
};

typedef nvMatrix<float> floatImage;
typedef nvMatrix<fpPixel> fpImage;
typedef nvMatrix<fpPixel3> fpImage3;
typedef nvVector<fpImage> fpImageArray;

class fpMipMappedImage
{
    fpImageArray mipArray; // array of images, one for each MIP map RGBA

public:
    fpMipMappedImage() {}
    fpMipMappedImage(size_t width, size_t height, size_t nMIPMaps) { resize(width, height, nMIPMaps); }
    // copy constructor
    fpMipMappedImage(const fpMipMappedImage& v)
    {
        // copy the images over
        mipArray.resize(v.mipArray.size());
        for (size_t mipLevel = 0; mipLevel < numMIPMaps(); mipLevel++)
        {
            fpImage& dst = mipArray[mipLevel];
            const fpImage& src = v.mipArray[mipLevel];
            dst = src;
        }
    }

    void resize(size_t width, size_t height, size_t nMIPMaps)
    {
        if (nMIPMaps == 0)
            nMIPMaps = nv::calcMaxMipmap(width, height);
        mipArray.resize(nMIPMaps);
        for (size_t mipLevel = 0; mipLevel < nMIPMaps; mipLevel++)
        {
            fpImage& mip = mipArray[mipLevel];
            mip.resize(width, height);
            width = nv::NextMip(width);
            height = nv::NextMip(height);
        }
    }

    void FlipTopToBottom()
    {
        // copy the images over
        for (size_t mipLevel = 0; mipLevel < numMIPMaps(); mipLevel++)
        {
            fpImage& mip = mipArray[mipLevel];
            mip.FlipTopToBottom();
        }
    }

    fpImage& operator[](size_t i) { return mipArray[i]; }
    const fpImage& operator[](size_t i) const { return mipArray[i]; }
    void SetToZero()
    {
        for (size_t mipLevel = 0; mipLevel < numMIPMaps(); mipLevel++)
        {
            fpImage& mip = mipArray[mipLevel];
            mip.SetToZero();
        }
    }

    void clear() { mipArray.clear(); }
    void realloc(size_t size) { mipArray.realloc(size); }
    void resize(size_t nMIPLevels) { mipArray.resize(nMIPLevels); }
    size_t numMIPMaps() const { return mipArray.size(); }
    size_t width() const { return mipArray[0].width(); }
    size_t height() const { return mipArray[0].height(); }
};

class fpMipMappedCubeMap
{
private:
    fpMipMappedImage cubeFaces[6]; // array of images, one for each MIP map RGBA

public:
    void resize(size_t width, size_t height, size_t nMIPMaps)
    {
        if (nMIPMaps == 0)
            nMIPMaps = nv::calcMaxMipmap(width, height);
        for (size_t f = 0; f < 6; f++)
        {
            fpMipMappedImage& mipFace = cubeFaces[f];
            mipFace.resize(width, height, nMIPMaps);
        }
    }

    fpMipMappedCubeMap() {}
    fpMipMappedCubeMap(int width, int height, int nMIPMaps) { resize(width, height, nMIPMaps); }
    fpMipMappedImage& operator[](size_t i) { return cubeFaces[i]; }
    const fpMipMappedImage& operator[](size_t i) const { return cubeFaces[i]; }
    size_t numMIPMaps() const { return cubeFaces[0].numMIPMaps(); }
    size_t height() const { return cubeFaces[0].height(); }
    size_t width() const { return cubeFaces[0].width(); }
    void clear()
    {
        for (size_t f = 0; f < 6; f++)
        {
            fpMipMappedImage& mipFace = cubeFaces[f];
            mipFace.clear();
        }
    }

    void FlipTopToBottom()
    {
        for (size_t f = 0; f < 6; f++)
        {
            fpMipMappedImage& mipFace = cubeFaces[f];
            mipFace.FlipTopToBottom();
        }
    }
};

// mip level, array
typedef nvVector<fpImageArray> fpVolume;

class fpMipMappedVolumeMap
{
private:
    // array of MIP mapped images
    fpVolume volumeArray;

public:
    void resize(size_t width, size_t height, size_t depth, size_t nMIPMaps)
    {
        if (nMIPMaps == 0)
            nMIPMaps = nv::calcMaxMipmap(width, height, depth);
        volumeArray.resize(nMIPMaps);
        size_t w = width;
        size_t h = height;
        size_t d = depth;
        for (size_t mipLevel = 0; mipLevel < nMIPMaps; mipLevel++)
        {
            fpImageArray& volImage = volumeArray[mipLevel];
            volImage.resize(d);
            for (size_t slice = 0; slice < d; slice++)
            {
                fpImage& mipFace = volImage[slice];
                mipFace.resize(w, h);
            }
            w = nv::NextMip(w);
            h = nv::NextMip(h);
            d = nv::NextMip(d);
        }
    }

    void FlipTopToBottom()
    {
        for (size_t mipLevel = 0; mipLevel < numMIPMaps(); mipLevel++)
        {
            fpImageArray& volImage = volumeArray[mipLevel];
            for (size_t slice = 0; slice < volImage.size(); slice++)
            {
                fpImage& mipFace = volImage[slice];
                mipFace.FlipTopToBottom();
            }
        }
    }

    void realloc(size_t size) { volumeArray.realloc(size); }
    fpMipMappedVolumeMap() {}
    fpMipMappedVolumeMap(size_t width, size_t height, size_t depth, size_t nMIPMaps)
    {
        resize(width, height, depth, nMIPMaps);
    }

    // mip level
    fpImageArray& operator[](size_t i) { return volumeArray[i]; }
    const fpImageArray& operator[](size_t i) const { return volumeArray[i]; }
    size_t numMIPMaps() const { return volumeArray.size(); }
    const fpImageArray* vol0() const
    {
        if (numMIPMaps() == 0)
            return 0;
        return &volumeArray[0];
    }

    const fpImage* slice0() const
    {
        const fpImageArray* v0 = vol0();
        if (v0 == 0)
            return 0;
        if (v0->size() == 0)
            return 0;
        const fpImageArray& array = *v0;
        return &array[0];
    }

    size_t width() const
    {
        const fpImage* image0 = slice0();
        if (image0 == 0)
            return 0;
        else
            return image0->width();
    }

    size_t height() const
    {
        const fpImage* image0 = slice0();
        if (image0 == 0)
            return 0;
        else
            return image0->height();
    }

    size_t depth() const
    {
        const fpImageArray* v0 = vol0();
        if (v0 == 0)
            return 0;
        return v0->size();
    }

    void clear() { volumeArray.clear(); }
};
#pragma pack(pop)

#include <dds/ConvertColor.h>
