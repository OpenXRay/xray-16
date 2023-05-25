﻿//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop
#define dSINGLE
#include "animation_blend.h"
#include "..\..\XrPhysics\Physics.h"
#include "..\XrECore\Editor\EditMesh.h"
#include "KinematicAnimatedDefs.h"
#include "SkeletonAnimated.h"
CActorTools *ATools = (CActorTools *)Tools;
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

void EngineModel::DeleteVisual()
{
    DeletePhysicsShell();
    Render->model_Delete(m_pVisual);
    m_pVisual = 0;
    m_pBlend = 0;
}

void EngineModel::OnRender()
{
    Fmatrix temp = Fmatrix();
    UpdateObjectXform(temp);
}

/*
void PreviewModel::RestoreParams(TFormStorage* s)
{
    m_Props->RestoreParams(s);
    m_LastObjectName	= s->ReadString	("preview_name","");
    int val;
    val					= s->ReadInteger("preview_speed",0); 	m_fSpeed 	= *((float*)&val);
    val					= s->ReadInteger("preview_segment",0); 	m_fSegment	= *((float*)&val);
    m_Flags.assign		(s->ReadInteger("preview_flags",0));
    m_ScrollAxis		= (EScrollAxis)s->ReadInteger("preview_scaxis",0);
}

void PreviewModel::SaveParams(TFormStorage* s)
{
    m_Props->SaveParams(s);
    s->WriteString	("preview_name",	m_LastObjectName);
    s->WriteInteger	("preview_speed",	*((int*)&m_fSpeed));
    s->WriteInteger	("preview_segment",	*((int*)&m_fSegment));
    s->WriteInteger	("preview_flags",	m_Flags.get());
    s->WriteInteger	("preview_scaxis",	m_ScrollAxis);
}
*/
/*
void PreviewModel::OnDestroy()
{
    TProperties::DestroyForm(m_Props);
}

void PreviewModel::OnCreate()
{
    m_Props = TProperties::CreateForm("Preview prefs",0,alNone);
}

void PreviewModel::Clear()
{
    Lib.RemoveEditObject(m_pObject);
}
void PreviewModel::SelectObject()
{
    LPCSTR fn;
    if (!TfrmChoseItem::SelectItem(smObject,fn,1,m_LastObjectName.c_str())) return;
    Lib.RemoveEditObject(m_pObject);
    m_pObject = Lib.CreateEditObject(fn);
    if (!m_pObject)	ELog.DlgMsg(mtError,"Object '%s' can't find in object library.",fn);
    else			m_LastObjectName = fn;
}

xr_token		sa_token					[ ]={
    { "+Z",		PreviewModel::saZp	},
    { "-Z",		PreviewModel::saZn	},
    { "+X",		PreviewModel::saXp	},
    { "-X",		PreviewModel::saXn	},
    { 0,		0								}
};

void PreviewModel::SetPreferences()
{
    PropItemVec items;
    PHelper().CreateFlag32		(items, 	"Scroll", 		&m_Flags, 		pmScroll);
    PHelper().CreateFloat		(items, 	"Speed (m/c)",	&m_fSpeed,		-10000.f,10000.f,0.01f,2);
    PHelper().CreateFloat		(items, 	"Segment (m)",	&m_fSegment,	-10000.f,10000.f,0.01f,2);
    PHelper().CreateToken32		(items,		"Scroll axis",	(u32*)&m_ScrollAxis,	sa_token);
    m_Props->AssignItems		(items);
    m_Props->ShowProperties		();
}
void PreviewModel::Render()
{
    if (m_pObject){
        float angle;
        switch (m_ScrollAxis){
        case saZp: angle = 0;		break;
        case saZn: angle = PI;		break;
        case saXp: angle = PI_DIV_2;break;
        case saXn: angle =-PI_DIV_2;break;
        default: THROW;
        }
        Fmatrix R,T;
        R.rotateY(angle);
        T.translate(m_vPosition);
        T.mulA_43(R);
        m_pObject->RenderSingle(T);
    }
}

void PreviewModel::Update()
{
    if (m_Flags.is(pmScroll))
    {
        m_vPosition.z += m_fSpeed*EDevice.fTimeDelta;
        if (m_vPosition.z>m_fSegment) m_vPosition.z-=m_fSegment;
    }
}

void _SynchronizeTextures()
{
    FS_FileSet M_THUM;
    FS.file_list(M_THUM,_textures_,FS_ListFiles|FS_ClampExt,"*.thm");

    FS_FileSetIt it		= M_THUM.begin();
    FS_FileSetIt _E 	= M_THUM.end();
    for (; it!=_E; it++){
        ETextureThumbnail* THM=0;
        THM = xr_new<ETextureThumbnail>(it->name.c_str());
        STextureParams& fmt = THM->_Format();
        if (fmt.material==STextureParams::tmOrenNayar_Blin){
            fmt.material=STextureParams::tmBlin_Phong;
            THM->Save(0,0);
        }
        xr_delete(THM);
    }
}

*/

