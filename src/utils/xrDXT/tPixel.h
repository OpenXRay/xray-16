#ifndef TPIXEL_H
#define TPIXEL_H

#include <math.h>
#include <memory.h>
#include "tVector.h"


#pragma pack(push,4)

// rad and green
class u16v16_t	
{
public:

    union
    {
        unsigned short rg[4];
        struct
        {
            unsigned short r;
            unsigned short g;
        };
    };
    u16v16_t & operator += ( const u16v16_t & v );     // incrementation by a Vec4f


    void set(unsigned short _r, unsigned short _g)
    {
        r = _r;
        g = _g;
    }
};

class r12g12b8_t	
{
public:

    union
    {
        
        struct
        {
            unsigned long r:12;
            unsigned long g:12;
            unsigned long b:8;
        };
    };
    r12g12b8_t & operator += ( const r12g12b8_t& v );     // incrementation by a Vec4f


    void set(unsigned long _r, unsigned long _g, unsigned long _b)
    {
        r = _r;
        g = _g;
        b = _b;
    }
};
inline int iClamp(int a, int lo, int hi)
{
    if (a < lo)
        a = lo;
    if (a > hi)
        a = hi;
    return a;
}

inline float fClamp(float a, float lo, float hi)
{
    if (a < lo)
        a = lo;
    if (a > hi)
        a = hi;
    return a;
}



class rgba_t	
{
public:

    union
    {
        unsigned long u;
        unsigned char rgba[4];
        struct
        {
            unsigned char r;
            unsigned char g;
            unsigned char b;
            unsigned char a;
        };
    };

    rgba_t() 
    {
    }
    rgba_t(unsigned char _r, unsigned char _g, unsigned char _b,unsigned char _a) 
    {
        a = _a; 
        r = _r; 
        g = _g;
        b = _b;
    }

    rgba_t & operator += ( const rgba_t& v )     // incrementation by a rgba_t
    {
        r = iClamp((int)r + (int)v.r, 0, 255);   
        g = iClamp((int)g + (int)v.g, 0, 255);   
        b = iClamp((int)b + (int)v.b, 0, 255);   
        a = iClamp((int)a + (int)v.a, 0, 255);   

        return *this;
    }

    rgba_t & operator -= ( const rgba_t& v );     // decrementation by a rgba_t
    rgba_t & operator *= ( const float d );     // multiplication by a constant
    rgba_t & operator /= ( const float d );     // division by a constant


    rgba_t& operator = (const rgba_t& v)
    { 
        r = v.r; 
        g = v.g; 
        b = v.b; 
        a = v.a; 
        return *this; 
    }

    friend rgba_t operator + (const rgba_t & v1, const rgba_t& v2)
    {

        int r,g,b,a;
        r = iClamp((int)v1.r + (int)v2.r, 0, 255);   
        g = iClamp((int)v1.g + (int)v2.g, 0, 255);   
        b = iClamp((int)v1.b + (int)v2.b, 0, 255);   
        a = iClamp((int)v1.a + (int)v2.a, 0, 255);  

        return rgba_t(r, g, b, a);
    }

    friend rgba_t operator / (const rgba_t& v, float s)
    { 
        return rgba_t(v.r/s, v.g/s, v.b/s, v.a/s);
    }

    friend rgba_t operator / (const rgba_t& v, int s)
    {
        return rgba_t(v.r/s, v.g/s, v.b/s, v.a/s);
    }

    void set(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
};

class rgba16_t	
{
public:

    union
    {
        //unsigned __int64 u;
        unsigned short rgba[4];
        struct
        {
            unsigned short r;
            unsigned short g;
            unsigned short b;
            unsigned short a;
        };
    };

    rgba16_t() 
    {
    }
    rgba16_t(unsigned short _r, unsigned short _g, unsigned short _b,unsigned short _a) 
    {
        a = _a; 
        r = _r; 
        g = _g;
        b = _b;
    }

    rgba16_t & operator += ( const rgba16_t& v )     // incrementation by a rgba_t
    {
        r = iClamp((int)r + (int)v.r, 0, 65535);   
        g = iClamp((int)g + (int)v.g, 0, 65535);   
        b = iClamp((int)b + (int)v.b, 0, 65535);   
        a = iClamp((int)a + (int)v.a, 0, 65535);   

        return *this;
    }

    rgba16_t & operator -= ( const rgba16_t& v );     // decrementation by a rgba_t
    rgba16_t & operator *= ( const float d );     // multiplication by a constant
    rgba16_t & operator /= ( const float d );     // division by a constant


    rgba16_t& operator = (const rgba16_t& v)
    { 
        r = v.r; 
        g = v.g; 
        b = v.b; 
        a = v.a; 
        return *this; 
    }

