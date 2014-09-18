//---------------------------------------------------------------------------
#ifndef GameMtlLibH
#define GameMtlLibH
//---------------------------------------------------------------------------
#pragma once

#define GAMEMTL_CURRENT_VERSION			0x0001
//----------------------------------------------------

#define GAMEMTLS_CHUNK_VERSION        	0x1000
#define GAMEMTLS_CHUNK_AUTOINC	        0x1001
#define GAMEMTLS_CHUNK_MTLS	        	0x1002
#define GAMEMTLS_CHUNK_MTLS_PAIR       	0x1003
//----------------------------------------------------
#define GAMEMTL_CHUNK_MAIN	        	0x1000
#define GAMEMTL_CHUNK_FLAGS	        	0x1001
#define GAMEMTL_CHUNK_PHYSICS        	0x1002
#define GAMEMTL_CHUNK_FACTORS        	0x1003
#define GAMEMTL_CHUNK_FLOTATION   		0x1004
#define GAMEMTL_CHUNK_DESC				0x1005
#define GAMEMTL_CHUNK_INJURIOUS			0x1006
//----------------------------------------------------
#define GAMEMTLPAIR_CHUNK_PAIR   		0x1000
//#define GAMEMTLPAIR_CHUNK_FLOTATION   0x1001 - obsolete
#define GAMEMTLPAIR_CHUNK_BREAKING   	0x1002
#define GAMEMTLPAIR_CHUNK_STEP   		0x1003
//#define GAMEMTLPAIR_CHUNK_COLLIDE   	0x1004 - obsolete / rename HIT
#define GAMEMTLPAIR_CHUNK_COLLIDE	  	0x1005
//----------------------------------------------------

#define GAMEMTL_SUBITEM_COUNT			4

#define GAMEMTL_NONE_ID					u32(-1)
#define GAMEMTL_NONE_IDX				u16(-1)
#define GAMEMTL_FILENAME				"gamemtl.xr"

#ifdef _EDITOR
#define GM_NON_GAME
#endif
#ifdef _MAX_PLUGIN
#define GM_NON_GAME
#endif
#ifdef _LW_SHADER
#define GM_NON_GAME
#endif
#ifdef _MAYA_PLUGIN
#define GM_NON_GAME
#endif

#ifdef _EDITOR
#include "ElTree.hpp"
#endif

#ifdef GM_NON_GAME
	#define SoundVec	shared_str
	#define PSVec 		shared_str
	#define ShaderVec 	shared_str
#else
	DEFINE_VECTOR(ref_sound,SoundVec,SoundIt);
	DEFINE_VECTOR(shared_str,PSVec,PSIt);
	DEFINE_VECTOR(ref_shader,ShaderVec,ShaderIt);
#endif

struct ECORE_API SGameMtl
{
	friend class CGameMtlLibrary;
protected:
	int 				ID; 	// auto number
public:
	enum{
    	flBreakable		= (1ul<<0ul),
//		flShootable 	= (1ul<<1ul),
        flBounceable	= (1ul<<2ul),
        flSkidmark		= (1ul<<3ul),
        flBloodmark		= (1ul<<4ul),
        flClimable		= (1ul<<5ul),
//		flWalkOn		= (1ul<<6ul), // obsolette
        flPassable		= (1ul<<7ul),
        flDynamic		= (1ul<<8ul),
        flLiquid 		= (1ul<<9ul),
		flSuppressShadows	= (1ul<<10ul),
		flSuppressWallmarks	= (1ul<<11ul),
		flActorObstacle	= (1ul<<12ul),

        flInjurious		= (1ul<<28ul),// flInjurious = fInjuriousSpeed > 0.f
		flShootable 	= (1ul<<29ul),
        flTransparent	= (1ul<<30ul),
        flSlowDown		= (1ul<<31ul) // flSlowDown = (fFlotationFactor<1.f)
    };
public:
	shared_str			m_Name;
    shared_str			m_Desc;

