#include "stdafx.h"

void CRenderTarget::accum_reflected		(light* L)
{
	phase_accumulator				();
	RImplementation.stats.l_visible	++;

	// *** assume accumulator setted up ***
	// *****************************	Mask by stencil		*************************************
	ref_shader		shader				= s_accum_reflected;
	ref_shader*   shader_msaa		= s_accum_reflected_msaa;

	BOOL	bIntersect			= FALSE; //enable_scissor(L);
	L->xform_calc				();
	RCache.set_xform_world		(L->m_xform			);
	RCache.set_xform_view		(Device.mView		);
	RCache.set_xform_project	(Device.mProject	);
	bIntersect					= enable_scissor	(L);
	enable_dbt_bounds			(L);

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RCache.set_ColorWriteEnable				();
	if (bIntersect)	RCache.set_CullMode		(CULL_CW);		// back
	else			RCache.set_CullMode		(CULL_CCW);		// front

	// 2D texgen (texture adjustment matrix)
	Fmatrix			m_Texgen;
	{
		float	_w						= float(Device.dwWidth);
		float	_h						= float(Device.dwHeight);
		float	o_w						= (.5f / _w);
		float	o_h						= (.5f / _h);
		Fmatrix			m_TexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f + o_w,			0.5f + o_h,			0.0f,			1.0f
		};
		m_Texgen.mul	(m_TexelAdjust,RCache.xforms.m_wvp);
	}

	// Common constants
	Fvector		L_dir,L_clr,L_pos;	float L_spec;
	L_clr.set					(L->color.r,L->color.g,L->color.b);
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_tiny	(L_pos,L->position);
	Device.mView.transform_dir	(L_dir,L->direction);
	L_dir.normalize				();

	{
		// Lighting
		RCache.set_Shader			(shader);

		// Constants
		RCache.set_c				("Ldynamic_pos",	L_pos.x,L_pos.y,L_pos.z,1/(L->range*L->range));
		RCache.set_c				("Ldynamic_color",	L_clr.x,L_clr.y,L_clr.z,L_spec);
		RCache.set_c				("direction",		L_dir.x,L_dir.y,L_dir.z,0);
		RCache.set_c				("m_texgen",		m_Texgen);

      if( ! RImplementation.o.dx10_msaa )
      {
		   RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,0x01,0xff,0x00);
		   draw_volume				(L);
      }
      else // checked Holger
      {
		   // per pixel 
		   RCache.set_Stencil	(TRUE,D3DCMP_EQUAL,0x01,0x81,0x00);
		   draw_volume				(L);
   		
		   // per sample
         if( RImplementation.o.dx10_msaa_opt )
         {
		      RCache.set_Shader		(shader_msaa[0]);
            RCache.set_Stencil	(TRUE,D3DCMP_EQUAL,0x81,0x81,0x00);
   	      if (bIntersect)	RCache.set_CullMode		(CULL_CW);		// back
	         else			RCache.set_CullMode		(CULL_CCW);		// front
            draw_volume				(L);
         }
         else // checked Holger
         {
		      for( u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++ i )
		      {
			      RCache.set_Shader		      (shader_msaa[i]);
               RCache.set_Stencil	      (TRUE,D3DCMP_EQUAL,0x81,0x81,0x00);
      	      if (bIntersect)	RCache.set_CullMode		(CULL_CW);		// back
	            else			RCache.set_CullMode		(CULL_CCW);		// front
               StateManager.SetSampleMask ( u32(1)<<i );
			      draw_volume					   (L);
		      }
		      StateManager.SetSampleMask( 0xffffffff );
         }
		   RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,0x01,0xff,0x00);
      }
	}

	// blend-copy
	if (!RImplementation.o.fp16_blend)	{
      if( ! RImplementation.o.dx10_msaa )
   		u_setrt						(rt_Accumulator,NULL,NULL,HW.pBaseZB);
      else
		   u_setrt						(rt_Accumulator,NULL,NULL,rt_MSAADepth->pZRT);
		RCache.set_Element	(s_accum_mask->E[SE_MASK_ACCUM_VOL]	);
		RCache.set_c				("m_texgen",		m_Texgen);
      if( ! RImplementation.o.dx10_msaa )
      {
		   // per pixel
		   RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,0x01,0xff,0x00);		
		   draw_volume				(L);
      }
      else // checked holger
      {
         // per pixel
         RCache.set_Stencil	(TRUE,D3DCMP_EQUAL,0x01,0x81,0x00);		
         draw_volume				(L);
         // per sample
         if( RImplementation.o.dx10_msaa_opt )
         {
		      RCache.set_Element	(s_accum_mask_msaa[0]->E[SE_MASK_ACCUM_VOL]	);
            RCache.set_Stencil	(TRUE,D3DCMP_EQUAL,0x81,0x81,0x00);		
            draw_volume				(L);
         }
         else// checked holger
         {
		      for( u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i )
			      {
			      RCache.set_Element	      (s_accum_mask_msaa[i]->E[SE_MASK_ACCUM_VOL]	);
               RCache.set_Stencil	      (TRUE,D3DCMP_EQUAL,0x81,0x81,0x00);		
               StateManager.SetSampleMask ( u32(1) << i );
               draw_volume					   (L);
			      }
		      StateManager.SetSampleMask( 0xffffffff );
         }
		   RCache.set_Stencil	(TRUE,D3DCMP_LESSEQUAL,0x01,0xff,0x00);		
      }
	}

	// 
	u_DBT_disable	();
}
