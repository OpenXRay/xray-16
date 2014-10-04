#ifndef	MBLUR_H
#define MBLUR_H

#ifndef 	USE_MBLUR
half3 	mblur	(float2 UV, half3 pos, half3 c_original)	{ return c_original; }
#else
#include "common.h"

uniform half4x4	m_current;
uniform half4x4	m_previous;
uniform half2 	m_blur;		// scale_x / 12, scale_y / 12

#define MBLUR_SAMPLES 	half(12)
#define MBLUR_CLAMP	half(0.001)

half3 	mblur	(float2 UV, half3 pos, half3 c_original)	{
	half4 	pos4		= half4	(pos,1.h);

	half4 	p_current	= mul	(m_current,	pos4);
	half4 	p_previous 	= mul	(m_previous,	pos4);
	half2 	p_velocity 	= m_blur * ( (p_current.xy/p_current.w)-(p_previous.xy/p_previous.w) );
		p_velocity	= clamp	(p_velocity,-MBLUR_CLAMP,+MBLUR_CLAMP);

	// For each sample, sum up each sample's color in "Blurred" and then divide
	// to average the color after all the samples are added.
	half3 	blurred 	= 	c_original	;
        	blurred 	+= 	tex2D(s_image, p_velocity * 1.h  + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 2.h  + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 3.h  + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 4.h  + UV).rgb;
        	blurred 	+= 	tex2D(s_image, p_velocity * 5.h  + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 6.h  + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 7.h  + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 8.h  + UV).rgb;
        	blurred 	+= 	tex2D(s_image, p_velocity * 9.h  + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 10.h + UV).rgb;
		blurred		+= 	tex2D(s_image, p_velocity * 11.h + UV).rgb;
	return 	blurred/MBLUR_SAMPLES;
}
#endif

#endif
