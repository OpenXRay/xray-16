//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "UI_ParticleTools.h"

#include "ObjectAnimator.h"
#include "..\XrECore\Editor\ParticleEffectActions.h"
//------------------------------------------------------------------------------
CParticleTool *PTools = (CParticleTool *)Tools;
//------------------------------------------------------------------------------
#define CHECK_SNAP(R, A, C)   \
    {                         \
        R += A;               \
        if (fabsf(R) >= C)    \
        {                     \
            A = snapto(R, C); \
            R = 0;            \
        }                     \
        else                  \
        {                     \
            A = 0;            \
        }                     \
    }
// static Fvector zero_vec={0.f,0.f,0.f};

EParticleAction *pCreateEActionImpl(PAPI::PActionEnum type);

CParticleTool::CParticleTool()
{
    m_CreatingParticle = false;
    m_EditMode = emNone;
    m_ItemProps = 0;
    m_EditObject = 0;
    m_bModified = false;
    m_bReady = false;
    m_Transform.identity();
    m_Vel.set(0, 0, 0);
    fFogness = 0.9f;
    dwFogColor = 0xffffffff;
    m_Flags.zero();
    pCreateEAction = pCreateEActionImpl;
    m_LibPED = 0;
    m_EditPG = 0;
    m_EditPE = 0;
}
//---------------------------------------------------------------------------

CParticleTool::~CParticleTool()
{
}
//---------------------------------------------------------------------------

bool CParticleTool::OnCreate()
{

    m_bReady = true;

    Load(0);

    SetAction(etaSelect);

    m_EditPE = (PS::CParticleEffect *)::Render->Models->CreatePE(0);
    m_EditPG = (PS::CParticleGroup *)::Render->Models->CreatePG(0);
    m_ItemProps = xr_new<UIPropertiesForm>();
    m_ItemProps->SetModifiedEvent(TOnModifiedEvent(this, &CParticleTool::OnItemModified));

    // item list
    m_PList = xr_new<UIItemListForm>();
    m_PList->m_Flags.set(UIItemListForm::fMenuEdit, true);
    m_PList->SetOnItemFocusedEvent(TOnILItemFocused(this, &CParticleTool::OnParticleItemFocused));
    m_PList->SetOnItemCloneEvent(TOnItemClone(this, &CParticleTool::OnParticleCloneItem));
    m_PList->SetOnItemCreaetEvent(TOnItemCreate(this, &CParticleTool::OnParticleCreateItem));
    m_PList->SetOnItemRenameEvent(TOnItemRename(this, &CParticleTool::OnParticleItemRename));
    m_PList->SetOnItemRemoveEvent(TOnItemRemove(this, &CParticleTool::OnParticleItemRemove));

    m_ParentAnimator = xr_new<CObjectAnimator>();

    m_ObjectProps = xr_new<UIPropertiesForm>();
    FillObjectPrefs();
    return true;
}

void CParticleTool::OnDestroy()
{
    VERIFY(m_bReady);
    m_bReady = false;

    xr_delete(m_ParentAnimator);

    Lib.RemoveEditObject(m_EditObject);

    xr_delete(m_ObjectProps);
    xr_delete(m_ItemProps);
    xr_delete(m_PList);
    xr_delete(m_EditPG);
    xr_delete(m_EditPE);
}
//---------------------------------------------------------------------------

bool CParticleTool::IfModified()
{
    if (m_bModified)
    {
        int mr = ELog.DlgMsg(mtConfirmation, "The particles has been modified.\nDo you want to save your changes?");
        switch (mr)
        {
        case mrYes:
            if (!ExecCommand(COMMAND_SAVE))
                return false;
            else
                m_bModified = FALSE;
            break;
        case mrNo:
            m_bModified = FALSE;
            break;
        case mrCancel:
            return false;
        }
    }
    return true;
}

void CParticleTool::Modified()
{
    m_bModified = true;
    ExecCommand(COMMAND_UPDATE_CAPTION);
}
//---------------------------------------------------------------------------

