#pragma once
struct SHit;

#include "../include/xrRender/animation_motion.h"
struct MotionID;

class  CInifile;
class  IKinematicsAnimated;
class  CEntityAlive;



template<typename T>
void	vec_clear ( T* &p)
{
	xr_delete( p );
}

class rnd_motion
{
private:
	xr_vector<MotionID>		motions;

public:
			rnd_motion	( );

rnd_motion*	setup		( IKinematicsAnimated* k, LPCSTR s );
MotionID	motion		( )	const;
};

class	type_motion
{

public:
	enum edirection
	{
		front = 0,
		back,
		left,
		right,
		not_definite
	};
static const u16		dirs_number		= 4;

public:	
	type_motion			( ){}
virtual				~type_motion		( );
		type_motion	*setup				( IKinematicsAnimated* k, CInifile const * ini, LPCSTR section, LPCSTR type );
		MotionID	motion				( edirection dr )	const;
virtual	bool		predicate			( CEntityAlive& ea, const SHit& H, MotionID &m, float &angle ) const=0;
private:
	void			set_motion			( IKinematicsAnimated* k, u16 motion_id, LPCSTR dir_anim );
	void			clear				( );
public:
static	edirection	dir					( CEntityAlive& ea, const SHit& H, float &angle );
private:
	xr_vector<rnd_motion*>		anims;
};

class death_anims
{

public:
					death_anims	( );
					~death_anims( );
	void			setup		( IKinematicsAnimated* k, LPCSTR section, CInifile const * ini );
	void			clear		( );
	MotionID		motion		( CEntityAlive& ea, const SHit& H, float& angle )	const;
	
private:
	static const u16		types_number	= 7;
	xr_vector<type_motion*>	anims;
	rnd_motion				rnd_anims;
};

#ifdef	DEBUG
 extern	BOOL death_anim_debug;
#endif