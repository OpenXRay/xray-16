#include <ode/common.h>
#include "joint.h"
#include "ode/odemath.h"
#include "Lcp33.h"
#include "lcp.h"
#include "StepJointInternal.h"


#define	   DOT6(A,B)	(A[0]*B[0]+A[1]*B[1]+A[2]*B[2]+A[4]*B[4]+A[5]*B[5]+A[6]*B[6])

static void Multiply2_sym_3p8p (dReal * A, dReal * B, dReal * C)
{
	A[0]=DOT6(B,C);
	C+=8;
	A[4]=
	A[1]=DOT6(B,C);
	
	dReal* C3=C+8;
	A[2]=
	A[8]=DOT6(B,C3);

	B+=8;
	A[5]=DOT6(B,C);

	A[9]=
	A[6]=DOT6(B,C3);

	B+=8;
	A[10]=DOT6(B,C3);

}

static void
MultiplyAdd2_sym_3p8p (dReal * A, dReal * B, dReal * C)
{
	A[0]+=DOT6(B,C);///2*
	C+=8;
	dReal add;
	add=DOT6(B,C);
	A[4]+=add;
	A[1]+=add;
	
	dReal* C3=C+8;
	add=DOT6(B,C3);
	A[2]+=add;
	A[8]+=add;

	B+=8;
	add=DOT6(B,C);
	A[5]+=add;//2*
	
	add=DOT6(B,C3);
	A[9]+=add;
	A[6]+=add;

	B+=8;
	A[10]+=DOT6(B,C3);//2*

}


// this assumes the 4th and 8th rows of B are zero.

static void
Multiply0_3p81 (dReal * A, dReal * B, dReal * C)
{
A[0]=DOT6(B,C);
B+=8;
A[1]=DOT6(B,C);
B+=8;
A[2]=DOT6(B,C);

}


// this assumes the 4th and 8th rows of B are zero.

static void
MultiplyAdd0_3p81 (dReal * A, dReal * B, dReal * C)
{
A[0]+=DOT6(B,C);
B+=8;
A[1]+=DOT6(B,C);
B+=8;
A[2]+=DOT6(B,C);
}


#define	   DOTs6(A,B)	(A[0]*B[0]+A[8]*B[1]+A[16]*B[2])

static void
Multiply1_3p8q1 (dReal * A, dReal * B, dReal * C)
{

	A[0] = DOTs6(B,C);
	B++;
	A[1] =DOTs6(B,C);
	B++;
	A[2] =DOTs6(B,C);
	B+=2;
	A[4] =DOTs6(B,C);
	B++;
	A[5] =DOTs6(B,C);
	B++;
	A[6] =DOTs6(B,C);
}



