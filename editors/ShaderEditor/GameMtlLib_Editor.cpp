//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "GameMtlLib.h"
#include "xr_trims.h"
#include "../../xrServerEntities/PropertiesListTypes.h"
#include "../../xrServerEntities/PropertiesListHelper.h"
#include "../xrEProps/FolderLib.h"
#include "LeftBar.h"
#include "../xrEProps/ChoseForm.h"
#include "../ECore/Editor/ui_main.h"

//------------------------------------------------------------------------------
// material routines
//------------------------------------------------------------------------------
void SGameMtl::FillProp		(PropItemVec& items, ListItem* owner)
{
	PropValue* V=0;
    PHelper().CreateName			(items,	"Name",						        &m_Name, owner);
    PHelper().CreateRText			(items,	"Desc",						        &m_Desc);
	// flags                                                      	
	V=PHelper().CreateFlag32		(items,	"Flags\\Dynamic",			        &Flags,	flDynamic);	V->Owner()->Enable(FALSE);
	PHelper().CreateFlag32			(items,	"Flags\\Passable",			        &Flags,	flPassable);
    if (Flags.is(flDynamic))
	    PHelper().CreateFlag32		(items,	"Flags\\Breakable",			        &Flags,	flBreakable);
    PHelper().CreateFlag32			(items,	"Flags\\Bounceable",		        &Flags,	flBounceable);
    PHelper().CreateFlag32			(items,	"Flags\\Skidmark",			        &Flags,	flSkidmark);
    PHelper().CreateFlag32			(items,	"Flags\\Bloodmark",			        &Flags,	flBloodmark);
    PHelper().CreateFlag32			(items,	"Flags\\Climable",			        &Flags,	flClimable);
    PHelper().CreateFlag32			(items,	"Flags\\Liquid",			        &Flags,	flLiquid);
    PHelper().CreateFlag32			(items,	"Flags\\Suppress Shadows",	        &Flags,	flSuppressShadows);
    PHelper().CreateFlag32			(items,	"Flags\\Suppress Wallmarks",        &Flags,	flSuppressWallmarks);
    PHelper().CreateFlag32			(items,	"Flags\\Actor Obstacle",        	&Flags,	flActorObstacle);
    PHelper().CreateFlag32			(items,	"Flags\\Bullet No Ricoshet",        &Flags,	flNoRicoshet);
    // physics part
    PHelper().CreateFloat			(items,	"Physics\\Friction",		        &fPHFriction,			0.f, 	100.f, 	0.001f, 3); 
    PHelper().CreateFloat			(items,	"Physics\\Damping",			        &fPHDamping,			0.001f,	100.f, 	0.001f, 3); 
    PHelper().CreateFloat			(items,	"Physics\\Spring",			        &fPHSpring,				0.001f,	100.f, 	0.001f, 3); 
    PHelper().CreateFloat			(items,	"Physics\\Bounce start vel",        &fPHBounceStartVelocity,0.f,	100.f, 	0.01f, 	2); 
    PHelper().CreateFloat			(items,	"Physics\\Bouncing",		        &fPHBouncing,			0.f,	1.f, 	0.001f, 3); 
    // factors
    PHelper().CreateFloat			(items,	"Factors\\Bounce Damage",			   		&fBounceDamageFactor,   0.f,100.f,0.1f,1);
    PHelper().CreateFloat			(items,	"Factors\\Injurious",		   				&fInjuriousSpeed,   	0.f,10000.f);
    PHelper().CreateFloat			(items,	"Factors\\Shooting (1-went through)",	    &fShootFactor);
    PHelper().CreateFloat			(items,	"Factors\\Shooting MP (1-went through)",	&fShootFactorMP);
    PHelper().CreateFloat			(items,	"Factors\\Transparency (1-full transp)",	&fVisTransparencyFactor);
    PHelper().CreateFloat			(items,	"Factors\\Sound occlusion (1-full hear)",	&fSndOcclusionFactor);
    PHelper().CreateFloat			(items,	"Factors\\Flotation (1-full passable)",		&fFlotationFactor);

    PHelper().CreateFloat			(items,	"Factors\\Density Factor",					&fDensityFactor, 0.0f, 1000.0f, 1.0f, 1);
}

