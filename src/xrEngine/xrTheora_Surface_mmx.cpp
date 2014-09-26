#include "stdafx.h"

#include "xrTheora_Surface_mmx.h"

#pragma warning( disable : 4731 )

#pragma pack( push )
#pragma pack( 1 )

typedef tv_sshort tv_sshort_tables[ 256 ][ 4 ];

#pragma pack( pop )
/*

//. width_diff = surface_width - theora_width

		u32 pos = 0;
		for (u32 h=0; h<height; ++h){

			u8* Y		= yuv.y+yuv.y_stride*h;
			u8* U		= yuv.u+yuv.uv_stride*(h/uv_h);
			u8* V		= yuv.v+yuv.uv_stride*(h/uv_h);

			for (u32 w=0; w<width; ++w){

				u8 y	= Y[w];
				u8 u	= U[w/uv_w];
				u8 v	= V[w/uv_w];

				int C	= y - 16;
				int D	= u - 128;
				int E	= v - 128;

				int R	= clampr(( 298 * C           + 409 * E + 128) >> 8,0,255);
				int G	= clampr(( 298 * C - 100 * D - 208 * E + 128) >> 8,0,255);
				int B	= clampr(( 298 * C + 516 * D           + 128) >> 8,0,255);

				data[++pos] = color_rgba(R,G,B,255);

				if(w==(width-1))
					pos += width_diff;
			}
		}
*/

