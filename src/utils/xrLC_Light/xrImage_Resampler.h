#ifndef xrImage_ResamplerH
#define xrImage_ResamplerH

#pragma		once

enum		EIMF_Type
{
	imf_filter=0,
	imf_box,
	imf_triangle,
	imf_bell,
	imf_b_spline,
	imf_lanczos3,
	imf_mitchell,

	imf_FORCEDWORD=0xffffffff
};
XRLC_LIGHT_API	void imf_Process(u32* dst, u32 dstW, u32 dstH, u32* src, u32 srcW, u32 srcH, EIMF_Type FILTER);

#endif