void CGameMtlLibrary::CopyMtlPairs(SGameMtl* from, SGameMtl* to)
{
    for (GameMtlIt m1_it=materials.begin(); m1_it!=materials.end(); ++m1_it)
    {
        SGameMtl* M1 				= *m1_it;
        SGameMtlPair* p_from		= GetMaterialPair(from->GetID(), M1->GetID());
        SGameMtlPair* p_to			= GetMaterialPair(to->GetID(), M1->GetID());

        if(p_from && p_to)
        	p_to->CopyFrom(p_from);
    }

}

BOOL CGameMtlLibrary::UpdateMtlPairs(SGameMtl* src)
{
	BOOL bRes = FALSE;
    SGameMtl* M0 		= src;
    for (GameMtlIt m1_it=materials.begin(); m1_it!=materials.end(); ++m1_it)
    {
        SGameMtl* M1 	= *m1_it;
        GameMtlPairIt p_it = GetMaterialPairIt(M0->GetID(),M1->GetID());
        if ((!M0->Flags.is(SGameMtl::flDynamic))&&(!M1->Flags.is(SGameMtl::flDynamic)))
        {
        	R_ASSERT	(p_it==material_pairs.end());
            continue;
        }else{
	        if (p_it==material_pairs.end())
            {
                // create pair
                CreateMaterialPair(M0->GetID(),M1->GetID(),0);
                bRes = TRUE;
            }
        }
    }
    return bRes;
}

BOOL CGameMtlLibrary::UpdateMtlPairs()
{
	BOOL bRes = FALSE;
    for (GameMtlIt m0_it=materials.begin(); m0_it!=materials.end(); m0_it++)
    	if (UpdateMtlPairs(*m0_it)) bRes = TRUE;
    return bRes;
}


SGameMtl* CGameMtlLibrary::AppendMaterial(SGameMtl* parent)
{
    SGameMtl* M	= xr_new<SGameMtl>();
    if (parent)	
    *M		=*parent;//base params
    
    M->ID		= material_index++;
    
    materials.push_back		(M);
    UpdateMtlPairs			(M);
    CopyMtlPairs			(parent, M);
    return 					M;
}
void CGameMtlLibrary::RemoveMaterial(LPCSTR name)
{
    // find material
    GameMtlIt 	rem_it=GetMaterialIt(name);
    R_ASSERT	(rem_it!=materials.end());
    // remove dependent pairs
    RemoveMaterialPair((*rem_it)->GetID());
    // destroy material
    xr_delete		(*rem_it);
    materials.erase	(rem_it);
}

//------------------------------------------------------------------------------
// material pair routines
//------------------------------------------------------------------------------
void __fastcall SGameMtlPair::OnFlagChange(PropValue* sender)
{
	bool bChecked = sender->Owner()->m_Flags.is(PropItem::flCBChecked);
    u32 mask=0;
    if (sender==propBreakingSounds)			mask = flBreakingSounds;
    else if (sender==propStepSounds)		mask = flStepSounds;
    else if (sender==propCollideSounds)		mask = flCollideSounds;
    else if (sender==propCollideParticles)	mask = flCollideParticles;
    else if (sender==propCollideMarks)		mask = flCollideMarks;
    else THROW;

    OwnProps.set				(mask,bChecked);
    sender->Owner()->m_Flags.set(PropItem::flDisabled,!bChecked);

    ExecCommand					(COMMAND_UPDATE_PROPERTIES);
}

IC u32 SetMask(u32 mask, Flags32 flags, u32 flag )
{
    return mask?(mask|(flags.is(flag)?PropItem::flCBChecked:PropItem::flDisabled)):0;
}

IC SGameMtlPair* GetLastParentValue(SGameMtlPair* who, u32 flag)
{
	if (!who)					return 0;
	if ((GAMEMTL_NONE_ID==who->GetParent())||(who->OwnProps.is(flag))) return who;
    else						return GetLastParentValue(who->m_Owner->GetMaterialPair(who->GetParent()),flag);
}

IC BOOL ValidateParent(SGameMtlPair* who, SGameMtlPair* parent)
{
	if (!parent)				return TRUE;
	if (who==parent)			return FALSE;
    else						return ValidateParent(who,parent->m_Owner->GetMaterialPair(parent->GetParent()));
}

