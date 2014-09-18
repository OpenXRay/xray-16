//---------------------------------------------------------------------------
#ifndef particle_actions_collectionH
#define particle_actions_collectionH

#include "particle_actions.h"
#include "particle_core.h"
namespace PAPI{
#define _METHODS	virtual void 	Load		(IReader& F);\
                    virtual void 	Save		(IWriter& F);\
                    virtual void 	Execute		(ParticleEffect *pe, const float dt, float& m_max);\
                    virtual void 	Transform	(const Fmatrix& m);

	struct PARTICLES_API PAAvoid : public ParticleAction
	{
		pDomain positionL;	// Avoid region (in local space)
		pDomain position;	// Avoid region
		float look_ahead;	// how many time units ahead to look
		float magnitude;	// what percent of the way to go each time
		float epsilon;		// add to r^2 for softening

        _METHODS;
	};

	struct PARTICLES_API PABounce : public ParticleAction
	{
		pDomain positionL;	// Bounce region (in local space)
		pDomain position;	// Bounce region
		float oneMinusFriction;	// Friction tangent to surface
		float resilience;	// Resilence perpendicular to surface
		float cutoffSqr;	// cutoff velocity; friction applies iff v > cutoff

        _METHODS;
	};

	struct PARTICLES_API PACopyVertexB : public ParticleAction
	{
		BOOL copy_pos;		// True to copy pos to posB.

        _METHODS;
	};

	struct PARTICLES_API PADamping : public ParticleAction
	{
		pVector damping;	// Damping constant applied to velocity
		float vlowSqr;		// Low and high cutoff velocities
		float vhighSqr;

        _METHODS;
	};

	struct PARTICLES_API PAExplosion : public ParticleAction
	{
		pVector centerL;	// The center of the explosion (in local space)
		pVector center;		// The center of the explosion
		float velocity;		// Of shock wave
		float magnitude;	// At unit radius
		float stdev;		// Sharpness or width of shock wave
		float age;			// How long it's been going on
		float epsilon;		// Softening parameter

        _METHODS;
	};

	struct PARTICLES_API PAFollow : public ParticleAction
	{
		float magnitude;	// The grav of each particle
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

	struct PARTICLES_API PAGravitate : public ParticleAction
	{
		float magnitude;	// The grav of each particle
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

	struct PARTICLES_API PAGravity : public ParticleAction
	{
		pVector directionL;	// Amount to increment velocity (in local space)
		pVector direction;	// Amount to increment velocity

        _METHODS;
	};

	struct PARTICLES_API PAJet : public ParticleAction
	{
		pVector	centerL;	// Center of the fan (in local space)
		pDomain accL;		// Acceleration vector domain  (in local space)
		pVector	center;		// Center of the fan
		pDomain acc;		// Acceleration vector domain
		float magnitude;	// Scales acceleration
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

	struct PARTICLES_API PAKillOld : public ParticleAction
	{
    	float age_limit;		// Exact age at which to kill particles.
		BOOL kill_less_than;	// True to kill particles less than limit.

        _METHODS;
	};

	struct PARTICLES_API PAMatchVelocity : public ParticleAction
	{
		float magnitude;	// The grav of each particle
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

	struct PARTICLES_API PAMove : public ParticleAction
	{
        _METHODS;
	};

	struct PARTICLES_API PAOrbitLine : public ParticleAction
	{
		pVector pL, axisL;	// Endpoints of line to which particles are attracted (in local space)
		pVector p, axis;	// Endpoints of line to which particles are attracted
		float magnitude;	// Scales acceleration
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

	struct PARTICLES_API PAOrbitPoint : public ParticleAction
	{
		pVector centerL;	// Point to which particles are attracted (in local space)
		pVector center;		// Point to which particles are attracted
		float magnitude;	// Scales acceleration
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

	struct PARTICLES_API PARandomAccel : public ParticleAction
	{
		pDomain gen_accL;	// The domain of random accelerations.(in local space)
		pDomain gen_acc;	// The domain of random accelerations.

        _METHODS;
	};

	struct PARTICLES_API PARandomDisplace : public ParticleAction
	{
		pDomain gen_dispL;	// The domain of random displacements.(in local space)
		pDomain gen_disp;	// The domain of random displacements.

        _METHODS;
	};

