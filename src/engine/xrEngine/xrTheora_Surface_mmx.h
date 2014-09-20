#pragma once

typedef unsigned char  tv_uchar, * lp_tv_uchar ;
typedef signed char   tv_schar, * lp_tv_schar ;
typedef unsigned short int tv_ushort, * lp_tv_ushort ;
typedef signed short int tv_sshort, * lp_tv_sshort ;
typedef unsigned long int tv_ulong, * lp_tv_ulong ;
typedef signed long int  tv_slong, * lp_tv_slong ;

lp_tv_uchar tv_yuv2argb(lp_tv_uchar argb_plane, tv_slong argb_width, tv_slong argb_height, 
						lp_tv_uchar y_plane, tv_slong y_width, tv_slong y_height, tv_slong y_stride,
						lp_tv_uchar u_plane, lp_tv_uchar v_plane,
						tv_slong uv_width, tv_slong uv_height, tv_slong uv_stride, tv_slong width_diff );