BOOL SGameMtlPair::SetParent(int parent)
{
	int ID_parent_save 			= ID_parent;
	ID_parent					= parent;

    for (GameMtlPairIt it=m_Owner->FirstMaterialPair(); it!=m_Owner->LastMaterialPair(); it++){
    	if (!ValidateParent(*it,m_Owner->GetMaterialPair((*it)->GetParent()))){
			ID_parent			= ID_parent_save;
        	return FALSE;
        }
    }
    // all right
    if (GAMEMTL_NONE_ID==ID_parent){
        OwnProps.one	();
    }else{
        OwnProps.zero	();
        OwnProps.set	(flBreakingSounds,	BreakingSounds.size());
        OwnProps.set	(flStepSounds,		StepSounds.size());
        OwnProps.set	(flCollideSounds,	CollideSounds.size());
        OwnProps.set	(flCollideParticles,CollideParticles.size());
        OwnProps.set	(flCollideMarks,	CollideMarks.size());
    }
    return TRUE;
}

void __fastcall SGameMtlPair::FillChooseMtl(ChooseItemVec& items, void* param)
{
    for (GameMtlIt m0_it=m_Owner->FirstMaterial(); m0_it!=m_Owner->LastMaterial(); m0_it++){
        SGameMtl* M0 		= *m0_it;
        for (GameMtlIt m1_it=m_Owner->FirstMaterial(); m1_it!=m_Owner->LastMaterial(); m1_it++){
            SGameMtl* M1 	= *m1_it;
            GameMtlPairIt p_it = GMLib.GetMaterialPairIt(M0->GetID(),M1->GetID());
            if (p_it!=GMLib.LastMaterialPair())
                items.push_back	(SChooseItem(GMLib.MtlPairToName(M0->GetID(),M1->GetID()),""));
        }
    }
}

void __fastcall SGameMtlPair::OnParentClick(ButtonValue* V, bool& bModif, bool& bSafe)
{
    bModif = false;
    switch (V->btn_num){
    case 0:{
        LPCSTR MP=0;
	    SGameMtlPair* P	= m_Owner->GetMaterialPair(ID_parent);
        AnsiString nm	= P?m_Owner->MtlPairToName(P->GetMtl0(),P->GetMtl1()):NONE_CAPTION;

        if (TfrmChoseItem::SelectItem(smCustom,MP,1,(nm==NONE_CAPTION)?0:nm.c_str(),fastdelegate::bind<TOnChooseFillItems>(this,&SGameMtlPair::FillChooseMtl))){
        	if (MP){
                int m0, m1;
                m_Owner->NameToMtlPair	(MP,m0,m1);
                SGameMtlPair* p	= m_Owner->GetMaterialPair(m0,m1); VERIFY(p);
                if (!SetParent	(p->GetID())){
                	ELog.DlgMsg(mtError,"Pair can't inherit from self.");
                }else{
			    	bModif 		= true;
	                ExecCommand	(COMMAND_UPDATE_PROPERTIES);
                }
            }else{
            	SetParent		(GAMEMTL_NONE_ID);
			    bModif 			= true;
                ExecCommand		(COMMAND_UPDATE_PROPERTIES);
            }
        }
    }break;
	}
}

void __fastcall SGameMtlPair::OnCommandClick(ButtonValue* V, bool& bModif, bool& bSafe)
{
    bModif = false;
    switch (V->btn_num){
    case 0:{
        LPCSTR MP=0;
	    SGameMtlPair* P	= m_Owner->GetMaterialPair(ID_parent);
        AnsiString nm	= P?m_Owner->MtlPairToName(P->GetMtl0(),P->GetMtl1()):NONE_CAPTION;
        if (TfrmChoseItem::SelectItem(smCustom,MP,128,0,fastdelegate::bind<TOnChooseFillItems>(this,&SGameMtlPair::FillChooseMtl))){
        	if (MP){
                AStringVec lst;
                _SequenceToList(lst,MP);
                for (AStringIt it=lst.begin(); it!=lst.end(); it++){
                    int m0, m1;
                    m_Owner->NameToMtlPair	(it->c_str(),m0,m1);
                    SGameMtlPair* p	= m_Owner->GetMaterialPair(m0,m1); VERIFY(p);
                    if (!p->SetParent(GetID())){
                        ELog.DlgMsg(mtError,"Pair can't inherit from self.");
                    }else{
                        bModif 		= true;
                    }
                }
                if (bModif)		ExecCommand(COMMAND_UPDATE_PROPERTIES);
            }
        }
    }break;
	}
}

