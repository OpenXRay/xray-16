//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "GameMtlLib.h"

CGameMtlLibrary GMLib;

void SGameMtl::Load(IReader& fs)
{
	R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_MAIN));
	ID						= fs.r_u32();
    fs.r_stringZ			(m_Name);

    if (fs.find_chunk(GAMEMTL_CHUNK_DESC)){
		fs.r_stringZ		(m_Desc);
    }
    
	R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_FLAGS));
    Flags.assign			(fs.r_u32());

	R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_PHYSICS));
    fPHFriction				= fs.r_float();
    fPHDamping				= fs.r_float();
    fPHSpring				= fs.r_float();
    fPHBounceStartVelocity 	= fs.r_float();
    fPHBouncing				= fs.r_float();

	R_ASSERT(fs.find_chunk(GAMEMTL_CHUNK_FACTORS));
    fShootFactor			= fs.r_float();
    fBounceDamageFactor		= fs.r_float();
    fVisTransparencyFactor	= fs.r_float();
    fSndOcclusionFactor		= fs.r_float();

	if(fs.find_chunk(GAMEMTL_CHUNK_FLOTATION))
	    fFlotationFactor	= fs.r_float();

    if(fs.find_chunk(GAMEMTL_CHUNK_INJURIOUS))
    	fInjuriousSpeed		= fs.r_float();
}

void CGameMtlLibrary::Load()
{
	string_path			name;
	if (!FS.exist(name,	_game_data_,GAMEMTL_FILENAME)){
    	Log				("! Can't find game material file: ",name);
    	return;
    }

    R_ASSERT			(material_pairs.empty());
    R_ASSERT			(materials.empty());
    		
	IReader*	F		= FS.r_open(name);
    IReader& fs			= *F;

    R_ASSERT(fs.find_chunk(GAMEMTLS_CHUNK_VERSION));
    u16 version			= fs.r_u16();
    if (GAMEMTL_CURRENT_VERSION!=version){
        Log				("CGameMtlLibrary: invalid version. Library can't load.");
		FS.r_close		(F);
    	return;
    }

    R_ASSERT(fs.find_chunk(GAMEMTLS_CHUNK_AUTOINC));
    material_index		= fs.r_u32();
    material_pair_index	= fs.r_u32();

    materials.clear		();
    material_pairs.clear();

    IReader* OBJ 		= fs.open_chunk(GAMEMTLS_CHUNK_MTLS);
    if (OBJ) {
        u32				count;
        for (IReader* O = OBJ->open_chunk_iterator(count); O; O = OBJ->open_chunk_iterator(count,O)) {
        	SGameMtl*	M = xr_new<SGameMtl> ();
	        M->Load		(*O);
        	materials.push_back(M);
        }
        OBJ->close		();
    }

    OBJ 				= fs.open_chunk(GAMEMTLS_CHUNK_MTLS_PAIR);
    if (OBJ){
        u32				count;
        for (IReader* O = OBJ->open_chunk_iterator(count); O; O = OBJ->open_chunk_iterator(count,O)) {
        	SGameMtlPair* M	= xr_new<SGameMtlPair> (this);
	        M->Load		(*O);
        	material_pairs.push_back(M);
        }
        OBJ->close		();
    }

#ifndef _EDITOR
	material_count		= (u32)materials.size();
    material_pairs_rt.resize(material_count*material_count,0);
    for (GameMtlPairIt p_it=material_pairs.begin(); material_pairs.end() != p_it; ++p_it){
		SGameMtlPair* S	= *p_it;
    	int idx0		= GetMaterialIdx(S->mtl0)*material_count+GetMaterialIdx(S->mtl1);
    	int idx1		= GetMaterialIdx(S->mtl1)*material_count+GetMaterialIdx(S->mtl0);
	    material_pairs_rt[idx0]=S;
	    material_pairs_rt[idx1]=S;
    }
#endif

/*
	for (GameMtlPairIt p_it=material_pairs.begin(); material_pairs.end() != p_it; ++p_it){
		SGameMtlPair* S	= *p_it;
		for (int k=0; k<S->StepSounds.size(); k++){
			Msg("%40s - 0x%x", S->StepSounds[k].handle->file_name(), S->StepSounds[k].g_type);
		}
	}
*/
	FS.r_close		(F);
}

#ifdef GM_NON_GAME
SGameMtlPair::~SGameMtlPair		()
{
}                
void SGameMtlPair::Load(IReader& fs)
{
	shared_str				buf;

	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_PAIR));
	mtl0				= fs.r_u32();
	mtl1				= fs.r_u32();
	ID					= fs.r_u32();
	ID_parent			= fs.r_u32();
    u32 own_mask		= fs.r_u32(); 
    if (GAMEMTL_NONE_ID==ID_parent) OwnProps.one	();
    else							OwnProps.assign	(own_mask);

	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_BREAKING));
	fs.r_stringZ		(buf); 	BreakingSounds	= buf.size()?*buf:"";

	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_STEP));
	fs.r_stringZ		(buf);	StepSounds		= buf.size()?*buf:"";

	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_COLLIDE));
	fs.r_stringZ		(buf);	CollideSounds	= buf.size()?*buf:"";
	fs.r_stringZ		(buf);	CollideParticles= buf.size()?*buf:"";
	fs.r_stringZ		(buf);	CollideMarks	= buf.size()?*buf:"";
}
#endif

#ifdef DEBUG
LPCSTR SGameMtlPair::dbg_Name()
{
	static string256 nm;
	SGameMtl* M0 = GMLib.GetMaterialByID(GetMtl0());
	SGameMtl* M1 = GMLib.GetMaterialByID(GetMtl1());
	sprintf_s(nm,"Pair: %s - %s",*M0->m_Name,*M1->m_Name);
	return nm;
}
#endif