CActorTools::CActorTools()
{
    m_IsPhysics = false;
    m_ChooseSkeletonBones = false;
    m_Props = 0;
    m_pEditObject = 0;
    m_bObjectModified = false;
    m_ObjectItems = 0;
    m_Flags.zero();
    m_EditMode = emObject;
    fFogness = 0.9f;
    dwFogColor = 0xffffffff;
}

CActorTools::~CActorTools()
{
}

#include "..\xrECore\editor\d3dutils.h"

const u32 color_bone_sel_color = 0xFFFFFFFF;
const u32 color_bone_norm_color = 0xFFFFFF00;
const u32 color_bone_link_color = 0xFFA0A000;
const u32 color_bone_sel_cm = 0xFFFF0000;
const u32 color_bone_norm_cm = 0xFF700000;
const float joint_size = 0.025f;

void CActorTools::Render()
{
    if (!m_bReady)
        return;
    PrepareLighting();
    m_PreviewObject.Render();
    if (m_pEditObject)
    {
        Fmatrix World = Fidentity;
        {
            m_pEditObject->UpdateObjectXform(World);
        }
        m_RenderObject.OnRender();
        if (m_RenderObject.IsRenderable() && MainForm->GetLeftBarForm()->GetRenderMode() == UILeftBarForm::Render_Engine)
        {
            ::Render->model_RenderSingle(m_RenderObject.m_pVisual, m_RenderObject.ObjectXFORM(), m_RenderObject.m_fLOD);
            RCache.set_xform_world(World);
            if (EPrefs->object_flags.is(epoDrawJoints))
            {
                CKinematics *K = dynamic_cast<CKinematics *>(m_RenderObject.m_pVisual);
                if (K)
                {
                    u16 bcnt = K->LL_BoneCount();
                    for (u16 bidx = 0; bidx < bcnt; ++bidx)
                    {
                        EDevice.SetShader(EDevice.m_WireShader);

                        Fmatrix M = Fmatrix().mul(m_RenderObject.ObjectXFORM(), K->LL_GetTransform(bidx));

                        Fvector p1 = M.c;
                        u32 c_joint = color_bone_norm_color;
                        DU_impl.DrawJoint(p1, joint_size, c_joint);

                        if (EPrefs->object_flags.is(epoDrawBoneAxis))
                        {
                            DU_impl.DrawObjectAxis(M, 0.03f, false);
                        }
                    }
                }
            }
        }
        else
        {
            // update transform matrix
            if (!IsPhysics())
                World = m_AVTransform;
            m_pEditObject->RenderSkeletonSingle(World);
        }
    }

    inherited::Render();
}

void CActorTools::RenderEnvironment()
{
}

void CActorTools::OnFrame()
{
    if (!m_bReady)
        return;
    //.    if (m_KeyBar) m_KeyBar->UpdateBar();
    m_PreviewObject.Update();
    if (m_pEditObject)
    {
        // update matrix
        Fmatrix mTranslate, mRotate, mScale;
        mRotate.setHPB(m_pEditObject->a_vRotate.y, m_pEditObject->a_vRotate.x, m_pEditObject->a_vRotate.z);
        mTranslate.translate(m_pEditObject->a_vPosition);
        mScale.scale(m_pEditObject->t_vScale);
        m_AVTransform.mul(mTranslate, mRotate);
        m_AVTransform.mulB_43(mScale);

        if (!MainForm->GetLeftBarForm()->GetRenderMode() == UILeftBarForm::Render_Engine)
            m_pEditObject->OnFrame();

        if (!MainForm->GetKeyForm()->AutoChange())
        {
            m_pEditObject->m_SMParam.t_current = float(MainForm->GetKeyForm()->Position());
        }
        // CKinematicsAnimated* KA = dynamic_cast<CKinematicsAnimated*>(m_RenderObject.m_pVisual);
        if (m_RenderObject.IsRenderable() && m_pEditObject->IsSkeleton() && m_RenderObject.m_pVisual)
        {

            CKinematicsAnimated *KA = dynamic_cast<CKinematicsAnimated *>(m_RenderObject.m_pVisual);
            if (KA && !MainForm->GetKeyForm()->AutoChange() && m_RenderObject.m_pBlend)
            {
                float tm = float(MainForm->GetKeyForm()->Position());
                for (int k = 0; k < MAX_PARTS; k++)
                {
                    if (KA->LL_PartBlendsCount(k))
                    {
                        CBlend *B = KA->LL_PartBlend(k, 0);
                        B->timeCurrent = tm;
                    }
                }
            }

            IKinematics *K = m_RenderObject.m_pVisual->dcast_PKinematics();
            VERIFY(K);
            // K->Bone_Calculate			(&K->LL_GetData(K->LL_GetBoneRoot()),&Fidentity);
            if (!MainForm->GetKeyForm()->AutoChange())
                K->Bone_Calculate(&K->LL_GetData(K->LL_GetBoneRoot()), &Fidentity);
            else
                K->CalculateBones(TRUE);
        }
    }
    if (m_Flags.is(flRefreshShaders))
    {
        m_Flags.set(flRefreshShaders, FALSE);
        m_pEditObject->OnDeviceDestroy();
    }
    if (m_Flags.is(flMakeThumbnail))
    {
        m_Flags.set(flMakeThumbnail, FALSE);
        RealMakeThumbnail();
    }
    if (m_Flags.is(flGenerateLODHQ) || m_Flags.is(flGenerateLODLQ))
    {
        RealGenerateLOD(m_Flags.is(flGenerateLODHQ));
        m_Flags.set(flGenerateLODHQ, FALSE);
        m_Flags.set(flGenerateLODLQ, FALSE);
    }
    if (m_Flags.is(flRefreshSubProps))
    {
        m_Flags.set(flRefreshSubProps, FALSE);

        xr_vector<ListItem *> items;
        m_ObjectItems->GetSelected(0, items, false);
        OnObjectItemsFocused(items);
    }

    if (m_Flags.is(flRefreshProps))
        RealUpdateProperties();

    /*if (fraLeftBar->ebRenderEditorStyle->Down && !m_CurrentMotion.IsEmpty() && NULL == m_pEditObject->GetActiveSMotion())
    {
        AnsiString	tmp = m_CurrentMotion;
        m_CurrentMotion = "";

        SetCurrentMotion(tmp.c_str(), m_CurrentSlot);
    }*/
}

