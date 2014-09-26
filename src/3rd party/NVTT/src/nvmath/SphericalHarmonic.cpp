// This code is in the public domain -- castanyo@yahoo.es

#include <nvmath/SphericalHarmonic.h>

using namespace nv;


namespace
{
	
	// Basic integer factorial.
	inline static int factorial( int v )
	{
		if (v == 0) {
			return 1;
		}
	
		int result = v;
		while (--v > 0) {
			result *= v;
		}
		return result;
	}
	
	
	// Double factorial. 
	// Defined as: n!! = n*(n - 2)*(n - 4)..., n!!(0,-1) = 1.
	inline static int doubleFactorial( int x )
	{
		if (x == 0 || x == -1) {
			return 1;
		}
	
		int result = x;
		while ((x -= 2) > 0) {
			result *= x;
		}
	
		return result;
	}	
	
	/// Normalization constant for spherical harmonic.
	/// @param l is the band.
	/// @param m is the argument, in the range [0, m]
	inline static float K( int l, int m )
	{
		nvDebugCheck( m >= 0 );
		return sqrtf(((2 * l + 1) * factorial(l - m)) / (4 * PI * factorial(l + m)));
	}
	
	/// Normalization constant for hemispherical harmonic.
	inline static float HK( int l, int m )
	{
		nvDebugCheck( m >= 0 );
		return sqrtf(((2 * l + 1) * factorial(l - m)) / (2 * PI * factorial(l + m)));
	}

	/// Evaluate Legendre polynomial. */
	static float legendre( int l, int m, float x )
	{
	//	piDebugCheck( m >= 0 );
	//	piDebugCheck( m <= l );
	//	piDebugCheck( fabs(x) <= 1 );
	
		// Rule 2 needs no previous results
		if (l == m) {
			return powf(-1.0f, float(m)) * doubleFactorial(2 * m - 1) * powf(1 - x*x, 0.5f * m);
		}
	
		// Rule 3 requires the result for the same argument of the previous band
		if (l == m + 1) {
			return x * (2 * m + 1) * legendrePolynomial(m, m, x);
		}
	
		// Main reccurence used by rule 1 that uses result of the same argument from
		// the previous two bands
		return (x * (2 * l - 1) * legendrePolynomial(l - 1, m, x) - (l + m - 1) * legendrePolynomial(l - 2, m, x)) / (l - m);
	}
	
	
	template <int l, int m> float legendre(float x);
	
	template <> float legendre<0, 0>(float x) {
		return 1;
	}
	
	template <> float legendre<1, 0>(float x) {
		return x;
	}
	template <> float legendre<1, 1>(float x) {
		return -sqrtf(1 - x * x);
	}
	
	template <> float legendre<2, 0>(float x) {
		return -0.5f + (3 * x * x) / 2;
	}
	template <> float legendre<2, 1>(float x) {
		return -3 * x * sqrtf(1 - x * x);
	}
	template <> float legendre<2, 2>(float x) {
		return -3 * (-1 + x * x);
	}
	
	template <> float legendre<3, 0>(float x) {
		return -(3 * x) / 2 + (5 * x * x * x) / 2;
	}
	template <> float legendre<3, 1>(float x) {
		return -3 * sqrtf(1 - x * x) / 2 * (-1 + 5 * x * x);
	}
	template <> float legendre<3, 2>(float x) {
		return -15 * (-x + x * x * x);
	}
	template <> float legendre<3, 3>(float x) {
		return -15 * powf(1 - x * x, 1.5f);
	}
	
	template <> float legendre<4, 0>(float x) {
		return 0.125f * (3.0f - 30.0f * x * x + 35.0f * x * x * x * x);
	}
	template <> float legendre<4, 1>(float x) {
		return -2.5f * x * sqrtf(1.0f - x * x) * (7.0f * x * x - 3.0f);
	}
	template <> float legendre<4, 2>(float x) {
		return -7.5f * (1.0f - 8.0f * x * x + 7.0f * x * x * x * x);
	}
	template <> float legendre<4, 3>(float x) {
		return -105.0f * x * powf(1 - x * x, 1.5f);
	}
	template <> float legendre<4, 4>(float x) {
		return 105.0f * (x * x - 1.0f) * (x * x - 1.0f);
	}

} // namespace