void CParticleTool::OnItemModified()
{
    Modified();
    if (m_LibPED)
        CompileEffect();
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}
#include "igame_persistent.h"

void CParticleTool::RenderEnvironment()
{
    /*
        if (psDeviceFlags.is(rsEnvironment)){
            g_pGamePersistent->Environment().RenderSky	();
            g_pGamePersistent->Environment().RenderClouds	();
        }
    */
}

void CParticleTool::Render()
{
    if (!m_bReady)
        return;

    PrepareLighting();

    if (m_EditObject)
        m_EditObject->RenderSingle(Fidentity);
    // draw parent axis
    DU_impl.DrawObjectAxis(m_Transform, 0.05f, true);
    // draw domains
    switch (m_EditMode)
    {
    case emNone:
        break;
    case emEffect:
    {
        if (m_EditPE && m_EditPE->GetDefinition())
            m_EditPE->GetDefinition()->Render(m_Transform);
    }
    break;
    case emGroup:
    {
        if (m_EditPG)
        {
            int cnt = m_EditPG->items.size();
            for (int k = 0; k < cnt; k++)
            {
                PS::CParticleEffect *E = (PS::CParticleEffect *)m_EditPG->items[k]._effect;
                if (E && E->GetDefinition() && m_LibPGD->m_Effects[k]->m_Flags.is(PS::CPGDef::SEffect::flEnabled))
                    E->GetDefinition()->Render(m_Transform);
            }
        }
    }
    break;
    default:
        THROW;
    }
    // Draw the particles.
    ::Render->Models->RenderSingle(m_EditPG, Fidentity, 1.f);
    ::Render->Models->RenderSingle(m_EditPE, Fidentity, 1.f);

    if (m_Flags.is(flAnimatedPath))
        m_ParentAnimator->DrawPath();

    //.    if (psDeviceFlags.is(rsEnvironment)) g_pGamePersistent->Environment().RenderLast	();
    inherited::Render();
}

void CParticleTool::OnFrame()
{
    if (!m_bReady)
        return;
    if (m_EditObject)
        m_EditObject->OnFrame();

    if (m_Flags.is(flAnimatedParent))
    {
        m_ParentAnimator->Update(EDevice.fTimeDelta);
        if (m_ParentAnimator->IsPlaying())
        {
            Fvector new_vel;
            new_vel.sub(m_ParentAnimator->XFORM().c, m_Transform.c);
            new_vel.div(EDevice.fTimeDelta);
            m_Vel.lerp(m_Vel, new_vel, 0.9);
            m_Transform = m_ParentAnimator->XFORM();
            m_Flags.set(flApplyParent, TRUE);
        }
    }

    if (m_Flags.is(flRemoveAction))
        RealRemoveAction();
    if (m_Flags.is(flApplyParent))
        RealApplyParent();
    if (m_Flags.is(flCompileEffect))
        RealCompileEffect();

    m_EditPE->OnFrame(EDevice.dwTimeDelta);
    m_EditPG->OnFrame(EDevice.dwTimeDelta);

    if (m_Flags.is(flRefreshProps))
        RealUpdateProperties();

    if (m_Flags.is(flSelectEffect))
    {
        m_PList->SelectItem(sel_eff_name.c_str());
        m_Flags.set(flSelectEffect, FALSE);
        sel_eff_name = "";
    }

    xr_string tmp;
    switch (m_EditMode)
    {
    case emNone:
        break;
    case emEffect:
        if (m_EditPE->IsPlaying())
        {

            xr_string nn;
            nn.sprintf(" PE Playing...[%d]", m_EditPE->ParticlesCount()).c_str();
            UI->SetStatus(nn.c_str(), false);
        }

        else
            UI->SetStatus(" Stopped.", false);
        break;
    case emGroup:
        if (m_EditPG->IsPlaying())
            UI->SetStatus(xr_string().sprintf(" PE Playing...[%d]", m_EditPG->ParticlesCount()).c_str(), false);
        else
            UI->SetStatus(" Stopped.", false);
        break;
    default:
        THROW;
    }
}