bool CActorTools::OnCreate()
{

    inherited::OnCreate();
    m_ObjectItems = xr_new<UIItemListForm>();
    m_ObjectItems->m_Flags.set(UIItemListForm::fMultiSelect, true);
    m_Props = xr_new<UIPropertiesForm>();
    m_ObjectItems->SetOnItemsFocusedEvent(TOnILItemsFocused(this, &CActorTools::OnObjectItemsFocused));
    m_PreviewObject.OnCreate();

    // key bar

    OnDeviceCreate();
    return true;
}

void CActorTools::OnDestroy()
{
    inherited::OnDestroy();

    xr_delete(m_Props);
    xr_delete(m_ObjectItems);
    m_PreviewObject.OnDestroy();

    m_PreviewObject.Clear();
    m_RenderObject.Clear();
    xr_delete(m_pEditObject);
}

bool CActorTools::IfModified()
{
    if (IsModified())
    {
        int mr = ELog.DlgMsg(mtConfirmation, "The '%s' has been modified.\nDo you want to save your changes?", GetEditFileName().c_str());
        switch (mr)
        {
        case mrYes:
            if (!ExecCommand(COMMAND_SAVE))
                return false;
            else
            {
                m_bObjectModified = false;
            }
            break;
        case mrNo:
            m_bObjectModified = false;
            break;
        case mrCancel:
            return false;
        }
    }
    return true;
}

void CActorTools::Modified()
{
    m_bObjectModified = true;
    ExecCommand(COMMAND_UPDATE_CAPTION);
}
void CActorTools::OnBoneModified(void)
{
    Modified();
    RefreshSubProperties();
    UndoSave();
}
void CActorTools::OnItemModified(void)
{
    switch (m_EditMode)
    {
    case emObject:
        OnObjectModified();
        break;
    case emMotion:
        OnMotionDefsModified();
        break;
    case emBone:
        OnBoneModified();
        break;
    case emSurface:
        OnObjectModified();
        break;
    case emMesh:
        break;
    }
}

LPCSTR CActorTools::GetInfo()
{
    return 0;
}

void CActorTools::ZoomObject(BOOL bSelOnly)
{
    VERIFY(m_bReady);
    if (m_pEditObject)
    {
        Fbox BB;
        switch (m_EditMode)
        {
        case emBone:
        {
            BoneVec lst;
            if (m_pEditObject->GetSelectedBones(lst))
            {
                BB.invalidate();
                for (BoneIt b_it = lst.begin(); b_it != lst.end(); b_it++)
                {
                    Fvector C = {0, 0, 0};
                    float r = 0.5f;
                    switch ((*b_it)->shape.type)
                    {
                    case SBoneShape::stBox:
                        r = _max(_max((*b_it)->shape.box.m_halfsize.x, (*b_it)->shape.box.m_halfsize.y), (*b_it)->shape.box.m_halfsize.z);
                        C = (*b_it)->shape.box.m_translate;
                        break;
                    case SBoneShape::stSphere:
                        r = (*b_it)->shape.sphere.R;
                        C = (*b_it)->shape.sphere.P;
                        break;
                    case SBoneShape::stCylinder:
                        r = _max((*b_it)->shape.cylinder.m_height, (*b_it)->shape.cylinder.m_radius);
                        C = (*b_it)->shape.cylinder.m_center;
                        break;
                    }
                    (*b_it)->_LTransform().transform_tiny(C);
                    m_AVTransform.transform_tiny(C);
                    Fbox bb;
                    bb.set(C, C);
                    bb.grow(r);
                    BB.merge(bb);
                }
            }
        }
        break;
        default:
            BB = m_pEditObject->GetBox();
        }
        EDevice.m_Camera.ZoomExtents(BB);
    }
}

