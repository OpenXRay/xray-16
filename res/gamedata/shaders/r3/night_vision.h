#include "common.h"
uniform float4  shader_param_8;
uniform float4  shader_param_7;

///////////////////////////////////////////////////////
//      BEEF'S SHADER BASED NIGHT VISION EFFECT      //
///////////////////////////////////////////////////////
// Huge credit TO LVutner from Anomaly Discord, who  //
// literally taught me everything I know, to Sky4Ace //
// who's simple_blur function I've adapted for this  //
// shader, and to Meltac, who provided some advice   //
// and inspiration for developing this shader.       //
///////////////////////////////////////////////////////
// Note: You are free to distribute and adapt this   //
// Shader and any components, just please provide    //
// credit to myself and/or the above individuals. I  //
// have provided credit for individual functions and //
// their original authors where applicable.	- BEEF   //
///////////////////////////////////////////////////////


///////////////////////////////////////////////////////
// USER SETTINGS HERE
///////////////////////////////////////////////////////

	//////// GLOBAL SETTINGS(ALL GENERATIONS)////////
		#define sky_brightness_factor float (1.0f)			// Sky brightness factor. If your sky is too bright, decrease this slightly, if too dim, increase this slightly.
		
	// NVG CRT & NOISE VALUES 
	//(BE AWARE THAT NOISE AND SCINTILLATION SCALE WITH NVG GAIN)
		
		#define gen_1_crt_effect_factor float(0.05)				// Default 0.05 - How much CRT effect to add to NVG image (0 = none, 1 = max).
		#define gen_1_nvg_noise_factor float (0.15)				// Default 0.15 - How much noise to add to NVG image. (0 = none, 0.5 = insane)
		#define gen_1_scintillation_constant float (0.999f) 	// Default 0.999 - The closer the number is to 1.00000, the less scintillation effect.
		#define gen_1_luma_sharpen_factor float(0.0f)			// Default 0.0 - How much luma sharpen is added in. (0 = none, 1 = max)
		
		#define gen_2_crt_effect_factor float(0.07)				// Default 0.07 - How much CRT effect to add to NVG image (0 = none, 1 = max).
		#define gen_2_nvg_noise_factor float (0.1)				// Default 0.15 - How much noise to add to NVG image. (0 = none, 0.5 = insane)
		#define gen_2_scintillation_constant float (0.9991f) 	// Default 0.9991 - The closer the number is to 1.00000, the less scintillation effect.
		#define gen_2_luma_sharpen_factor float(0.25f)			// Default 0.25f - How much luma sharpen is added in. (0 = none, 1 = max)
	
		#define gen_3_crt_effect_factor float(0.1)				// Default 0.1 - How much CRT effect to add to NVG image (0 = none, 1 = max).
		#define gen_3_nvg_noise_factor float (0.08)				// Default 0.15 - How much noise to add to NVG image. (0 = none, 0.5 = insane)
		#define gen_3_scintillation_constant float (0.9992f) 	// Default 0.9992 - The closer the number is to 1.00000, the less scintillation effect.
		#define gen_3_luma_sharpen_factor float(0.5f)			// Default 0.5 - How much luma sharpen is added in. (0 = none, 1 = max)
		
	// NVG COLOR OPTIONS:

		#define gen_1_saturation_color float3 (0.7,1,0.6)	// Gen1 NVG color - it defines the max amount of color from 0 to 1 using (Red,Green,Blue)
		#define gen_2_saturation_color float3 (0.5,1,0.4)	// Gen1 NVG color - it defines the max amount of color from 0 to 1 using (Red,Green,Blue)
		#define gen_3_saturation_color float3 (0.2,1,0.9)	// Gen1 NVG color - it defines the max amount of color from 0 to 1 using (Red,Green,Blue)

		// EXAMPLE COLOR SCHEMES:
		//  (0.7,1,0.6) - gen 1 soft green
		//	(0.5,1,0.4) - gen 2 hard green
		//  (0.2,1,0.9) - gen 3 blueish
		//	(1.0,1.0,1.0) - black and white
		// 	(1.0,0.7,0.1) - yellow or amber color
		
