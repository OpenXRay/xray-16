#ifndef ODE_REDEFINE
#define ODE_REDEFINE

#ifdef  XRGAME_EXPORTS

#ifdef	dSqrt
#undef	dSqrt
#define dSqrt(x) ((float)_sqrt(x))		/* square root */
#endif

#ifdef  dRecipSqrt
#undef  dRecipSqrt
#define dRecipSqrt(x) ((float)(1.0f/_sqrt(x)))	/* reciprocal square root */
#endif

#ifdef	dSin
#undef  dSin
#define dSin(x) ((float)_sin(x))			/* sine */
#endif

#ifdef  dCos
#undef  dCos
#define dCos(x) ((float)_cos(x))			/* cosine */
#endif

#ifdef  dFabs
#undef  dFabs
#define dFabs(x) ((float)_abs(x))		/* absolute value */
#endif

#endif//XRGAME_EXPORTS

#endif//ODE_REDEFINE