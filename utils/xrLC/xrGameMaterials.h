#ifndef _XR_GAME_MATERIALS_
#define _XR_GAME_MATERIALS_
#pragma once

enum	EGameMaterial
{
	gm_Wood,
	gm_Stone,
	gm_Metal,

	gm_Sand, 
};

class	Material
{
	float		transparency_for_sound;
	float		transparency_for_hit;
public:
	virtual		u32	getID			()					= 0;
	virtual		LPCSTR	getName			()					= 0;
	virtual		LPCSTR	getWallmark		(Material* other)	= 0;
	virtual		LPCSTR	getParticles	(Material* other)	= 0;
	virtual		LPCSTR	getSoundHIT		(Material* other)	= 0;
	virtual		LPCSTR	getSoundSTEP	(Material* other)	= 0;
	virtual		float	getFriction		(Material* other)	= 0;
};

#endif