///////////////////////////////////////////////////////
// PROBABLY DON'T CHANGE STUFF BELOW HERE UNLESS YOU KNOW WHAT YOU'RE DOING
///////////////////////////////////////////////////////
										
	// Constants
		#define tube_radius float (frac(shader_param_7.y))
		
		#define single_tube_centered float2(0.5f, -0.5f + (floor(shader_param_7.x) / 100))
		#define single_tube_offset_left float2(0.25f, -0.5f + (floor(shader_param_7.x) / 100))		// Single tube screen position (0.5, 0.5 is centered)
		#define single_tube_offset_right float2(0.75f, -0.5f + (floor(shader_param_7.x) / 100))	// Single tube screen position (0.5, 0.5 is centered)
		
		#define dual_tube_offset_1 float2(0.25f, -0.5f + (floor(shader_param_7.x) / 100))			// Offset for dual tube left eye
		#define dual_tube_offset_2 float2(0.75f, -0.5f + (floor(shader_param_7.x) / 100))			// Offset for dual tube right eye
		
		#define quad_tube_offset_1 float2(0.05f, -0.5f + (floor(shader_param_7.x) / 100))			// Offset for quad tube left outer tube
		#define quad_tube_offset_2 float2(0.3f, -0.5f + (floor(shader_param_7.x) / 100))			// Offset for quad tube left inner tube
		#define quad_tube_offset_3 float2(0.7f, -0.5f + (floor(shader_param_7.x) / 100))			// Offset for quad tube right inner tube
		#define quad_tube_offset_4 float2(0.95f, -0.5f + (floor(shader_param_7.x) / 100))			// Offset for quad tube right outer tube
	
	#define luma_conversion_coeff float3 (0.299, 0.587, 0.114)// When we convert to YUV, these are the coefficients for Y (since we discard UV)
	#define farthest_depth float (25.0f) 						// The farthest far place that we can reach in regards to DOF effects
	
	
	// LUA PACKING
		//	local x_1 = tostring(nvg_generation)                           -- Generation (1,2,3) - outputs 1.x to 3.x
		//	local x_2 = tostring(nvg_num_tubes)                            -- Num Tubes (1,2,4,11,12) outputs x.1, x.2, x.4, x.11, or x.12
		//	local y_1 = tostring(math.floor(nvg_gain_current * 10))        -- Gain Adjust (0.1 to 3) -- outputs 1.y to 30.y in 1. increment
		//	local y_2 = tostring(math.floor(nvg_washout_thresh * 10))      -- Washout Thresh (0.0 - 0.9) - outputs y.0 to y.9 in .1 increment
		//	local z_1 = tostring(math.floor(vignette_current * 100))       -- Vignette Amount (0.0 to 1.0) outputs 0.z to 100.z in 1. increment
		//	local z_2 = tostring(math.floor(glitch_power * 10))            -- Glitch Power (0.0 - 0.9) - outputs z.0 to z.9 in .1 increment
		//	local w_1 = tostring((math.floor(nvg_gain_offset * 10)) )      -- Gain Offset (0.5 to 3.0) - outputs 5.w to 30.w in 1. increment
		//	local w_2 = tostring(nvg_mode)                  		         -- Mode (0,1) - outputs w.0 or w.1 depending on mode

	// SHADER UNPACKING
		// float lua_param_nvg_generation = floor(shader_param_8.x);           	  // 1, 2, or 3
		// float lua_param_nvg_num_tubes = frac(shader_param_8.x) * 10.0f;   	  // 1, 2, 4, 1.1, or 1.2
		// float lua_param_nvg_gain_current = floor(shader_param_8.y) / 10.0f;	  // 0.0 to 3.0
		// float lua_param_nvg_washout_thresh = frac(shader_param_8.y);    		  // 0.0 to 0.9
		// float lua_param_vignette_current = floor(shader_param_8.z) / 100.0f;   // 0.0 to 1.0
		// float lua_param_glitch_power = frac(shader_param_8.z);        		  // 0.0 to 0.9
		// float lua_param_nvg_gain_offset = floor(shader_param_8.w) / 10.0f;	  // 0.5 to 3.0
		// float lua_param_nvg_mode = frac(shader_param_8.w) * 10;                // 0 to 9 in 1.0 increment 
			// mode 0: blurred background (default)
			// mode 1: black background
			// mode 2: image overlay
			// mode 3: no changes (clear vision)
			
			
		// float lua_param_flip_down = floor(shader_param_7.x);
		
		
		// float lua_param_nvg_radius = frac(shader_param_7.y);

	