void SGameMtlPair::FillProp(PropItemVec& items)
{
	PropValue::TOnChange OnChange	= 0;
    u32 show_CB			= 0;
    SGameMtlPair* P		= 0;
    if (ID_parent!=GAMEMTL_NONE_ID){ 
       	OnChange.bind	(this,&SGameMtlPair::OnFlagChange);
        show_CB		    = PropItem::flShowCB;
	    P				= m_Owner->GetMaterialPair(ID_parent);
    }
    ButtonValue* B;
    B					= PHelper().CreateButton(items,		"Command",			"Set As Parent To...",0);
    B->OnBtnClickEvent.bind(this,&SGameMtlPair::OnCommandClick);
    B					= PHelper().CreateButton(items,		"Parent", 			P?m_Owner->MtlPairToName(P->GetMtl0(),P->GetMtl1()):NONE_CAPTION,0);
    B->OnBtnClickEvent.bind(this,&SGameMtlPair::OnParentClick);
    
    propBreakingSounds	= PHelper().CreateChoose	(items,	"Breaking Sounds",	&BreakingSounds, 	smSoundSource, 0,0, GAMEMTL_SUBITEM_COUNT);
    propStepSounds		= PHelper().CreateChoose	(items,	"Step Sounds",		&StepSounds, 		smSoundSource, 0,0, GAMEMTL_SUBITEM_COUNT+2);
    propCollideSounds	= PHelper().CreateChoose	(items,	"Collide Sounds",	&CollideSounds, 	smSoundSource, 0,0, GAMEMTL_SUBITEM_COUNT);
    propCollideParticles= PHelper().CreateChoose	(items,	"Collide Particles",&CollideParticles, 	smParticles, 0,0, GAMEMTL_SUBITEM_COUNT);
    propCollideMarks	= PHelper().CreateChoose	(items,	"Collide Marks",	&CollideMarks,		smTexture, 0,0, GAMEMTL_SUBITEM_COUNT);

    propBreakingSounds->Owner()->m_Flags.assign	(SetMask(show_CB,OwnProps,flBreakingSounds));
    propStepSounds->Owner()->m_Flags.assign		(SetMask(show_CB,OwnProps,flStepSounds));
    propCollideSounds->Owner()->m_Flags.assign	(SetMask(show_CB,OwnProps,flCollideSounds));
    propCollideParticles->Owner()->m_Flags.assign(SetMask(show_CB,OwnProps,flCollideParticles));
    propCollideMarks->Owner()->m_Flags.assign	(SetMask(show_CB,OwnProps,flCollideMarks));

    propBreakingSounds->OnChangeEvent			= OnChange;
    propStepSounds->OnChangeEvent				= OnChange;
    propCollideSounds->OnChangeEvent			= OnChange;
    propCollideParticles->OnChangeEvent			= OnChange;
    propCollideMarks->OnChangeEvent				= OnChange;

    if (show_CB)
    {
		SGameMtlPair* O; 
    	if (0!=(O=GetLastParentValue(this,flBreakingSounds)))	BreakingSounds	= O->BreakingSounds;
    	if (0!=(O=GetLastParentValue(this,flStepSounds))) 		StepSounds		= O->StepSounds;
    	if (0!=(O=GetLastParentValue(this,flCollideSounds))) 	CollideSounds	= O->CollideSounds;
    	if (0!=(O=GetLastParentValue(this,flCollideParticles))) CollideParticles= O->CollideParticles;
    	if (0!=(O=GetLastParentValue(this,flCollideMarks))) 	CollideMarks	= O->CollideMarks;
    }
}

void SGameMtlPair::TransferFromParent(SGameMtlPair* parent)
{
    R_ASSERT(parent);
    if (!OwnProps.is(flBreakingSounds))		BreakingSounds  = parent->BreakingSounds;
    if (!OwnProps.is(flStepSounds))			StepSounds		= parent->StepSounds;
    if (!OwnProps.is(flCollideSounds))		CollideSounds	= parent->CollideSounds;
    if (!OwnProps.is(flCollideParticles))	CollideParticles= parent->CollideParticles;
    if (!OwnProps.is(flCollideMarks))		CollideMarks	= parent->CollideMarks;
}

