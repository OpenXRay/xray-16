//---------------------------------------------------------------------------
#ifndef particle_effectH
#define particle_effectH

namespace PAPI{
	// A effect of particles - Info and an array of Particles
	struct ParticleEffect
	{
		u32			p_count;				// Number of particles currently existing.
		u32			max_particles;			// Max particles allowed in effect.
		u32			particles_allocated;	// Actual allocated size.
		Particle*	particles;				// Actually, num_particles in size
        OnBirthParticleCB 	b_cb;
        OnDeadParticleCB	d_cb;
        void*				owner;
        u32					param;
        
        public:
					ParticleEffect	(int mp)
		{
        	owner					= 0;
            param 					= 0;
        	b_cb					= 0;
        	d_cb					= 0;
   			p_count					= 0;
			max_particles			= mp;
			particles_allocated		= max_particles;
			particles				= xr_alloc<Particle>(max_particles);
		}
					~ParticleEffect	()
		{
			xr_free					(particles);
		}
		IC int		Resize			(u32 max_count)
		{
			// Reducing max.
			if(particles_allocated >= max_count)
			{
				max_particles		= max_count;

				// May have to kill particles.
				if(p_count > max_particles)
					p_count			= max_particles;

				return max_count;
			}

			// Allocate particles.
			Particle* new_particles	= xr_alloc<Particle>(max_count);
			if(new_particles==NULL){
				// ERROR - Not enough memory. Just give all we've got.
				max_particles		= particles_allocated;
				return max_particles;
			}

			CopyMemory			(new_particles, particles, p_count * sizeof(Particle));
			xr_free					(particles);
			particles				= new_particles;

			max_particles			= max_count;
			particles_allocated		= max_count;
			return max_count;
		}
		IC void		Remove			(int i)
		{
        	if (0==p_count)			return;
			Particle& m				= particles[i];
            if (d_cb)				d_cb(owner,param,m,i);
            m 						= particles[--p_count]; // не менять правило удаления !!! (dependence ParticleGroup)
		}

		IC BOOL		Add				(const pVector &pos, const pVector &posB,
									const pVector &size, const pVector &rot, const pVector &vel, u32 color,
									const float age = 0.0f, u16 frame=0, u16 flags=0)
		{
			if(p_count >= max_particles)	return FALSE;
			else{
				Particle& P = particles[p_count];
				P.pos 		= pos;
				P.posB 		= posB;
				P.size 		= size;
				P.rot 		= rot;
				P.vel 		= vel;
				P.color 	= color;
				P.age 		= age;
				P.frame 	= frame;
				P.flags.assign(flags); 
	            if (b_cb)	b_cb(owner,param,P,p_count);
				p_count++;
				return TRUE;
			}
		}
	};
};
//---------------------------------------------------------------------------
#endif
