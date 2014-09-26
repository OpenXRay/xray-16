/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#ifndef SQUISH_SIMD_3DNOW_H
#define SQUISH_SIMD_3DNOW_H

//#include <xmmintrin.h>
#include <mm3dnow.h>
#include <cassert>

//#define SQUISH_SSE_SPLAT( a )										\
	( ( a ) | ( ( a ) << 2 ) | ( ( a ) << 4 ) | ( ( a ) << 6 ) )

namespace squish {

//#define VEC4_CONST( X ) Vec4( _mm_set1_ps( X ) )

class Vec4
{
public:
	typedef Vec4 const& Arg;

	Vec4() {}
		
	Vec4( __m64 v0, __m64 v1 ) : m_v0( v0 ), m_v1( v1 ) {}
	
	Vec4( Vec4 const& arg ) : m_v0( arg.m_v0 ), m_v1( arg.m_v1 ) {}
	
	Vec4& operator=( Vec4 const& arg )
	{
		m_v0 = arg.m_v0;
		m_v1 = arg.m_v1;
		return *this;
	}
	
	Vec4( float x, float y, float z, float w )
	{
		m_v0 = _mm_set_pi32( *(int *)&x, *(int *)&y );
		m_v1 = _mm_set_pi32( *(int *)&z, *(int *)&w );
	}

/*	Vec3 GetVec3() const
	{
#ifdef __GNUC__
		__attribute__ ((__aligned__ (16))) float c[4];
#else
		__declspec(align(16)) float c[4];
#endif
		//_mm_store_ps( c, m_v );
		return Vec3( c[0], c[1], c[2] );
	}
*/	
//	Vec4 SplatX() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 0 ) ) ); }
//	Vec4 SplatY() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 1 ) ) ); }
//	Vec4 SplatZ() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 2 ) ) ); }
//	Vec4 SplatW() const { return Vec4( _mm_shuffle_ps( m_v, m_v, SQUISH_SSE_SPLAT( 3 ) ) ); }

	Vec4& operator+=( Arg v )
	{
		m_v0 = _m_pfadd( m_v0, v.m_v0 );
		m_v1 = _m_pfadd( m_v1, v.m_v1 );
		return *this;
	}
	
	Vec4& operator-=( Arg v )
	{
		m_v0 = _m_pfsub( m_v0, v.m_v0 );
		m_v1 = _m_pfsub( m_v1, v.m_v1 );
		return *this;
	}
	
	Vec4& operator*=( Arg v )
	{
		m_v0 = _m_pfmul( m_v0, v.m_v0 );
		m_v1 = _m_pfmul( m_v1, v.m_v1 );
		return *this;
	}
	
	friend Vec4 operator+( Vec4::Arg left, Vec4::Arg right  )
	{
		return Vec4(
				_m_pfadd( left.m_v0, right.m_v0 ),
				_m_pfadd( left.m_v1, right.m_v1 ));
	}
	
	friend Vec4 operator-( Vec4::Arg left, Vec4::Arg right  )
	{
		return Vec4(
				_m_pfsub( left.m_v0, right.m_v0 ),
				_m_pfsub( left.m_v1, right.m_v1 ));
	}
	
	friend Vec4 operator*( Vec4::Arg left, Vec4::Arg right  )
	{
		return Vec4(
				_m_pfmul( left.m_v0, right.m_v0 ),
				_m_pfmul( left.m_v1, right.m_v1 ));
	}
	
	//! Returns a*b + c
	friend Vec4 MultiplyAdd( Vec4::Arg a, Vec4::Arg b, Vec4::Arg c )
	{
		return Vec4(
				_m_pfadd( _m_pfmul( a.m_v0, b.m_v0 ), c.m_v0 ),
				_m_pfadd( _m_pfmul( a.m_v1, b.m_v1 ), c.m_v1 ));
	}
	
	//! Returns -( a*b - c )
	friend Vec4 NegativeMultiplySubtract( Vec4::Arg a, Vec4::Arg b, Vec4::Arg c )
	{
		return Vec4(
				_m_pfsub( c.m_v0, _m_pfmul( a.m_v0, b.m_v0 ) ),
				_m_pfsub( c.m_v1, _m_pfmul( a.m_v1, b.m_v1 ) ));
	}
	
	friend Vec4 Reciprocal( Vec4::Arg v )
	{
		// get the reciprocal estimate
		__m64 x0 = _m_pfrcp(v.m_v0);
		__m64 y1 = _m_pfrcp(v.m_v1);

		// Newton-Rhaphson refinement
		__m64 x1 = _m_pfrcpit1(v.m_v0, x0);
		__m64 y1 = _m_pfrcpit1(v.m_v1, y0);

		__m64 x2 = _m_pfrcpit2(x1, x0);
		__m64 y2 = _m_pfrcpit2(y1, y0);

		return Vec4(x2, y2);
	}
	
	friend Vec4 Min( Vec4::Arg left, Vec4::Arg right )
	{
		return Vec4(
				_m_pfmin( left.m_v0, right.m_v0 ),
				_m_pfmin( left.m_v1, right.m_v1 ));
	}
	
	friend Vec4 Max( Vec4::Arg left, Vec4::Arg right )
	{
		return Vec4(
				_m_pfmax( left.m_v0, right.m_v0 ),
				_m_pfmax( left.m_v1, right.m_v1 ));
	}
	
	friend Vec4 Truncate( Vec4::Arg v )
	{
		// convert to ints
		__m64 i0 = _m_pf2id( v.m_v0 );
		__m64 i1 = _m_pf2id( v.m_v1 );

		// convert to floats
		__m64 f0 = _m_pi2fd( i0 );
		__m64 f1 = _m_pi2fd( i1 );
		
		// clear out the MMX multimedia state to allow FP calls later
		//_m_femms();
		
		return Vec4( f0, f1 );
	}
	
	friend Vec4 CompareEqual( Vec4::Arg left, Vec4::Arg right )
	{
		return Vec4(
				_m_pfcmpeq( left.m_v0, right.m_v0 ),
				_m_pfcmpeq( left.m_v1, right.m_v1 ));
	}
/*	
	friend Vec4 Select( Vec4::Arg off, Vec4::Arg on, Vec4::Arg bits )
	{
        __m128 a = _mm_andnot_ps( bits.m_v, off.m_v );
        __m128 b = _mm_and_ps( bits.m_v, on.m_v );

        return Vec4( _mm_or_ps( a, b ) );
	}
*//*
	friend bool CompareAnyLessThan( Vec4::Arg left, Vec4::Arg right ) 
	{
		__m128 bits = _mm_cmplt_ps( left.m_v, right.m_v );
		int value = _mm_movemask_ps( bits );
		return value != 0;
	}
*/
private:
	__m64 m_v0;
	__m64 m_v1;
};

} // namespace squish

#endif // ndef SQUISH_SIMD_3DNOW_H