void SGameMtlPair::CopyFrom(SGameMtlPair* parent)
{
   R_ASSERT			(parent);
   OwnProps 		= parent->OwnProps;
   ID_parent 		= parent->ID_parent;
    
   BreakingSounds  	= parent->BreakingSounds;
  	

   StepSounds		= parent->StepSounds;
            
   CollideSounds	= parent->CollideSounds;
            
   CollideParticles	= parent->CollideParticles;
            
   CollideMarks		= parent->CollideMarks;
}

//------------------------------------------------------------------------------
// material library routines
//------------------------------------------------------------------------------
LPCSTR CGameMtlLibrary::MtlPairToName		(int mtl0, int mtl1)
{
    static string512 buf;
    SGameMtl* M0	= GetMaterialByID(mtl0);	R_ASSERT(M0);
    SGameMtl* M1	= GetMaterialByID(mtl1);	R_ASSERT(M1);
    string256 buf0, buf1;
    strcpy			(buf0,*M0->m_Name);	_ChangeSymbol	(buf0,'\\','/');
    strcpy			(buf1,*M1->m_Name);	_ChangeSymbol	(buf1,'\\','/');
    sprintf			(buf,"%s \\ %s",buf0,buf1);
    return buf;
}
void CGameMtlLibrary::NameToMtlPair			(LPCSTR name, int& mtl0, int& mtl1)
{
    string256 		buf0, buf1;
    if (_GetItemCount(name,'\\')<2){
        mtl0		= GAMEMTL_NONE_ID;
        mtl1		= GAMEMTL_NONE_ID;
    	return;
    }
    _GetItem		(name,0,buf0,'\\');
    _GetItem		(name,1,buf1,'\\');
    _ChangeSymbol	(buf0,'/','\\');
    _ChangeSymbol	(buf1,'/','\\');
    SGameMtl* M0	= GetMaterial(buf0);	mtl0=M0?M0->GetID():GAMEMTL_NONE_ID;
    SGameMtl* M1	= GetMaterial(buf1);	mtl1=M1?M1->GetID():GAMEMTL_NONE_ID;
}
void CGameMtlLibrary::MtlNameToMtlPair		(LPCSTR name, int& mtl0, int& mtl1)
{
    string256 buf;
    SGameMtl* M0 	= GetMaterial(_GetItem(name,0,buf,','));	R_ASSERT(M0); 	mtl0=M0->GetID();
    SGameMtl* M1 	= GetMaterial(_GetItem(name,1,buf,','));	R_ASSERT(M1);	mtl1=M1->GetID();
}

SGameMtlPair* CGameMtlLibrary::CreateMaterialPair(int m0, int m1, SGameMtlPair* parent)
{
    SGameMtlPair* M	= xr_new<SGameMtlPair>(this);
    if (parent){
        M->ID_parent = parent->ID;
        M->OwnProps.zero();
    }
    M->ID 		= material_pair_index++;
    M->SetPair	(m0,m1);
    material_pairs.push_back			(M);
    return 		M;
}
SGameMtlPair* CGameMtlLibrary::AppendMaterialPair(int m0, int m1, SGameMtlPair* parent)
{
    SGameMtlPair*	S = GetMaterialPair(m0,m1);
    if (!S){
    	return CreateMaterialPair(m0,m1,parent);
    }else{
        return 		S;
     }
}
void CGameMtlLibrary::RemoveMaterialPair(LPCSTR name)
{
    int mtl0,mtl1;
    NameToMtlPair	(name,mtl0,mtl1);
    RemoveMaterialPair(mtl0, mtl1);
}
void CGameMtlLibrary::RemoveMaterialPair(GameMtlPairIt rem_it)
{
	if (rem_it==material_pairs.end()) return;
    // delete parent dependent
    for (GameMtlPairIt it=material_pairs.begin(); it!=material_pairs.end(); it++)
        if ((*it)->ID_parent==(*rem_it)->ID){ 
            // transfer parented props to child
            (*it)->TransferFromParent(*rem_it);
            // reset parenting
            (*it)->ID_parent=-1;
        }
    // erase from list and remove physically
    xr_delete			(*rem_it);
    material_pairs.erase	(rem_it);
}
void CGameMtlLibrary::RemoveMaterialPair(int mtl)
{
    for (int i=0; i<(int)material_pairs.size(); i++){
        GameMtlPairIt it = material_pairs.begin()+i;
        if (((*it)->mtl0==mtl)||((*it)->mtl1==mtl)){
            RemoveMaterialPair(it);
            i--;
        }
    }
}
void CGameMtlLibrary::RemoveMaterialPair(int mtl0, int mtl1)
{
    GameMtlPairIt 	rem_it=GetMaterialPairIt(mtl0,mtl1);
    if (rem_it==material_pairs.end()) return;
    RemoveMaterialPair	(rem_it);
}
GameMtlPairIt CGameMtlLibrary::GetMaterialPairIt(int id)
{
    for (GameMtlPairIt it=material_pairs.begin(); it!=material_pairs.end(); it++)
        if ((*it)->ID==id) return it;
    return material_pairs.end();
}
SGameMtlPair* CGameMtlLibrary::GetMaterialPair(int id)
{
    GameMtlPairIt it=GetMaterialPairIt(id);
    return it!=material_pairs.end()?*it:0;
}
GameMtlPairIt CGameMtlLibrary::GetMaterialPairIt	(int mtl0, int mtl1)
{
    for (GameMtlPairIt it=material_pairs.begin(); it!=material_pairs.end(); it++)
        if ((*it)->IsPair(mtl0,mtl1)) return it;
    return material_pairs.end();
}
SGameMtlPair* CGameMtlLibrary::GetMaterialPair(int mtl0, int mtl1)
{
    GameMtlPairIt it=GetMaterialPairIt(mtl0, mtl1);
    return it!=material_pairs.end()?*it:0;
}
SGameMtlPair* CGameMtlLibrary::GetMaterialPair(LPCSTR name)
{
    if (name&&name[0]){
        int mtl0, mtl1;
        NameToMtlPair	(name,mtl0,mtl1);
        GameMtlPairIt it=GetMaterialPairIt(mtl0, mtl1);
        return it!=material_pairs.end()?*it:0;
    }
    return 0;
}

