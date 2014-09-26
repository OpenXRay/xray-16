/*
 *		Filtered Image Rescaling
 *
 *		  by Dale Schumacher
 */

#include "stdafx.h"
#pragma hdrstop

#include "xrImage_Resampler.h"

typedef	u32	Pixel;
struct Image
{
	int		xsize;		/* horizontal size of the image in Pixels */
	int		ysize;		/* vertical size of the image in Pixels */
	Pixel *	data;		/* pointer to first scanline of image */
	int		span;		/* byte offset between two scanlines */
};

#define	WHITE_PIXEL		(255)
#define	BLACK_PIXEL		(0)

/*
 *	generic image access and i/o support routines
 */

Image *	new_image(int xsize, int ysize)		/* create a blank image */
{
	Image *image;

	if((0!=(image = (Image *)xr_malloc(sizeof(Image)))) && (0!=(image->data = (Pixel *)xr_malloc(ysize*xsize*sizeof(Pixel)))))
	{
		ZeroMemory(image->data,ysize*xsize*sizeof(Pixel));
		image->xsize	= xsize;
		image->ysize	= ysize;
		image->span		= xsize;
	}
	return	(image);
}

void	free_image(Image* image)
{
	xr_free(image->data);
	xr_free(image);
}

Pixel	get_pixel	(Image* image, int x, int y)
{
	if((x < 0) || (x >= image->xsize) || (y < 0) || (y >= image->ysize)) return 0;
	return image->data[(y * image->span) + x];
}

void	get_row		(Pixel* row, Image* image, int y)
{
	if((y < 0) || (y >= image->ysize)) return;
	CopyMemory(row,	image->data + (y * image->span), (sizeof(Pixel) * image->xsize));
}

void	get_column	(Pixel* column, Image* image, int x)
{
	int i, d;
	Pixel *p;

	if((x < 0) || (x >= image->xsize)) return;

	d	= image->span;
	for(i = image->ysize, p = image->data + x; i-- > 0; p += d) {
		*column++ = *p;
	}
}

Pixel	put_pixel	(Image* image, int x, int y, Pixel data)
{
	if((x < 0) || (x >= image->xsize) || (y < 0) || (y >= image->ysize)) return 0;
	return	image->data[(y * image->span)+x] = data;
}


/*
 *	filter function definitions
 */