void CParticleTool::ZoomObject(BOOL bSelOnly)
{
    VERIFY(m_bReady);
    if (!bSelOnly && m_EditObject)
    {
        EDevice.m_Camera.ZoomExtents(m_EditObject->GetBox());
    }
    else
    {
        Fbox box;
        box.invalidate();
        switch (m_EditMode)
        {
        case emNone:
            break;
        case emEffect:
            box.set(m_EditPE->vis.box);
            break;
        case emGroup:
            box.set(m_EditPG->vis.box);
            break;
        default:
            THROW;
        }
        if (box.is_valid())
        {
            box.grow(1.f);
            EDevice.m_Camera.ZoomExtents(box);
        }
    }
}

void CParticleTool::PrepareLighting()
{
    // add directional light
    Flight L;
    ZeroMemory(&L, sizeof(Flight));
    L.type = D3DLIGHT_DIRECTIONAL;
    L.diffuse.set(1, 1, 1, 1);
    L.direction.set(1, -1, 1);
    L.direction.normalize();
    EDevice.SetLight(0, L);
    EDevice.LightEnable(0, true);

    L.diffuse.set(0.3, 0.3, 0.3, 1);
    L.direction.set(-1, -1, -1);
    L.direction.normalize();
    EDevice.SetLight(1, L);
    EDevice.LightEnable(1, true);

    L.diffuse.set(0.3, 0.3, 0.3, 1);
    L.direction.set(1, -1, -1);
    L.direction.normalize();
    EDevice.SetLight(2, L);
    EDevice.LightEnable(2, true);

    L.diffuse.set(0.3, 0.3, 0.3, 1);
    L.direction.set(-1, -1, 1);
    L.direction.normalize();
    EDevice.SetLight(3, L);
    EDevice.LightEnable(3, true);

    L.diffuse.set(1.0, 0.8, 0.7, 1);
    L.direction.set(0, 1, 0);
    L.direction.normalize();
    EDevice.SetLight(4, L);
    EDevice.LightEnable(4, true);
}

void CParticleTool::OnDeviceCreate()
{
}

void CParticleTool::OnDeviceDestroy()
{
}

void CParticleTool::SelectPreviewObject(int p)
{
}

void CParticleTool::ResetPreviewObject()
{
    VERIFY(m_bReady);
    UI->RedrawScene();
}

bool CParticleTool::Load(LPCSTR name)
{
    VERIFY(m_bReady);
    UpdateProperties();
    return true;
}

bool CParticleTool::Save(bool bAsXR)
{
    VERIFY(m_bReady);

    // validate
    if (!Validate(true))
    {
        ELog.DlgMsg(mtError, "Invalid particle's found. Validate library and try again.");
        return false;
    }
    bool bRes = false;
    if (bAsXR)
    {
        bRes = ::Render->PSLibrary.Save();
    }
    else
    {
        bRes = ::Render->PSLibrary.Save2();
    }

    if (bRes)
        m_bModified = false;

    return bRes;
}

void CParticleTool::Reload()
{
    VERIFY(m_bReady);
    ResetCurrent();
    ::Render->PSLibrary.Reload();
    // visual part
    m_ItemProps->ClearProperties();
    UpdateProperties(true);
}