bool CActorTools::Load(LPCSTR obj_name)
{
    xr_string full_name;
    full_name = obj_name;

    VERIFY(m_bReady);
    CEditableObject *O = xr_new<CEditableObject>(obj_name);
    if (FS.exist(full_name.c_str()) && O->Load(full_name.c_str()))
    {
        xr_delete(m_pEditObject);
        m_pEditObject = O;
        ///  m_pEditObject->Optimize ();
        // delete visual
        m_RenderObject.Clear();

        MainForm->GetLeftBarForm()->SetRenderMode(false);

        UpdateProperties();
        return true;
    }
    else
    {
        ELog.DlgMsg(mtError, "Can't load object file '%s'.", obj_name);
    }
    xr_delete(O);
    return false;
}

bool CActorTools::Save(LPCSTR obj_name, bool bInternal)
{
    xr_string full_name;
    full_name = obj_name;
    VERIFY(m_bReady);
    if (m_pEditObject)
    {
        EFS.MarkFile(full_name.c_str(), true);
        if (m_pEditObject->Save(full_name.c_str()))
        {
            if (!bInternal)
                m_bObjectModified = false;
            return true;
        }
    }
    return false;
}

void CActorTools::Reload()
{
    VERIFY(m_bReady);
    // visual part
}

void CActorTools::OnDeviceCreate()
{
    if (m_pEditObject)
    {
        m_pEditObject->OnDeviceCreate();
        MakePreview();
    }
}

void CActorTools::OnDeviceDestroy()
{
    if (m_pEditObject)
    {
        m_pEditObject->OnDeviceDestroy();
        m_RenderObject.DeleteVisual();
    }
}

void CActorTools::Clear()
{
    PhysicsStopSimulate();
    inherited::Clear();
    m_CurrentMotion = "";
    m_CurrentSlot = 0;
    // delete visuals
    if (m_pEditObject)
        m_pEditObject->DeletePhysicsShell();
    xr_delete(m_pEditObject);
    m_RenderObject.Clear();
    //	m_PreviewObject.Clear();
    m_ObjectItems->ClearList();
    m_Props->ClearProperties();

    m_bObjectModified = false;
    m_Flags.set(flUpdateGeometry | flUpdateMotionDefs | flUpdateMotionKeys | flReadOnlyMode, FALSE);
    m_EditMode = emObject;

    UI->RedrawScene();
}

void CActorTools::OnShowHint(AStringVec &SS)
{
}
extern xr_string MakeFullBoneName(CBone *bone);
bool CActorTools::MouseStart(TShiftState Shift)
{
    inherited::MouseStart(Shift);
    switch (m_Action)
    {
    case etaSelect:
        switch (MainForm->GetLeftBarForm()->GetPickMode())
        {
        case 2:
        {
            CBone *B = m_pEditObject->PickBone(UI->m_CurrentRStart, UI->m_CurrentRDir, m_AVTransform);
            bool bVal = B ? (Shift | ssAlt) ? false : ((Shift | ssCtrl) ? !B->Selected() : true) : false;
            if (B)
                SelectListItem(BONES_PREFIX, B ? MakeFullBoneName(B).c_str() : 0, bVal, (Shift | ssCtrl) || (Shift | ssAlt), true);
        }
        break;
        case 1:
        {
            SRayPickInfo pinf;
            float dis = UI->ZFar();
            Fmatrix iTransform;
            iTransform.invert(m_AVTransform);
            if (m_pEditObject->RayPick(dis, UI->m_CurrentRStart, UI->m_CurrentRDir, iTransform, &pinf))
            {
                CSurface *surf = pinf.e_mesh->GetSurfaceByFaceID(pinf.inf.id);
                xr_string s_name = xr_string("Surfaces\\") + xr_string(surf->_Name());
                m_ObjectItems->SelectItem(s_name.c_str());
            }
        }
        break;
        }

        break;
    case etaAdd:
        break;
    case etaMove:
        break;
    case etaRotate:
        break;
    }
    return m_bHiddenMode;
}

bool CActorTools::MouseEnd(TShiftState Shift)
{
    inherited::MouseEnd(Shift);
    switch (m_Action)
    {
    case etaSelect:
        break;
    case etaAdd:
        break;
    case etaMove:
    {
        switch (m_EditMode)
        {
        case emObject:
            if (Shift | ssCtrl)
                OnMotionKeysModified();
            break;

        case emBone:
            if (Shift | ssCtrl)
                OnBoneModified();

            if (Shift | ssAlt)
                OnBoneModified();
            break;
        }
    }
    break;
    case etaRotate:
    {
        switch (m_EditMode)
        {
        case emObject:
            if (Shift | ssCtrl)
                OnMotionKeysModified();
            break;

        case emBone:
            if (Shift | ssCtrl)
                OnBoneModified();

            if (Shift | ssAlt)
                OnBoneModified();
            break;
        }
    }
    break;
    case etaScale:
    {
        switch (m_EditMode)
        {
        case emBone:
            if (Shift | ssCtrl)
                OnBoneModified();
            break;
        }
    }
    break;
    }
    return true;
}

