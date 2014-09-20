//---------------------------------------------------------------------------
#ifndef particle_managerH
#define particle_managerH
//---------------------------------------------------------------------------
#include "particle_actions.h"

namespace PAPI{
    class CParticleManager: public IParticleManager
    {
		// These are static because all threads access the same effects.
		// All accesses to these should be locked.
		DEFINE_VECTOR				(ParticleEffect*,ParticleEffectVec,ParticleEffectVecIt);
		DEFINE_VECTOR				(ParticleActions*,ParticleActionsVec,ParticleActionsVecIt);
		ParticleEffectVec			effect_vec;
		ParticleActionsVec			m_alist_vec;
    public:
		    						CParticleManager	();
        virtual						~CParticleManager	();
		// Return an index into the list of particle effects where
		ParticleEffect*				GetEffectPtr		(int effect_id);
		ParticleActions*			GetActionListPtr	(int alist_id);

		// create&destroy
		virtual int					CreateEffect		(u32 max_particles);
		virtual void				DestroyEffect		(int effect_id);
		virtual int					CreateActionList	();
		virtual void				DestroyActionList	(int alist_id);

        // control
        virtual void				PlayEffect			(int effect_id, int alist_id);
        virtual void				StopEffect			(int effect_id, int alist_id, BOOL deffered=TRUE);

        // update&render
        virtual void				Update				(int effect_id, int alist_id, float dt);
        virtual void				Render				(int effect_id);
        virtual void				Transform			(int alist_id, const Fmatrix& m, const Fvector& velocity);

        // effect
        virtual void				RemoveParticle		(int effect_id, u32 p_id);
        virtual void				SetMaxParticles		(int effect_id, u32 max_particles);
        virtual void				SetCallback			(int effect_id, OnBirthParticleCB b, OnDeadParticleCB d, void* owner, u32 param);
    	virtual void				GetParticles		(int effect_id, Particle*& particles, u32& cnt);
    	virtual u32					GetParticlesCount	(int effect_id);

        // action
        virtual ParticleAction*		CreateAction		(PActionEnum action_id);
        virtual u32					LoadActions			(int alist_id, IReader& R);
        virtual void				SaveActions			(int alist_id, IWriter& W);
    };
};
//---------------------------------------------------------------------------
#endif