///////////////////////////////////////////////////////
// DEFINE NVG MASK (Credit to LVutner for huge assistance in designing the functions)
///////////////////////////////////////////////////////
float compute_lens_mask(float2 masktc, float num_tubes)
{
	float lua_param_flip_down = floor(shader_param_7.x);
	lua_param_flip_down = clamp(5 - (lua_param_flip_down / 20.0f),1.0f,5.0f);
	masktc.y = masktc.y * lua_param_flip_down;
	
	if (num_tubes > 0.99f && num_tubes < 1.01f) // One tube centered
		{
			return step(distance(masktc,single_tube_centered), tube_radius);
		}
	else if (num_tubes > 1.09f && num_tubes < 1.11f) // One tube left offset
		{
			return step(distance(masktc,single_tube_offset_left), tube_radius);
		}
	else if (num_tubes > 1.19f && num_tubes < 1.21f) // One tube right offset
		{
			return step(distance(masktc,single_tube_offset_right), tube_radius);
		}
	else if (num_tubes > 1.99f && num_tubes < 2.01f) // Two tubes
		{
			if ( (step(distance(masktc,dual_tube_offset_1), tube_radius) == 1) || (step(distance(masktc,dual_tube_offset_2), tube_radius) == 1))
				{
				return 1.0f;
				}
			else
				{
				return 0.0f;
				}
		}
	else if (num_tubes > 3.99f && num_tubes < 4.01f) // Four Tubes
		{
		if  (((step(distance(masktc,quad_tube_offset_1), tube_radius) == 1) || (step(distance(masktc,quad_tube_offset_2), tube_radius) == 1)) || ((step(distance(masktc,quad_tube_offset_3), tube_radius) == 1) || (step(distance(masktc,quad_tube_offset_4), tube_radius) == 1)))
				{
				return 1.0f;
				}
			else
				{
				return 0.0f;
				}
		}
	else
	{
		return 0.0f;
	}
}

///////////////////////////////////////////////////////
// ASPECT RATIO CORRECTION (Credit LVutner)
///////////////////////////////////////////////////////
float2 aspect_ratio_correction (float2 tc)
	{
	tc.x -= 0.5f;
    tc.x *= (screen_res.x / screen_res.y);
    tc.x += 0.5f;
	return tc;
	}


///////////////////////////////////////////////////////
// CRT EFFECT (adapted from MattiasCRT on ShaderToy, credit Mattias)
///////////////////////////////////////////////////////
float2 curve_texturecoords(float2 curved_tc)
{
	curved_tc = (curved_tc - 0.5) * 2.0;
	curved_tc *= 1.1;	
	curved_tc.x *= 1.0 + pow((abs(curved_tc.y) / 5.0), 2.0);
	curved_tc.y *= 1.0 + pow((abs(curved_tc.x) / 4.0), 2.0);
	curved_tc  = (curved_tc / 2.0) + 0.5;
	curved_tc =  curved_tc *0.92 + 0.04;
	return curved_tc;
}

float3 make_crt_ified(float3 fragColor, float2 tc )
{
	//float2 screen_res = screen_res.xy; // define screen res variable
    float2 uv = curve_texturecoords(tc);
    float3 col;
	float x =  sin(0.3*timers.x+uv.y*21.0)*sin(0.7*timers.x+uv.y*29.0)*sin(0.3+0.33*timers.x+uv.y*31.0)*0.0017;

	col.r = fragColor.r;
	col.g = fragColor.g;
	col.b = fragColor.b;
	
	col.r += 0.08f * fragColor.r;
	col.g += 0.05f * fragColor.g;
   	col.b += 0.08f * fragColor.b;

    col = clamp(col*0.6+0.4*col*col*1.0,0.0,1.0);

    float vig = (0.0 + 1.0*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y));
	col *= float(pow(abs(vig),0.3));

    col *= float3(0.95,1.05,0.95);
	col *= 2.8;
	
	float scans = clamp( 0.35+0.35*sin(uv.y*screen_res.y*2.0), 0.0, 1.0);
	
	float s = pow(scans,1.7);
	col = col*float( 0.4+0.7*s) ;

    col *= 1.0+0.01*sin(110.0*timers.x);
	if (uv.x < 0.0 || uv.x > 1.0)
		col *= 0.0;
	if (uv.y < 0.0 || uv.y > 1.0)
		col *= 0.0;
	
	col*=1.0-0.65*float(clamp(((tc.x % 2.0)-1.0)*2.0,0.0f,1.0f));
	
    float comp = smoothstep( 0.1, 0.9, sin(timers.x) );
 
    fragColor = col;
	return fragColor;
}

