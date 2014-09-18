#include "stdafx.h"
#pragma hdrstop

#ifdef _EDITOR
	#include "skeletonX.h"
 	#include "skeletoncustom.h"
#else
#include "../xrEngine/bone.h"
#include "../Layers/xrRender/SkeletonXVertRender.h"
//#	include "../xrEngine/skeletonX.h"
//#	include "../xrEngine/skeletoncustom.h"
//#	include "../Layers/xrRender/skeletonX.h"
//#	include "../Layers/xrRender/skeletoncustom.h"
#endif

void __stdcall xrSkin1W_x86(	vertRender*		D,
								vertBoned1W*	S,
								u32				vCount,
								CBoneInstance*	Bones) 
{
//	return;
	// Prepare
	int U_Count			= vCount/8;
	vertBoned1W*	V	= S;
	vertBoned1W*	E	= V+U_Count*8;

	// Unrolled loop
	for (; S!=E; )
	{
		Fmatrix& M0		= Bones[S->matrix].mRenderTransform;
		M0.transform_tiny(D->P,S->P);
		M0.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++;
		
		Fmatrix& M1		= Bones[S->matrix].mRenderTransform;
		M1.transform_tiny(D->P,S->P);
		M1.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++;
		
		Fmatrix& M2		= Bones[S->matrix].mRenderTransform;
		M2.transform_tiny(D->P,S->P);
		M2.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++;
		
		Fmatrix& M3		= Bones[S->matrix].mRenderTransform;
		M3.transform_tiny(D->P,S->P);
		M3.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++; 
		
		Fmatrix& M4		= Bones[S->matrix].mRenderTransform;
		M4.transform_tiny(D->P,S->P);
		M4.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++;
		
		Fmatrix& M5		= Bones[S->matrix].mRenderTransform;
		M5.transform_tiny(D->P,S->P);
		M5.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++;
		
		Fmatrix& M6		= Bones[S->matrix].mRenderTransform;
		M6.transform_tiny(D->P,S->P);
		M6.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++;
		
		Fmatrix& M7		= Bones[S->matrix].mRenderTransform;
		M7.transform_tiny(D->P,S->P);
		M7.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++; 
	}
	
	// The end part
	vertBoned1W* E2 = V+vCount;
	for (; S!=E2; )
	{
		Fmatrix& M		= Bones[S->matrix].mRenderTransform;
		M.transform_tiny(D->P,S->P);
		M.transform_dir (D->N,S->N);
		D->u			= S->u;
		D->v			= S->v;
		S++; D++;
	}
}
 