    Flags32				Flags;
    // physics part
    float				fPHFriction;            // ?
    float				fPHDamping;             // ?
    float				fPHSpring;              // ?
    float				fPHBounceStartVelocity;	// ?
	float				fPHBouncing;            // ?
	// shoot&bounce&visibility&flotation
    float		    	fFlotationFactor;		// 0.f - 1.f   	(1.f-полностью проходимый)
    float				fShootFactor;			// 0.f - 1.f	(1.f-полностью простреливаемый)
    float				fBounceDamageFactor;	// 0.f - 100.f
    float				fInjuriousSpeed; 		// 0.f - ...	(0.f-не отбирает здоровье (скорость уменьшения здоровья))
    float				fVisTransparencyFactor;	// 0.f - 1.f	(1.f-полностью прозрачный)
    float				fSndOcclusionFactor;	// 0.f - 1.f    (1.f-полностью слышен)
public:
	SGameMtl			()
    {
        ID						= -1;
		m_Name					= "unknown";
        Flags.zero				();
        // factors
        fFlotationFactor		= 1.f;
        fShootFactor			= 0.f;
        fBounceDamageFactor		= 1.f;
        fInjuriousSpeed 		= 0.f;
        fVisTransparencyFactor	= 0.f;
        fSndOcclusionFactor		= 0.f;
        // physics
        fPHFriction				= 1.f;
        fPHDamping				= 1.f;
        fPHSpring				= 1.f;
        fPHBounceStartVelocity  = 0.f;
        fPHBouncing				= 0.1f;
	}
    void 				Load			(IReader& fs);
    void 				Save			(IWriter& fs);
    IC int				GetID			(){return ID;}
#ifdef _EDITOR
    void 				FillProp		(PropItemVec& values, ListItem* owner);
#endif
};
DEFINE_VECTOR(SGameMtl*,GameMtlVec,GameMtlIt);

struct ECORE_API SGameMtlPair{
	friend class CGameMtlLibrary;
    CGameMtlLibrary*	m_Owner;
private:
	int					mtl0;
	int					mtl1;
protected:
	int 				ID; 	// auto number
    int					ID_parent;
public:
	enum{
//		flFlotation		= (1<<0),
        flBreakingSounds= (1<<1),
        flStepSounds	= (1<<2),
//		flCollideSounds	= (1<<3),
        flCollideSounds		= (1<<4),
        flCollideParticles	= (1<<5),
        flCollideMarks		= (1<<6)
    };
    Flags32				OwnProps;
//	properties
    SoundVec			BreakingSounds;
    SoundVec			StepSounds;
    SoundVec			CollideSounds;
    PSVec				CollideParticles;
    ShaderVec			CollideMarks;
#ifdef _EDITOR
    PropValue*			propBreakingSounds;
    PropValue*			propStepSounds;
    PropValue*			propCollideSounds;
    PropValue*			propCollideParticles;
    PropValue*			propCollideMarks;
    void __stdcall	 	OnFlagChange	(PropValue* sender);
	void __stdcall	 	OnParentClick	(ButtonValue* sender, bool& bModif, bool& bSafe);
	void __stdcall	 	OnCommandClick	(ButtonValue* sender, bool& bModif, bool& bSafe);
    void __stdcall	 	FillChooseMtl	(ChooseItemVec& items, void* param);
#endif
public:
	SGameMtlPair		(CGameMtlLibrary* owner)
    {
    	mtl0			= -1;
    	mtl1			= -1;
        ID				= -1;
        ID_parent		= -1;
        m_Owner			= owner;
        OwnProps.one	();
	}
	~SGameMtlPair		();
    IC int				GetMtl0			(){return mtl0;}
    IC int				GetMtl1			(){return mtl1;}
    IC int				GetID			(){return ID;}
    IC void				SetPair			(int m0, int m1){mtl0=m0; mtl1=m1;}
	IC bool				IsPair			(int m0, int m1){return !!(((mtl0==m0)&&(mtl1==m1))||((mtl0==m1)&&(mtl1==m0)));}
    void				Save			(IWriter& fs);
    void				Load			(IReader& fs);
    IC int 				GetParent		(){return ID_parent;}
    BOOL				SetParent		(int parent);
#ifdef _EDITOR
	void 				FillProp		(PropItemVec& values);
    void				TransferFromParent(SGameMtlPair* parent);
#endif
#ifdef DEBUG
	LPCSTR				dbg_Name		();
#endif
};

DEFINE_VECTOR(SGameMtlPair*,GameMtlPairVec,GameMtlPairIt);

class ECORE_API CGameMtlLibrary{
	int					material_index;
	int					material_pair_index;