void CheckEffect(const xr_string &group_path, const shared_str &eff_full_name, xr_string &res_name, bool bRenameOnly)
{
    res_name = group_path + "effects\\" + EFS.ExtractFileName(eff_full_name.c_str());

    if (0 != stricmp(res_name.c_str(), eff_full_name.c_str()))
    {
        PS::CPEDef *old_ped = ::Render->PSLibrary.FindPED(eff_full_name.c_str());
        PS::CPEDef *new_ped = ::Render->PSLibrary.FindPED(res_name.c_str());
        if (bRenameOnly)
        {
            ::Render->PSLibrary.Remove(res_name.c_str());
            new_ped = NULL;
        }

        if (!new_ped)
        {
            new_ped = (bRenameOnly) ? old_ped : ::Render->PSLibrary.AppendPED(old_ped);
            new_ped->m_Name = res_name.c_str();
            if (bRenameOnly)
                Msg("rename effect [%s]->[%s]", eff_full_name.c_str(), res_name.c_str());
            else
                Msg("create new effect [%s]", res_name.c_str());
        }
        VERIFY(0 == stricmp(new_ped->m_Name.c_str(), res_name.c_str()));
    }
}

CCommandVar CParticleTool::CreateGroupFromSelected(CCommandVar p1, CCommandVar p2)
{
    /*PS::CPEDef* curr = m_LibPED;
    if(!curr)
    {
        ELog.DlgMsg	(mtError,"Select Effect first.");
        return false;
    }
    const shared_str& eff_name		= curr->m_Name;
    PS::CPGDef* pg					= AppendPG(0);

    xr_string grp_name				= eff_name.c_str();
    pg->m_Name						= grp_name.c_str();

    pg->m_fTimeLimit				= 0.0f;
    PS::CPGDef::SEffect* eff 		= xr_new<PS::CPGDef::SEffect>();
    pg->m_Effects.push_back	   		(eff);
    eff->m_EffectName				= eff_name;

    eff->m_Flags.set				(PS::CPGDef::SEffect::flEnabled,TRUE);
    eff->m_Time0					= 0.0f;
    eff->m_Time1					= 0.0f;


    xr_string						tmp;
    xr_string group_path 			= EFS.ExtractFilePath(grp_name.c_str());
    CheckEffect						(group_path, eff->m_EffectName, tmp, true);

    eff->m_EffectName				= tmp.c_str();

    curr->m_Name					= tmp.c_str();

    ExecCommand						(COMMAND_UPDATE_PROPERTIES);

    m_PList->SelectItem				(grp_name.c_str());*/

    return TRUE;
}

CCommandVar CParticleTool::Compact(CCommandVar p1, CCommandVar p2)
{
    if (!Validate(true))
    {
        ELog.DlgMsg(mtError, "Invalid particle's found. Validate library and try again.");
        return false;
    }

    for (PS::PGDIt g_it = ::Render->PSLibrary.FirstPGD(); g_it != ::Render->PSLibrary.LastPGD(); ++g_it)
    {
        PS::CPGDef *pg = (*g_it);
        shared_str &group_name = pg->m_Name;
        xr_string group_path = EFS.ExtractFilePath(group_name.c_str());

        xr_vector<PS::CPGDef::SEffect *>::const_iterator pe_it = pg->m_Effects.begin();
        xr_vector<PS::CPGDef::SEffect *>::const_iterator pe_it_e = pg->m_Effects.end();

        xr_string tmp;

        for (; pe_it != pe_it_e; ++pe_it)
        {
            PS::CPGDef::SEffect *Eff = (*pe_it);
            CheckEffect(group_path, Eff->m_EffectName, tmp, false);
            Eff->m_EffectName = tmp.c_str();

            if (Eff->m_Flags.test(PS::CPGDef::SEffect::flOnPlayChild))
            {
                CheckEffect(group_path, Eff->m_OnPlayChildName, tmp, false);
                Eff->m_OnPlayChildName = tmp.c_str();
            }
            if (Eff->m_Flags.test(PS::CPGDef::SEffect::flOnBirthChild))
            {
                CheckEffect(group_path, Eff->m_OnBirthChildName, tmp, false);
                Eff->m_OnBirthChildName = tmp.c_str();
            }
            if (Eff->m_Flags.test(PS::CPGDef::SEffect::flOnDeadChild))
            {
                CheckEffect(group_path, Eff->m_OnDeadChildName, tmp, false);
                Eff->m_OnDeadChildName = tmp.c_str();
            }
        }
    }

    ResetCurrent();
    UpdateProperties(true);

    return TRUE;
}

