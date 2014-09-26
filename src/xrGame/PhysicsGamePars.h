#ifndef PHYSICS_GAME_PARS_H
#define PHYSICS_GAME_PARS_H
//extern float object_damage_factor;
extern float collide_volume_max;
extern float collide_volume_min;


struct EffectPars
{
	const static  float vel_cret_sound;
	const static float vel_cret_particles;
	const static float vel_cret_wallmark;
};

struct CharacterEffectPars
{
	const static  float vel_cret_sound;
	const static float vel_cret_particles;
	const static float vel_cret_wallmark;
};

void	LoadPhysicsGameParams	()	;
#endif