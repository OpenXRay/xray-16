#ifndef EditorChooseEventsH
#define EditorChooseEventsH

#include "xrCore/ChooseTypes.h"
#include "Layers/xrRender/SkeletonAnimated.h"
#include "Layers/xrRender/ResourceManager.h"

#include "Layers/xrRender/ParticleEffect.h"
#include "Layers/xrRender/ParticleGroup.h"

ref_sound* choose_snd;

namespace ChoseEvents
{
void __stdcall FillEntity(ChooseItemVec& items, void* param)
{
    //.    AppendItem	   					(RPOINT_CHOOSE_NAME);
    for (auto& it : pSettings->sections())
    {
        pcstr val;
        if (it->line_exist("$spawn", &val))
            items.push_back(SChooseItem(it->Name.c_str(), ""));
    }
}
//---------------------------------------------------------------------------
void __stdcall SelectSoundSource(SChooseItem* item, PropItemVec& info_items)
{
    choose_snd->stop();
    choose_snd->create(item->name.c_str(), st_Effect, sg_Undefined);
    choose_snd->play(0, sm_2D);
    //    snd.pla
    /*
    //.
        ECustomThumbnail*& thm, ref_sound& snd,
        thm 		= new ESoundThumbnail(item->name.c_str());
    */
}
void __stdcall CloseSoundSource() { choose_snd->destroy(); }
void __stdcall FillSoundSource(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (SndLib->GetGameSounds(lst))
    {
        for (auto& it : lst)
            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}
//---------------------------------------------------------------------------
void __stdcall FillSoundEnv(ChooseItemVec& items, void* param)
{
    AStringVec lst;
    if (SndLib->GetSoundEnvs(lst))
    {
        for (auto& it : lst)
            items.push_back(SChooseItem(it.c_str(), ""));
    }
}
//---------------------------------------------------------------------------
void __stdcall FillObject(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (Lib.GetObjects(lst))
    {
        for (auto& it : lst)
            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}
void __stdcall SelectObject(SChooseItem* item, PropItemVec& info_items)
{
    EObjectThumbnail* thm = new EObjectThumbnail(*item->name);
    if (thm->Valid())
        thm->FillInfo(info_items);
    xr_delete(thm);
}
void __stdcall DrawObjectTHM(LPCSTR name, HDC hdc, const Irect& r)
{
    EObjectThumbnail* thm = new EObjectThumbnail(name);
    if (thm->Valid())
        thm->Draw(hdc, r);
    xr_delete(thm);
}
//---------------------------------------------------------------------------
void __stdcall FillGroup(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (FS.file_list(lst, _groups_, FS_ListFiles | FS_ClampExt, "*.group"))
    {
        for (auto& it : lst)
            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}
void __stdcall SelectGroup(SChooseItem* item, PropItemVec& info_items)
{
    EGroupThumbnail* thm = new EGroupThumbnail(*item->name);
    if (thm->Valid())
        thm->FillInfo(info_items);
    xr_delete(thm);
}
void __stdcall DrawGroupTHM(LPCSTR name, HDC hdc, const Irect& r)
{
    EGroupThumbnail* thm = new EGroupThumbnail(name);
    if (thm->Valid())
        thm->Draw(hdc, r);
    xr_delete(thm);
}
//---------------------------------------------------------------------------
void __stdcall FillVisual(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (FS.file_list(lst, _game_meshes_, FS_ListFiles | FS_ClampExt, "*.ogf"))
    {
        for (auto& it : lst)
            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}
void __stdcall SelectVisual(SChooseItem* item, PropItemVec& info_items)
{
    /*
    //.
        AnsiString fn					= ChangeFileExt(item->name.c_str(),".ogf");
        IRender_Visual* visual			= ::Render->model_Create(fn.c_str());
        if (visual){
            PHelper().CreateCaption	(info_items,	"Source",
    *visual->desc.source_file?*visual->desc.source_file:"unknown");
            PHelper().CreateCaption	(info_items, 	"Creator
    N",*visual->desc.create_name?*visual->desc.create_name:"unknown");
            PHelper().CreateCaption	(info_items,	"Creator
    T",Trim(AnsiString(ctime(&visual->desc.create_time))).c_str());
            PHelper().CreateCaption	(info_items,	"Modif N",	*visual->desc.modif_name ?*visual->desc.modif_name
    :"unknown");
            PHelper().CreateCaption	(info_items,	"Modif T",
    Trim(AnsiString(ctime(&visual->desc.modif_time))).c_str());
            PHelper().CreateCaption	(info_items,	"Build N",	*visual->desc.build_name ?*visual->desc.build_name
    :"unknown");
            PHelper().CreateCaption	(info_items,	"Build T",
    Trim(AnsiString(ctime(&visual->desc.build_time))).c_str());
        }
        ::Render->model_Delete(visual);
    */
}
//---------------------------------------------------------------------------
void __stdcall FillGameObjectMots(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (FS.file_list(lst, _game_meshes_, FS_ListFiles | FS_ClampExt, "*.omf"))
    {
        for (auto& it : lst)
            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}
void __stdcall SelectGameObjectMots(SChooseItem* item, PropItemVec& info_items) {}
//---------------------------------------------------------------------------
void __stdcall FillGameAnim(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (FS.file_list(lst, "$game_anims$", FS_ListFiles, "*.anm,*.anms"))
    {
        for (auto& it : lst)

            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}
//---------------------------------------------------------------------------
void __stdcall FillLAnim(ChooseItemVec& items, void* param)
{
    for (auto& it : LALib.Objects())
        items.push_back(SChooseItem((*it)->cName.c_str(), ""));
}
void __stdcall DrawLAnim(LPCSTR name, HDC hdc, const Irect& r)
{
    int frame;
    CLAItem* item = LALib.FindItem(name);
    if (item)
    {
        HBRUSH hbr = CreateSolidBrush(item->CalculateBGR(EDevice.fTimeGlobal, frame));
        FillRect(hdc, (RECT*)&r, hbr);
        DeleteObject(hbr);
    }
}
//---------------------------------------------------------------------------
void __stdcall FillEShader(ChooseItemVec& items, void* param)
{
    for (auto& it : EDevice.Resources->_GetBlenders())
        items.push_back(SChooseItem(it.first, ""));
}
//---------------------------------------------------------------------------
void __stdcall FillCShader(ChooseItemVec& items, void* param)
{
    for (auto& it : EDevice.ShaderXRLC.Library())
        items.push_back(SChooseItem(it.Name, ""));
}
//---------------------------------------------------------------------------
void __stdcall FillPE(ChooseItemVec& items, void* param)
{
    for (PS::PEDIt E = GEnv.Render->PSLibrary.FirstPED(); E != GEnv.Render->PSLibrary.LastPED(); ++E)
        items.push_back(SChooseItem(*(*E)->m_Name, "EFFECT"));
}
//---------------------------------------------------------------------------
void __stdcall FillParticles(ChooseItemVec& items, void* param)
{
    for (PS::PEDIt E = GEnv.Render->PSLibrary.FirstPED(); E != GEnv.Render->PSLibrary.LastPED(); ++E)
        items.push_back(SChooseItem(*(*E)->m_Name, "EFFECT"));

    for (PS::PGDIt G = GEnv.Render->PSLibrary.FirstPGD(); G != GEnv.Render->PSLibrary.LastPGD(); ++G)
        items.push_back(SChooseItem(*(*G)->m_Name, "GROUP"));
}

void __stdcall SelectPE(SChooseItem* item, PropItemVec& info_items)
{
    string64 str;
    u32 i = 0;
    PHelper().CreateCaption(info_items, "", "used in groups");
    for (PS::PGDIt G = GEnv.Render->PSLibrary.FirstPGD(); G != GEnv.Render->PSLibrary.LastPGD(); ++G)
    {
        PS::CPGDef* def = (*G);
        for (auto& pe_it : def->m_Effects)
        {
            if (pe_it->m_EffectName == item->name)
            {
                xr_sprintf(str, sizeof(str), "%d", ++i);
                PHelper().CreateCaption(info_items, str, def->m_Name);
            }
        }
    }
}

void __stdcall SelectPG(SChooseItem* item, PropItemVec& info_items)
{
    string64 str;
    u32 i = 0;
    PHelper().CreateCaption(info_items, "", "using effects");
    for (PS::PGDIt G = GEnv.Render->PSLibrary.FirstPGD(); G != GEnv.Render->PSLibrary.LastPGD(); ++G)
    {
        PS::CPGDef* def = (*G);
        if (def->m_Name == item->name)
        {
            for (auto& pe_it : def->m_Effects)
            {
                xr_sprintf(str, sizeof(str), "%d", ++i);
                PHelper().CreateCaption(info_items, str, pe_it->m_EffectName);
            }
            break;
        }
    }
}

//---------------------------------------------------------------------------
void __stdcall FillTexture(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (ImageLib.GetTextures(lst))
    {
        for (auto& it : lst)

            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}

void __stdcall DrawTextureTHM(LPCSTR name, HDC hdc, const Irect& r)
{
    if (name && name[0])
    {
        ETextureThumbnail* thm = new ETextureThumbnail(name);
        if (thm->Valid())
            thm->Draw(hdc, r);
        xr_delete(thm);
    }
}

//---------------------------------------------------------------------------
void __stdcall FillTextureRaw(ChooseItemVec& items, void* param)
{
    FS_FileSet lst;
    if (ImageLib.GetTexturesRaw(lst))
    {
        for (auto& it : lst)

            items.push_back(SChooseItem(it.name.c_str(), ""));
    }
}

void __stdcall DrawTextureTHMRaw(LPCSTR name, HDC hdc, const Irect& r)
{
    if (name && name[0])
    {
        ETextureThumbnail* thm = new ETextureThumbnail(name);
        if (thm->Valid())
            thm->Draw(hdc, r);
        xr_delete(thm);
    }
}

void __stdcall SelectTexture(SChooseItem* item, PropItemVec& info_items)
{
    if (item->name.size())
    {
        ETextureThumbnail* thm = new ETextureThumbnail(*item->name);
        if (thm->Valid())
            thm->FillInfo(info_items);
        xr_delete(thm);
    }
}
void __stdcall SelectTextureRaw(SChooseItem* item, PropItemVec& info_items)
{
    if (item->name.size())
    {
        ETextureThumbnail* thm = new ETextureThumbnail(*item->name);
        if (thm->Valid())
            thm->FillInfo(info_items);
        xr_delete(thm);
    }
}
//---------------------------------------------------------------------------
void __stdcall FillGameMaterial(ChooseItemVec& items, void* param)
{
    GameMtlIt _F = GMLib.FirstMaterial();
    GameMtlIt _E = GMLib.LastMaterial();
    for (; _F != _E; _F++)
        items.push_back(SChooseItem(*(*_F)->m_Name, ""));
}
//---------------------------------------------------------------------------

void __stdcall FillSkeletonAnims(ChooseItemVec& items, void* param)
{
    IRenderVisual* V = GEnv.Render->model_Create((LPCSTR)param);
    if (PKinematicsAnimated(V))
    {
        u32 cnt = PKinematicsAnimated(V)->LL_MotionsSlotCount();
        for (u32 k = 0; k < cnt; k++)
        {
            for (auto& it : PKinematicsAnimated(V)->LL_Motions(k))
            {
                bool found = false;
                for (auto& it2 : items)
                {
                    if (it2.name == it.first)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    items.push_back(SChooseItem(it.first.c_str(), ""));
            }
        }
    }
    GEnv.Render->model_Delete(V);
}

void __stdcall FillSkeletonBones(ChooseItemVec& items, void* param)
{
    IRenderVisual* V = GEnv.Render->model_Create((LPCSTR)param);
    if (PKinematics(V))
    {
        for (auto& it : *PKinematics(V)->LL_Bones())
            items.push_back(SChooseItem(it.first.c_str(), ""));
    }
    GEnv.Render->model_Delete(V);
}

void __stdcall FillSkeletonBonesObject(ChooseItemVec& items, void* param)
{
    CEditableObject* eo = (CEditableObject*)param;

    auto _I = eo->FirstBone();
    auto _E = eo->LastBone();
    for (; _I != _E; ++_I)
    {
        items.push_back(SChooseItem((*_I)->Name().c_str(), ""));
    }
}

} // namespace

void FillChooseEvents()
{
    TfrmChoseItem::AppendEvents(smSoundSource, "Select Sound Source", ChoseEvents::FillSoundSource,
        ChoseEvents::SelectSoundSource, 0, ChoseEvents::CloseSoundSource, 0);
    TfrmChoseItem::AppendEvents(smSoundEnv, "Select Sound Environment", ChoseEvents::FillSoundEnv, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smObject, "Select Library Object", ChoseEvents::FillObject, ChoseEvents::SelectObject,
        ChoseEvents::DrawObjectTHM, 0, 0);
    TfrmChoseItem::AppendEvents(
        smGroup, "Select Group", ChoseEvents::FillGroup, ChoseEvents::SelectGroup, ChoseEvents::DrawGroupTHM, 0, 0);
    TfrmChoseItem::AppendEvents(smEShader, "Select Engine Shader", ChoseEvents::FillEShader, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smCShader, "Select Compiler Shader", ChoseEvents::FillCShader, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(
        smPE, "Select Particle Effect", ChoseEvents::FillPE, 0 /*ChoseEvents::SelectPE*/, 0, 0, 0);
    TfrmChoseItem::AppendEvents(
        smParticles, "Select Particle System", ChoseEvents::FillParticles, 0 /*ChoseEvents::SelectPG*/, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smTextureRaw, "Select Source Texture", ChoseEvents::FillTextureRaw,
        ChoseEvents::SelectTextureRaw, ChoseEvents::DrawTextureTHMRaw, 0, 0);
    TfrmChoseItem::AppendEvents(smTexture, "Select Texture", ChoseEvents::FillTexture, ChoseEvents::SelectTexture,
        ChoseEvents::DrawTextureTHM, 0, 0);
    TfrmChoseItem::AppendEvents(smEntityType, "Select Entity", ChoseEvents::FillEntity, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smLAnim, "Select Light Animation", ChoseEvents::FillLAnim, 0, ChoseEvents::DrawLAnim, 0,
        SChooseEvents::flAnimated);
    TfrmChoseItem::AppendEvents(smVisual, "Select Visual", ChoseEvents::FillVisual, ChoseEvents::SelectVisual, 0, 0, 0);
    TfrmChoseItem::AppendEvents(
        smSkeletonAnims, "Select Skeleton Animation", ChoseEvents::FillSkeletonAnims, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smSkeletonBones, "Select Skeleton Bones", ChoseEvents::FillSkeletonBones, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(
        smSkeletonBonesInObject, "Select Skeleton Bones", ChoseEvents::FillSkeletonBonesObject, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smGameMaterial, "Select Game Material", ChoseEvents::FillGameMaterial, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smGameAnim, "Select Animation", ChoseEvents::FillGameAnim, 0, 0, 0, 0);
    TfrmChoseItem::AppendEvents(smGameSMotions, "Select Game Object Motions", ChoseEvents::FillGameObjectMots,
        ChoseEvents::SelectGameObjectMots, 0, 0, 0);
    choose_snd = new ref_sound();
}
void ClearChooseEvents()
{
    TfrmChoseItem::ClearEvents();
    xr_delete(choose_snd);
}

//---------------------------------------------------------------------------
#endif