//------------------------------------------------------------------------------
// IO - routines
//------------------------------------------------------------------------------
void SGameMtl::Save(IWriter& fs)
{
	Flags.set				(flSlowDown,	!fis_zero(1.f-fFlotationFactor,EPS_L));
	Flags.set				(flShootable,	fis_zero(fShootFactor,EPS_L));
	Flags.set				(flTransparent,	fis_zero(fVisTransparencyFactor,EPS_L));
    Flags.set 				(flInjurious,	!fis_zero(fInjuriousSpeed,EPS_L));

	fs.open_chunk			(GAMEMTL_CHUNK_MAIN);
	fs.w_u32				(ID);
	fs.w_stringZ			(m_Name);
    fs.close_chunk			();

	fs.open_chunk			(GAMEMTL_CHUNK_DESC);
    fs.w_stringZ			(m_Desc);
    fs.close_chunk			();
    
	fs.open_chunk			(GAMEMTL_CHUNK_FLAGS);
    fs.w_u32				(Flags.get());
    fs.close_chunk			();

	fs.open_chunk			(GAMEMTL_CHUNK_PHYSICS);
    fs.w_float				(fPHFriction);
    fs.w_float				(fPHDamping);
    fs.w_float				(fPHSpring);
    fs.w_float				(fPHBounceStartVelocity);
    fs.w_float				(fPHBouncing);
    fs.close_chunk			();

	fs.open_chunk			(GAMEMTL_CHUNK_FACTORS);
    fs.w_float				(fShootFactor);
    fs.w_float				(fBounceDamageFactor);
    fs.w_float				(fVisTransparencyFactor);
    fs.w_float				(fSndOcclusionFactor);
    fs.close_chunk			();

	fs.open_chunk			(GAMEMTL_CHUNK_FACTORS_MP);
    fs.w_float				(fShootFactorMP);
    fs.close_chunk			();

	fs.open_chunk			(GAMEMTL_CHUNK_FLOTATION);
    fs.w_float				(fFlotationFactor);
    fs.close_chunk			();

	fs.open_chunk			(GAMEMTL_CHUNK_INJURIOUS);
    fs.w_float				(fInjuriousSpeed);
    fs.close_chunk			();

	fs.open_chunk			(GAMEMTL_CHUNK_DENSITY);
    fs.w_float				(fDensityFactor);
    fs.close_chunk			();

 }

