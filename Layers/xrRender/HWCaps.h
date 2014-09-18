#ifndef _HW_CAPS_
#define _HW_CAPS_
#pragma once

#define	CAP_VERSION(a,b)	(u32(a)*10 + u32(b))

class  CHWCaps
{
public:
	enum
	{
		MAX_GPUS		= 8
	};

public:
	struct		caps_Geometry
	{
		u32	dwRegisters		: 16;
		u32 dwInstructions	: 16;
		u32	bSoftware		: 1;
		u32	bPointSprites	: 1;
		u32	bVTF			: 1;		// vertex-texture-fetch
		u32	bNPatches		: 1;
		u32 dwClipPlanes	: 4;
		u32 dwVertexCache	: 8;
	};
	struct		caps_Raster
	{
		u32	dwRegisters		: 16;
		u32 dwInstructions	: 16;
		u32	dwStages		: 4;		// number of tex-stages
		u32	dwMRT_count		: 4;
		u32 b_MRT_mixdepth	: 1;
		u32	bNonPow2		: 1;
		u32	bCubemap		: 1;
	};
public:
	// force flags
	BOOL			bForceGPU_REF;
	BOOL			bForceGPU_SW;
	BOOL			bForceGPU_NonPure;
	BOOL			SceneMode;

	u32				iGPUNum;

	// device format
	D3DFORMAT		fTarget;
	D3DFORMAT		fDepth;
	u32				dwRefreshRate;

	// caps itself
	u16				geometry_major	;
	u16				geometry_minor	;
	caps_Geometry	geometry		;
	u16				raster_major	;
	u16				raster_minor	;
	caps_Raster		raster			;

	u32				id_vendor		;
	u32				id_device		;

	BOOL			bStencil;			// stencil buffer present
	BOOL			bScissor;			// scissor rect supported
	BOOL			bTableFog;			//

	// some precalculated values
	D3DSTENCILOP	soDec, soInc;		// best stencil OPs for shadows
	u32				dwMaxStencilValue;  // maximum value the stencil buffer can hold

	void			Update(void);
};

#endif