void CActorTools::MouseMove(TShiftState Shift)
{
    inherited::MouseMove(Shift);
    if (!m_pEditObject)
        return;

    switch (m_Action)
    {
    case etaSelect:
        break;
    case etaAdd:
        break;
    case etaMove:
    {
        switch (m_EditMode)
        {
        case emObject:
            if (true || Shift | ssCtrl)
                m_pEditObject->a_vPosition.add(m_MovedAmount);
            break;

        case emBone:
            BoneVec lst;
            if (m_pEditObject->GetSelectedBones(lst))
            {
                if (Shift | ssCtrl)
                {
                    for (BoneIt b_it = lst.begin(); b_it != lst.end(); ++b_it)
                        (*b_it)->ShapeMove(m_MovedAmount);
                }
                else if (Shift | ssAlt)
                {
                    for (BoneIt b_it = lst.begin(); b_it != lst.end(); ++b_it)
                        (*b_it)->BindMove(m_MovedAmount);

                    m_pEditObject->OnBindTransformChange();
                    RefreshSubProperties();
                }
                else
                {
                    for (BoneIt b_it = lst.begin(); b_it != lst.end(); ++b_it)
                        (*b_it)->BoneMove(m_MovedAmount);

                    RefreshSubProperties();
                }
            }
            break;
        }
    }
    break;
    case etaRotate:
    {
        switch (m_EditMode)
        {
        case emObject:
            if (Shift | ssCtrl)
                m_pEditObject->a_vRotate.mad(m_RotateVector, m_RotateAmount);
            break;

        case emBone:
        {
            BoneVec lst;
            Fvector rot;
            rot.mul(m_RotateVector, m_RotateAmount);
            if (m_pEditObject->GetSelectedBones(lst))
            {
                if (Shift | ssCtrl)
                {
                    for (BoneIt b_it = lst.begin(); b_it != lst.end(); ++b_it)
                        (*b_it)->ShapeRotate(rot);
                }
                else if (Shift | ssAlt)
                {
                    for (BoneIt b_it = lst.begin(); b_it != lst.end(); ++b_it)
                        (*b_it)->BindRotate(rot);

                    m_pEditObject->OnBindTransformChange();
                    RefreshSubProperties();
                }
                else
                {
                    for (BoneIt b_it = lst.begin(); b_it != lst.end(); ++b_it)
                        (*b_it)->BoneRotate(m_RotateVector, m_RotateAmount);

                    RefreshSubProperties();
                }
            }
        }
        break;
        }
    }
    break;
    case etaScale:
    {
        switch (m_EditMode)
        {
        case emBone:
            if (Shift | ssCtrl)
            {
                BoneVec lst;
                if (m_pEditObject->GetSelectedBones(lst))
                    for (BoneIt b_it = lst.begin(); b_it != lst.end(); ++b_it)
                        (*b_it)->ShapeScale(m_ScaleAmount);
            }
            break;
        }
    }
    break;
    }
}

bool CActorTools::Pick(TShiftState Shift)
{
    return false;
}

