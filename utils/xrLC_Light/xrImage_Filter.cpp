#include "stdafx.h"
#include "xrImage_Filter.h"

float imk_blur_gauss [7][7] =
{
	{	0,	0,	0,	5,	0,	0,	0	},
	{	0,	5,	18,	32,	18,	5,	0	},
	{	0,	18,	64,	100,64,	18,	0	},
	{	5,	32,	100,100,100,32,	5	},
	{	0,	18,	64,	100,64,	18,	0	},
	{	0,	5,	18,	32,	18,	5,	0	},
	{	0,	0,	0,	5,	0,	0,	0	}
};

/* Un-optimized code to perform general image filtering
outputs to dst using a filter kernel in ker which must be a 2D float
array of size [2*k+1][2*k+1] */
void imf_BuildKernel	(float* dest, float* src, int DIM, float norm)
{
	float	*I,*E;

	float	sum		= 0;
	int		size	= 2*DIM+1;
	E				= src + (size*size);
	for (I=src; I!=E; I++) sum += *I;
	float	scale	= norm/sum;
	for (I=src; I!=E; I++) *dest++ = *I * scale;
}
void imf_ProcessKernel	(Fvector* dest, Fvector* src, int w, int h, float* kern, int DIM)
{
	for (int y=0;y<h;y++)
	{
		for (int x=0;x<w;x++)
		{
			Fvector total; 
			total.set	(0,0,0);
			float *kp	= kern;
			for (int j=-DIM;j<=DIM;j++)
			for (int i=-DIM;i<=DIM;i++)
			{
				int x2=x+i,y2=y+j;

				// wrap pixels
				if (x2<0) x2+=w; else if (x2>=w) x2-=w;
				if (y2<0) y2+=h; else if (y2>=h) y2-=h;
				
				total.mad	(total,src[y2*w+x2],*kp);
				kp++;
			}
			dest->set(total);
			dest++;
		}
	}
}
