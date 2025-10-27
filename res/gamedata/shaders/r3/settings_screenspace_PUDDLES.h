// [ SETTINGS ] [ PUDDLES ]

// NOTE: Reflection quality is defined by G_SSR_QUALITY ( settings_screenspace_SSR.h )

#define G_PUDDLES_GLOBAL_SIZE				1.0f	// Puddles global size. ( This affect distance and puddles size )
#define G_PUDDLES_SIZE						0.8f	// Puddles individual size. ( This only affect puddles size )
#define G_PUDDLES_BORDER_HARDNESS			0.7f	// Border hardness. ( 1.0f = Extremely defined border )
#define G_PUDDLES_TERRAIN_EXTRA_WETNESS		0.15f	// Terrain extra wetness when raining. ( Over time like puddles )
#define G_PUDDLES_REFLECTIVITY				0.4f	// Reflectivity. ( 1.0f = Mirror like )
#define G_PUDDLES_TINT						float3(0.66f, 0.63f, 0.6f) // Puddles tint RGB.

#define G_PUDDLES_RIPPLES							// Comment to disable ripples
#define G_PUDDLES_RIPPLES_SCALE				1.0f	// Ripples scale
#define G_PUDDLES_RIPPLES_INTENSITY			1.0f	// Puddles ripples intensity
#define G_PUDDLES_RIPPLES_RAINING_INT		0.1f	// Extra ripple intensity when raining ( Affected by rain intensity )
#define G_PUDDLES_RIPPLES_SPEED				1.0f	// Puddles ripples movement speed

#define G_PUDDLES_RAIN_RIPPLES_INTENSITY	1.0f	// Rain ripples intensity
#define G_PUDDLES_RAIN_RIPPLES_SCALE		1.0f	// Rain ripples scale

#define G_PUDDLES_REFRACTION_INTENSITY		1.0f	// Refraction intensity

//#define G_PUDDLES_ALLWAYS					// Uncomment to allways render puddles ( Raining or not )