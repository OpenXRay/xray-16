//---------------------------------------------------------------------------
#ifndef ParticleGroupH
#define ParticleGroupH

#include "../xrRender/dxParticleCustom.h"

namespace PS
{
	class CParticleEffect;

	class ECORE_API CPGDef
	{
	public:
		shared_str			m_Name;
		Flags32				m_Flags;
		float				m_fTimeLimit;
		struct SEffect{
			enum{
				flDefferedStop		= (1<<0),
				flOnPlayChild		= (1<<1),
				flEnabled			= (1<<2),
                flOnPlayChildRewind	= (1<<4),
                flOnBirthChild		= (1<<5),
				flOnDeadChild		= (1<<6),
			};
			Flags32			m_Flags;
			shared_str		m_EffectName;  
			shared_str		m_OnPlayChildName;
			shared_str		m_OnBirthChildName;
			shared_str		m_OnDeadChildName;
			float			m_Time0;
			float			m_Time1;
							SEffect				(){m_Flags.zero();/*set(flEnabled)*/m_Time0=0;m_Time1=0;}
#ifdef _EDITOR
            BOOL			Equal				(const SEffect&);
#endif
		};
		DEFINE_VECTOR(SEffect*,EffectVec,EffectIt);
		EffectVec			m_Effects;
#ifdef _EDITOR
// change Equal if variables changed 
		void __stdcall  	OnEffectsEditClick	(ButtonValue* sender, bool& bDataModified, bool& bSafe);
		void __stdcall  	OnEffectTypeChange	(PropValue* sender);
		void __stdcall  	OnEffectEditClick	(ButtonValue* sender, bool& bDataModified, bool& bSafe);
		void __stdcall  	OnControlClick	(ButtonValue* sender, bool& bDataModified, bool& bSafe);
		void __stdcall  	OnParamsChange	(PropValue* sender);
		void				FillProp	   	(LPCSTR pref, ::PropItemVec& items, ::ListItem* owner);
		BOOL				Equal			(const CPGDef* pe);
		bool				Validate 			(bool bMsg);
#endif
	public:
							CPGDef		  	();
							~CPGDef		  	();
		void				SetName		  	(LPCSTR name);

		void 				Save		  	(IWriter& F);
		BOOL 				Load		 	(IReader& F);

		void 				Save2		  	(CInifile& ini);
		BOOL 				Load2		 	(CInifile& ini);

#ifdef _EDITOR
        void				Clone			(CPGDef* source);
#endif
	};

	class ECORE_API CParticleGroup: public dxParticleCustom
	{
		const CPGDef*		m_Def;
		float				m_CurrentTime;
		Fvector				m_InitialPosition;
	public:
    	DEFINE_VECTOR(dxRender_Visual*,VisualVec,VisualVecIt);
    	struct SItem		{
        	dxRender_Visual*	_effect;
            VisualVec		_children_related;
            VisualVec		_children_free;
        public:
        	void			Set				(dxRender_Visual* e);
            void			Clear			();

            IC u32			GetVisuals		(xr_vector<dxRender_Visual*>& visuals)
            {
            	visuals.reserve				(_children_related.size()+_children_free.size()+1);
                if (_effect)				visuals.push_back(_effect);
                visuals.insert				(visuals.end(),_children_related.begin(),_children_related.end());
                visuals.insert				(visuals.end(),_children_free.begin(),_children_free.end());
                return visuals.size();
            }
            
            void			OnDeviceCreate	();
            void			OnDeviceDestroy	();

            void			StartRelatedChild	(CParticleEffect* emitter, LPCSTR eff_name, PAPI::Particle& m);
            void			StopRelatedChild	(u32 idx);
            void			StartFreeChild		(CParticleEffect* emitter, LPCSTR eff_name, PAPI::Particle& m);

            void 			UpdateParent	(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM);
            void			OnFrame			(u32 u_dt, const CPGDef::SEffect& def, Fbox& box, bool& bPlaying);

            u32				ParticlesCount	();
            BOOL			IsPlaying		();
            void			Play			();
            void			Stop			(BOOL def_stop);
        };
        DEFINE_VECTOR(SItem,SItemVec,SItemVecIt)
		SItemVec			items;
	public:
		enum{
			flRT_Playing		= (1<<0),
			flRT_DefferedStop	= (1<<1),
		};
		Flags8				m_RT_Flags;
	public:
		CParticleGroup	();
		virtual				~CParticleGroup	();
		virtual void	 	OnFrame			(u32 dt);

		virtual void		Copy			(dxRender_Visual* pFrom) {FATAL("Can't duplicate particle system - NOT IMPLEMENTED");}

		virtual void 		OnDeviceCreate	();
		virtual void 		OnDeviceDestroy	();

		virtual void		UpdateParent	(const Fmatrix& m, const Fvector& velocity, BOOL bXFORM);

		BOOL				Compile			(CPGDef* def);

		const CPGDef*		GetDefinition	(){return m_Def;}

		virtual void		Play			();
		virtual void		Stop			(BOOL bDefferedStop=TRUE);
		virtual BOOL		IsPlaying		(){return m_RT_Flags.is(flRT_Playing);}

		virtual void		SetHudMode			(BOOL b);
		virtual BOOL		GetHudMode			();

		virtual float		GetTimeLimit	(){VERIFY(m_Def); return m_Def->m_fTimeLimit;}

		virtual const shared_str	Name		(){VERIFY(m_Def); return m_Def->m_Name;}

        virtual u32 		ParticlesCount	();
	};

}
//----------------------------------------------------
#define PGD_VERSION				0x0003
#define PGD_CHUNK_VERSION		0x0001
#define PGD_CHUNK_NAME			0x0002
#define PGD_CHUNK_FLAGS			0x0003
#define PGD_CHUNK_EFFECTS		0x0004 // obsolete
#define PGD_CHUNK_TIME_LIMIT	0x0005
#define PGD_CHUNK_EFFECTS2		0x0007

//---------------------------------------------------------------------------
#endif