///////////////////////////////////////////////////////
// VIGNETTE CALCULATOR (USED IN NVG AS WELL AS BLOOM PHASES TO DARKEN EDGES OF SHIT)
///////////////////////////////////////////////////////
float calc_vignette (float num_tubes, float2 tc, float vignette_amount)
{
	float vignette;
	float2 corrected_texturecoords = aspect_ratio_correction(tc);
	
	float lua_param_flip_down = floor(shader_param_7.x);
	lua_param_flip_down = clamp(5 - (lua_param_flip_down / 20.0f),1.0f,5.0f);
	
	corrected_texturecoords.y = corrected_texturecoords.y * lua_param_flip_down;
	
	if (num_tubes > 0.99f && num_tubes < 1.01f)
	{
		float gen1_vignette = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,single_tube_centered)),3);
		vignette = 1.0 - (1.0 - gen1_vignette); // apply vignette
	}
	else if (num_tubes > 1.09f && num_tubes < 1.11f)
	{
		float gen1_vignette = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,single_tube_offset_left)),3);
		vignette = 1.0 - (1.0 - gen1_vignette); // apply vignette
	}
	
	else if (num_tubes > 1.19f && num_tubes < 1.21f)
	{
		float gen1_vignette = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,single_tube_offset_right)),3);
		vignette = 1.0 - (1.0 - gen1_vignette); // apply vignette
	}
	else if (num_tubes > 1.99f && num_tubes < 2.01f)
	{
		float gen2_vignette_1 = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,dual_tube_offset_1)),3);
		float gen2_vignette_2 = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,dual_tube_offset_2)),3);
		vignette = 1.0 - ((1.0 - gen2_vignette_1) * (1.0 - gen2_vignette_2)); // apply vignette
	}
	else if (num_tubes > 3.99f && num_tubes < 4.01f)
	{
		float gen3_vignette_1 = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,quad_tube_offset_1)),3);
		float gen3_vignette_2 = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,quad_tube_offset_2)),3);
		float gen3_vignette_3 = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,quad_tube_offset_3)),3);
		float gen3_vignette_4 = pow(smoothstep(tube_radius,tube_radius-vignette_amount, distance(corrected_texturecoords,quad_tube_offset_4)),3);
		vignette = 1.0 - ((1.0 - gen3_vignette_1) * (1.0 - gen3_vignette_2) * (1.0 - gen3_vignette_3) * (1.0 - gen3_vignette_4)); // apply vignette
	}
	return vignette;
}			



///// blur functions
float IGN_calc(float2 uv)
{
    uv = frac(uv);
    float f = 0.06711056 * uv.x + 0.00583715 * uv.y;
    return 6.28*(frac(52.9829189 * frac(f)));
}

float2x2 rot(float a)
{
    float c;
    float s;
	sincos(a,s,c);
    return float2x2 (c, s, -s, c);
}


///////////////////////////////////////////////////////
// LUMA SHARPEN	(adapted from  Simple Luma Sharpen on ShaderToy, credit xwize)
///////////////////////////////////////////////////////

float3 YUVFromRGB( float3 color)
{
	return float3 (color.r * 0.299 + color.g * 0.587 + color.b * 0.114,
					color.r * -0.147 + color.g * -0.289 + color.b * 0.436,
					color.r * 0.615 + color.g * -0.515 + color.b * -0.100);
}

float3 RGBFromYUV ( float3 color)
{
	return float3 (color.r * 1.0 + color.b * 1.140,
					color.r * 1.0 + color.g * -0.395 + color.b * 0.581 ,
					color.r * 1.0 + color.g * 2.032);
}

float extractLuma(float3 c)
{
    return (c.r * 0.299 + c.g * 0.587 + c.b * 0.114);
}

float3 luma_sharpen(float3 image, float2 uv)
{
    float3 yuv = YUVFromRGB(image);
    
    float2 imgSize = screen_res.xy;
    
    float accumY = 0.0; 
    for(int i = -1; i <= 1; ++i) {
        for(int j = -1; j <= 1; ++j) {
            float2 offset = float2(i,j) / imgSize;
            
            float s = extractLuma(s_blur_2.SampleLevel(smp_rtlinear,uv + offset,0).rrr);
            float notCentre = min(float(i*i + j*j),1.0);
            accumY += s * (9.0 - notCentre*10.0);
        }
    }
    
    accumY /= 9.0;
    
    float gain = 0.9;
    accumY = (accumY + yuv.x)*gain;
    
		image = RGBFromYUV (float3(accumY,yuv.y,yuv.z)); // sharpened
		return image;   
}



