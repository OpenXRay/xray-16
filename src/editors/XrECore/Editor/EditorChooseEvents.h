#ifndef EditorChooseEventsH
#define EditorChooseEventsH
#include "SoundManager.h"
#include "Library.h"
#include "GameMtlLib.h"
#include "LightAnimLibrary.h"
#include "SkeletonAnimated.h"
#include "ResourceManager.h"
#include "ParticleEffect.h"
#include "ParticleGroup.h"
#include "defines.h"
#include "EditObject.h"
ref_sound *choose_snd;

namespace ChoseEvents
{
    void FillEntity(ChooseItemVec &items, void *param)
    {
        //.    AppendItem	   					(RPOINT_CHOOSE_NAME);
        CInifile::Root const &data = pSettings->sections();
        for (CInifile::RootCIt it = data.begin(); it != data.end(); it++)
        {
            LPCSTR val;
            if ((*it)->line_exist("$spawn", &val))
                items.push_back(SChooseItem(*(*it)->Name, ""));
        }
    }
    //---------------------------------------------------------------------------
    void SelectSoundSource(SChooseItem *item, PropItemVec &info_items)
    {
        choose_snd->stop();
        choose_snd->create(item->name.c_str(), st_Effect, sg_Undefined);
        choose_snd->play(0, sm_2D);
        //    snd.pla
        /*
        //.
            ECustomThumbnail*& thm, ref_sound& snd,
            thm 		= xr_new<ESoundThumbnail>(item->name.c_str());
        */
    }
    void CloseSoundSource()
    {
        choose_snd->destroy();
    }
    void FillSoundSource(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (SndLib->GetGameSounds(lst))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }
    //---------------------------------------------------------------------------
    void FillSoundEnv(ChooseItemVec &items, void *param)
    {
        AStringVec lst;
        if (SndLib->GetSoundEnvs(lst))
        {
            AStringIt it = lst.begin();
            AStringIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->c_str(), ""));
        }
    }
    //---------------------------------------------------------------------------
    void FillObject(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (Lib.GetObjects(lst))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }
    void SelectObject(SChooseItem *item, PropItemVec &info_items)
    {
        EObjectThumbnail *thm = xr_new<EObjectThumbnail>(*item->name);
        if (thm->Valid())
            thm->FillInfo(info_items);
        xr_delete(thm);
    }
    void UpdateObjectTHM(LPCSTR name, ImTextureID &ID)
    {
        EObjectThumbnail *thm = xr_new<EObjectThumbnail>(name);
        if (thm->Valid())
        {
            thm->Update(ID);
        }
        else if (ID)
        {
            ID->Release();
            ID = 0;
        }
        xr_delete(thm);
    }
    //---------------------------------------------------------------------------
    void FillGroup(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (FS.file_list(lst, _groups_, FS_ListFiles | FS_ClampExt, "*.group"))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }
    void SelectGroup(SChooseItem *item, PropItemVec &info_items)
    {
        EGroupThumbnail *thm = xr_new<EGroupThumbnail>(*item->name);
        if (thm->Valid())
            thm->FillInfo(info_items);
        xr_delete(thm);
    }
    void UpdateGroupTHM(LPCSTR name, ImTextureID &ID)
    {
        EGroupThumbnail *thm = xr_new<EGroupThumbnail>(name);
        if (thm->Valid())
        {
            thm->Update(ID);
        }
        else if (ID)
        {
            ID->Release();
            ID = 0;
        }
        xr_delete(thm);
    }
    //---------------------------------------------------------------------------
    void FillVisual(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (FS.file_list(lst, _game_meshes_, FS_ListFiles | FS_ClampExt, "*.ogf"))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }
    void SelectVisual(SChooseItem *item, PropItemVec &info_items)
    {
        /*
        //.
            AnsiString fn					= ChangeFileExt(item->name.c_str(),".ogf");
            IRender_Visual* visual			= ::Render->model_Create(fn.c_str());
            if (visual){
                PHelper().CreateCaption	(info_items,	"Source",	*visual->desc.source_file?*visual->desc.source_file:"unknown");
                PHelper().CreateCaption	(info_items, 	"Creator N",*visual->desc.create_name?*visual->desc.create_name:"unknown");
                PHelper().CreateCaption	(info_items,	"Creator T",Trim(AnsiString(ctime(&visual->desc.create_time))).c_str());
                PHelper().CreateCaption	(info_items,	"Modif N",	*visual->desc.modif_name ?*visual->desc.modif_name :"unknown");
                PHelper().CreateCaption	(info_items,	"Modif T",	Trim(AnsiString(ctime(&visual->desc.modif_time))).c_str());
                PHelper().CreateCaption	(info_items,	"Build N",	*visual->desc.build_name ?*visual->desc.build_name :"unknown");
                PHelper().CreateCaption	(info_items,	"Build T",	Trim(AnsiString(ctime(&visual->desc.build_time))).c_str());
            }
            ::Render->model_Delete(visual);
        */
    }
    //---------------------------------------------------------------------------
    void FillGameObjectMots(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (FS.file_list(lst, _game_meshes_, FS_ListFiles | FS_ClampExt, "*.omf"))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }
    void SelectGameObjectMots(SChooseItem *item, PropItemVec &info_items)
    {
    }
    //---------------------------------------------------------------------------
    void FillGameAnim(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (FS.file_list(lst, "$game_anims$", FS_ListFiles, "*.anm,*.anms"))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }
    //---------------------------------------------------------------------------
    void FillLAnim(ChooseItemVec &items, void *param)
    {
        LAItemVec &lst = LALib.Objects();
        LAItemIt it = lst.begin();
        LAItemIt _E = lst.end();
        for (; it != _E; it++)
            items.push_back(SChooseItem(*(*it)->cName, ""));
    }
    void UpdateLAnim(LPCSTR name, ImTextureID &Texture)
    {
        int frame;
        CLAItem *item = LALib.FindItem(name);
        if (item)
        {

            ID3DTexture2D *pTexture = nullptr;
            if (Texture != nullptr)
            {
                R_CHK(Texture->QueryInterface(__uuidof(ID3DTexture2D), (void **)&pTexture));
            }
            else
            {
                R_CHK(HW.pDevice->CreateTexture(THUMB_WIDTH, THUMB_HEIGHT, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pTexture, 0));
                Texture = pTexture;
            }
            {
                D3DLOCKED_RECT rect;
                R_CHK(pTexture->LockRect(0, &rect, 0, 0));
                u32 *dest = nullptr;

                for (u32 y = 0; y < THUMB_HEIGHT; y++)
                {
                    dest = reinterpret_cast<u32 *>(reinterpret_cast<char *>(rect.pBits) + (rect.Pitch * y));
                    for (u32 i = 0; i < THUMB_WIDTH; i++)
                    {
                        dest[i] = item->CalculateBGR(EDevice.fTimeGlobal, frame);
                        dest[i] = subst_alpha(dest[i], 0xFF);
                    }
                }
                R_CHK(pTexture->UnlockRect(0));
            }
        }
    }
    //---------------------------------------------------------------------------
    void FillEShader(ChooseItemVec &items, void *param)
    {
        CResourceManager::map_Blender &blenders = EDevice.Resources->_GetBlenders();
        CResourceManager::map_BlenderIt _S = blenders.begin();
        CResourceManager::map_BlenderIt _E = blenders.end();
        for (; _S != _E; _S++)
            items.push_back(SChooseItem(_S->first, ""));
    }
    //---------------------------------------------------------------------------
    void FillCShader(ChooseItemVec &items, void *param)
    {
        Shader_xrLCVec &shaders = EDevice.ShaderXRLC.Library();
        Shader_xrLCIt _F = shaders.begin();
        Shader_xrLCIt _E = shaders.end();
        for (; _F != _E; _F++)
            items.push_back(SChooseItem(_F->Name, ""));
    }
    //---------------------------------------------------------------------------
    void FillPE(ChooseItemVec &items, void *param)
    {
        for (PS::PEDIt E = ::Render->PSLibrary.FirstPED(); E != ::Render->PSLibrary.LastPED(); E++)
            items.push_back(SChooseItem(*(*E)->m_Name, "EFFECT"));
    }
    //---------------------------------------------------------------------------
    void FillParticles(ChooseItemVec &items, void *param)
    {
        for (PS::PEDIt E = ::Render->PSLibrary.FirstPED(); E != ::Render->PSLibrary.LastPED(); E++)
            items.push_back(SChooseItem(*(*E)->m_Name, "EFFECT"));
        for (PS::PGDIt G = ::Render->PSLibrary.FirstPGD(); G != ::Render->PSLibrary.LastPGD(); G++)
            items.push_back(SChooseItem(*(*G)->m_Name, "GROUP"));
    }

    void SelectPE(SChooseItem *item, PropItemVec &info_items)
    {
        string64 str;
        u32 i = 0;
        PHelper().CreateCaption(info_items, "", "used in groups");
        for (PS::PGDIt G = ::Render->PSLibrary.FirstPGD(); G != ::Render->PSLibrary.LastPGD(); ++G)
        {
            PS::CPGDef *def = (*G);
            PS::CPGDef::EffectIt pe_it = def->m_Effects.begin();
            PS::CPGDef::EffectIt pe_it_e = def->m_Effects.end();
            for (; pe_it != pe_it_e; ++pe_it)
            {
                if ((*pe_it)->m_EffectName == item->name)
                {
                    xr_sprintf(str, sizeof(str), "%d", ++i);
                    PHelper().CreateCaption(info_items, str, def->m_Name);
                }
            }
        }
    }

    void SelectPG(SChooseItem *item, PropItemVec &info_items)
    {
        string64 str;
        u32 i = 0;
        PHelper().CreateCaption(info_items, "", "using effects");
        for (PS::PGDIt G = ::Render->PSLibrary.FirstPGD(); G != ::Render->PSLibrary.LastPGD(); G++)
        {
            PS::CPGDef *def = (*G);
            if (def->m_Name == item->name)
            {
                PS::CPGDef::EffectIt pe_it = def->m_Effects.begin();
                PS::CPGDef::EffectIt pe_it_e = def->m_Effects.end();
                for (; pe_it != pe_it_e; ++pe_it)
                {
                    xr_sprintf(str, sizeof(str), "%d", ++i);
                    PHelper().CreateCaption(info_items, str, (*pe_it)->m_EffectName);
                }
                break;
            }
        }
    }

    //---------------------------------------------------------------------------
    void FillTexture(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (ImageLib.GetTextures(lst))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }

    void UpdateTextureTHM(LPCSTR name, ImTextureID &Texture)
    {
        if (name && name[0])
        {
            ETextureThumbnail *thm = xr_new<ETextureThumbnail>(name);
            if (thm->Valid())
                thm->Update(Texture);
            xr_delete(thm);
        }
    }

    //---------------------------------------------------------------------------
    void FillTextureRaw(ChooseItemVec &items, void *param)
    {
        FS_FileSet lst;
        if (ImageLib.GetTexturesRaw(lst))
        {
            FS_FileSetIt it = lst.begin();
            FS_FileSetIt _E = lst.end();
            for (; it != _E; it++)
                items.push_back(SChooseItem(it->name.c_str(), ""));
        }
    }

    void UpdateTextureTHMRaw(LPCSTR name, ImTextureID &ID)
    {
        if (name && name[0])
        {
            ETextureThumbnail *thm = xr_new<ETextureThumbnail>(name);
            if (thm->Valid())
                thm->Update(ID);
            xr_delete(thm);
        }
    }

    void SelectTexture(SChooseItem *item, PropItemVec &info_items)
    {
        if (item->name.size())
        {
            ETextureThumbnail *thm = xr_new<ETextureThumbnail>(*item->name);
            if (thm->Valid())
                thm->FillInfo(info_items);
            xr_delete(thm);
        }
    }
    void SelectTextureRaw(SChooseItem *item, PropItemVec &info_items)
    {
        if (item->name.size())
        {
            ETextureThumbnail *thm = xr_new<ETextureThumbnail>(*item->name);
            if (thm->Valid())
                thm->FillInfo(info_items);
            xr_delete(thm);
        }
    }
    //---------------------------------------------------------------------------
    void FillGameMaterial(ChooseItemVec &items, void *param)
    {
        GameMtlIt _F = GMLib.FirstMaterial();
        GameMtlIt _E = GMLib.LastMaterial();
        for (; _F != _E; _F++)
            items.push_back(SChooseItem(*(*_F)->m_Name, ""));
    }
    //---------------------------------------------------------------------------

    void FillSkeletonAnims(ChooseItemVec &items, void *param)
    {
        IRenderVisual *V = ::Render->model_Create((LPCSTR)param);
        if (PKinematicsAnimated(V))
        {
            u32 cnt = PKinematicsAnimated(V)->LL_MotionsSlotCount();
            for (u32 k = 0; k < cnt; k++)
            {
                accel_map *ll_motions = PKinematicsAnimated(V)->LL_Motions(k);
                accel_map::iterator _I, _E;
                _I = ll_motions->begin();
                _E = ll_motions->end();
                for (; _I != _E; ++_I)
                {
                    bool bFound = false;
                    for (ChooseItemVecIt it = items.begin(); it != items.end(); it++)
                        if (it->name == _I->first)
                        {
                            bFound = true;
                            break;
                        }
                    if (!bFound)
                        items.push_back(SChooseItem(*_I->first, ""));
                }
            }
        }
        ::Render->model_Delete(V);
    }

    void FillSkeletonBones(ChooseItemVec &items, void *param)
    {
        IRenderVisual *V = ::Render->model_Create((LPCSTR)param);
        if (PKinematics(V))
        {
            CKinematicsAnimated::accel *ll_bones = PKinematics(V)->LL_Bones();
            CKinematicsAnimated::accel::iterator _I, _E;
            _I = ll_bones->begin();
            _E = ll_bones->end();
            for (; _I != _E; ++_I)
                items.push_back(SChooseItem(*_I->first, ""));
        }
        ::Render->model_Delete(V);
    }

    void FillSkeletonBonesObject(ChooseItemVec &items, void *param)
    {
        CEditableObject *eo = (CEditableObject *)param;

        BoneIt _I = eo->FirstBone();
        BoneIt _E = eo->LastBone();
        for (; _I != _E; ++_I)
        {
            items.push_back(SChooseItem((*_I)->Name().c_str(), ""));
        }
    }

} // namespace

