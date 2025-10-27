// [ SETTINGS ] [ WATER ]

#define G_SSR_WATER_QUALITY				1		// Quality of the water. ( 0 = low | 1 = medium | 2 = high | 3 = Very High | 4 = Ultra )

#define G_SSR_WATER_WAVES				1.0f	// Water waves intensity
#define G_SSR_WATER_REFLECTION			0.5f	// Reflection intensity. ( 1.0f = 100% )
#define G_SSR_WATER_REFRACTION			0.05f	// Intensity of refraction distortion

#define G_SSR_WATER_SKY_REFLECTION		1.0f	// Sky reflection factor. ( 1.0f = 100% )
#define G_SSR_WATER_MAP_REFLECTION		1.0f	// Objects reflection factor. ( 1.0f = 100% )

#define G_SSR_WATER_TEX_DISTORTION		0.2f	// Water texture distortion.
#define G_SSR_WATER_TURBIDITY			3.0f	// Turbidity factor. ( 0.0f = Clear water )

#define G_SSR_WATER_FOG_MAXDEPTH		2.0f	// Maximum visibility underwater.

#define G_SSR_WATER_RAIN				0.4f	// Max intensity of rain drops

#define G_SSR_WATER_SPECULAR			6.0f	// Sun/Moon specular intensity
#define G_SSR_WATER_SPECULAR_NORMAL		0.2f	// Specular normal intensity. ( You may need to adjust this if you change the value of G_SSR_WATER_WAVES )

#define G_SSR_WATER_CAUSTICS			0.25f	// Caustics intensity
#define G_SSR_WATER_CAUSTICS_SCALE		1.0f	// Caustics Size

#define G_SSR_WATER_SOFTBORDER			0.1f	// Soft border factor. ( 0.0f = hard edge )

#define G_SSR_WATER_CHEAPFRESNEL				// Uncomment/comment this if you want to use a faster/accurate fresnel calc