///////////////////////////////////////////////////////
// DEPTH BLUR - LITERALLY BLURS THE DEPTH VALUE IN S_POSITION
///////////////////////////////////////////////////////
float blurred_depth (float2 tc)
{
    float Pi = 6.28318530718; // Pi*2
    
    float Directions = 12.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
    float Quality = 4.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
	float Size = 6;
	float2 Radius = Size/screen_res.xy;

	// how far away is the center of our COC
   	float center_depth = s_position.Load( int3( (tc)*screen_res.xy, 0 ), 0 ).z;
		if (center_depth == 0) { center_depth = 10000; }
	// set our average depth to be center depth
	float depth_average = 0;
	// where we store the depth_sample
	float depth_sample;
	// where we store the weighted value from sample
	float depth_tap;
	// where we store our weighted
	float weight = 0.0;
	// where we store the total weighted
	float total_weight = 0.0;
	
	// Blur calculations
	for(float i=1.0; i<=Quality; i++) // how far away are we
    {
		for( float d=0.0; d<Pi; d+=Pi/Directions) // where are we around the circle
		{
			// pull depth at our sample point
			depth_sample = s_position.Load( int3( ((tc+float2(cos(d),sin(d))*Radius*i) * screen_res.xy), 0 ), 0 ).z;
			
			// if we hit the sky, give it a depth of 10k
			if (depth_sample == 0) {depth_sample = 10000.0f; }
			
			// if the point we hit is closer than our (center? average), then that point should add blurring to our centerpoint
			if (depth_sample <= center_depth ) 
			{
				weight = Quality - pow((i - 1),0.5);
				depth_average += depth_sample * weight;
				total_weight += weight;
			}
			// but if the point we hit is farther than our (center? average?) then that point should not be adding blur to our centerpoint

        }
    }
	depth_average /= total_weight;
    return depth_average;
}


////////////////////////////////////////////////////////////////////
//////// GLITCH EFFECT /////////////////

float3 glitchEffect( float3 image, float2 tc, float power)
{
    //tc.y = 1.0 - tc.y;
	
	float time = fmod(timers.x, 32.0); // + modelmat[0].x + modelmat[0].z;

	float GLITCH = saturate(power);

	float rnd0 = rand(timers.x);
	float r0 = saturate((1.0-GLITCH)*0.7 + rnd0);
	float rnd1 = rand(floor(tc.x*10.0f * r0) / (10.0f * r0) * time);
	float r1 = 0.5 - (0.5 * GLITCH) + rnd1;
	r1 = 1.0 - max( 0.0, r1);
	float rnd2 = rand(floor(tc.y*40.0f * r1) / (40.0f * r1) * time);
	float r2 = saturate(rnd2);
	float rnd3 = rand(floor(tc.y*10.0f * r0) / (10.0f * r0) * time);
	float r3 = (1.0 - saturate(rnd3 + 0.8)) - 0.1f;

	float pxrnd = rand( dot(tc.x,tc.y) + time );

	float test_result;
	if (rnd0 > 0.5f)
	{ test_result = 1.0f; }
	else
	{test_result = -1.0f; }
	
	float ofs = 0.05 * r2 * GLITCH * test_result;
	ofs += 0.5 * pxrnd * ofs;

	tc.y += 0.1 * r3 * GLITCH;

    int NUM_SAMPLES = 10;
    float RCP_NUM_SAMPLES_F = 1.0 / (float)NUM_SAMPLES;
    
	float3 sum = 0.0f;
	float3 wsum = 0.0f;
	float t0 = 0.0f;
	
	for( int i=0; i<NUM_SAMPLES; ++i )
	{
		float t = (float)i * RCP_NUM_SAMPLES_F;
		tc.x = saturate(tc.x + ofs * t);
		float3 samplecol = s_blur_2.SampleLevel(smp_rtlinear, tc, 0).rgb;
//		float3 s = spectrum_offset( t );
//		samplecol *= s;
		sum += samplecol;
		wsum += 1.0f;
	}
return 	sum.rgb /= wsum;

}