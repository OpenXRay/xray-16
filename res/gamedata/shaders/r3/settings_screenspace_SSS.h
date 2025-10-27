// [ SETTINGS ] [ SCREEN SPACE SHADOWS ]

#define G_SSS_STEPS						32		// More steps = better quality / poor performance. ( 24 = Low | 32 = Medium | 48 = High | 64 = Ultra )

#define G_SSS_INTENSITY					1.0f	// Shadow general intensity. [ 0.5f = 50% ~ 1.0f = 100% ]
#define G_SSS_DETAILS					0.02f	// Limit detail. Lower values will introduce more details to shadows but also incorrect results.
#define G_SSS_FORCE_FADE				0.5f	// Force shadow to start to fade at [ 0.5f = 50% ~ 1.0f = no fade ]

#define G_SSDO_SCENARY_HARDNESS			1.0f	// Scenary shadow hardness. 0.0f to disable scenary shadows.
#define G_SSDO_SCENARY_SHADOW_LENGTH	1.5f	// Scenary maximum shadow length.

#define G_SSDO_GRASS_HARDNESS			1.0f	// Grass shadow hardness.
#define G_SSDO_GRASS_SHADOW_LENGTH		0.5f	// Grass maximum shadow length.

#define G_SSDO_WEAPON_LENGTH			1.5f	// Maximum distance to apply weapon factors.
#define G_SSDO_WEAPON_HARDNESS			1.0f	// Weapon shadow hardness. 0.0f to disable weapon shadows.
#define G_SSDO_WEAPON_SHADOW_LENGTH		0.09f	// Weapon maximum shadow length.

