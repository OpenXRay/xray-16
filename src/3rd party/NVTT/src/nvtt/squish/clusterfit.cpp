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
   
#include "clusterfit.h"
#include "colourset.h"
#include "colourblock.h"
#include <cfloat>

namespace squish {

ClusterFit::ClusterFit()
{
}

void ClusterFit::SetColourSet( ColourSet const* colours, int flags )
{
	ColourFit::SetColourSet( colours, flags );

	// initialise the best error
#if SQUISH_USE_SIMD
	m_besterror = VEC4_CONST( FLT_MAX );
	Vec3 metric = m_metric.GetVec3();
#else
	m_besterror = FLT_MAX;
	Vec3 metric = m_metric;
#endif

	// cache some values
	int const count = m_colours->GetCount();
	Vec3 const* values = m_colours->GetPoints();
	
	// get the covariance matrix
	Sym3x3 covariance = ComputeWeightedCovariance( count, values, m_colours->GetWeights(), metric );
	
	// compute the principle component
	Vec3 principle = ComputePrincipleComponent( covariance );

	// build the list of values
	float dps[16];
	for( int i = 0; i < count; ++i )
	{
		dps[i] = Dot( values[i], principle );
		m_order[i] = i;
	}
	
	// stable sort
	for( int i = 0; i < count; ++i )
	{
		for( int j = i; j > 0 && dps[j] < dps[j - 1]; --j )
		{
			std::swap( dps[j], dps[j - 1] );
			std::swap( m_order[j], m_order[j - 1] );
		}
	}

	// weight all the points
#if SQUISH_USE_SIMD
	Vec4 const* unweighted = m_colours->GetPointsSimd();
	Vec4 const* weights = m_colours->GetWeightsSimd();
	m_xxsum = VEC4_CONST( 0.0f );
#else
	Vec3 const* unweighted = m_colours->GetPoints();
	float const* weights = m_colours->GetWeights();
	m_xxsum = Vec3( 0.0f );
#endif
	for( int i = 0; i < count; ++i )
	{
		int p = m_order[i];
		m_unweighted[i] = unweighted[p];
		m_weights[i] = weights[p];
		m_weighted[i] = weights[p]*unweighted[p];
		m_xxsum += m_weighted[i]*m_weighted[i];
	}
}


void ClusterFit::SetMetric(float r, float g, float b)
{
#if SQUISH_USE_SIMD
	m_metric = Vec4(r, g, b, 0);
#else
	m_metric = Vec3(r, g, b);
#endif
	m_metricSqr = m_metric * m_metric;
}

float ClusterFit::GetBestError() const
{
#if SQUISH_USE_SIMD
	return m_besterror.GetVec3().X();
#else
	return m_besterror;
#endif
}


void ClusterFit::Compress3( void* block )
{
	// declare variables
	int const count = m_colours->GetCount();
#if SQUISH_USE_SIMD
	Vec4 beststart = VEC4_CONST( 0.0f );
	Vec4 bestend = VEC4_CONST( 0.0f );
	Vec4 besterror = VEC4_CONST( FLT_MAX );
	Vec4 const half = VEC4_CONST( 0.5f );
	Vec4 const zero = VEC4_CONST( 0.0f );
#else
	Vec3 beststart( 0.0f );
	Vec3 bestend( 0.0f );
	float besterror = FLT_MAX;
	float const half = 0.5f;
	float const zero = 0.0f;
#endif	

	// check all possible clusters for this total order
	u8 indices[16];
	u8 bestindices[16];
	
	// first cluster [0,i) is at the start
	for( int m = 0; m < count; ++m )
	{
		indices[m] = 0;
		m_alpha[m] = m_weights[m];
		m_beta[m] = zero;
	}
	for( int i = count; i >= 0; --i )
	{
		// second cluster [i,j) is half along
		for( int m = i; m < count; ++m )
		{
			indices[m] = 2;
			m_alpha[m] = m_beta[m] = half*m_weights[m];
		}		
		for( int j = count; j > i; --j )
		{
			// last cluster [j,k) is at the end
			if( j < count )
			{
				indices[j] = 1;
				m_alpha[j] = zero;
				m_beta[j] = m_weights[j];
			}		
			
			// solve a least squares problem to place the endpoints
#if SQUISH_USE_SIMD
			Vec4 start, end;
			Vec4 error = SolveLeastSquares( start, end );
#else
			Vec3 start, end;
			float error = SolveLeastSquares( start, end );
#endif

			// keep the solution if it wins
#if SQUISH_USE_SIMD
			if( CompareAnyLessThan( error, besterror ) )
#else
			if( error < besterror )
#endif
			{
				beststart = start;
				bestend = end;
				for( int m = 0; m < 16; ++m )	// TODO: make this faster?
					bestindices[m] = indices[m];
				besterror = error;
			}
		}
	}
	
	// save the block if necessary
#if SQUISH_USE_SIMD
	if( CompareAnyLessThan( besterror, m_besterror ) )
#else
	if( besterror < m_besterror )
#endif
	{
		// remap the indices
		u8 unordered[16];
		for( int i = 0; i < count; ++i )
			unordered[m_order[i]] = bestindices[i];
		m_colours->RemapIndices( unordered, bestindices );
		
		// save the block
#if SQUISH_USE_SIMD
		WriteColourBlock3( beststart.GetVec3(), bestend.GetVec3(), bestindices, block );
#else
		WriteColourBlock3( beststart, bestend, bestindices, block );
#endif

		// save the error
		m_besterror = besterror;
	}
}

//static int run = 0;
//static bool debug = false;

void ClusterFit::Compress4( void* block )
{
	//debug = (run == 1);
	//run++;

	// declare variables
	int const count = m_colours->GetCount();
#if SQUISH_USE_SIMD
	Vec4 beststart = VEC4_CONST( 0.0f );
	Vec4 bestend = VEC4_CONST( 0.0f );
	Vec4 besterror = m_besterror;
	Vec4 const twothirds = VEC4_CONST( 2.0f/3.0f );
	Vec4 const onethird = VEC4_CONST( 1.0f/3.0f );
	Vec4 const zero = VEC4_CONST( 0.0f );
#else
	Vec3 beststart( 0.0f );
	Vec3 bestend( 0.0f );
	float besterror = m_besterror;
	float const twothirds = 2.0f/3.0f;
	float const onethird = 1.0f/3.0f;
	float const zero = 0.0f;
#endif

	// check all possible clusters for this total order
	u8 indices[16];
	u8 bestindices[16];
	
	// first cluster [0,i) is at the start
	for( int m = 0; m < count; ++m )
	{
		indices[m] = 0;
		m_alpha[m] = m_weights[m];
		m_beta[m] = zero;
	}
	for( int i = count; i >= 0; --i )
	{
		// second cluster [i,j) is one third along
		for( int m = i; m < count; ++m )
		{
			indices[m] = 2;
			m_alpha[m] = twothirds*m_weights[m];
			m_beta[m] = onethird*m_weights[m];
		}		
		for( int j = count; j >= i; --j )
		{
			// third cluster [j,k) is two thirds along
			for( int m = j; m < count; ++m )
			{
				indices[m] = 3;
				m_alpha[m] = onethird*m_weights[m];
				m_beta[m] = twothirds*m_weights[m];
			}		
			for( int k = count; k >= j; --k )
			{
				if (j + k == 0) continue;
				
				// last cluster [k,n) is at the end
				if( k < count )
				{
					indices[k] = 1;
					m_alpha[k] = zero;
					m_beta[k] = m_weights[k];
				}

				/*unsigned int permutation = 0;
				for(int p = 0; p < 16; p++) {
					permutation |= indices[p] << (p * 2);
				}
				if (debug) printf("%X:\t", permutation);

				if (debug && permutation == 0x55FFFFAA) __debugbreak();
				*/

				// solve a least squares problem to place the endpoints
#if SQUISH_USE_SIMD
				Vec4 start, end;
				Vec4 error = SolveLeastSquares( start, end );
#else
				Vec3 start, end;
				float error = SolveLeastSquares( start, end );
#endif

				// keep the solution if it wins
#if SQUISH_USE_SIMD
				if( CompareAnyLessThan( error, besterror ) )
#else
				if( error < besterror )
#endif
				{
					beststart = start;
					bestend = end;
					for( int m = 0; m < 16; ++m )	// TODO: make this faster?
						bestindices[m] = indices[m];	
					besterror = error;
				}
			}
		}
	}

	// save the block if necessary
#if SQUISH_USE_SIMD
	if( CompareAnyLessThan( besterror, m_besterror ) )
#else
	if( besterror < m_besterror )
#endif
	{
		// remap the indices
		u8 unordered[16];
		for( int i = 0; i < count; ++i )
			unordered[m_order[i]] = bestindices[i];
		m_colours->RemapIndices( unordered, bestindices );
		
		// save the block
#if SQUISH_USE_SIMD
		WriteColourBlock4( beststart.GetVec3(), bestend.GetVec3(), bestindices, block );
#else
		WriteColourBlock4( beststart, bestend, bestindices, block );
#endif

		// save the error
		m_besterror = besterror;
	}
}

#if SQUISH_USE_SIMD
Vec4 ClusterFit::SolveLeastSquares( Vec4& start, Vec4& end ) const
{
	// accumulate all the quantities we need
	int const count = m_colours->GetCount();
	Vec4 alpha2_sum = VEC4_CONST( 0.0f );
	Vec4 beta2_sum = VEC4_CONST( 0.0f );
	Vec4 alphabeta_sum = VEC4_CONST( 0.0f );
	Vec4 alphax_sum = VEC4_CONST( 0.0f );
	Vec4 betax_sum = VEC4_CONST( 0.0f );
	for( int i = 0; i < count; ++i )
	{
		Vec4 alpha = m_alpha[i];
		Vec4 beta = m_beta[i];
		Vec4 x = m_weighted[i];
	
		alpha2_sum = MultiplyAdd( alpha, alpha, alpha2_sum );
		beta2_sum = MultiplyAdd( beta, beta, beta2_sum );
		alphabeta_sum = MultiplyAdd( alpha, beta, alphabeta_sum );
		alphax_sum = MultiplyAdd( alpha, x, alphax_sum );
		betax_sum = MultiplyAdd( beta, x, betax_sum );	
	}

	// select the results
	Vec4 const zero = VEC4_CONST( 0.0f );
	Vec4 beta2_sum_zero = CompareEqual( beta2_sum, zero );
	Vec4 alpha2_sum_zero = CompareEqual( alpha2_sum, zero );
	
	Vec4 a1 = alphax_sum*Reciprocal( alpha2_sum );
	Vec4 b1 = betax_sum*Reciprocal( beta2_sum );
	
	Vec4 factor = Reciprocal( NegativeMultiplySubtract( 
		alphabeta_sum, alphabeta_sum, alpha2_sum*beta2_sum 
	) );
	Vec4 a2 = NegativeMultiplySubtract( 
		betax_sum, alphabeta_sum, alphax_sum*beta2_sum
	)*factor;
	Vec4 b2 = NegativeMultiplySubtract(
		alphax_sum, alphabeta_sum, betax_sum*alpha2_sum
	)*factor;
	
	Vec4 a = Select( Select( a2, a1, beta2_sum_zero ), zero, alpha2_sum_zero );
	Vec4 b = Select( Select( b2, b1, alpha2_sum_zero ), zero, beta2_sum_zero );

	// clamp the output to [0, 1]
	Vec4 const one = VEC4_CONST( 1.0f );
	Vec4 const half = VEC4_CONST( 0.5f );
	a = Min( one, Max( zero, a ) );
	b = Min( one, Max( zero, b ) );

	// clamp to the grid
	Vec4 const grid( 31.0f, 63.0f, 31.0f, 0.0f );
//	Vec4 const gridrcp( 1.0f/31.0f, 1.0f/63.0f, 1.0f/31.0f, 0.0f );
	Vec4 const gridrcp( 0.03227752766457f, 0.01583151765563f, 0.03227752766457f, 0.0f ); // IC: use approximate grid fitting.
	Vec4 const onethird = VEC4_CONST( 1.0f/3.0f );
	Vec4 const twothirds = VEC4_CONST( 2.0f/3.0f );
	a = Truncate( MultiplyAdd( grid, a, half ) )*gridrcp;
	b = Truncate( MultiplyAdd( grid, b, half ) )*gridrcp;

	// compute the error
	Vec4 const two = VEC4_CONST( 2.0 );
	Vec4 e1 = MultiplyAdd( b*b, beta2_sum, m_xxsum );
	Vec4 e2 = MultiplyAdd( a, alphax_sum, b*betax_sum );
	Vec4 e3 = MultiplyAdd( a*a, alpha2_sum, e1 );
	Vec4 e4 = MultiplyAdd( a*b*alphabeta_sum - e2, two, e3 );

	// apply the metric to the error term
	Vec4 e5 = e4*m_metricSqr;
	Vec4 error = e5.SplatX() + e5.SplatY() + e5.SplatZ();
	
	// save the start and end
	start = a;
	end = b;
	return error;
}
#else
float ClusterFit::SolveLeastSquares( Vec3& start, Vec3& end ) const
{
	// accumulate all the quantities we need
	int const count = m_colours->GetCount();
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	Vec3 alphax_sum( 0.0f );
	Vec3 betax_sum( 0.0f );	
	for( int i = 0; i < count; ++i )
	{
		float alpha = m_alpha[i];
		float beta = m_beta[i];
		Vec3 const& x = m_weighted[i];
		
		alpha2_sum += alpha*alpha;
		beta2_sum += beta*beta;
		alphabeta_sum += alpha*beta;
		alphax_sum += alpha*x;
		betax_sum += beta*x;
	}

	//if (debug) printf("%f %f %f", alpha2_sum, beta2_sum, alphabeta_sum);

	// zero where non-determinate
	Vec3 a, b;
	if( beta2_sum == 0.0f )
	{
		a = alphax_sum/alpha2_sum;
		b = Vec3( 0.0f );
	}
	else if( alpha2_sum == 0.0f )
	{
		a = Vec3( 0.0f );
		b = betax_sum/beta2_sum;
	}
	else
	{
		float factor = 1.0f/( alpha2_sum*beta2_sum - alphabeta_sum*alphabeta_sum );
		
		a = ( alphax_sum*beta2_sum - betax_sum*alphabeta_sum )*factor;
		b = ( betax_sum*alpha2_sum - alphax_sum*alphabeta_sum )*factor;
	}
	
	// clamp the output to [0, 1]
	Vec3 const one( 1.0f );
	Vec3 const zero( 0.0f );
	a = Min( one, Max( zero, a ) );
	b = Min( one, Max( zero, b ) );

	// clamp to the grid
	Vec3 const grid( 31.0f, 63.0f, 31.0f );
	//Vec3 const gridrcp( 1.0f/31.0f, 1.0f/63.0f, 1.0f/31.0f );
	Vec3 const gridrcp(0.03227752766457f, 0.01583151765563f, 0.03227752766457f); // IC: use approximate grid fitting.
	Vec3 const half( 0.5f );
	a = Floor( grid*a + half )*gridrcp;
	b = Floor( grid*b + half )*gridrcp;

	// compute the error
	Vec3 e1 = a*a*alpha2_sum + b*b*beta2_sum /*+ m_xxsum*/
		+ 2.0f*( a*b*alphabeta_sum - a*alphax_sum - b*betax_sum );

	// apply the metric to the error term
	float error = Dot( e1, m_metricSqr );
	
	//if (debug) printf(" - %f\n", error);

	// save the start and end
	start = a;
	end = b;
	return error;
}
#endif

} // namespace squish