    friend rgba16_t operator + (const rgba16_t & v1, const rgba16_t& v2)
    {

        int r,g,b,a;
        r = iClamp((int)v1.r + (int)v2.r, 0, 65535);   
        g = iClamp((int)v1.g + (int)v2.g, 0, 65535);   
        b = iClamp((int)v1.b + (int)v2.b, 0, 65535);   
        a = iClamp((int)v1.a + (int)v2.a, 0, 65535);  

        return rgba16_t(r, g, b, a);
    }

    friend rgba16_t operator / (const rgba16_t& v, float s)
    {
        return rgba16_t(v.r/s, v.g/s, v.b/s, v.a/s);
    }

    friend rgba16_t operator / (const rgba16_t& v, int s)
    {
        return rgba16_t(v.r/s, v.g/s, v.b/s, v.a/s);
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
    urgba_t & operator += ( const urgba_t& v );     // incrementation by a Vec4f


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
    q8w8v8u8_t & operator += ( const q8w8v8u8_t& v );     // incrementation by a Vec4f


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
    }          // copy constructor

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

    fpPixel & operator += ( const fpPixel& v );     // incrementation by a Vec4f

    fpPixel & operator = ( const fpPixel& v );      // assignment of a Vec3f         
    fpPixel & operator -= ( const fpPixel& v );     // decrementation by a Vec3f
    fpPixel & operator *= ( const float d );     // multiplication by a constant
    fpPixel & operator /= ( const float d );     // division by a constant



    friend fpPixel operator + (const fpPixel& v1, const fpPixel& v2)
    {
        return fpPixel(v1.r + v2.r, v1.g + v2.g, v1.b + v2.b, v1.a + v2.a);
    }

    friend fpPixel operator / (const fpPixel& v, float s)
    {
        return fpPixel(v.r/s, v.g/s, v.b/s, v.a/s);

    }
    friend int operator == (const fpPixel& v1, const fpPixel& v2);      // v1 == v2 ?

    int normalize()
    {
        double u;
        u = x * x + y * y + z * z;

        if ( fabs(u - 1.0) < 1e-12)
            return 0; // already normalized

        if ( fabs((double)u) < 1e-12)
        {
            x = y = z = 0.0;
            return -1;
        }


        u = 1.0 / sqrt(u);


        x *= u;
        y *= u;
        z *= u;

        return 0;
    }


};

inline int operator == (const fpPixel& v1, const fpPixel& v2)
{
    return 
        v1.a == v2.a && 
        v1.r == v2.r && 
        v1.b == v2.g && 
        v1.g == v2.b;
}

inline fpPixel& fpPixel::operator = (const fpPixel& v)
{ 
    a = v.a; 
    r = v.r; 
    g = v.g; 
    b = v.b; 
    return *this; 
}




class CNormalData
{
public:
    CNormalData()
    {
        normals = 0;
    }

     
    ~CNormalData()
    {
        if (normals)
        {
            delete [] normals;
            normals = 0;
        }
    }
    int width;
    int height;

    fpPixel * normals;

}; 




template <class _type>
class CImage 
{
    int m_width;
    int m_height;
    nvVector<_type> m_pixels;

public:
    
    CImage < _type > & operator = ( const CImage < _type >& v ) 
    {

        // resize and copy over
        resize(v.width(), v.height());

        m_pixels = v.m_pixels;

        return *this; 
    }


    _type& operator [] ( int i) 
    {
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i]; 
    };  
    
    const _type& operator[](int i) const 
    { 
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i];
    }

    int nPlanesInFile;

    int width() const
    {
        return m_width;

    }

    int height() const
    {
        return m_height;

    }



    _type * pixels()
    {
        int s = m_pixels.size();
        return &m_pixels[0];
    }

    CImage()
    {
        m_width = 0;
        m_height = 0;

        nPlanesInFile = -1;

        m_pixels.clear();
        
    };
   ~CImage()
    {
 
    }
    void clear()
    {
        m_width = 0;
        m_height = 0;
        m_pixels.clear();
    }

    void resize(int width, int height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;

    }

    CImage<_type>(int width, int height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;
        nPlanesInFile = -1;

    };


    void SwapRB()
    {
		
        _type * p = &m_pixels[0];
		for(int i=0; i < m_width * m_height; i++ );
		{

            int r = p->r;
            p->r = p->b;
            p->b = r;
		} 
    }


    void	FlipTopToBottom()
    {

        _type * swap = new _type[ m_width];

        unsigned long row;

        _type * end_row;
        _type * start_row;

        int len = sizeof(_type) * m_width;

        for( row = 0; row < m_height / 2; row ++ )
        {
            end_row =   &m_pixels[ m_width * ( m_height - row - 1) ];
            start_row = &m_pixels[ m_width * row ];

            // copy row toward end of image into temporary swap buffer
            memcpy( swap, end_row, len );

            // copy row at beginning to row at end
            memcpy( end_row, start_row, len );

            // copy old bytes from row at end (in swap) to row at beginning
            memcpy( start_row, swap, len );
        }

        delete [] swap;
    }


 
};



