#pragma	once

#include <numeric>

typedef float REAL;

extern void vfOptimizeParameters(xr_vector<xr_vector<REAL> > &A, xr_vector<xr_vector<REAL> > &B, xr_vector<REAL> &C, xr_vector<REAL> &D, REAL dEpsilon = REAL(0.001), REAL dAlpha = REAL(1.0), REAL dBeta = REAL(0.01), REAL dNormaFactor = REAL(10.0), u32 dwMaxIterationCount = 10000);

template<typename T, typename T2> void vfComputeLinearRegression(xr_vector<T> &A, xr_vector<T> &B, T2 &C, T2 &D)
{
	u32				N = A.size();
	T				sx = T(0), sy = T(0), sxy = T(0), sx2 = T(0), l_tDenominator;
	sx				= std::accumulate		(A.begin(),A.end(),sx);
	sy				= std::accumulate		(B.begin(),B.end(),sy);
	sxy				= std::inner_product	(A.begin(),A.end(),B.begin(),sxy);
	sx2				= std::inner_product	(A.begin(),A.end(),A.begin(),sx2);
	l_tDenominator	= T(N)*sx2 - sx*sx;
	if (_abs(l_tDenominator) > EPS_S) 
		C			= T2( (T(N)*sxy - sx*sy)/l_tDenominator );
	else
		C			= T2(0);
	if (N)
		D			= T2( (sy - C*sx)/T(N) );
	else
		D			= T2(0);
}