float nv::legendrePolynomial(int l, int m, float x)
{
	switch(l)
	{
		case 0:
			return legendre<0, 0>(x);
		case 1:
			if(m == 0) return legendre<1, 0>(x);
			return legendre<1, 1>(x);
		case 2:
			if(m == 0) return legendre<2, 0>(x);
			else if(m == 1) return legendre<2, 1>(x);
			return legendre<2, 2>(x);
		case 3:
			if(m == 0) return legendre<3, 0>(x);
			else if(m == 1) return legendre<3, 1>(x);
			else if(m == 2) return legendre<3, 2>(x);
			return legendre<3, 3>(x);
		case 4:
			if(m == 0) return legendre<4, 0>(x);
			else if(m == 1) return legendre<4, 1>(x);
			else if(m == 2) return legendre<4, 2>(x);
			else if(m == 3) return legendre<4, 3>(x);
			else return legendre<4, 4>(x);
	}
	
	// Fallback to the expensive version.
	return legendre(l, m, x);
}


/** 
 * Evaluate the spherical harmonic function for the given angles.
 * @param l is the band.
 * @param m is the argument, in the range [-l,l]
 * @param theta is the altitude, in the range [0, PI]
 * @param phi is the azimuth, in the range [0, 2*PI]
 */
float nv::y( int l, int m, float theta, float phi )
{
	if( m == 0 ) {
		// K(l, 0) = sqrt((2*l+1)/(4*PI))
		return sqrtf((2 * l + 1) / (4 * PI)) * legendrePolynomial(l, 0, cosf(theta));
	}
	else if( m > 0 ) {
		return sqrtf(2.0f) * K(l, m) * cosf(m * phi) * legendrePolynomial(l, m, cosf(theta));
	}
	else {
		return sqrtf(2.0f) * K(l, -m) * sinf(-m * phi) * legendrePolynomial(l, -m, cosf(theta));
	}
}


/**
 * Real spherical harmonic function of an unit vector. Uses the following
 * equalities to call the angular function:
 * x = sin(theta)*cos(phi)
 * y = sin(theta)*sin(phi)
 * z = cos(theta)
 */
float nv::y( int l, int m, Vector3::Arg v )
{
	float theta = acosf(v.z());
	float phi = atan2f(v.y(), v.x());
	return y( l, m, theta, phi );
}


/**
 * Evaluate the hemispherical harmonic function for the given angles.
 * @param l is the band.
 * @param m is the argument, in the range [-l,l]
 * @param theta is the altitude, in the range [0, PI/2]
 * @param phi is the azimuth, in the range [0, 2*PI]
 */
float nv::hy( int l, int m, float theta, float phi )
{
	if( m == 0 ) {
		// HK(l, 0) = sqrt((2*l+1)/(2*PI))
		return sqrtf((2 * l + 1) / (2 * PI)) * legendrePolynomial(l, 0, 2*cosf(theta)-1);
	}
	else if( m > 0 ) {
		return sqrtf(2.0f) * HK(l, m) * cosf(m * phi) * legendrePolynomial(l, m, 2*cosf(theta)-1);
	}
	else {
		return sqrtf(2.0f) * HK(l, -m) * sinf(-m * phi) * legendrePolynomial(l, -m, 2*cosf(theta)-1);
	}
}


/**
 * Real hemispherical harmonic function of an unit vector. Uses the following
 * equalities to call the angular function:
 * x = sin(theta)*cos(phi)
 * y = sin(theta)*sin(phi)
 * z = cos(theta)
 */
float nv::hy( int l, int m, Vector3::Arg v )
{
	float theta = acosf(v.z());
	float phi = atan2f(v.y(), v.x());
	return y( l, m, theta, phi );
}