class RGBAImage 
{
    int m_width;
    int m_height;
    nvVector<rgba_t> m_pixels;

public:
    
    RGBAImage & operator = ( const RGBAImage& v ) 
    {

        // resize and copy over
        resize(v.width(), v.height());

        m_pixels = v.m_pixels;

        return *this; 
    }


    rgba_t& operator [] ( int i) 
    {
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i]; 
    };  
    
    const rgba_t& operator[](int i) const 
    { 
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i];
    }

    int nPlanesInFile;

    int width() const
    {
        return m_width;

    }

    int height() const
    {
        return m_height;

    }



    rgba_t * pixels()
    {
        int s = m_pixels.size();
        return &m_pixels[0];
    }

    RGBAImage()
    {
        m_width = 0;
        m_height = 0;

        nPlanesInFile = -1;

        m_pixels.clear();
        
    };
   ~RGBAImage()
    {
 
    }
    void clear()
    {
        m_width = 0;
        m_height = 0;
        m_pixels.clear();
    }

    void resize(int width, int height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;

    }

    RGBAImage(int width, int height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;
        nPlanesInFile = -1;

    };


    void SwapRB()
    {
		
        rgba_t * p = &m_pixels[0];
		for(int i=0; i < m_width * m_height; i++ );
		{

            int r = p->r;
            p->r = p->b;
            p->b = r;
		} 
    }


    void	FlipTopToBottom()
    {

        rgba_t * swap = new rgba_t[ m_width];

        unsigned long row;

        rgba_t * end_row;
        rgba_t * start_row;

        int len = sizeof(rgba_t) * m_width;

        for( row = 0; row < m_height / 2; row ++ )
        {
            end_row =   &m_pixels[ m_width * ( m_height - row - 1) ];
            start_row = &m_pixels[ m_width * row ];

            // copy row toward end of image into temporary swap buffer
            memcpy( swap, end_row, len );

            // copy row at beginning to row at end
            memcpy( end_row, start_row, len );

            // copy old bytes from row at end (in swap) to row at beginning
            memcpy( start_row, swap, len );
        }

        delete [] swap;
    }


 
};




class fpImage 
{
    int m_width;
    int m_height;
    nvVector<fpPixel> m_pixels;

public:
    fpImage & operator = ( const fpImage& v )
    {

        // resize and copy over
        resize(v.width(), v.height());

        
        m_pixels = v.m_pixels;
        return *this; 
    }



    fpPixel& operator [] ( int i) 
    {
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i]; 
    };  
    
    const fpPixel& operator[](int i) const 
    { 
#ifdef _DEBUG
        assert(i < m_width * m_height);
#endif
        return m_pixels[i];
    }


    int nPlanesInFile;

    int width() const
    {
        return m_width;

    }

    int height() const
    {
        return m_height;

    }



    fpPixel * pixels()
    {
        return &m_pixels[0];
    }

    fpImage()
    {
        m_width = 0;
        m_height = 0;
        nPlanesInFile = 0;
        m_pixels.clear();

        
    };
   ~fpImage()
    {
 
    }

    void zeroize()
    {
        fpPixel * p = &m_pixels[0];
		for(int i=0; i < m_width * m_height; i++ );
		{
            p->r = 0;
            p->g = 0;
            p->b = 0;
            p->a = 0;
		} 

    }

    void resize(int width, int height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;
    }

    fpImage(int width, int height)
    {
        m_pixels.resize(width * height);
        m_width = width;
        m_height = height;
        nPlanesInFile = -1;
    };


    void SwapRB()
    {
		
        fpPixel * p = &m_pixels[0];
		for(int i=0; i < m_width * m_height; i++ );
		{

            int r = p->r;
            p->r = p->b;
            p->b = r;
		} 
    }


    void	FlipTopToBottom()
    {

        fpPixel * swap = new fpPixel[ m_width];

        unsigned long row;

        fpPixel * end_row;
        fpPixel * start_row;

        int len = sizeof(rgba_t) * m_width;

        for( row = 0; row < m_height / 2; row ++ )
        {
            end_row =   &m_pixels[ m_width * ( m_height - row - 1) ];
            start_row = &m_pixels[ m_width * row ];

            // copy row toward end of image into temporary swap buffer
            memcpy( swap, end_row, len );

            // copy row at beginning to row at end
            memcpy( end_row, start_row, len );

            // copy old bytes from row at end (in swap) to row at beginning
            memcpy( start_row, swap, len );
        }

        delete [] swap;
    }

};




#pragma pack(pop)



#include "ColorConvert.h"


#endif