bool CParticleTool::Validate(bool bMsg)
{
    if (bMsg)
        ELog.Msg(mtInformation, "Begin validation...");
    PS::PEDIt _eI = ::Render->PSLibrary.FirstPED();
    PS::PEDIt _eE = ::Render->PSLibrary.LastPED();
    u32 error_cnt = 0;
    for (; _eI != _eE; ++_eI)
    {
        if (!(*_eI)->Validate(bMsg))
            error_cnt++;
    }
    for (PS::PGDIt g_it = ::Render->PSLibrary.FirstPGD(); g_it != ::Render->PSLibrary.LastPGD(); ++g_it)
    {
        PS::CPGDef *pg = (*g_it);
        if (!pg->Validate(bMsg))
            error_cnt++;
    }

    if (bMsg)
    {
        if (error_cnt > 0)
            ELog.DlgMsg(mtError, "Validation FAILED! Found %d error's.", error_cnt);
        else
            ELog.DlgMsg(mtInformation, "Validation OK.");
    }
    return error_cnt == 0;
}

void CParticleTool::Rename(LPCSTR old_full_name, LPCSTR ren_part, int level)
{
    VERIFY(level < _GetItemCount(old_full_name, '\\'));
    xr_string new_full_name;
    Rename(old_full_name, new_full_name.c_str());
}

void CParticleTool::Rename(LPCSTR old_full_name, LPCSTR new_full_name)
{
    VERIFY(m_bReady);
    // is effect
    PS::CPEDef *E = ::Render->PSLibrary.FindPED(old_full_name);
    if (E)
    {
        ::Render->PSLibrary.RenamePED(E, new_full_name);
        return;
    }
    // is group
    PS::CPGDef *G = ::Render->PSLibrary.FindPGD(old_full_name);
    if (G)
    {
        ::Render->PSLibrary.RenamePGD(G, new_full_name);
        return;
    }
}

void CParticleTool::Remove(LPCSTR name)
{

    if (::Render->PSLibrary.FindPED(name) == m_LibPED || ::Render->PSLibrary.FindPGD(name) == m_LibPGD)
    {
        m_ItemProps->ClearProperties();
    }

    VERIFY(m_bReady);
    SetCurrentPE(0);
    SetCurrentPG(0);
    ::Render->PSLibrary.Remove(name);
}

void CParticleTool::RemoveCurrent()
{
    R_ASSERT(0);
}

void CParticleTool::CloneCurrent()
{
    R_ASSERT(0);
}

void CParticleTool::ResetCurrent()
{
    VERIFY(m_bReady);
    if (m_LibPED)
        m_EditPE->Stop(FALSE);
    if (m_LibPGD)
        m_EditPG->Stop(FALSE);
    m_LibPED = 0;
    m_LibPGD = 0;
}

void CParticleTool::SetCurrentPE(PS::CPEDef *P)
{
    VERIFY(m_bReady);
    m_EditPG->Compile(0);
    if (m_LibPED != P)
    {
        m_LibPED = P;
        m_EditPE->Compile(m_LibPED);
        if (m_LibPED)
            m_EditMode = emEffect;
    }
}

void CParticleTool::SetCurrentPG(PS::CPGDef *P)
{
    VERIFY(m_bReady);
    m_EditPE->Compile(0);
    if (m_LibPGD != P)
    {
        m_LibPGD = P;
        m_EditPG->Compile(m_LibPGD);
        if (m_LibPGD)
            m_EditMode = emGroup;
    }
}

