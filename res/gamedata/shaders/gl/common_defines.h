#ifndef	common_defines_h_included
#define	common_defines_h_included

//////////////////////////////////////////////////////////////////////////////////////////
// Defines
#define def_gloss       float(2.f /255.f)
#define def_aref        float(200.f/255.f)
#define def_dbumph      float(0.333f)
#define def_virtualh    float(0.05f)              // 5cm
#define def_distort     float(0.05f)              // we get -0.5 .. 0.5 range, this is -512 .. 512 for 1024, so scale it
#define def_hdr         float(9.f)                // hight luminance range float(3.h)
#define def_hdr_clip	  float(0.75f)              //

#define	LUMINANCE_VECTOR	float3(0.3f, 0.38f, 0.22f)

//////////////////////////////////////////////////////////////////////////////////////////
// skyloader: if you want to resize smaps in renderer, then do not forget to change this file too
#ifndef SMAP_QUALITY
const float SMAP_size = 2048.f;
#elif SMAP_QUALITY == 1
const float SMAP_size = 1536.f;
#elif SMAP_QUALITY == 2
const float SMAP_size = 2048.f;
#elif SMAP_QUALITY == 3
const float SMAP_size = 2560.f;
#elif SMAP_QUALITY == 4
const float SMAP_size = 3072.f;
#elif SMAP_QUALITY == 5
const float SMAP_size = 4096.f;
#endif
//////////////////////////////////////////////////////////////////////////////////////////
#define PARALLAX_H      float(0.02f)
#define parallax        float2(PARALLAX_H, -PARALLAX_H/2)
//////////////////////////////////////////////////////////////////////////////////////////

#endif	//	common_defines_h_included