void FillChooseEvents()
{
    {
        ref_texture texture_null;
        texture_null.create("ed\\ed_nodata");
        texture_null->Load();
        VERIFY(texture_null->surface_get());
        texture_null->surface_get()->AddRef();
        UIChooseForm::SetNullTexture(texture_null->surface_get());
    }
    UIChooseForm::AppendEvents(smSoundSource, "Select Sound Source", ChoseEvents::FillSoundSource, ChoseEvents::SelectSoundSource, 0, ChoseEvents::CloseSoundSource, 0);
    UIChooseForm::AppendEvents(smSoundEnv, "Select Sound Environment", ChoseEvents::FillSoundEnv, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smObject, "Select Library Object", ChoseEvents::FillObject, ChoseEvents::SelectObject, ChoseEvents::UpdateObjectTHM, 0, 0);
    UIChooseForm::AppendEvents(smGroup, "Select Group", ChoseEvents::FillGroup, ChoseEvents::SelectGroup, ChoseEvents::UpdateGroupTHM, 0, 0);
    UIChooseForm::AppendEvents(smEShader, "Select Engine Shader", ChoseEvents::FillEShader, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smCShader, "Select Compiler Shader", ChoseEvents::FillCShader, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smPE, "Select Particle Effect", ChoseEvents::FillPE, 0 /*ChoseEvents::SelectPE*/, 0, 0, 0);
    UIChooseForm::AppendEvents(smParticles, "Select Particle System", ChoseEvents::FillParticles, 0 /*ChoseEvents::SelectPG*/, 0, 0, 0);
    UIChooseForm::AppendEvents(smTextureRaw, "Select Source Texture", ChoseEvents::FillTextureRaw, ChoseEvents::SelectTextureRaw, ChoseEvents::UpdateTextureTHMRaw, 0, SChooseEvents::flClearTexture);
    UIChooseForm::AppendEvents(smTexture, "Select Texture", ChoseEvents::FillTexture, ChoseEvents::SelectTexture, ChoseEvents::UpdateTextureTHM, 0, SChooseEvents::flClearTexture);
    UIChooseForm::AppendEvents(smEntityType, "Select Entity", ChoseEvents::FillEntity, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smLAnim, "Select Light Animation", ChoseEvents::FillLAnim, 0, ChoseEvents::UpdateLAnim, 0, SChooseEvents::flAnimated);
    UIChooseForm::AppendEvents(smVisual, "Select Visual", ChoseEvents::FillVisual, ChoseEvents::SelectVisual, 0, 0, 0);
    UIChooseForm::AppendEvents(smSkeletonAnims, "Select Skeleton Animation", ChoseEvents::FillSkeletonAnims, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smSkeletonBones, "Select Skeleton Bones", ChoseEvents::FillSkeletonBones, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smSkeletonBonesInObject, "Select Skeleton Bones", ChoseEvents::FillSkeletonBonesObject, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smGameMaterial, "Select Game Material", ChoseEvents::FillGameMaterial, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smGameAnim, "Select Animation", ChoseEvents::FillGameAnim, 0, 0, 0, 0);
    UIChooseForm::AppendEvents(smGameSMotions, "Select Game Object Motions", ChoseEvents::FillGameObjectMots, ChoseEvents::SelectGameObjectMots, 0, 0, 0);
    choose_snd = xr_new<ref_sound>();
}
void ClearChooseEvents()
{
    UIChooseForm::ClearEvents();
    xr_delete(choose_snd);
}

//---------------------------------------------------------------------------
#endif
