#include "stdafx.h"

// static part
void CCustomObject::SnapMove(Fvector &pos, Fvector &rot, const Fmatrix &rotRP, const Fvector &amount)
{
    // !!! Hide object before test
    SRayPickInfo pinf;
    Fvector up, dn = {0, -1, 0};
    rotRP.transform_dir(dn);
    up.invert(dn);
    Fvector s2, s1 = pos;
    s1.add(amount);
    s2.mad(s1, up, EPrefs->snap_moveto);

    pinf.inf.range = EPrefs->snap_moveto;
    if (Scene->RayPickObject(pinf.inf.range, s1, dn, OBJCLASS_SCENEOBJECT, &pinf, Scene->GetSnapList(false)) || Scene->RayPickObject(pinf.inf.range, s2, dn, OBJCLASS_SCENEOBJECT, &pinf, Scene->GetSnapList(false)))
    {
        pos.set(pinf.pt);
        if (Tools->GetSettings(etfNormalAlign))
        {
            Fvector verts[3];
            pinf.e_obj->GetFaceWorld(pinf.s_obj->_Transform(), pinf.e_mesh, pinf.inf.id, verts);
            Fvector vR, vD, vN;
            vN.mknormal(verts[0], verts[1], verts[2]);

            vD.set(rotRP.k);
            vR.crossproduct(vN, vD);
            vR.normalize();
            vD.crossproduct(vR, vN);
            vD.normalize();

            Fmatrix M;
            M.set(vR, vN, vD, vR);
            M.getXYZ(rot);
        }
    }
    else
        pos.add(amount);
}
void CCustomObject::NormalAlign(Fvector &rot, const Fvector &up, const Fvector &dir)
{
    Fmatrix mR;
    Fvector vR, vD, vN;
    vN.set(up);
    //   vD.set(0,0,1);
    vD.set(dir);
    if (fabsf(vN.z) > 0.99f)
        vD.set(1, 0, 0);
    vR.crossproduct(vN, vD);
    vR.normalize();
    vD.crossproduct(vR, vN);
    vD.normalize();
    mR.set(vR, vN, vD, vR);
    mR.getXYZ(rot);
}

//------------------------------------------------------------------------------

void CCustomObject::OnDetach()
{
    if (m_pOwnerObject)
    {
        m_pOwnerObject = 0;
        //.        string64 			new_name;
        //.        Scene->GenObjectName(ClassID,new_name,Name);
        //.        Name 				= new_name;
        //.        Scene->AppendObject	(this,false);
    }
    m_CO_Flags.set(flObjectInGroup, FALSE);
    Select(true);
    FParentTools->SetChanged(TRUE);
}

void CCustomObject::OnAttach(CCustomObject *owner)
{
    R_ASSERT(owner);
    R_ASSERT2(((!m_pOwnerObject) || (m_pOwnerObject == owner)), "Object already has owner!");
    m_pOwnerObject = owner;
    //.    Scene->RemoveObject		(this,false,false);
    if (owner->FClassID == OBJCLASS_GROUP)
        m_CO_Flags.set(flObjectInGroup, TRUE);

    Select(false);
    FParentTools->SetChanged(TRUE);
}

void CCustomObject::Move(Fvector &amount)
{
    UI->UpdateScene();
    Fvector v = GetPosition();
    Fvector r = FRotation;
    if (Tools->GetSettings(etfMTSnap))
    {
        BOOL bVis = Visible();
        BOOL bSel = Selected();
        Show(FALSE);
        Select(FALSE);
        SnapMove(v, r, FTransformRP, amount);
        Show(bVis);
        Select(bSel);
    }
    else
    {
        v.add(amount);
    }
    SetPosition(v);
    FRotation = r;
}

void CCustomObject::MoveTo(const Fvector &pos, const Fvector &up)
{
    UI->UpdateScene();
    Fvector v = GetPosition();
    v.set(pos);
    if (Tools->GetSettings(etfNormalAlign))
    {
        Fmatrix M;
        M.setXYZ(FRotation);
        Fvector ret_rot = FRotation;
        NormalAlign(ret_rot, up, M.k);
        FRotation = ret_rot;
    }
    SetPosition(v);
}

void CCustomObject::RotatePivot(const Fmatrix &prev_inv, const Fmatrix &current)
{
    Fmatrix Ol, On;
    Ol.mul(prev_inv, FTransformRP);
    On.mul(current, Ol);
    Fvector xyz;
    On.getXYZ(xyz);
    FRotation = xyz;
    SetPosition(On.c);
}