#define SZM		3
void
dInternalStepJointContact (dxWorld * world, dxBody * body[2], dReal * GI[2], dReal * GinvI[2], dxJoint * joint, dxJoint::Info1 info, dxJoint::Info2 Jinfo, dReal stepsize)
{
	int i, j, k;
# ifdef TIMING
	dTimerNow ("constraint preprocessing");
# endif

	dReal stepsize1 = dRecip (stepsize);

	int m = info.m;
	// nothing to do if no constraints.
	
	int nub = 0;
	if (info.nub == info.m)
		nub = m;

	// compute A = J*invM*J'. first compute JinvM = J*invM. this has the same
	// format as J so we just go through the constraints in J multiplying by
	// the appropriate scalars and matrices.
#   ifdef TIMING
	dTimerNow ("compute A");
#   endif
	dReal JinvM[2 * SZM * 8];
	//dSetZero (JinvM, 2 * m * 8);

	dReal *Jsrc = Jinfo.J1l;
	dReal *Jdst = JinvM;
	if (body[0])
	{
		for (j = SZM - 1; j >= 0; j--)
		{
			for (k = 0; k < 3; k++)
				Jdst[k] = Jsrc[k] * body[0]->invMass;
			dMULTIPLY0_133 (Jdst + 4, Jsrc + 4, GinvI[0]);
			Jsrc += 8;
			Jdst += 8;
		}
	}
	if (body[1])
	{
		Jsrc = Jinfo.J2l;
		Jdst = JinvM + 8 * SZM;
		for (j = SZM - 1; j >= 0; j--)
		{
			for (k = 0; k < 3; k++)
				Jdst[k] = Jsrc[k] * body[1]->invMass;
			dMULTIPLY0_133 (Jdst + 4, Jsrc + 4, GinvI[1]);
			Jsrc += 8;
			Jdst += 8;
		}
	}


	// now compute A = JinvM * J'.
	int mskip = 4;
	dReal A[SZM * 4];
	//dSetZero (A, 6 * 8);

	if (body[0])
	{
		Multiply2_sym_3p8p (A, JinvM, Jinfo.J1l);
		//Multiply2_sym_p8p (A, JinvM, Jinfo.J1l,3,4);
	}
	if (body[1])
	{
		//MultiplyAdd2_sym_p8p (A, JinvM + 8 * 3, Jinfo.J2l,3,4);
		MultiplyAdd2_sym_3p8p (A, JinvM + 8 * 3, Jinfo.J2l);
	}

	// add cfm to the diagonal of A
	for (i = 0; i < 3; i++)
		A[i * mskip + i] += Jinfo.cfm[i] * stepsize1;

	// compute the right hand side `rhs'
#   ifdef TIMING
	dTimerNow ("compute rhs");
#   endif
	dReal tmp1[16];
	//dSetZero (tmp1, 16);
	// put v/h + invM*fe into tmp1
	for (i = 0; i < 2; i++)
	{
		if (!body[i])
			continue;
		for (j = 0; j < 3; j++)
			tmp1[i * 8 + j] = body[i]->facc[j] * body[i]->invMass + body[i]->lvel[j] * stepsize1;
		dMULTIPLY0_331 (tmp1 + i * 8 + 4, GinvI[i], body[i]->tacc);
		for (j = 0; j < 3; j++)
			tmp1[i * 8 + 4 + j] += body[i]->avel[j] * stepsize1;
	}
	// put J*tmp1 into rhs
	dReal rhs[6];
	//dSetZero (rhs, 6);

	if (body[0])
		Multiply0_3p81 (rhs, Jinfo.J1l, tmp1);
	if (body[1])
		MultiplyAdd0_3p81 (rhs, Jinfo.J2l, tmp1 + 8);

	// complete rhs
	for (i = 0; i < 3; i++)
		rhs[i] = Jinfo.c[i] * stepsize1 - rhs[i];


	// solve the LCP problem and get lambda.
	// this will destroy A but that's okay
#	ifdef TIMING
	dTimerNow ("solving LCP problem");
#	endif
	dReal lambda[3];
	//dReal lambda1[3];
	dReal residual[3];
	dReal lo[3], hi[3];//,lo1[6],hi1[6];
	memcpy (lo, Jinfo.lo, 3 * sizeof (dReal));
	memcpy (hi, Jinfo.hi, 3 * sizeof (dReal));
	//memcpy (lo1, Jinfo.lo, 3 * sizeof (dReal));
	//memcpy (hi1, Jinfo.hi, 3 * sizeof (dReal));

		dSolveLCP33(m, A, lambda, rhs, residual, nub, lo, hi, Jinfo.findex);
		//dSolveLCP (m, A, lambda, rhs, residual, nub, lo, hi, Jinfo.findex);
	//dReal dif[3];
	//dif[0]=lambda1[0]-lambda[0];lambda1[1]=lambda1[1]-lambda[1];lambda1[2]=lambda1[2]-lambda[2];
	//dReal lengh=dDOT(lambda,lambda);
	//dReal ldif=dDOT(dif,dif);
	//if(lengh>0.001&&ldif/lengh>0.1)
	//	i++;
	// compute the constraint force `cforce'
#	ifdef TIMING
	dTimerNow ("compute constraint force");
#endif

	// compute cforce = J'*lambda
	dJointFeedback *fb = joint->feedback;
	dReal cforce[16];
	//dSetZero (cforce, 16);

	if (fb)
	{
		// the user has requested feedback on the amount of force that this
		// joint is applying to the bodies. we use a slightly slower
		// computation that splits out the force components and puts them
		// in the feedback structure.
		dReal data1[8], data2[8];
		if (body[0])
		{
			Multiply1_3p8q1 (data1, Jinfo.J1l, lambda);
			dReal *cf1 = cforce;
			cf1[0] = (fb->f1[0] = data1[0]);
			cf1[1] = (fb->f1[1] = data1[1]);
			cf1[2] = (fb->f1[2] = data1[2]);
			cf1[4] = (fb->t1[0] = data1[4]);
			cf1[5] = (fb->t1[1] = data1[5]);
			cf1[6] = (fb->t1[2] = data1[6]);
		}
		if (body[1])
		{
			Multiply1_3p8q1 (data2, Jinfo.J2l, lambda);
			dReal *cf2 = cforce + 8;
			cf2[0] = (fb->f2[0] = data2[0]);
			cf2[1] = (fb->f2[1] = data2[1]);
			cf2[2] = (fb->f2[2] = data2[2]);
			cf2[4] = (fb->t2[0] = data2[4]);
			cf2[5] = (fb->t2[1] = data2[5]);
			cf2[6] = (fb->t2[2] = data2[6]);
		}
	}
	else
	{
		// no feedback is required, let's compute cforce the faster way
		if (body[0])
			Multiply1_3p8q1 (cforce, Jinfo.J1l, lambda);
		if (body[1])
			Multiply1_3p8q1 (cforce + 8, Jinfo.J2l, lambda);
	}

	for (i = 0; i < 2; i++)
	{
		if (!body[i])
			continue;
		for (j = 0; j < 3; j++)
		{
			body[i]->facc[j] += cforce[i * 8 + j];
			body[i]->tacc[j] += cforce[i * 8 + 4 + j];
		}
	}
}