void SGameMtlPair::Load(IReader& fs)
{
	shared_str				buf;

	R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_PAIR));
    mtl0				= fs.r_u32();
    mtl1				= fs.r_u32();
    ID					= fs.r_u32();
    ID_parent			= fs.r_u32();
    OwnProps.assign		(fs.r_u32());

    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_BREAKING));
    fs.r_stringZ			(buf); 	BreakingSounds	= *buf;
    
    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_STEP));
    fs.r_stringZ			(buf);	StepSounds		= *buf;
    
    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_COLLIDE));
    fs.r_stringZ			(buf);	CollideSounds	= *buf;
    fs.r_stringZ			(buf);	CollideParticles= *buf;
    fs.r_stringZ			(buf);	CollideMarks	= *buf;
}

void SGameMtlPair::Save(IWriter& fs)
{
    fs.open_chunk		(GAMEMTLPAIR_CHUNK_PAIR);
    fs.w_u32			(mtl0);
    fs.w_u32			(mtl1);
    fs.w_u32			(ID);
    fs.w_u32			(ID_parent);
    fs.w_u32			(OwnProps.get());
	fs.close_chunk		();

// copy from parent
	if (ID_parent!=GAMEMTL_NONE_ID){
        SGameMtlPair* P; 
        if ((0!=(P=GetLastParentValue(this,flBreakingSounds)))&&(P!=this))	
            BreakingSounds	= P->BreakingSounds;
        if ((0!=(P=GetLastParentValue(this,flStepSounds)))&&(P!=this)) 		
            StepSounds		= P->StepSounds;
        if ((0!=(P=GetLastParentValue(this,flCollideSounds)))&&(P!=this)) 	
            CollideSounds	= P->CollideSounds;
        if ((0!=(P=GetLastParentValue(this,flCollideParticles)))&&(P!=this)) 
            CollideParticles= P->CollideParticles;
        if ((0!=(P=GetLastParentValue(this,flCollideMarks)))&&(P!=this)) 	
            CollideMarks	= P->CollideMarks;
    }
/*
    else{
    	OwnProps.zero();
        if (!BreakingSounds.IsEmpty())	OwnProps.set(flBreakingSounds,TRUE);
        if (!StepSounds.IsEmpty())		OwnProps.set(flStepSounds,TRUE);
        if (!CollideSounds.IsEmpty())	OwnProps.set(flCollideSounds,TRUE);
        if (!CollideParticles.IsEmpty())OwnProps.set(flCollideParticles,TRUE);
        if (!CollideMarks.IsEmpty())	OwnProps.set(flCollideMarks,TRUE);
    }
*/    
// save    
    fs.open_chunk		(GAMEMTLPAIR_CHUNK_BREAKING);
    fs.w_stringZ		(BreakingSounds);
	fs.close_chunk		();

    fs.open_chunk		(GAMEMTLPAIR_CHUNK_STEP);
    fs.w_stringZ		(StepSounds);
	fs.close_chunk		();

    fs.open_chunk		(GAMEMTLPAIR_CHUNK_COLLIDE);
    fs.w_stringZ		(CollideSounds);
    fs.w_stringZ		(CollideParticles);
    fs.w_stringZ		(CollideMarks);
	fs.close_chunk		();
}

bool CGameMtlLibrary::Save()
{
	R_ASSERT			(FALSE==UpdateMtlPairs());
	// save
	CMemoryWriter fs;
    fs.open_chunk		(GAMEMTLS_CHUNK_VERSION);
    fs.w_u16			(GAMEMTL_CURRENT_VERSION);
	fs.close_chunk		();

    fs.open_chunk		(GAMEMTLS_CHUNK_AUTOINC);
    fs.w_u32			(material_index);
    fs.w_u32			(material_pair_index);
	fs.close_chunk		();
    
    fs.open_chunk		(GAMEMTLS_CHUNK_MTLS);
    int count = 0;
    for(GameMtlIt m_it=materials.begin(); m_it!=materials.end(); m_it++){
        fs.open_chunk	(count++);
        (*m_it)->Save	(fs);
        fs.close_chunk	();
    }
	fs.close_chunk		();
                                                    
    fs.open_chunk		(GAMEMTLS_CHUNK_MTLS_PAIR);
    count = 0;
    for(GameMtlPairIt p_it=material_pairs.begin(); p_it!=material_pairs.end(); p_it++){
        fs.open_chunk	(count++);
        (*p_it)->Save	(fs);
        fs.close_chunk	();
    }
	fs.close_chunk		();

	string_path fn;
    FS.update_path		(fn,_game_data_,GAMEMTL_FILENAME);
    return fs.save_to	(fn);
}