	GameMtlVec			materials;
    GameMtlPairVec		material_pairs;

#ifndef _EDITOR
    // game part
    u32					material_count;
    GameMtlPairVec		material_pairs_rt;
#endif
public:
	CGameMtlLibrary		()
	{
	    material_index 		= 0;
	    material_pair_index = 0;
#ifndef _EDITOR
        material_count	    = 0;
#endif
    }
	~CGameMtlLibrary	()
	{
		/*
    	R_ASSERT		(0==material_pairs_rt.size());
    	R_ASSERT		(0==material_pairs.size());
    	R_ASSERT		(0==materials.size());
		*/
    }
	IC void				Unload			()
	{
#ifndef _EDITOR
		material_count	= 0;
		material_pairs_rt.clear();
#endif
		for (GameMtlIt m_it=materials.begin(); materials.end() != m_it; ++m_it)
			xr_delete	(*m_it);
		materials.clear();
		for (GameMtlPairIt p_it=material_pairs.begin(); material_pairs.end() != p_it; ++p_it)
			xr_delete	(*p_it);
		material_pairs.clear();
	}
    // material routine
    IC GameMtlIt 		GetMaterialIt	(LPCSTR name)
    {
        for (GameMtlIt it=materials.begin(); materials.end() != it; ++it)
            if (0==strcmpi(*(*it)->m_Name,name)) return it;
        return materials.end();
    }
    IC GameMtlIt 		GetMaterialIt	(shared_str& name)
    {
        for (GameMtlIt it=materials.begin(); materials.end() != it; ++it)
            if (name.equal((*it)->m_Name)) return it;
        return materials.end();
    }
    IC GameMtlIt 		GetMaterialItByID(int id)
    {
        for (GameMtlIt it=materials.begin(); materials.end() != it; ++it)
            if ((*it)->ID==id) return it;
        return materials.end();
    }
	IC u32				GetMaterialID	(LPCSTR name)
    {
    	GameMtlIt it	= GetMaterialIt	(name);
        return (it==materials.end())?GAMEMTL_NONE_ID:(*it)->ID;
    }
#ifdef _EDITOR
	// editor
	SGameMtl*			AppendMaterial	(SGameMtl* parent);
	void				RemoveMaterial	(LPCSTR name);
	IC SGameMtl*		GetMaterialByID	(int ID)
    {
	    GameMtlIt it=GetMaterialItByID(ID);
    	return materials.end() != it?*it:0;
    }
	IC SGameMtl*		GetMaterial		(LPCSTR name)
    {
	    GameMtlIt it=GetMaterialIt(name);
    	return materials.end() != it?*it:0;
	}
#else
	// game
	IC u16				GetMaterialIdx	(int ID)		{GameMtlIt it=GetMaterialItByID(ID);VERIFY(materials.end() != it); return (u16)(it-materials.begin());}
	IC u16				GetMaterialIdx	(LPCSTR name)	{GameMtlIt it=GetMaterialIt(name);VERIFY(materials.end() != it); return (u16)(it-materials.begin());}
	IC SGameMtl*		GetMaterialByIdx(u16 idx)		{VERIFY(idx<materials.size()); return materials[idx];}
	IC SGameMtl*		GetMaterialByID (s32 id)		{return GetMaterialByIdx(GetMaterialIdx(id));}
#endif

	IC GameMtlIt		FirstMaterial	(){return materials.begin();}
	IC GameMtlIt		LastMaterial	(){return materials.end();}
	IC u32				CountMaterial	(){return materials.size();}

// material pair routine
#ifdef _EDITOR
	BOOL				UpdateMtlPairs		(SGameMtl* src);
	BOOL				UpdateMtlPairs		();
	LPCSTR				MtlPairToName		(int mtl0, int mtl1);
	void				NameToMtlPair		(LPCSTR name, int& mtl0, int& mtl1);
	void				MtlNameToMtlPair	(LPCSTR name, int& mtl0, int& mtl1);
	SGameMtlPair*		CreateMaterialPair	(int m0, int m1, SGameMtlPair* parent=0);
	SGameMtlPair*		AppendMaterialPair	(int m0, int m1, SGameMtlPair* parent=0);
	void				RemoveMaterialPair	(LPCSTR name);
	void				RemoveMaterialPair	(GameMtlPairIt rem_it);
	void				RemoveMaterialPair	(int mtl);
	void				RemoveMaterialPair	(int mtl0, int mtl1);
	GameMtlPairIt		GetMaterialPairIt	(int id);
	SGameMtlPair*		GetMaterialPair		(int id);
	GameMtlPairIt		GetMaterialPairIt	(int mtl0, int mtl1);
	SGameMtlPair*		GetMaterialPair		(int mtl0, int mtl1);
	SGameMtlPair*		GetMaterialPair		(LPCSTR name);
#else
	// game
	IC SGameMtlPair*	GetMaterialPair		(u16 idx0, u16 idx1){R_ASSERT((idx0<material_count)&&(idx1<material_count)); return material_pairs_rt[idx1*material_count+idx0];}
#endif
	IC GameMtlPairIt	FirstMaterialPair	(){return material_pairs.begin();}
	IC GameMtlPairIt	LastMaterialPair	(){return material_pairs.end();}

	// IO routines
	void				Load				();
	bool				Save				();
};

#define GET_RANDOM(a_vector)			(a_vector[Random.randI(a_vector.size())])

#define CLONE_MTL_SOUND(_res_, _mtl_pair_, _a_vector_)\
	{ VERIFY2(!_mtl_pair_##->_a_vector_.empty(),_mtl_pair_->dbg_Name());\
	_res_.clone(GET_RANDOM(_mtl_pair_##->_a_vector_));\
	}

extern ECORE_API CGameMtlLibrary GMLib;
#endif