void CParticleTool::DrawReferenceList()
{
    if (m_EditMode == emGroup)
    {
        if (m_EditPG->GetDefinition())
        {
            xr_vector<PS::CPGDef::SEffect *>::const_iterator pe_it = m_EditPG->GetDefinition()->m_Effects.begin();
            xr_vector<PS::CPGDef::SEffect *>::const_iterator pe_it_e = m_EditPG->GetDefinition()->m_Effects.end();
            for (; pe_it != pe_it_e; ++pe_it)
            {
                ImGui::Text((*pe_it)->m_EffectName.c_str() ? (*pe_it)->m_EffectName.c_str() : 0);
            }
            if (m_EditPG->GetDefinition()->m_Flags.test(PS::CPGDef::SEffect::flOnPlayChild))
                ImGui::Text((*pe_it)->m_OnPlayChildName.c_str());
            if (m_EditPG->GetDefinition()->m_Flags.test(PS::CPGDef::SEffect::flOnBirthChild))
                ImGui::Text((*pe_it)->m_OnBirthChildName.c_str());
            if (m_EditPG->GetDefinition()->m_Flags.test(PS::CPGDef::SEffect::flOnDeadChild))
                ImGui::Text((*pe_it)->m_OnDeadChildName.c_str());
        }
    }
    else
    {
        if (m_EditPE->GetDefinition())
        {
            PS::PGDIt G = ::Render->PSLibrary.FirstPGD();
            PS::PGDIt G_e = ::Render->PSLibrary.LastPGD();
            for (; G != G_e; ++G)
            {
                PS::CPGDef *def = (*G);
                PS::CPGDef::EffectIt pe_it = def->m_Effects.begin();
                PS::CPGDef::EffectIt pe_it_e = def->m_Effects.end();
                for (; pe_it != pe_it_e; ++pe_it)
                {
                    if ((*pe_it)->m_EffectName == m_EditPE->Name())
                        ImGui::Text(def->m_Name.c_str());
                    else if ((*pe_it)->m_OnPlayChildName == m_EditPE->Name())
                        ImGui::Text(def->m_Name.c_str());
                    else if ((*pe_it)->m_OnBirthChildName == m_EditPE->Name())
                        ImGui::Text(def->m_Name.c_str());
                    else if ((*pe_it)->m_OnDeadChildName == m_EditPE->Name())
                        ImGui::Text(def->m_Name.c_str());
                }
            }
        }
    }
}

void CParticleTool::CommandJumpToItem()
{
    /* for(int i=0; i<fraLeftBar->refLB->Count; ++i)
     {
          if(fraLeftBar->refLB->Selected[i])
          {
              m_PList->SelectItem((fraLeftBar->refLB->Items->Strings[i]).c_str(),true,false,true);
              break;
          }
      }*/
}

PS::CPEDef *CParticleTool::FindPE(LPCSTR name)
{
    return ::Render->PSLibrary.FindPED(name);
}

PS::CPGDef *CParticleTool::FindPG(LPCSTR name)
{
    return ::Render->PSLibrary.FindPGD(name);
}

void CParticleTool::PlayCurrent(int idx)
{
    VERIFY(m_bReady);
    StopCurrent(false);
    switch (m_EditMode)
    {
    case emNone:
        break;
    case emEffect:
        m_EditPE->Play();
        break;
    case emGroup:
        if (idx > -1)
        {
            VERIFY(idx < (int)m_EditPG->items.size());
            m_LibPED = ((PS::CParticleEffect *)m_EditPG->items[idx]._effect)->GetDefinition();
            m_EditPE->Compile(m_LibPED);
            m_EditPE->Play();
        }
        else
        {
            // play all
            m_EditPG->Play();
        }
        break;
    default:
        THROW;
    }
    ApplyParent();
}

void CParticleTool::StopCurrent(bool bFinishPlaying)
{
    VERIFY(m_bReady);
    m_EditPE->Stop(bFinishPlaying);
    m_EditPG->Stop(bFinishPlaying);
}

void CParticleTool::SelectEffect(LPCSTR name)
{
    sel_eff_name = name;
    m_Flags.set(flSelectEffect, TRUE);
}

void CParticleTool::OnShowHint(AStringVec &SS)
{
}