	struct PARTICLES_API PARandomVelocity : public ParticleAction
	{
		pDomain gen_velL;	// The domain of random velocities.(in local space)
		pDomain gen_vel;	// The domain of random velocities.

        _METHODS;
	};

	struct PARTICLES_API PARestore : public ParticleAction
	{
		float time_left;	// Time remaining until they should be in position.

        _METHODS;
	};

	struct PARTICLES_API PAScatter : public ParticleAction
	{
		pVector	centerL;	// Center of the fan (in local space)
		pVector	center;		// Center of the fan
		float magnitude;	// Scales acceleration
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

	struct PARTICLES_API PASink : public ParticleAction
	{
		BOOL kill_inside;	// True to dispose of particles *inside* domain
		pDomain positionL;	// Disposal region (in local space)
		pDomain position;	// Disposal region

        _METHODS;
	};

	struct PARTICLES_API PASinkVelocity : public ParticleAction
	{
		BOOL kill_inside;	// True to dispose of particles with vel *inside* domain
		pDomain velocityL;	// Disposal region (in local space)
		pDomain velocity;	// Disposal region

        _METHODS;
	};

	struct PARTICLES_API PASpeedLimit : public ParticleAction
	{
		float min_speed;		// Clamp speed to this minimum.
		float max_speed;		// Clamp speed to this maximum.

        _METHODS;
	};

	struct PARTICLES_API PASource : public ParticleAction
	{
		enum{
			flSingleSize		= (1ul<<29ul),// True to get positionB from position.
			flSilent			= (1ul<<30ul),
			flVertexB_tracks	= (1ul<<31ul),// True to get positionB from position.
			fl_FORCEDWORD		= u32(-1)
		};
		pDomain positionL;	// Choose a position in this domain. (local_space)
		pDomain velocityL;	// Choose a velocity in this domain. (local_space)
		pDomain position;	// Choose a position in this domain.
		pDomain velocity;	// Choose a velocity in this domain.
		pDomain rot;		// Choose a rotation in this domain.
		pDomain size;		// Choose a size in this domain.
		pDomain color;		// Choose a color in this domain.
		float alpha;		// Alpha of all generated particles
		float particle_rate;// Particles to generate per unit time
		float age;			// Initial age of the particles
		float age_sigma;	// St. dev. of initial age of the particles
		pVector parent_vel;	
		float parent_motion;

        _METHODS;
	};

	struct PARTICLES_API PATargetColor : public ParticleAction
	{
		PATargetColor():timeFrom(0.0f),timeTo(1.0f){}
		pVector color;		// Color to shift towards
		float alpha;		// Alpha value to shift towards
		float scale;		// Amount to shift by (1 == all the way)
		float timeFrom;
		float timeTo;

        _METHODS;
	};

	struct PARTICLES_API PATargetSize : public ParticleAction
	{
		pVector size;		// Size to shift towards
		pVector scale;		// Amount to shift by per frame (1 == all the way)

        _METHODS;
	};

	struct PARTICLES_API PATargetRotate : public ParticleAction
	{
		pVector rot;		// Rotation to shift towards
		float scale;		// Amount to shift by per frame (1 == all the way)

        _METHODS;
	};

	struct PARTICLES_API PATargetVelocity : public ParticleAction
	{
		pVector velocityL;	// Velocity to shift towards (in local space)
		pVector velocity;	// Velocity to shift towards
		float scale;		// Amount to shift by (1 == all the way)

        _METHODS;
	};

	struct PARTICLES_API PAVortex : public ParticleAction
	{
		pVector centerL;	// Center of vortex (in local space)
		pVector axisL;		// Axis around which vortex is applied (in local space)
		pVector center;		// Center of vortex
		pVector axis;		// Axis around which vortex is applied
		float magnitude;	// Scale for rotation around axis
		float epsilon;		// Softening parameter
		float max_radius;	// Only influence particles within max_radius

        _METHODS;
	};

    struct PARTICLES_API PATurbulence : public ParticleAction
    {
		float frequency;	// Frequency
		int	octaves;		// Octaves
		float magnitude;	// Scale for rotation around axis
		float epsilon;		// Softening parameter
        pVector offset;		// Offset
        float age;

        _METHODS;
    };
};

//---------------------------------------------------------------------------
#endif