void CCustomObject::RotateParent(Fvector &axis, float angle)
{
    UI->UpdateScene();
    Fvector r = FRotation;
    r.mad(axis, angle);
    FRotation = r;
}

void CCustomObject::RotateLocal(Fvector &axis, float angle)
{
    Fmatrix m;
    Fvector r;
    m.rotation(axis, angle);
    FTransformRP.mulB_43(m);
    FTransformRP.getXYZ(r);
    SetRotation(r);
}

void CCustomObject::ScalePivot(const Fmatrix &prev_inv, const Fmatrix &current, Fvector &amount)
{
    UI->UpdateScene();
    Fvector p = GetPosition();
    Fvector s = GetScale();
    ;
    s.add(amount);
    if (s.x < EPS)
        s.x = EPS;
    if (s.y < EPS)
        s.y = EPS;
    if (s.z < EPS)
        s.z = EPS;
    SetScale(s);

    // translate position
    prev_inv.transform_tiny(p);
    current.transform_tiny(p);

    SetPosition(p);
}

void CCustomObject::Scale(Fvector &amount)
{
    UI->UpdateScene();
    Fvector s = GetScale();
    s.add(amount);
    if (s.x < EPS)
        s.x = EPS;
    if (s.y < EPS)
        s.y = EPS;
    if (s.z < EPS)
        s.z = EPS;
    SetScale(s);
}

bool CCustomObject::OnObjectNameAfterEdit(PropValue *sender, shared_str &edit_val)
{
    RTextValue *V = dynamic_cast<RTextValue *>(sender);
    VERIFY(V);
    if (0 == edit_val.size())
        return false;
    xr_string temp = edit_val.c_str();
    xr_strlwr(temp);

    edit_val = temp.c_str();
    return !Scene->FindObjectByName(edit_val.c_str(), (CCustomObject *)0);
}

void CCustomObject::OnNumChangePosition(PropValue *sender)
{
    NumSetPosition(EPosition);
}
void CCustomObject::OnNumChangeRotation(PropValue *sender)
{
    NumSetRotation(ERotation);
}
void CCustomObject::OnNumChangeScale(PropValue *sender)
{
    NumSetScale(EScale);
}
void CCustomObject::OnNameChange(PropValue *sender)
{
    SetName(EName.c_str());
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}

void CCustomObject::FillProp(LPCSTR pref, PropItemVec &items)
{
    PropValue *V;
    EName = GetName();
    V = PHelper().CreateNameCB(items, PrepareKey(pref, "Name"), &EName, NULL, NULL, RTextValue::TOnAfterEditEvent(this, &CCustomObject::OnObjectNameAfterEdit));
    V->OnChangeEvent.bind(this, &CCustomObject::OnNameChange);
    EPosition = GetPosition();
    V = PHelper().CreateVector(items, PrepareKey(pref, "Transform\\Position"), &EPosition, -10000, 10000, 0.01, 2);
    V->OnChangeEvent.bind(this, &CCustomObject::OnNumChangePosition);
    ERotation = GetRotation();
    V = PHelper().CreateAngle3(items, PrepareKey(pref, "Transform\\Rotation"), &ERotation, -10000, 10000, 0.1, 1);
    V->OnChangeEvent.bind(this, &CCustomObject::OnNumChangeRotation);
    EScale = GetScale();
    V = PHelper().CreateVector(items, PrepareKey(pref, "Transform\\Scale"), &EScale, 0.01, 10000, 0.01, 2);
    V->OnChangeEvent.bind(this, &CCustomObject::OnNumChangeScale);

    if (m_CO_Flags.test(flObjectInGroup))
    {
        V = PHelper().CreateFlag32(items, PrepareKey(pref, "In group editable"), &m_CO_Flags, flObjectInGroupUnique);
        V->OnChangeEvent.bind(this, &CCustomObject::OnChangeIngroupUnique);
    }
}

void CCustomObject::OnChangeIngroupUnique(PropValue *sender)
{
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
}

//----------------------------------------------------

void CCustomObject::OnShowHint(AStringVec &dest)
{
    //	dest.push_back(xr_string("Class: ")+xr_string(ParentTools->ClassDesc()));
    dest.push_back(xr_string("Name:  ") + xr_string(GetName()));
    dest.push_back(xr_string("-------"));
}
//----------------------------------------------------