bool CParticleTool::MouseStart(TShiftState Shift)
{
    inherited::MouseStart(Shift);
    switch (m_Action)
    {
    case etaSelect:
        break;
    case etaAdd:
        break;
    case etaMove:
    {
        if (Shift | ssCtrl)
        {
            if (m_EditObject)
            {
                float dist = UI->ZFar();
                SRayPickInfo pinf;
                if (m_EditObject->RayPick(dist, UI->m_CurrentRStart, UI->m_CurrentRDir, Fidentity, &pinf))
                    m_Transform.c.set(pinf.pt);
            }
            else
            {
                // pick grid
                Fvector normal = {0.f, 1.f, 0.f};
                float clcheck = UI->m_CurrentRDir.dotproduct(normal);
                if (fis_zero(clcheck))
                    return false;
                float alpha = -UI->m_CurrentRStart.dotproduct(normal) / clcheck;
                if (alpha <= 0)
                    return false;

                m_Transform.c.mad(UI->m_CurrentRStart, UI->m_CurrentRDir, alpha);

                if (m_Settings.is(etfGSnap))
                {
                    m_Transform.c.x = snapto(m_Transform.c.x, m_MoveSnap);
                    m_Transform.c.z = snapto(m_Transform.c.z, m_MoveSnap);
                    m_Transform.c.y = 0.f;
                }
            }
        }
    }
    break;
    case etaRotate:
        break;
    case etaScale:
        break;
    }
    ApplyParent();
    return m_bHiddenMode;
}

bool CParticleTool::MouseEnd(TShiftState Shift)
{
    inherited::MouseEnd(Shift);
    return true;
}

void CParticleTool::MouseMove(TShiftState Shift)
{
    inherited::MouseMove(Shift);
    switch (m_Action)
    {
    case etaSelect:
        break;
    case etaAdd:
        break;
    case etaMove:
        m_Transform.c.add(m_MovedAmount);
        break;
    case etaRotate:
    {
        Fmatrix mR;
        mR.identity();
        if (!fis_zero(m_RotateVector.x))
            mR.rotateX(m_RotateAmount);
        else if (!fis_zero(m_RotateVector.y))
            mR.rotateY(m_RotateAmount);
        else if (!fis_zero(m_RotateVector.z))
            mR.rotateZ(m_RotateAmount);
        m_Transform.mulB_43(mR);
    }
    break;
    case etaScale:
        break;
    }
    ApplyParent();
}
//------------------------------------------------------------------------------

void CParticleTool::RealApplyParent()
{
    switch (m_EditMode)
    {
    case emNone:
        break;
    case emEffect:
        m_EditPE->UpdateParent(m_Transform, m_Vel, m_Flags.is(flSetXFORM));
        break;
    case emGroup:
        m_EditPG->UpdateParent(m_Transform, m_Vel, m_Flags.is(flSetXFORM));
        break;
    default:
        THROW;
    }
    m_Flags.set(flApplyParent, FALSE);
}

void CParticleTool::RealCompileEffect()
{
    if (m_LibPED)
        m_LibPED->Compile(m_LibPED->m_EActionList);
    m_Flags.set(flCompileEffect, FALSE);
}

void CParticleTool::RealRemoveAction()
{
    if (m_LibPED)
    {
        xr_delete(m_LibPED->m_EActionList[remove_action_num]);
        m_LibPED->m_EActionList.erase(m_LibPED->m_EActionList.begin() + remove_action_num);
    }
    m_Flags.set(flRemoveAction, FALSE);
}

LPCSTR CParticleTool::GetInfo()
{
    return 0;
}
//------------------------------------------------------------------------------

void CParticleTool::SelectListItem(LPCSTR pref, LPCSTR name, bool bVal, bool bLeaveSel, bool bExpand)
{
    xr_string nm = (name && name[0]) ? PrepareKey(pref, name).c_str() : pref;
    m_PList->SelectItem(nm.c_str());
    if (pref)
    {
        m_PList->SelectItem(pref);
    }
}
//------------------------------------------------------------------------------