//
#define	filter_support		(1.0)
float	filter				(float t)
{
	/* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
	if(t < 0.0) t = -t;
	if(t < 1.0) return		float((2.0 * t - 3.0) * t * t + 1.0);
	return(0.0);
}

//
#define	box_support			(0.5)
float	box_filter			(float t)
{
	if((t > -0.5) && (t <= 0.5)) return(1.0);
	return(0.0);
}

//
#define	triangle_support	(1.0)
float	triangle_filter		(float t)
{
	if(t < 0.0f) t = -t;
	if(t < 1.0f) return(1.0f - t);
	return(0.0f);
}

//
#define	bell_support		(1.5)
float	bell_filter			(float t)		/* box (*) box (*) box */
{
	if(t < 0) t = -t;
	if(t < .5) return float(.75 - (t * t));
	if(t < 1.5) {
		t = (t - 1.5f);
		return(.5f * (t * t));
	}
	return(0.0);
}

//
#define	B_spline_support	(2.0)
float	B_spline_filter		(float t)	/* box (*) box (*) box (*) box */
{
	float tt;

	if(t < 0) t = -t;
	if(t < 1) {
		tt = t * t;
		return((.5f * tt * t) - tt + (2.0f / 3.0f));
	} else if(t < 2) {
		t = 2 - t;
		return((1.0f / 6.0f) * (t * t * t));
	}
	return(0.0f);
}

//
#define	Lanczos3_support	(3.0)
float	sinc				(float x)
{
	x *= 3.1415926f;
	if(x != 0) return(_sin(x) / x);
	return(1.0);
}
float	Lanczos3_filter		(float t)
{
	if(t < 0) t = -t;
	if(t < 3.0f) return	float(sinc(t) * sinc(t/3.0f));
	return(0.0);
}

//
#define	Mitchell_support	(2.0)
#define	B	(1.0f / 3.0f)
#define	C	(1.0f / 3.0f)

float	Mitchell_filter		(float t)
{
	float tt;

	tt = t * t;
	if(t < 0) t = -t;
	if(t < 1.0f) {
		t = (((12.0f - 9.0f * B - 6.0f * C) * (t * tt))
		   + ((-18.0f + 12.0f * B + 6.0f * C) * tt)
		   + (6.0f - 2.0f * B));
		return(t / 6.0f);
	} else if(t < 2.0f) {
		t = (((-1.0f * B - 6.0f * C) * (t * tt))
		   + ((6.0f * B + 30.0f * C) * tt)
		   + ((-12.0f * B - 48.0f * C) * t)
		   + (8.0f * B + 24.f * C));
		return(t / 6.0f);
	}
	return(0.0);
}

/*
 *	image rescaling routine
 */

struct CONTRIB
{
	int		pixel;
	float	weight;
};

struct CLIST
{
	int		n;					/* number of contributors */
	CONTRIB	*p;					/* pointer to _list_ of contributions */
};

u32	CC	(float a)
{
	int	p		= iFloor(float(a)+.5f);
	if	(p<0)	return 0; else if (p>255) return 255;
	return p;
}

void	imf_Process	(u32* dstI, u32 dstW, u32 dstH, u32* srcI, u32 srcW, u32 srcH, EIMF_Type FILTER)
{
	R_ASSERT		(dstI);	R_ASSERT	(dstW>1);	R_ASSERT	(dstH>1);
	R_ASSERT		(srcI);	R_ASSERT	(srcW>1);	R_ASSERT	(srcH>1);

	// SRC & DST images
	Image		src;	src.xsize	= srcW;	src.ysize	= srcH;	src.data	= srcI;	src.span	= srcW;
	Image		dst;	dst.xsize	= dstW;	dst.ysize	= dstH;	dst.data	= dstI;	dst.span	= dstW;

	// Select filter
	float		(*filterf)(float);	filterf		= 0;
	float		fwidth	= 0;
	switch		(FILTER)
	{
	case imf_filter:	filterf=filter;				fwidth = filter_support;	break;
	case imf_box:		filterf=box_filter;			fwidth = box_support;		break;
	case imf_triangle:	filterf=triangle_filter;	fwidth = triangle_support;	break;
	case imf_bell:		filterf=bell_filter;		fwidth = bell_support;		break;
	case imf_b_spline:	filterf=B_spline_filter;	fwidth = B_spline_support;	break;
	case imf_lanczos3:	filterf=Lanczos3_filter;	fwidth = Lanczos3_support;	break;
	case imf_mitchell:	filterf=Mitchell_filter;	fwidth = Mitchell_support;	break;
	}


	//
	Image	*tmp	= 0;			/* intermediate image */
	float	xscale	= 0, yscale = 0;/* zoom scale factors */
	int		i, j, k;				/* loop variables */
	int		n;						/* pixel number */
	float	center, left,	right;	/* filter calculation variables */
	float	width,	fscale, weight;	/* filter calculation variables */
	Pixel	*raster	= 0;			/* a row or column of pixels */
	CLIST	*contrib= 0;			/* array of contribution lists */

	/* create intermediate image to hold horizontal zoom */
	try	{
		tmp		= new_image	(dst.xsize, src.ysize);
		xscale	= float	(dst.xsize) / float(src.xsize);
		yscale	= float	(dst.ysize) / float(src.ysize);
	} catch (...) {
		Msg		("imf_Process::1");
	};

	/* pre-calculate filter contributions for a row */
	try	{
		contrib = (CLIST *)	xr_malloc	(dst.xsize*sizeof(CLIST));
		ZeroMemory(contrib,dst.xsize*sizeof(CLIST));
	} catch (...) {
		Msg		("imf_Process::2");
	};
	if(xscale < 1.0) {
		try	{
			width	= fwidth / xscale;
			fscale	= 1.0f / xscale;
			for(i = 0; i < dst.xsize; ++i)
			{
				contrib[i].n	= 0;
				contrib[i].p	= (CONTRIB *)xr_malloc((int) (width * 2 + 1)*sizeof(CONTRIB));
				ZeroMemory(contrib[i].p,(int) (width * 2 + 1)*sizeof(CONTRIB));
				center			= float(i) / xscale;
				left			= ceil	(center - width);
				right			= floor	(center + width);
				for(j = int(left); j <= int(right); ++j)
				{
					weight	= center - float(j);
					weight	= filterf(weight / fscale) / fscale;
					if(j < 0) {
						n = -j;
					} else if(j >= src.xsize) {
						n = (src.xsize - j) + src.xsize - 1;
					} else {
						n = j;
					}
					k		= contrib[i].n++;
					contrib[i].p[k].pixel	= (n<src.xsize)?n:(src.xsize-1);
					contrib[i].p[k].weight	= weight;
				}
			}
		} catch (...) {
			Msg		("imf_Process::3 (xscale<1.0)");
		};
	} else {
		try	{
			for(i = 0; i < dst.xsize; ++i)
			{
				contrib[i].n	= 0;
				contrib[i].p	= (CONTRIB *)xr_malloc((int) (fwidth * 2 + 1)*sizeof(CONTRIB));
				ZeroMemory(contrib[i].p,(int) (fwidth * 2 + 1)*sizeof(CONTRIB));
				center			= float(i) / xscale;
				left			= ceil	(center - fwidth);
				right			= floor	(center + fwidth);
				for(j = int(left); j <= int(right); ++j)
				{
					weight	= center - (float) j;
					weight	= (*filterf)(weight);
					if(j < 0) {
						n = -j;
					} else if(j >= src.xsize) {
						n = (src.xsize - j) + src.xsize - 1;
					} else {
						n = j;
					}
					k		= contrib[i].n++;
					contrib[i].p[k].pixel	= (n<src.xsize)?n:(src.xsize-1);
					contrib[i].p[k].weight	= weight;
				}
			}
		} catch (...) {
			Msg		("imf_Process::3 (xscale>1.0)");
		};
	}

	/* apply filter to zoom horizontally from src to tmp */
	try	{
		raster	= (Pixel *)xr_malloc(src.xsize*sizeof(Pixel));
		ZeroMemory(raster,src.xsize*sizeof(Pixel));
	} catch (...) {	Msg		("imf_Process::4");	};
	try	{
		for	(k = 0; k < tmp->ysize; ++k)
		{
			get_row	(raster, &src, k);
			for(i = 0; i < tmp->xsize; ++i)
			{
				float	w_r	= 0., w_g = 0., w_b	= 0., w_a = 0.;

				for	(j = 0; j < contrib[i].n; ++j)
				{
					float	W	=	contrib[i].p[j].weight;
					Pixel	P	=	raster[contrib[i].p[j].pixel];
					w_r			+=	W*float(color_get_R(P));
					w_g			+=	W*float(color_get_G(P));
					w_b			+=	W*float(color_get_B(P));
					w_a			+=	W*float(color_get_A(P));
				}
				put_pixel(tmp, i, k, color_rgba(CC(w_r),CC(w_g),CC(w_b),CC(w_a+0.5f)));
			}
		}
		xr_free(raster);
	} catch (...) {	Msg		("imf_Process::5");	};

	/* xr_free the memory allocated for horizontal filter weights */
	try	{
		for(i = 0; i < tmp->xsize; ++i) xr_free(contrib[i].p);
		xr_free(contrib);
	} catch (...) {	Msg		("imf_Process::6");	};

	/* pre-calculate filter contributions for a column */
	try	{
		contrib = (CLIST *)xr_malloc(dst.ysize*sizeof(CLIST));
		ZeroMemory(contrib,dst.ysize*sizeof(CLIST));
	} catch (...) {	Msg		("imf_Process::7");	};
	if(yscale < 1.0) {
		try	{
			width	= fwidth / yscale;
			fscale	= 1.0f / yscale;
			for	(i = 0; i < dst.ysize; ++i)
			{
				contrib[i].n	= 0;
				contrib[i].p	= (CONTRIB *)xr_malloc((int) (width * 2 + 1)*sizeof(CONTRIB));
				ZeroMemory(contrib[i].p,(int) (width * 2 + 1)*sizeof(CONTRIB));
				center			= (float) i / yscale;
				left			= ceil	(center - width);
				right			= floor	(center + width);
				for(j = int(left); j <= int(right); ++j)
				{
					weight	= center - (float) j;
					weight	= filterf(weight / fscale) / fscale;
					if(j < 0) {
						n = -j;
					} else if(j >= tmp->ysize) {
						n = (tmp->ysize - j) + tmp->ysize - 1;
					} else {
						n = j;
					}
					k = contrib[i].n++;
					contrib[i].p[k].pixel	= (n<tmp->ysize)?n:(tmp->ysize-1);
					contrib[i].p[k].weight	= weight;
				}
			}
		} catch (...) {	Msg		("imf_Process::8 (yscale<1.0)");	};
	} else {
		try	{
			for(i = 0; i < dst.ysize; ++i)
			{
				contrib[i].n	= 0;
				contrib[i].p	= (CONTRIB *)xr_malloc((int) (fwidth * 2 + 1)*sizeof(CONTRIB));
				ZeroMemory(contrib[i].p,(int) (fwidth * 2 + 1)*sizeof(CONTRIB));
				center			= (float) i / yscale;
				left			= ceil	(center - fwidth);
				right			= floor	(center + fwidth);
				for(j = int(left); j <= int(right); ++j) {
					weight = center - (float) j;
					weight = (*filterf)(weight);
					if(j < 0) {
						n = -j;
					} else if(j >= tmp->ysize) {
						n = (tmp->ysize - j) + tmp->ysize - 1;
					} else {
						n = j;
					}
					k = contrib[i].n++;
					contrib[i].p[k].pixel	= (n<tmp->ysize)?n:(tmp->ysize-1);
					contrib[i].p[k].weight	= weight;
				}
			}
		} catch (...) {	Msg		("imf_Process::8 (yscale>1.0)");	};
	}

	/* apply filter to zoom vertically from tmp to dst */
	try	{
		raster = (Pixel *)xr_malloc(tmp->ysize*sizeof(Pixel));
		ZeroMemory(raster,tmp->ysize*sizeof(Pixel));
	} catch (...) {	Msg		("imf_Process::9");	};
	try	{
		for(k = 0; k < dst.xsize; ++k)
		{
			get_column	(raster, tmp, k);
			for(i = 0; i < dst.ysize; ++i)
			{
				float	w_r	= 0., w_g = 0., w_b	= 0., w_a = 0.;

				for	(j = 0; j < contrib[i].n; ++j)
				{
					float	W	=	contrib[i].p[j].weight;
					Pixel	P	=	raster[contrib[i].p[j].pixel];
					w_r			+=	W*float(color_get_R(P));
					w_g			+=	W*float(color_get_G(P));
					w_b			+=	W*float(color_get_B(P));
					w_a			+=	W*float(color_get_A(P));
				}
				put_pixel(&dst, k, i, color_rgba(CC(w_r),CC(w_g),CC(w_b),CC(w_a+0.5f)));
			}

		}
		xr_free(raster);
	} catch (...) {	Msg		("imf_Process::A");	};

	/* xr_free the memory allocated for vertical filter weights */
	try	{
		for	(i = 0; i < dst.ysize; ++i) xr_free(contrib[i].p);
		xr_free(contrib);
	} catch (...) {	Msg		("imf_Process::B");	};

	free_image(tmp);
}
