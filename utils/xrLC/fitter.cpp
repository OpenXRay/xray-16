////////////////////////////////////////////////////////////////////////////
//	Module 		: fitter.cpp
//	Created 	: 25.03.2002
//  Modified 	: 28.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Weight Fitting Algorithm
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "fitter.h"
#include <functional>

IC REAL dfEvaluation(REAL &A, REAL &C, REAL &D)
{
	return					(A*C + D);
}

REAL dfComputeEvalResults(xr_vector<xr_vector<REAL> >	&daEvalResults, xr_vector<xr_vector<REAL> > &A, xr_vector<xr_vector<REAL> > &B, xr_vector<REAL> &C, xr_vector<REAL> &D)
{
	REAL dResult			= 0.0;
	u32 dwTestCount			= (u32)B.size();
	u32 dwParameterCount	= (u32)B[0].size();
	
	for (u32 i=0; i<dwTestCount; i++) {
		for (u32 j=0; j<dwParameterCount; j++) {
			
			daEvalResults[i][j] = dfEvaluation(A[i][j],C[j],D[j]);
			REAL dTemp			= B[i][j] - daEvalResults[i][j];
			dResult				+= dTemp*dTemp;
		}
	}
	return					(dResult);
}

xr_vector<REAL> &dafGradient(xr_vector<xr_vector<REAL> >	&daEvalResults, xr_vector<REAL> &daResult, xr_vector<xr_vector<REAL> > &B, REAL dNormaFactor)
{
	REAL					dNorma = 0.0;
	u32						dwTestCount = (u32)B.size();
	u32 dwParameterCount	= (u32)B[0].size();
	daResult.assign			(dwParameterCount,0);
	
	for (u32 i=0; i<dwParameterCount; i++) {
		for (u32 j=0; j<dwTestCount; j++)
			daResult[i]		-= B[j][i] - daEvalResults[j][i];
		dNorma				+= daResult[i]*daResult[i];
	}
	dNorma					= _sqrt(dNorma);

	for (u32 i=0; i<dwParameterCount; i++)
		daResult[i]			/= (dNorma > 1.f ? dNorma : 1.f)*dNormaFactor;

	return					(daResult);
}

void vfOptimizeParameters(xr_vector<xr_vector<REAL> > &A, xr_vector<xr_vector<REAL> > &B, xr_vector<REAL> &C, xr_vector<REAL> &D, REAL dEpsilon, REAL dAlpha, REAL dBeta, REAL dNormaFactor, u32 dwMaxIterationCount)
{
	u32						dwTestCount	= (u32)B.size();
	xr_vector<REAL>			daGradient;
	xr_vector<REAL>			daDelta;
	xr_vector<xr_vector<REAL> >	daEvalResults; daEvalResults.resize(dwTestCount);
	
	if (!B.size()) {
		clMsg				("ERROR : there are no parameters to fit!");
		return;
	}
	
	u32						dwParameterCount = (u32)B[0].size();
	C.assign				(dwParameterCount,1.0f);
	D.assign				(dwParameterCount,0.0f);
	daDelta.assign			(dwParameterCount,0);
	daGradient.assign		(dwParameterCount,0);
	{
		for (u32 i=0; i<dwTestCount; i++)
			daEvalResults[i].resize(dwParameterCount);
	}
	u32						i = 0;
	REAL					dFunctional = dfComputeEvalResults(daEvalResults,A,B,C,D), dPreviousFunctional;
	//clMsg					("***MU-fitter***: %6d : %17.8f (%17.8f)",i,dFunctional,dFunctional/dwTestCount);
	do {
		dPreviousFunctional = dFunctional;
		dafGradient			(daEvalResults,			daGradient,			B,					dNormaFactor);
		std::transform		(daGradient.begin(),	daGradient.end(),	daGradient.begin(),	std::bind2nd(std::multiplies<REAL>(), -dAlpha));
		std::transform		(daDelta.begin(),		daDelta.end(),		daDelta.begin(),	std::bind2nd(std::multiplies<REAL>(), dBeta));
		std::transform		(daGradient.begin(),	daGradient.end(),	daDelta.begin(),	daDelta.begin(),	std::plus<REAL>());
		std::transform		(C.begin(),				C.end(),			daDelta.begin(),	C.begin(),			std::plus<REAL>());
		std::transform		(D.begin(),				D.end(),			daDelta.begin(),	D.begin(),			std::plus<REAL>());
		dFunctional			= dfComputeEvalResults(daEvalResults,A,B,C,D);
		i++;
	}
	while ((((dPreviousFunctional - dFunctional)/dwTestCount) > dEpsilon) && (i <= dwMaxIterationCount));
	
	if (dPreviousFunctional < dFunctional) {
		std::transform		(daDelta.begin(),		daDelta.end(),		daDelta.begin(),	std::bind2nd(std::multiplies<REAL>(), -1));
		std::transform		(C.begin(),				C.end(),			daDelta.begin(),	C.begin(),			std::plus<REAL>());
		std::transform		(D.begin(),				D.end(),			daDelta.begin(),	D.begin(),			std::plus<REAL>());
	}
	
	dFunctional				= dfComputeEvalResults(daEvalResults,A,B,C,D);
	//clMsg					("***MU-fitter***: %6d : %17.8f (%17.8f)",i,dFunctional,dFunctional/dwTestCount);
}