PS::CPEDef *CParticleTool::AppendPE(PS::CPEDef *src, const char *path)
{
    VERIFY(m_bReady);
    PS::CPEDef *S = ::Render->PSLibrary.AppendPED(src);
    S->m_Name = path;
    ExecCommand(COMMAND_UPDATE_PROPERTIES, true);
    SelectListItem(0, path, true, false, true);
    return S;
}

PS::CPGDef *CParticleTool::AppendPG(PS::CPGDef *src, const char *path)
{
    VERIFY(m_bReady);
    PS::CPGDef *S = ::Render->PSLibrary.AppendPGD(src);
    S->m_Name = path;

    ExecCommand(COMMAND_UPDATE_PROPERTIES, true);
    SelectListItem(0, path, true, false, true);
    return S;
}

#include "../XrECore/Editor/EditMesh.h"

bool CParticleTool::RayPick(const Fvector &start, const Fvector &dir, float &dist, Fvector *pt, Fvector *n)
{
    if (m_EditObject)
    {
        SRayPickInfo pinf;
        if (m_EditObject->RayPick(dist, start, dir, Fidentity, &pinf))
        {
            if (pt)
                pt->set(pinf.pt);
            if (n)
            {
                const Fvector *PT[3];
                pinf.e_mesh->GetFacePT(pinf.inf.id, PT);
                n->mknormal(*PT[0], *PT[1], *PT[2]);
            }
            return true;
        }
        else
            return false;
    }
    else
    {
        Fvector np;
        np.mad(start, dir, dist);
        if ((start.y > 0) && (np.y < 0.f))
        {
            if (pt)
                pt->set(start);
            if (n)
                n->set(0.f, 1.f, 0.f);
            return true;
        }
        else
            return false;
    }
}

void CParticleTool::OnChangeMotion(PropValue *sender)
{
    ChooseValue *V = dynamic_cast<ChooseValue *>(sender);
    if (V)
    {
        m_ParentAnimator->Clear();
        if (V->value->size())
            m_ParentAnimator->Load(V->value->c_str());
    }
    if (m_Flags.is(flAnimatedParent))
        m_ParentAnimator->Play(true);
    FillObjectPrefs();
}

void CParticleTool::OnChangeObject(PropValue *sender)
{
    ChooseValue *V = dynamic_cast<ChooseValue *>(sender);
    if (V)
    {
        Lib.RemoveEditObject(m_EditObject);
        m_EditObject = V->value->c_str() ? Lib.CreateEditObject(V->value->c_str()) : 0;
        //	ZoomObject(TRUE);

        UI->RedrawScene();
    }
    FillObjectPrefs();
}

void CParticleTool::FillObjectPrefs()
{
    PropItemVec items;
    m_MotionName = m_ParentAnimator->Name();
    PropValue *V;
    V = PHelper().CreateChoose(items, "Object", &m_ObjectName, smObject);
    V->OnChangeEvent.bind(this, &CParticleTool::OnChangeObject);
    V = PHelper().CreateFlag32(items, "Parent\\Allow Animated", &m_Flags, flAnimatedParent);
    V->OnChangeEvent.bind(this, &CParticleTool::OnChangeMotion);
    PHelper().CreateFlag32(items, "Parent\\Draw Path", &m_Flags, flAnimatedPath);
    V = PHelper().CreateChoose(items, "Parent\\Motion", &m_MotionName, smGameAnim);
    V->OnChangeEvent.bind(this, &CParticleTool::OnChangeMotion);
    PHelper().CreateFloat(items, "Parent\\Motion Speed", &m_ParentAnimator->Speed(), 0.f, 10000.f);
    m_ObjectProps->AssignItems(items);
}

bool CParticleTool::GetSelectionPosition(Fmatrix &result)
{
    result = m_Transform;
    return true;
}
