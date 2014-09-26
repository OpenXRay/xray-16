//;******************************************************************************
#include "stdafx.h"

//;******************************************************************************
#pragma warning (disable:4799)
//;******************************************************************************
// A "magic" type to allow initialization with any type
template <class T>
union __m64t
{
    T       t;
    __m64   m64;

    operator __m64 ()       { return m64; }
    operator __m64 () const { return m64; }
};

// An instantiation for initializing with a pair of integers
typedef __m64t<int[2]> __m64i;
// An instantiation for initializing with a pair of floats
typedef __m64t<float[2]> __m64f;
//;******************************************************************************
//; Vector (3DNow!) data 
//;******************************************************************************
static const __m64f  PMOne		=	{ 1.0f		,	-1.0f			};
static const __m64f  HalfVal	=	{ 0.5f		,	0.5f			};
static const __m64f  HalfMin	=	{ 0.5f		,	-0.5f			};
static const __m64f  ones		=	{ 1.0f		,	1.0f			};
static const __m64f  twos		=	{ 2.0f		,	2.0f			};
static const __m64i izeros      =   { 0,            0               };
static const __m64i pinfs       =   { 0x7f800000,   0x7f800000      }; // dword PINH, PINH
static const __m64i smh_masks   =   { 0x807fffff,   0x807fffff      }; // dword MASKSMH, MASKSMH
static const __m64i sign_mask   =   { 0x7fffffff,   0x7fffffff      };
static const __m64i sh_masks    =   { 0x80000000,   0x80000000      }; // dword MASKSH, MASKSH
static const __m64i two_126s    =   { 126,          126             };
// ; SINCOS specific values
static const __m64f mo2s		=	{ -0.5f			,-0.5f			};
static const __m64f mo12_6		=	{ -0.0833333333f,-0.166666667f	};	
static const __m64f mo30_20		=	{ -0.0333333333f,-0.05f			};
static const __m64f mo56_42		=	{ -0.0178571f	,-0.0238095f	};
static const __m64i pio4ht      =   { 0xbf490000,   0xb97daa22      };  // approximately -0.000241913 | -0.785156
static const __m64i pio4s       =   { 0x3f490fdb,   0x3f490fdb      };  // approximately 0.785398 | 0.785398
//;******************************************************************************
//; Scalar (single float) data
//;******************************************************************************
static const __int32 fouropi    =       0x3fa2f983;             // 1.27324f
static const __int32 xmax       =       0x46c90fdb;             // 25735.9
static const __int32 sgn        =       0x80000000;
static const __int32 mabs       =       0x7FFFFFFF;
static const __int32 mant       =       0x007FFFFF;
static const __int32 expo       =       0x7F800000;
static const __int32 one        =       0x3F800000;
static const __int32 half       =       0x3F000000;
static const __int32 two        =       0x40000000;
static const __int32 oob        =       0x00000000;
static const __int32 nan        =       0x7fffffff;
static const __int32 pnan       =       0x7fc00000;
static const __int32 n0         =       0x40A008EF;
static const __int32 n1         =       0x3DAA7B3D;
static const __int32 d0         =       0x412008EF;
static const __int32 qq0        =       0x419D92C8;
static const __int32 qq1        =       0x41E6BD60;
static const __int32 qq2        =       0x41355DC0;
static const __int32 pp0        =       0xC0D21907;
static const __int32 pp1        =       0xC0B59883;
static const __int32 pp2        =       0xBF52C7EA;
static const __int32 bnd        =       0x3F133333;
static const __int32 asp0       =       0x3F6A4AA5;
static const __int32 asp1       =       0xBF004C2C;
static const __int32 asq0       =       0x40AFB829;
static const __int32 asq1       =       0xC0AF5123;
static const __int32 pio2       =       0x3FC90FDB;
static const __int32 npio2      =       0xBFC90FDB;
static const __int32 ooln2      =       0x3FB8AA3B;
static const __int32 upper      =       0x42B17218;
static const __int32 lower      =       0xC2AEAC50;
static const __int32 ln2hi      =       0x3F317200;
static const __int32 ln2lo      =       0x35BFBE8E;
static const __int32 rt2        =       0x3FB504F3;
static const __int32 edec       =       0x00800000;
static const __int32 bias       =       0x0000007F;
static const __int32 c2         =       0x3E18EFE2;
static const __int32 c1         =       0x3E4CAF6F;
static const __int32 c0         =       0x3EAAAABD;
static const __int32 tl2e       =       0x4038AA3B;
static const __int32 maxn       =       0xFF7FFFFF;
static const __int32 q1         =       0x43BC00B5;
static const __int32 p1         =       0x41E77545;
static const __int32 q0         =       0x45E451C5;
static const __int32 p0         =       0x451E424B;
static const __int32 mine       =       0xC2FC0000;
static const __int32 maxe       =       0x43000000;
static const __int32 max        =       0x7F7FFFFF;
static const __int32 rle10      =       0x3ede5bdb;
// For alt_acos
static const __m64f  a_c7		=	{ 2.838933f,	0.f};
static const __m64f  a_c5		=	{-3.853735f,	0.f};
static const __m64f  a_c3		=	{ 1.693204f,	0.f};
static const __m64f  a_c1		=	{ 0.892399f,	0.f};
static const __m64f  a_pi_div_2	=	{ PI_DIV_2 ,	0.f};
static const __int64 _msgn_		=	0x8000000080000000;
//;******************************************************************************
//; Routine:  alt_acos
//; Input:    mm0.lo
//; Result:   mm0.lo
//; Uses:     mm0-mm3
//; Comment:
//;   Compute acos(x) using MMX and 3DNow! instructions.Scalar version.
//;   © by Oles™	©
//;   © by ManOwaR™
//;******************************************************************************
__declspec(naked)	void alt_acos(void)
{	__asm {
// ------------------------------------------------------------------
	movq		mm3,QWORD PTR [a_c7]		;	mm3 = 0.0 | c7
	movq		mm2,QWORD PTR [a_pi_div_2]	;	mm2 = 0.0 | PI_DIV_2
	movq		mm1,mm0						;	mm1 = ?.? | x
	pfmul		mm1,mm1						;	mm1 = ?.? | x2=x*x
	pfmul		mm3,mm1						;	mm3 = ?.? | c7*x2
	pfadd		mm3,QWORD PTR [a_c5]		;	mm3 = ?.? | c7*x2+c5
	pfmul		mm3,mm1						;	mm3 = ?.? | (c7*x2+c5)*x2
	pfadd		mm3,QWORD PTR [a_c3]		;	mm3 = ?.? | (c7*x2+c5)*x2+c3
	pfmul		mm3,mm1						;	mm3 = ?.? | ((c7*x2+c5)*x2+c3)*x2
	pfadd		mm3,QWORD PTR [a_c1]		;	mm3 = ?.? | ((c7*x2+c5)*x2+c3)*x2+c1
	pfmul		mm0,mm3						;	mm0 = ?.? | (((c7*x2+c5)*x2+c3)*x2+c1)*x
	pfsub		mm2,mm0						;	mm2 = ?.? | PI_DIV_2 - (((c7*x2+c5)*x2+c3)*x2+c1)*x
	movq		mm0,mm2						;	mm0 = ?.? | PI_DIV_2 - (((c7*x2+c5)*x2+c3)*x2+c1)*x
	ret
// ------------------------------------------------------------------
}}
//;******************************************************************************
//; SINCOSMAC - sin/cos simultaneous computation
//; Input:    mm0 - angle in radians
//; Output:   mm0 - (sin|cos)
//; Uses:     mm0-mm7, eax, ebx, ecx, edx, esi
//; Comment:  This macro simultaneously computes sin and cos of the input
//;           parameter, and returns the result packed in mm0 as (sin|cos).
//;           Ultimately, this routine needs higher precision and a more
//;           efficient implementation (less inter-register bank traffic).
//;******************************************************************************
__declspec (naked) void SINCOSMAC ()
{
    __asm {
        push        ebx
        movd        eax,mm0
        movq        mm1,mm0
        movd        mm3,[mabs]
        mov         ebx,eax
        mov         edx,eax
        pand        mm0,mm3                 //;; m0 = fabs(x)
        and         ebx,0x80000000          //;; get sign bit
        shr         edx,01fh                //;; edx = (r0 < 0) ? 1: 0
        xor         eax,ebx                 //;; eax = fabs (eax)
        cmp         eax,[xmax]
        movd        mm2,[fouropi]
        jl          short x2
        movd        mm0,[one]
        jmp         ending
        ALIGN       16
x2:
        movq        mm1,mm0
        pfmul       mm0,mm2                 //;; mm0 = fabs(x) * 4 / PI
        movq        mm3,[pio4ht]
        pf2id       mm0,mm0
        movq        mm7,[mo56_42]
        movd        ecx,mm0
        pi2fd       mm0,mm0
        mov         esi,ecx
        movq        mm6,[mo30_20]
        punpckldq   mm0,mm0
        movq        mm5,[ones]
        pfmul       mm0,mm3
        pfadd       mm1,mm0
        shr         esi,2
        punpckhdq   mm0,mm0
        xor         edx,esi
        pfadd       mm1,mm0
        test        ecx,1
        punpckldq   mm1,mm1
        jz          short x5
        pfsubr      mm1,[pio4s]
x5:     movq        mm2,mm5
        shl         edx,01fh
        punpckldq   mm2,mm1
        pfmul       mm1,mm1
        mov         esi,ecx
        movq        mm4,[mo12_6]
        shr         esi,1
        pfmul       mm7,mm1
        xor         ecx,esi
        pfmul       mm6,mm1
        shl         esi,01fh
        pfadd       mm7,mm5
        xor         ebx,esi
        pfmul       mm4,mm1
        pfmul       mm7,mm6
        movq        mm6,[mo2s]
        pfadd       mm7,mm5
        pfmul       mm6,mm1
        pfmul       mm4,mm7
        movd        mm0,edx
        pfadd       mm4,mm5
        punpckldq   mm6,mm5
        psrlq       mm5,32
        pfmul       mm4,mm6
        punpckldq   mm0,mm0
        movd        mm1,ebx
        pfadd       mm4,mm5
        test        ecx,1
        pfmul       mm4,mm2
        jz          short x7
        punpckldq   mm5,mm4
        punpckhdq   mm4,mm5
x7:     pxor        mm4,mm1
        pxor        mm0,mm4

ending:
        pop ebx
        nop
        ret
    }
}
//;******************************************************************************
//; Routine:  a_acos
//; Input:    mm0.lo
//; Result:   mm0.lo
//; Uses:     mm0-mm7
//; Comment:
//;   Compute acos(x) using MMX and 3DNow! instructions.Scalar version.
//
//;   If the input has an exponent of 0xFF, the result of this routine
//;   is undefined. If the absolute value of the input is greater than
//;   1, a special result is returned (currently this is 0). Results
//;   for arguments in [-1, 1] are in [-pi/2, pi/2].
//;
//;   Let z=abs(x). Then acos(x) can be computed as follows:
//;
//;    -1 <= x <= -0.575: acos(x) = pi - 2 * asin(sqrt((1-z)/2))
//;    -0.575 <= x < 0  : acos(x) = pi/2 + asin(z)
//;    0 <= x < 0.575   : acos(x) = pi/2 - asin(z)
//;    0.575 <= x <= 1  : acos(x) = 2 * asin(sqrt((1-z)/2))
//;
//;   asin(z) for 0 <= z <= 0.575 is approximated by a rational minimax
//;   approximation.
//;
//;   Testing shows that this function has an error of less than 3.07
//;   single precision ulps.
//;
//;******************************************************************************
__declspec (naked) void a_acos ()
{
    __asm
    {
        movd        mm6, [sgn]  //; mask for sign bit
        movd        mm7, [mabs] //; mask for absolute value
        movd        mm4, [half] //; 0.5
        pand        mm6, mm0    //; extract sign bit
        movd        mm5, [one]  //; 1.0
        pand        mm0, mm7    //; z = abs(x)
        movq        mm3, mm0    //; z
        pcmpgtd     mm3, mm5    //; z > 1.0 ? 0xFFFFFFFF : 0
        movq        mm5, mm0    //; save z
        pfmul       mm0, mm4    //; z*0.5
        movd        mm2, [bnd]  //; 0.575
        pfsubr      mm0, mm4    //; 0.5 - z * 0.5
        pfrsqrt     mm7, mm0    //; 1/sqrt((1-z)/2) approx low
        movq        mm1, mm7    //; complete
        pfmul       mm7, mm7    //;  reciprocal
        pcmpgtd     mm2, mm5    //; z < 0.575 ? 0xfffffff : 0
        pfrsqit1    mm7, mm0    //;   square root
        movd        mm4, [asq1] //; asq1
        pfrcpit2    mm7, mm1    //;    computation
        pfmul       mm7, mm0    //; sqrt((1-z)/2)
        movq        mm0, mm2    //; duplicate mask
        pand        mm5, mm2    //; z < 0.575 ? z : 0
        pandn       mm0, mm7    //; z < 0.575 ? 0 : sqrt((1-z)/2)
        movd        mm7, [asp1] //; asp1
        por         mm0, mm5    //; z < 0.575 ? z : sqrt((1-z)/2)
        movq        mm1, mm0    //; save z
        pfmul       mm0, mm0    //; z^2
        movd        mm5, [asp0] //; asp0
        pfmul       mm7, mm0    //; asp1 * z^2
        pfadd       mm4, mm0    //; z^2 + asq1 
        pfadd       mm7, mm5    //; asp1 * z^2 + asp0
        movd        mm5, [asq0] //; asq0
        pfmul       mm7, mm0    //; (asp1 * z^2 + asp0) * z^2
        pfmul       mm0, mm4    //; (z^2 + asq1) * z^2
        pfadd       mm0, mm5    //; qx = (z^2 + asq1) * z^2 + asq0
        pfmul       mm7, mm1    //; z^3*px = (asp1 * z^2 + asp0) * z^3
        pfrcp       mm4, mm0    //; 1/qx approx
        pfrcpit1    mm0, mm4    //; 1/qx step 
        movd        mm5, [npio2]//; -pi/2
        pfrcpit2    mm0, mm4    //; 1/qx final
        pfmul       mm0, mm7    //; z^3*px/qx
        movq        mm4, mm2    //; z < 0.575 ? 0xfffffff : 0
        pandn       mm2, mm5    //; z < 0.575 ? 0 : -pi/2
        movd        mm5, [pio2] //; pi/2
        movq        mm7, mm4    //; z < 0.575 ? 0xfffffff : 0
        pxor        mm2, mm6    //; z < 0.575 ? 0 : (sign ? pi/2 : -pi/2)
        pfadd       mm1, mm0    //; r = z + z^3*px/qx
        movd        mm0, [oob]  //; special result for out of bound arguments
        pfadd       mm2, mm5    //; z < 0.575 ? pi/2 : (sign ? pi : 0)
        pslld       mm7, 31     //; z < 0.575 ? 0x80000000 : 0
        pandn       mm4, mm1    //; z < 0.575 ? 0 : r
        pxor        mm7, mm6    //; ((z < 0.575) != sign) ? 0x80000000 : 0
        pfadd       mm1, mm4    //; z < 0.575 ? r : 2*r
        pand        mm0, mm3    //; if abs(x) > 1 select special result
        por         mm1, mm7    //; ((z < 0.575) != sign) ? -r,-2*r : r,2*r
        pfadd       mm2, mm1    //; acos(x)
        pandn       mm3, mm2    //; if abs(x) <= 1, select regular result
        por         mm0, mm3    //; mux together results
        ret
    }
}
//;******************************************************************************
//; Routine:  a_asin
//; Input:    mm0.lo
//; Result:   mm0.lo
//; Uses:     mm0-mm7
//; Comment:
//;   Compute asin(x) using MMX and 3DNow! instructions.Scalar version.
//;
//;   If the input has an exponent of 0xFF, the result of this routine
//;   is undefined. Inputs with an exponent of 0 are treated as true
//;   zeroes and return a function value of 0. If the absolute value
//;   of the input is greater than 1, a special result is returned
//;   (currently this is 0). Results for arguments in [-1, 1] are in
//;   [-pi/2, pi/2].
//;
//;   asin(x)=sign(x)*asin(abs(x)). Let z=abs(x). If z>0.575, asin(z)=
//;   pi/2 - 2*asin(sqrt(0.5-0.5*z)). asin(z) for 0 <= z <= 0.575 is
//;   is approximated by a rational minimax approximation.
//;
//;   Testing shows that this function has an error of less than 3.25
//;   single precision ulps.
//;
//;******************************************************************************
__declspec (naked) void a_asin ()
{
    __asm
    {
        movd        mm6, [sgn]  //; mask for sign bit
        movd        mm7, [mabs] //; mask for absolute value
        movd        mm4, [half] //; 0.5
        pand        mm6, mm0    //; extract sign bit
        movd        mm5, [one]  //; 1.0
        pand        mm0, mm7    //; z = abs(x)
        movq        mm3, mm0    //; z
        pcmpgtd     mm3, mm5    //; z > 1.0 ? 0xFFFFFFFF : 0
        movq        mm5, mm0    //; save z
        pfmul       mm0, mm4    //; z*0.5
        movd        mm2, [bnd]  //; 0.575
        pfsubr      mm0, mm4    //; 0.5 - z * 0.5
        pfrsqrt     mm7, mm0    //; 1/sqrt((1-z)/2) approx
        movq        mm1, mm7    //; complete
        pfmul       mm7, mm7    //;  reciprocal
        pcmpgtd     mm2, mm5    //; z < 0.575 ? 0xfffffff : 0
        pfrsqit1    mm7, mm0    //;   square root
        movd        mm4, [asq1] //; asq1
        pfrcpit2    mm7, mm1    //;    computation
        pfmul       mm7, mm0    //; sqrt((1-z)/2)
        movq        mm0, mm2    //; duplicate mask
        pand        mm5, mm2    //; z < 0.575 ? z : 0
        pandn       mm0, mm7    //; z < 0.575 ? 0 : sqrt((1-z)/2)
        movd        mm7, [asp1] //; asp1
        por         mm0, mm5    //; z < 0.575 ? z : sqrt((1-z)/2)
        movq        mm1, mm0    //; save z
        pfmul       mm0, mm0    //; z^2
        movd        mm5, [asp0] //; asp0
        pfmul       mm7, mm0    //; asp1 * z^2
        pfadd       mm4, mm0    //; z^2 + asq1 
        pfadd       mm7, mm5    //; asp1 * z^2 + asp0
        movd        mm5, [asq0] //; asq0
        pfmul       mm7, mm0    //; (asp1 * z^2 + asp0) * z^2
        pfmul       mm0, mm4    //; (z^2 + asq1) * z^2
        pfadd       mm0, mm5    //; qx = (z^2 + asq1) * z^2 + asq0
        pfmul       mm7, mm1    //; z^3*px = (asp1 * z^2 + asp0) * z^3
        pfrcp       mm4, mm0    //; 1/qx approx
        pfrcpit1    mm0, mm4    //; 1/qx step 
        pfrcpit2    mm0, mm4    //; 1/qx final
        movd        mm4, [pio2] //; pi/2
        pfmul       mm7, mm0    //; z^3*px/qx
        movd        mm0, [oob]  //; special out-of-bounds result
        pfadd       mm1, mm7    //; r = z + z^3*px/qx
        movq        mm5, mm1    //; save r
        pfadd       mm1, mm1    //; 2*r
        pfsubr      mm1, mm4    //; pi/2 - 2*r
        pand        mm5, mm2    //; z < 0.575 ? r : 0
        pandn       mm2, mm1    //; z < 0.575 ? 0 : pi/2 - 2 * r
        pand        mm0, mm3    //; select special result if abs(x) > 1
        por         mm2, mm5    //; z < 0.575 ? r : pi/2 - 2 * r
        por         mm2, mm6    //; asin(x)=sign(x)*(z < 0.575 ? r : pi/2 - 2 * r)
        pandn       mm3, mm2    //; select regular result if abs(x) <= 1
        por         mm0, mm3    //; mux results together
        ret
    }
}
//;******************************************************************************
//; Routine:  a_sin
//; Input:    mm0.lo
//; Result:   mm0 (sin|sin)
//; Uses:     mm0-mm7, eax, ebx, ecx, edx, esi
//;******************************************************************************
__declspec (naked) void a_sin ()
{
    __asm
    {

        call SINCOSMAC          //; mm0 = cos(x) | sin(x)
        punpckhdq   mm0,mm0     //; select sin value
        ret
    }
}
//;******************************************************************************
#pragma warning (default:4799)
//;******************************************************************************
