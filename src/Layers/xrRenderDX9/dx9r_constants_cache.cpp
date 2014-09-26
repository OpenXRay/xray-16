#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/r_constants_cache.h"

void R_constants::flush_cache()
{
	if (a_pixel.b_dirty)
	{
		// fp
		R_constant_array::t_f&	F	= a_pixel.c_f;
		{
			if (F.r_lo() <= 32) //. hack
			{		
				u32		count		= F.r_hi()-F.r_lo();
				if (count)			{
					count = (count>31)?31:count;
					PGO		(Msg("PGO:P_CONST:%d",count));
					CHK_DX	(HW.pDevice->SetPixelShaderConstantF	(F.r_lo(), (float*)F.access(F.r_lo()),count));
					F.flush	();
				}
			}
		}
		a_pixel.b_dirty		= false;
	}
	if (a_vertex.b_dirty)
	{
		// fp
		R_constant_array::t_f&	F	= a_vertex.c_f;
		{
			u32		count		= F.r_hi()-F.r_lo();
			if (count)			{
#ifdef DEBUG
				if (F.r_hi() > HW.Caps.geometry.dwRegisters)
				{
					Debug.fatal(DEBUG_INFO,"Internal error setting VS-constants: overflow\nregs[%d],hi[%d]",
						HW.Caps.geometry.dwRegisters,F.r_hi()
						);
				}
				PGO		(Msg("PGO:V_CONST:%d",count));
#endif				&
				CHK_DX	(HW.pDevice->SetVertexShaderConstantF	(F.r_lo(), (float*)F.access(F.r_lo()),count));
				F.flush	();
			}
		}
		a_vertex.b_dirty	= false;
	}
}