bool CActorTools::RayPick(const Fvector &start, const Fvector &dir, float &dist, Fvector *pt, Fvector *n)
{
    if (m_PreviewObject.m_pObject)
    {
        SRayPickInfo pinf;
        if (m_PreviewObject.m_pObject->RayPick(dist, start, dir, Fidentity, &pinf))
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

void CActorTools::GetStatTime(float &a, float &b, float &c)
{
    if (m_RenderObject.IsRenderable() && MainForm->GetLeftBarForm()->GetRenderMode() == UILeftBarForm::Render_Engine && m_RenderObject.m_pBlend)
    {
        a = 0;
        b = m_RenderObject.m_pBlend->timeTotal / m_RenderObject.m_pBlend->speed;
        c = m_RenderObject.m_pBlend->timeCurrent / m_RenderObject.m_pBlend->speed;
        if (c > b)
        {
            int cnt = iFloor(c / b);
            c -= (cnt * b);
        }
    }
    else
    {
        if (MainForm->GetLeftBarForm()->GetRenderMode() == UILeftBarForm::Render_Editor && m_pEditObject && m_pEditObject->GetActiveSMotion())
        {
            SAnimParams &P = m_pEditObject->m_SMParam;
            a = P.min_t;
            b = P.max_t;
            c = P.t_current;
        }
        else
        {
            a = 0;
            b = 0;
            c = 0;
        }
    }
}

bool CActorTools::IsEngineMode()
{

    return MainForm->GetLeftBarForm()->GetRenderMode() == UILeftBarForm::Render_Engine;
}

void CActorTools::SelectPreviewObject(bool bClear)
{
    if (bClear)
    {
        m_PreviewObject.Clear();
        return;
    }
    m_PreviewObject.SelectObject();
}

void CActorTools::SetPreviewObjectPrefs()
{
    m_PreviewObject.SetPreferences();
}

void CActorTools::OnObjectModified(void)
{
    m_Flags.set(flUpdateGeometry, TRUE);
    OnGeometryModified();
}

void CActorTools::ShowClipMaker()
{
}

bool CActorTools::Import(LPCSTR initial, LPCSTR obj_name)
{
    string_path full_name;
    if (initial)
        FS.update_path(full_name, initial, obj_name);
    else
        strcpy(full_name, obj_name);

    VERIFY(m_bReady);
    CEditableObject *O = xr_new<CEditableObject>(obj_name);
    if (O->Load(full_name))
    {
        O->m_objectFlags.set(CEditableObject::eoDynamic, TRUE);
        O->m_objectFlags.set(CEditableObject::eoProgressive, TRUE);
        xr_delete(m_pEditObject);
        m_pEditObject = O;
        // delete visual
        m_RenderObject.Clear();
        MainForm->GetLeftBarForm()->SetRenderMode(false);

        UpdateProperties();
        return true;
    }
    else
    {
        ELog.DlgMsg(mtError, "Can't load object file '%s'.", obj_name);
    }
    xr_delete(O);

    return false;
}

bool CActorTools::ExportOGF(LPCSTR name)
{
    VERIFY(m_bReady);
    if (m_pEditObject && m_pEditObject->ExportOGF(name, 4))
        return true;
    return false;
}

bool CActorTools::ExportOMF(LPCSTR name)
{
    VERIFY(m_bReady);
    if (m_pEditObject && m_pEditObject->ExportOMF(name))
        return true;
    return false;
}

bool CActorTools::ExportOBJ(LPCSTR name)
{
    VERIFY(m_bReady);
    if (m_pEditObject && m_pEditObject->ExportOBJ(name))
        return true;
    return false;
}

bool CActorTools::ExportCPP(LPCSTR name)
{
    if (m_pEditObject)
    {
        EditMeshVec &meshes = m_pEditObject->Meshes();
        string128 tmp;
        IWriter *W = FS.w_open(name);

        for (EditMeshIt m_it = meshes.begin(); m_it != meshes.end(); m_it++)
        {
            CEditableMesh *mesh = *m_it;
            const st_Face *faces = mesh->GetFaces();
            const Fvector *verts = mesh->GetVertices();
            sprintf(tmp, "MESH %s {", mesh->Name().c_str());
            W->w_string(tmp);
            sprintf(tmp, "\tVERTEX_COUNT %d", mesh->GetVCount());
            W->w_string(tmp);
            sprintf(tmp, "\tFACE_COUNT %d", mesh->GetFCount());
            W->w_string(tmp);
            W->w_string("\tconst Fvector vertices[VERTEX_COUNT] = {");
            for (u32 v_id = 0; v_id < mesh->GetVCount(); v_id++)
            {
                sprintf(tmp, "\t\t{% 3.6f,\t% 3.6f,\t% 3.6f},", VPUSH(verts[v_id]));
                W->w_string(tmp);
            }
            W->w_string("\t}");
            W->w_string("\tconst u16 faces[FACE_COUNT*3] = {");
            for (u32 f_id = 0; f_id < mesh->GetFCount(); f_id++)
            {
                sprintf(tmp, "\t\t%-d,\t\t%-d,\t\t%-d,", faces[f_id].pv[0].pindex, faces[f_id].pv[1].pindex, faces[f_id].pv[2].pindex);
                W->w_string(tmp);
            }
            W->w_string("\t}");
            W->w_string("}");
        }
        FS.w_close(W);
        return true;
    }
    return false;
}

#include "../xrECore/Editor/EDetailModel.h"
bool CActorTools::ExportDM(LPCSTR name)
{
    VERIFY(m_bReady);
    if (m_pEditObject)
    {
        EDetail DM;
        if (!DM.Update(m_pEditObject->GetName()))
            return false;
        DM.Export(name);
        return true;
    }
    return false;
}

void CActorTools::WorldMotionRotate(const Fvector &R)
{
    R_ASSERT(m_pEditObject && (!m_CurrentMotion.empty()));
    CSMotion *M = m_pEditObject->FindSMotionByName(m_CurrentMotion.c_str());
    int rootId = m_pEditObject->GetRootBoneID();
    M->WorldRotate(rootId, R.y, R.x, R.z);
    OnMotionKeysModified();
}

void CActorTools::SetCurrentMotion(LPCSTR name, u16 slot)
{
    if (m_pEditObject)
    {
        if ((m_CurrentMotion != name) || (m_CurrentSlot != slot))
        {
            m_CurrentMotion = name;
            m_CurrentSlot = slot;
            CSMotion *M = m_pEditObject->FindSMotionByName(name);
            if (M)
            {
                m_pEditObject->SetActiveSMotion(M);
            }
            PlayMotion();
        }
    }
}

CSMotion *CActorTools::GetCurrentMotion()
{
    return m_pEditObject ? m_pEditObject->FindSMotionByName(m_CurrentMotion.c_str()) : 0;
}

CSMotion *CActorTools::FindMotion(LPCSTR name)
{
    return m_pEditObject ? m_pEditObject->FindSMotionByName(name) : 0;
}

LPCSTR CActorTools::ExtractMotionName(LPCSTR full_name, LPCSTR prefix)
{
    LPCSTR _templ = "\\Slot ";
    if (0 == strstr(full_name, _templ))
        return full_name + xr_strlen(prefix) + 1;
    else
    {
        LPCSTR mot_nm = strstr(full_name, _templ);
        mot_nm += xr_strlen(_templ);
        while (isdigit(*mot_nm))
        {
            ++mot_nm;
        }
        return mot_nm + 1;
    }
    //    	return full_name + xr_strlen(prefix) + 1 + xr_strlen(_templ) + 1;
}

u16 CActorTools::ExtractMotionSlot(LPCSTR full_name, LPCSTR prefix)
{
    LPCSTR _templ = "\\Slot ";
    u16 slot = 0;
    LPCSTR slot_nm = strstr(full_name, _templ);
    if (0 != slot_nm)
    {
        string16 tmp;
        slot_nm += xr_strlen(_templ);
        u32 ii = 0;
        while (isdigit(*slot_nm))
        {
            tmp[ii] = slot_nm[0];
            ++slot_nm;
            ++ii;
        }
        tmp[ii] = 0;
        slot = u16(atoi(tmp) - 1);
    }
    return slot;
}

xr_string CActorTools::BuildMotionPref(u16 slot, LPCSTR prefix)
{
    VERIFY(slot < MAX_ANIM_SLOT);
    string32 slot_nm;
    sprintf(slot_nm, "Slot %1d", slot + 1);
    return PrepareKey(prefix, slot_nm).c_str();
}

void CActorTools::OptimizeMotions()
{
    if (m_pEditObject)
    {
        m_pEditObject->OptimizeSMotions();
        Modified();
        UndoSave();
    }
}

void CActorTools::RealMakeThumbnail()
{
    if (CurrentObject())
    {
        CEditableObject *obj = CurrentObject();
        xr_string tex_name, obj_name;
        tex_name = ChangeFileExt(obj->GetName(), ".thm");
        obj_name = ChangeFileExt(obj->GetName(), ".object");
        FS_File F;
        string_path fname;
        //.     FS.update_path(fname,_objects_,obj_name.c_str());
        //.     R_ASSERT	(FS.file_find(fname,F));
        R_ASSERT(FS.file_find(obj_name.c_str(), F));

        if (ImageLib.CreateOBJThumbnail(tex_name.c_str(), obj, F.time_write))
        {
            ELog.Msg(mtInformation, "Thumbnail successfully created.");
        }
        else
        {
            ELog.Msg(mtError, "Making thumbnail failed.");
        }
    }
    else
    {
        ELog.DlgMsg(mtError, "Can't create thumbnail. Empty scene.");
    }
}

void CActorTools::RealGenerateLOD(bool hq)
{
    if (m_pEditObject)
    {
        bool engine_render = MainForm->GetLeftBarForm()->GetRenderMode() == UILeftBarForm::Render_Engine;
        MainForm->GetLeftBarForm()->SetRenderMode(false);
        CEditableObject *O = m_pEditObject;

        if (O && O->IsMUStatic())
        {
            BOOL bLod = O->m_objectFlags.is(CEditableObject::eoUsingLOD);
            O->m_objectFlags.set(CEditableObject::eoUsingLOD, FALSE);
            xr_string tex_name;
            tex_name = EFS.ChangeFileExt(O->GetName(), "");

            string_path fname;
            FS.update_path(fname, _objects_, "");

            if (tex_name.find(fname) == 0)
            {
                tex_name.erase(0, xr_strlen(fname));
            }

            string_path tmp;
            strcpy(tmp, tex_name.c_str());
            _ChangeSymbol(tmp, '\\', '_');
            _ChangeSymbol(tmp, ':', '_');
            tex_name = xr_string("lod_") + tmp;
            tex_name = ImageLib.UpdateFileName(tex_name);
            ImageLib.CreateLODTexture(O, tex_name.c_str(), LOD_IMAGE_SIZE, LOD_IMAGE_SIZE, LOD_SAMPLE_COUNT, O->Version(), hq ? 4 /*7*/ : 1);
            O->OnDeviceDestroy();
            O->m_objectFlags.set(CEditableObject::eoUsingLOD, bLod);
            ELog.Msg(mtInformation, "LOD for object '%s' successfully created.", O->GetName());
        }
        else
        {
            ELog.Msg(mtError, "Can't create LOD texture from non 'Multiple Usage' object.", O->GetName());
        }
        MainForm->GetLeftBarForm()->SetRenderMode(engine_render);
    }
}

bool CActorTools::BatchConvert(LPCSTR fn)
{
    bool bRes = true;
    CInifile *ini = CInifile::Create(fn);
    VERIFY(ini);
    if (ini->section_exist("ogf"))
    {
        CInifile::Sect &sect = ini->r_section("ogf");
        Msg("Start converting %d items...", sect.Data.size());
        for (auto it = sect.Data.begin(); it != sect.Data.end(); it++)
        {
            string_path src_name;
            string_path tgt_name;
            FS.update_path(src_name, _objects_, it->first.c_str());
            FS.update_path(tgt_name, _game_meshes_, it->second.c_str());
            strcpy(src_name, EFS.ChangeFileExt(src_name, ".object").c_str());
            strcpy(tgt_name, EFS.ChangeFileExt(tgt_name, ".ogf").c_str());
            if (FS.exist(src_name))
            {
                Msg(".Converting '%s' <-> '%s'", it->first.c_str(), it->second.c_str());
                CEditableObject *O = xr_new<CEditableObject>("convert");
                BOOL res = O->Load(src_name);
                if (res)
                    res = O->ExportOGF(tgt_name, 4);
                Log(res ? ".OK" : "!.FAILED");
                xr_delete(O);
            }
            else
            {
                Log("!Invalid source file name:", it->first.c_str());
                bRes = false;
            }
            if (UI->NeedAbort())
                break;
        }
    }
    if (ini->section_exist("omf"))
    {
        CInifile::Sect &sect = ini->r_section("omf");
        Msg("Start converting %d items...", sect.Data.size());
        for (auto it = sect.Data.begin(); it != sect.Data.end(); ++it)
        {
            string_path src_name;
            string_path tgt_name;
            FS.update_path(src_name, _objects_, it->first.c_str());
            FS.update_path(tgt_name, _game_meshes_, it->second.c_str());
            strcpy(src_name, EFS.ChangeFileExt(src_name, ".object").c_str());
            strcpy(tgt_name, EFS.ChangeFileExt(tgt_name, ".omf").c_str());
            if (FS.exist(src_name))
            {
                Msg(".Converting '%s' <-> '%s'", it->first.c_str(), it->second.c_str());
                CEditableObject *O = xr_new<CEditableObject>("convert");
                BOOL res = O->Load(src_name);
                if (res)
                    res = O->ExportOMF(tgt_name);
                Log(res ? ".OK" : "!.FAILED");
                xr_delete(O);
            }
            else
            {
                Log("!Invalid source file name:", it->first.c_str());
                bRes = false;
            }
            if (UI->NeedAbort())
                break;
        }
    }
    return bRes;
}

void CActorTools::PhysicsSimulate()
{
    if (!m_pEditObject)
        return;
    m_IsPhysics = true;
    CreatePhysicsWorld();
    if (MainForm->GetLeftBarForm()->GetRenderMode() != UILeftBarForm::Render_Editor)
        m_RenderObject.CreatePhysicsShell(&m_AVTransform);
    else
        m_pEditObject->CreatePhysicsShell(&m_AVTransform);
}

void CActorTools::PhysicsStopSimulate()
{
    if (m_IsPhysics)
    {
        m_IsPhysics = false;
        m_RenderObject.DeletePhysicsShell();
        m_pEditObject->DeletePhysicsShell();
        DestroyPhysicsWorld();
    }
}

CObjectSpace *os = 0;
void CActorTools::CreatePhysicsWorld()
{
    VERIFY(!os);
    VERIFY(!physics_world());
    set_mtl_lib(&GMLib);
    os = create_object_space();
    CRenderDeviceBase *rd = &EDevice;
    create_physics_world(false, os, 0, rd);
}

void CActorTools::DestroyPhysicsWorld()
{
    if (physics_world())
        destroy_physics_world();
    destroy_object_space(os);
}

bool CActorTools::GetSelectionPosition(Fmatrix &result)
{
    result = m_AVTransform;
    return true;
}

void PreviewModel::OnCreate()
{
}

void PreviewModel::OnDestroy()
{
}

void PreviewModel::Clear()
{
}

void PreviewModel::SelectObject()
{
}

void PreviewModel::SetPreferences()
{
}

void PreviewModel::Render()
{
}

void PreviewModel::Update()
{
}

void CActorTools::PrepareLighting()
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

    L.diffuse.set(0.2, 0.2, 0.2, 1);
    L.direction.set(-1, -1, -1);
    L.direction.normalize();
    EDevice.SetLight(1, L);
    EDevice.LightEnable(1, true);

    L.diffuse.set(0.2, 0.2, 0.2, 1);
    L.direction.set(1, -1, -1);
    L.direction.normalize();
    EDevice.SetLight(2, L);
    EDevice.LightEnable(2, true);

    L.diffuse.set(0.2, 0.2, 0.2, 1);
    L.direction.set(-1, -1, 1);
    L.direction.normalize();
    EDevice.SetLight(3, L);
    EDevice.LightEnable(3, true);

    L.diffuse.set(1.0, 0.4, 0.3, 1);
    L.direction.set(0, 1, 0);
    L.direction.normalize();
    EDevice.SetLight(4, L);
    EDevice.LightEnable(4, true);

    L.diffuse.set(0.3, 1.0, 0.4, 1);
    L.direction.set(-1, -1, -1);
    L.direction.normalize();
    EDevice.SetLight(5, L);
    EDevice.LightEnable(5, true);
}