lp_tv_uchar tv_yuv2argb(
						lp_tv_uchar			argb_plane ,
						tv_slong			argb_width ,
						tv_slong			argb_height ,
						lp_tv_uchar			y_plane ,
						tv_slong			y_width ,
						tv_slong			y_height ,
						tv_slong			y_stride ,
						lp_tv_uchar 		u_plane ,
						lp_tv_uchar 		v_plane ,
						tv_slong 			uv_width ,
						tv_slong 			uv_height ,
						tv_slong 			uv_stride,
						tv_slong 			width_diff 
						)
{
	tv_sshort_tables ttl;

	__asm{
		push  ebx
		// helper constants
		mov   esi,-14487936
		mov   edi,-5822464
		mov   ecx,-2785792
		mov   edx,-14496256

		lea   ebx,DWORD PTR [ttl + 2]

		// building helper tables
		ALIGN 4
_tb_loop:
		mov   eax,esi
		sar   eax,16
		mov   WORD PTR [ebx-2],ax

		mov   eax,edi
		sar   eax,16
		mov   WORD PTR [ebx+0],ax

		mov   eax,ecx
		sar   eax,16
		mov   WORD PTR [ebx+2],ax

		mov   eax,edx
		sar   eax,16
		mov   WORD PTR [ebx+4],ax

		add   esi,113443
		add   edi,45744
		add   ecx,22020
		add   edx,113508

		add   ebx,4 * ( TYPE tv_sshort )
		cmp   esi,14553472

		jl   _tb_loop

		pop   ebx
	}

	lp_tv_uchar line1 = argb_plane;
	lp_tv_uchar line2 = line1 + 4 * argb_width;

	lp_tv_uchar y1 = y_plane;
	lp_tv_uchar y2 = y1 + y_stride;

	lp_tv_uchar u = u_plane;
	lp_tv_uchar v = v_plane;

	int nTempX;
	int nTempY;
	int nTempX_;

	for( nTempY = 0 ; nTempY < argb_height ; nTempY += 2 ){
		for( nTempX = 0 ; nTempX < argb_width ; nTempX += 4 ){
			nTempX_ = nTempX >> 1;
			__asm{
				push ebx       ;

				mov  eax,DWORD PTR y1   ; eax = y1
				mov  ebx,DWORD PTR y2   ; ebx = y2
				mov  edi,DWORD PTR v    ; edi = v

				add  eax,DWORD PTR nTempX  ; eax = y1 + nTempX
				add  ebx,DWORD PTR nTempX  ; ebx = y2 + nTempX

				pxor mm2,mm2      ; mm2 = 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0

				add  edi,DWORD PTR nTempX_  ; edi = v + nTempX_
				lea  esi,DWORD PTR ttl   ; esi = ttl

				movd mm0,DWORD PTR [eax]   ; mm0 = 0 | 0 | 0 | 0 | nY4 | nY3 | nY2 | nY1
				movd mm1,DWORD PTR [ebx]   ; mm1 = 0 | 0 | 0 | 0 | nY8 | nY7 | nY6 | nY5

				movzx edx,DWORD PTR [edi]   ; edx = V1
				movzx ecx,DWORD PTR [edi+1]  ; ecx = V2

				punpcklbw mm0,mm2     ; mm0 = nY4 | nY3 | nY2 | nY1
				punpcklbw mm1,mm2     ; mm1 = nY8 | nY7 | nY6 | nY5

				pinsrw mm4,WORD PTR [esi+edx*8+0],00000000b ; mm4 = 0 | 0 | 0 | ttl[nV1][0]
				pinsrw mm5,WORD PTR [esi+ecx*8+0],00000000b ; mm5 = 0 | 0 | 0 | ttl[nV2][0]

				movq mm3,mm0      ; mm3 = nY4 | nY3 | nY2 | nY1
				mov  edi,DWORD PTR u    ; edi = u

				punpckldq mm3,mm1     ; mm3 = nY6 | nY5 | nY2 | nY1
				punpckhdq mm0,mm1     ; mm0 = nY8 | nY7 | nY4 | nY3

				pshufw mm4,mm4,00000000b   ; mm4 = ttl[nV1][0] | ttl[nV1][0] | ttl[nV1][0] | ttl[nV1][0]
				pshufw mm5,mm5,00000000b   ; mm5 = ttl[nV2][0] | ttl[nV2][0] | ttl[nV2][0] | ttl[nV2][0]

				add  edi,DWORD PTR nTempX_  ; edi = u + nTempX_

				paddsw mm4,mm3      ; mm4 = P6.R | P5.R | P2.R | P1.R
				paddsw mm5,mm0      ; mm5 = P8.R | P7.R | P4.R | P3.R

				pinsrw mm1,WORD PTR [esi+edx*8+2],00000000b ; mm1 = 0 | 0 | 0 | ttl[nV1][1]
				pinsrw mm2,WORD PTR [esi+ecx*8+2],00000000b ; mm2 = 0 | 0 | 0 | ttl[nV2][1]

				movq mm6,mm3      ; mm6 = nY6 | nY5 | nY2 | nY1
				movq mm7,mm0      ; mm7 = nY8 | nY7 | nY4 | nY3

				movzx edx,DWORD PTR [edi]   ; edx = U1
				movzx ecx,DWORD PTR [edi+1]  ; ecx = U2

				pshufw mm1,mm1,00000000b   ; mm1 = ttl[nV1][1] | ttl[nV1][1] | ttl[nV1][1] | ttl[nV1][1]
				pshufw mm2,mm2,00000000b   ; mm2 = ttl[nV2][1] | ttl[nV2][1] | ttl[nV2][1] | ttl[nV2][1]

				psubsw mm6,mm1      ; mm6 = nY6 - ttl[nV1][1] | nY5 - ttl[nV1][1] | nY2 - ttl[nV1][1] | nY1 - ttl[nV1][1]
				psubsw mm7,mm2      ; mm7 = nY8 - ttl[nV2][1] | nY7 - ttl[nV2][1] | nY4 - ttl[nV2][1] | nY3 - ttl[nV2][1]

				pinsrw mm1,WORD PTR [esi+edx*8+4],00000000b ; mm1 = 0 | 0 | 0 | ttl[nU1][2]
				pinsrw mm2,WORD PTR [esi+ecx*8+4],00000000b ; mm2 = 0 | 0 | 0 | ttl[nU2][2]

				pshufw mm1,mm1,00000000b   ; mm1 = ttl[nU1][2] | ttl[nU1][2] | ttl[nU1][2] | ttl[nU1][2]
				pshufw mm2,mm2,00000000b   ; mm2 = ttl[nU2][2] | ttl[nU2][2] | ttl[nU2][2] | ttl[nU2][2]

				psubsw mm6,mm1      ; mm6 = P6.G | P5.G | P2.G | P1.G
				psubsw mm7,mm2      ; mm7 = P8.G | P7.G | P4.G | P3.G

				pinsrw mm1,WORD PTR [esi+edx*8+6],00000000b ; mm1 = 0 | 0 | 0 | ttl[nU1][3]
				pinsrw mm2,WORD PTR [esi+ecx*8+6],00000000b ; mm2 = 0 | 0 | 0 | ttl[nU2][3]

				pshufw mm1,mm1,00000000b   ; mm1 = ttl[nU1][3] | ttl[nU1][3] | ttl[nU1][3] | ttl[nU1][3]
				pshufw mm2,mm2,00000000b   ; mm2 = ttl[nU2][3] | ttl[nU2][3] | ttl[nU2][3] | ttl[nU2][3]

				paddsw mm3,mm1      ; mm3 = P6.B | P5.B | P2.B | P1.B
				paddsw mm0,mm2      ; mm0 = P8.B | P7.B | P4.B | P3.B


				// we have
				; mm4 = P6.R | P5.R | P2.R | P1.R
				; mm6 = P6.G | P5.G | P2.G | P1.G
				; mm3 = P6.B | P5.B | P2.B | P1.B

				; mm5 = P8.R | P7.R | P4.R | P3.R
				; mm7 = P8.G | P7.G | P4.G | P3.G
				; mm0 = P8.B | P7.B | P4.B | P3.B

				// saturation
				packuswb mm4,mm5 ; mm4 = P8.R | P7.R | P4.R | P3.R | P6.R | P5.R | P2.R | P1.R
				packuswb mm6,mm7 ; mm6 = P8.G | P7.G | P4.G | P3.G | P6.G | P5.G | P2.G | P1.G
				packuswb mm3,mm0 ; mm3 = P8.B | P7.B | P4.B | P3.B | P6.B | P5.B | P2.B | P1.B

				// calculating effective store address
				mov  esi,DWORD PTR line1   ; esi = line1
				mov  edi,DWORD PTR line2   ; edi = line2

				// we want
				;px1 = 00 | P2.R | P2.G | P2.B | 00 | P1.R | P1.G | P1.B |
				;px2 = 00 | P4.R | P4.G | P4.B | 00 | P3.R | P3.G | P3.B |

				;px3 = 00 | P6.R | P6.G | P6.B | 00 | P5.R | P5.G | P5.B |
				;px4 = 00 | P8.R | P8.G | P8.B | 00 | P7.R | P7.G | P7.B |

				// Oh, real sex!
				pcmpeqd mm0,mm0  ; mm0 = 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1

				movq mm5,mm6  ; mm5 = P8.G | P7.G | P4.G | P3.G | P6.G | P5.G | P2.G | P1.G
				movq mm7,mm3  ; mm7 = P8.B | P7.B | P4.B | P3.B | P6.B | P5.B | P2.B | P1.B

				punpcklbw mm5,mm0 ; mm5 = 0 | P6.G | 0 | P5.G | 0 | P2.G | 0 | P1.G
				punpcklbw mm7,mm4 ; mm7 = P6.R | P6.B | P5.R | P5.B | P2.R | P2.B | P1.R | P1.B

				movq  mm1,mm7 ; mm1 = P6.R | P6.B | P5.R | P5.B | P2.R | P2.B | P1.R | P1.B

				punpcklbw mm7,mm5 ; mm7 = 0 | P2.R | P2.G | P2.B | 0 | P1.R | P1.G | P1.B 
				// px1
				punpckhbw mm1,mm5 ; mm1 = 0 | P6.R | P6.G | P6.B | 0 | P5.R | P5.G | P5.B 
				// px3

				movq  mm2,mm6 ; mm2 = P8.G | P7.G | P4.G | P3.G | P6.G | P5.G | P2.G | P1.G
				movq  mm5,mm3 ; mm5 = P8.B | P7.B | P4.B | P3.B | P6.B | P5.B | P2.B | P1.B

				punpckhbw mm2,mm0 ; mm2 = 0 | P8.G | 0 | P7.G | 0 | P4.G | 0 | P3.G
				punpckhbw mm5,mm4 ; mm5 = P8.R | P8.B | P7.R | P7.B | P4.R | P4.B | P3.R | P3.B

				movq  mm0,mm5 ; mm0 = P8.R | P8.B | P7.R | P7.B | P4.R | P4.B | P3.R | P3.B

				punpckhbw mm5,mm2 ; mm5 = 0 | P8.R | P8.G | P8.B | 0 | P7.R | P7.G | P7.B 
				// px4
				punpcklbw mm0,mm2 ; mm0 = 0 | P4.R | P4.G | P4.B | 0 | P3.R | P3.G | P3.B 
				// px2

				// storing using non-temporal hint
				movntq  MMWORD PTR [esi+0],mm7 ;
				movntq  MMWORD PTR [esi+8],mm0 ;

				movntq  MMWORD PTR [edi+0],mm1 ;
				movntq  MMWORD PTR [edi+8],mm5 ;

				// we are the champions
				pop  ebx       ;
			}

			line1 += 16;
			line2 += 16;
		}

		y1 += 2 * y_stride;
		y2 = y1 + y_stride;

		u += uv_stride;
		v += uv_stride;

		line1 += 4 * argb_width;
		line2 = line1 + 4 * argb_width;
	}

	__asm{
		sfence        ;
		emms        ;
	}

	return argb_plane;
} // tv_yuv2argb

#pragma warning( default : 4731 )
