#include "stdafx.h"

#include "xrMU_Model_Reference.h"
#include "xrMU_Model.h"

#include "light_point.h"
#include "fitter.h"
#include "xrface.h"
#include "xrLC_GlobalData.h"
template <typename T, typename T2>
T	simple_optimize				(xr_vector<T>& A, xr_vector<T>& B, T2& _scale, T2& _bias)
{
	T		accum;
	u32		it;


	T		scale	= _scale;
	T		bias	= _bias;
	T		error	= flt_max;
	T		elements= T(A.size());
	u32		count	= 0;
	for (;;)
	{
		count++;
		if (count>128)	{
			_scale		= (T2)scale;
			_bias		= (T2)bias;
			return error;
		}

		T	old_scale	= scale;
		T	old_bias	= bias;

		//1. scale
		u32		_ok			= 0;
		for (accum=0, it=0; it<A.size(); it++)
			if (_abs(A[it])>EPS_L)	
			{
				accum	+= (B[it]-bias)/A[it];
				_ok		+= 1;
			}
		T	s	= _ok?(accum/_ok):scale;

		//2. bias
		T	b	= bias;
		if (_abs(scale)>EPS)
		{
			for (accum=0, it=0; it<A.size(); it++)
				accum	+= B[it]-A[it]/scale;
			b	= accum	/ elements;
		}

		// mix
		T		conv	= 7;
		scale			= ((conv-1)*scale+s)/conv;
		bias			= ((conv-1)*bias +b)/conv;

		// error
		for (accum=0, it=0; it<A.size(); it++)
			accum	+= B[it] - (A[it]*scale + bias);
		T	err			= accum/elements;

		if (err<error)	
		{
			// continue?
			error	= err;
			if (error<EPS)	
			{
				_scale		= (T2)scale;
				_bias		= (T2)bias;
				return error;
			}
		}
		else
		{
			// exit
			_scale	= (T2)old_scale;
			_bias	= (T2)old_bias;
			return	error;
		}
	}
}



void	o_test (int iA, int iB, int count, base_color* A, base_color* B, float& C, float& D)
{
	xr_vector<double>	_A,_B;
	_A.resize			(count);
	_B.resize			(count);
	for (int it=0; it<count; it++)
	{
		base_color_c _a;	A[it]._get(_a);	float*	f_a	= (float*)&_a;
		base_color_c _b;	B[it]._get(_b);	float*	f_b	= (float*)&_b;
		_A[it]				= f_a[iA];
		_B[it]				= f_b[iB];
	}
	// C=1, D=0;
	simple_optimize		(_A,_B,C,D);
}


void xrMU_Reference::calc_lighting	()
{
	model->calc_lighting		(color,xform,inlc_global_data()->RCAST_Model(),inlc_global_data()->L_static(),(inlc_global_data()->b_nosun()?LP_dont_sun:0)|LP_DEFAULT);

	R_ASSERT					(color.size()==model->color.size());

	// A*C + D = B
	// build data
	{
		FPU::m64r			();
		xr_vector<double>	A;	A.resize(color.size());
		xr_vector<double>	B;	B.resize(color.size());
		float*				_s=(float*)&c_scale;
		float*				_b=(float*)&c_bias;
		for (u32 i=0; i<5; i++) {
			for (u32 it=0; it<color.size(); it++) {
				base_color_c		__A;	model->color	[it]._get(__A);
				base_color_c		__B;	color			[it]._get(__B);
				A[it]		= 	(__A.hemi);
				B[it]		=	((float*)&__B)[i];
			}
			vfComputeLinearRegression(A,B,_s[i],_b[i]);
		}

		for (u32 index=0; index<5; index++)
			o_test	(4,index,color.size(),&model->color.front(),&color.front(),_s[index],_b[index]);
	}
}
