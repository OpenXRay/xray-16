//---------------------------------------------------------------------------
#ifndef ParticleEffectDefH
#define ParticleEffectDefH

#include "Shader.h"

namespace PAPI
{
	struct Particle;
	struct ParticleEffect;
	struct PAHeader;
	struct ParticleAction;
    DEFINE_VECTOR(ParticleAction*,PAVec,PAVecIt);
}
struct EParticleAction;        

namespace PS
{
	class CParticleEffect;

	typedef BOOL ( * CollisionCallback)(CParticleEffect* E, PAPI::Particle& P, const Fvector& pt, const Fvector& norm); // TRUE-continue collision exec
	typedef void ( * DestroyCallback)	(CParticleEffect* E, PAPI::Particle& P);

	class PFunction;
	struct SFrame
	{
		Fvector2			m_fTexSize;
		Fvector2			reserved; 
		int     			m_iFrameDimX;
		int 				m_iFrameCount;
		float				m_fSpeed;

		void 				InitDefault()
		{
			m_fTexSize.set	(32.f/256.f,64.f/128.f);
			m_iFrameDimX 	= 8;
			m_iFrameCount 	= 16;
			m_fSpeed		= 24.f;
		}
		IC void       		CalculateTC(int frame, Fvector2& lt, Fvector2& rb)
		{
			lt.x       	 	= (frame%m_iFrameDimX)*m_fTexSize.x;
			lt.y        	= (frame/m_iFrameDimX)*m_fTexSize.y;
			rb.x        	= lt.x+m_fTexSize.x;
			rb.y        	= lt.y+m_fTexSize.y;
		}
	};

	class ECORE_API CPEDef
	{
	public:
		enum{
			dfSprite		= (1<<0),
//			dfObject		= (1<<1),

			dfFramed		= (1<<10),
			dfAnimated		= (1<<11),
			dfRandomFrame   = (1<<12),
			dfRandomPlayback= (1<<13),
            
			dfTimeLimit		= (1<<14),

            dfAlignToPath	= (1<<15),
            dfCollision		= (1<<16),
            dfCollisionDel	= (1<<17),
            dfVelocityScale	= (1<<18),
            dfCollisionDyn	= (1<<19),
			dfWorldAlign	= (1<<20),
            dfFaceAlign		= (1<<21),
            dfCulling		= (1<<22),
            dfCullCCW		= (1<<23),
		};
		shared_str		  	m_Name;
		Flags32				m_Flags;
	// texture
		shared_str		  	m_ShaderName;
		shared_str		  	m_TextureName;
		ref_shader			m_CachedShader;
		SFrame				m_Frame;
	// compiled actions
        CMemoryWriter		m_Actions;
	// def        
		float				m_fTimeLimit;			// time limit
		int					m_MaxParticles;			// max particle count
	    Fvector				m_VelocityScale;		// velocity scale
	    Fvector				m_APDefaultRotation;	// align to path
    // collision
	    float 				m_fCollideOneMinusFriction;
        float 				m_fCollideResilience;
        float 				m_fCollideSqrCutoff; 
	public:
		BOOL 				SaveActionList		(IWriter& F);
		BOOL 				LoadActionList		(IReader& F);
	// execute
		void				ExecuteAnimate		(PAPI::Particle *particles, u32 p_cnt, float dt);
        void				ExecuteCollision	(PAPI::Particle *particles, u32 p_cnt, float dt, CParticleEffect* owner, CollisionCallback cb);
	public:
                            CPEDef				();
                            ~CPEDef				();
        
		void				SetName				(LPCSTR name);
        IC LPCSTR			Name				()const{return *m_Name;}
        void				CreateShader		();
        void				DestroyShader		();

		void 				Save				(IWriter& F);
		BOOL 				Load				(IReader& F);

		void 				Save2				(CInifile& ini);
		BOOL 				Load2				(CInifile& ini);

#ifdef _EDITOR         
// change Copy&Equal if variables changed
	public:
	    DEFINE_VECTOR		(EParticleAction*,EPAVec,EPAVecIt);
		EPAVec 				m_EActionList;
	public:             
		void __stdcall  	FindActionByName	(LPCSTR new_name, bool& res);
		bool __stdcall  	NameOnAfterEdit					(PropValue* sender, shared_str& edit_val);
		bool __stdcall  	CollisionFrictionOnAfterEdit	(PropValue* sender, float& edit_val);
		void __stdcall  	CollisionFrictionOnBeforeEdit	(PropValue* sender, float& edit_val);
		void __stdcall  	CollisionFrictionOnDraw			(PropValue* sender, xr_string& draw_val);
		bool __stdcall  	CollisionCutoffOnAfterEdit		(PropValue* sender, float& edit_val);
		void __stdcall  	CollisionCutoffOnBeforeEdit		(PropValue* sender, float& edit_val);
		void __stdcall  	CollisionCutoffOnDraw			(PropValue* sender, xr_string& draw_val);
		void __stdcall  	OnActionEditClick	(ButtonValue* sender, bool& bDataModified, bool& bSafe);
	    void __stdcall  	OnFrameResize		(PropValue* sender);
	    void __stdcall  	OnShaderChange		(PropValue* sender);
	    void __stdcall  	OnFlagChange		(PropValue* sender);
		void __stdcall  	OnControlClick		(ButtonValue* sender, bool& bDataModified, bool& bSafe);
		void __stdcall  	OnActionsClick		(ButtonValue* sender, bool& bDataModified, bool& bSafe);
        bool __stdcall  	OnAfterActionNameEdit(PropValue* sender, shared_str& edit_val);
		void				FillProp		   	(LPCSTR pref, ::PropItemVec& items, ::ListItem* owner);
		void				Copy				(const CPEDef& src);
		BOOL				Equal				(const CPEDef* pe);
		void 				Render				(const Fmatrix& parent);
		static PFunction*	FindCommandPrototype(LPCSTR src, LPCSTR& dest);
		void __stdcall  	FillActionList		(ChooseItemVec& items, void* param);
        bool 				Validate 			(bool bMsg);
		void 				Compile				(EPAVec& v);
#endif
	};
};
#define PED_VERSION				0x0001
#define PED_CHUNK_VERSION		0x0001
#define PED_CHUNK_NAME			0x0002
#define PED_CHUNK_EFFECTDATA	0x0003
#define PED_CHUNK_ACTIONLIST   	0x0004
#define PED_CHUNK_FLAGS			0x0005
#define PED_CHUNK_FRAME			0x0006
#define PED_CHUNK_SPRITE	   	0x0007
#define PED_CHUNK_TIMELIMIT		0x0008
#define PED_CHUNK_TIMELIMIT2	0x0009
#define PED_CHUNK_SOURCETEXT_  	0x0020 // obsolete
#define PED_CHUNK_COLLISION	   	0x0021
#define PED_CHUNK_VEL_SCALE		0x0022
#define PED_CHUNK_EDATA			0x0024
#define PED_CHUNK_ALIGN_TO_PATH	0x0025
//---------------------------------------------------------------------------
#endif
