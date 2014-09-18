#include "stdafx.h"
#pragma hdrstop

#include "GroupObject.h"
#include "../../ecore/editor/D3DUtils.h"

static const float EMPTY_GROUP_SIZE = 0.5f;

bool CGroupObject::GetBox(Fbox& bb) const
{
    bb.invalidate		();
    // update box
    for (ObjectsInGroup::const_iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
        switch(it->pObject->ClassID)
        {
        case OBJCLASS_SPAWNPOINT:
        case OBJCLASS_SCENEOBJECT:
        case OBJCLASS_WAY:
        {
            Fbox 	box;
            if (it->pObject->GetBox(box))
                bb.merge(box);
        }break;
        default:
            bb.modify(it->pObject->PPosition);
        }
    }
    if (!bb.is_valid())
    {
    	bb.set			(PPosition,PPosition);
        bb.grow			(EMPTY_GROUP_SIZE);
    }
    return bb.is_valid();
}

void CGroupObject::OnUpdateTransform()
{
	inherited::OnUpdateTransform();
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    	it->pObject->OnUpdateTransform();
}

void CGroupObject::UpdatePivot(LPCSTR nm, bool center)
{
    // first init
    VERIFY(m_ObjectsInGroup.size());
    ObjectsInGroup::iterator it;
    if (false==center)
    {
    	CCustomObject* object = 0;
    	if (nm&&nm[0])
        {
            for (it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
            {
            	if (0==strcmp(nm,it->pObject->Name))
                {
                	object = it->pObject;
                	break;
                }
            }
        }else
        {
        	bool bValidPivot = false;
            for (it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
            {
                if (it->pObject->ClassID==OBJCLASS_SCENEOBJECT)
                {
                	object		= it->pObject;
                    bValidPivot = true;
                    break;
                }
            }
            if (!bValidPivot)
            	object			= m_ObjectsInGroup.front().pObject;
        }
        if (object)
        {
            PPosition = object->PPosition;
            PRotation = object->PRotation;
			UpdateTransform(true);
        }
    }else
    {
        // center alignment
        it=m_ObjectsInGroup.begin();
        Fvector C; 
        C.set(it->pObject->PPosition); 
        it++;

        for (; it!=m_ObjectsInGroup.end(); ++it)
            C.add(it->pObject->PPosition);
            
        FPosition.div(C,m_ObjectsInGroup.size());
        FRotation.set(0,0,0);
		UpdateTransform(true);
    }
}

void CGroupObject::MoveTo(const Fvector& pos, const Fvector& up)
{
	Fvector old_r=FRotation;
	inherited::MoveTo(pos,up);
    Fmatrix prev; prev.invert(FTransform);
    UpdateTransform(true);

    Fvector dr; dr.sub(FRotation,old_r);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	Fvector r=it->pObject->PRotation; 
        r.add(dr); 
        it->pObject->PRotation=r;
    	Fvector v=it->pObject->PPosition;
        prev.transform_tiny(v);
        FTransform.transform_tiny(v);
    	it->pObject->PPosition=v;
    }
}

void CGroupObject::NumSetPosition(const Fvector& pos)
{
	inherited::NumSetPosition(pos);
    Fmatrix prev; prev.invert(FTransform);
    UpdateTransform(true);

	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	Fvector v=it->pObject->PPosition;
        prev.transform_tiny(v);
        FTransform.transform_tiny(v);
    	it->pObject->PPosition=v;
    }
}
void CGroupObject::NumSetRotation(const Fvector& rot)
{
	Fvector old_r;
    FTransformR.getXYZ(old_r);
	inherited::NumSetRotation(rot);
    Fmatrix prev; prev.invert(FTransform);
    UpdateTransform(true);

    Fvector dr; dr.sub(FRotation,old_r);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	Fvector r=it->pObject->PRotation; 
        r.add(dr); 
        it->pObject->PRotation=r;
    	Fvector v=it->pObject->PPosition;
        prev.transform_tiny(v);
        FTransform.transform_tiny(v);
    	it->pObject->PPosition=v;
    }
}
void CGroupObject::NumSetScale(const Fvector& scale)
{
	Fvector old_s = PScale;
	inherited::NumSetScale(scale);
    Fmatrix prev; 
    prev.invert(FTransform);
    UpdateTransform(true);

    Fvector ds; ds.sub(FScale,old_s);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	Fvector s=it->pObject->PScale; 
        s.add(ds); 
        it->pObject->PScale=s;
    	Fvector v=it->pObject->PPosition;
        prev.transform_tiny(v);
        FTransform.transform_tiny(v);
    	it->pObject->PPosition=v;
    }
}

void CGroupObject::Move(Fvector& amount)
{
	Fvector old_r=FRotation;
	inherited::Move(amount);
    Fmatrix prev; prev.invert(FTransform);
    UpdateTransform(true);

    Fvector dr; dr.sub(FRotation,old_r);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	Fvector r=it->pObject->PRotation; 
        r.add(dr); 
        it->pObject->PRotation=r;
    	Fvector v=it->pObject->PPosition;
        prev.transform_tiny(v);
        FTransform.transform_tiny(v);
    	it->pObject->PPosition=v;
    }
}
void CGroupObject::RotateParent(Fvector& axis, float angle )
{
	inherited::RotateParent(axis,angle);
    Fmatrix  Ginv;
    Ginv.set		(FITransformRP);
	UpdateTransform	(true);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	Fmatrix 	O,On;
        O.mul		(Ginv,it->pObject->FTransformRP);
        On.mul		(FTransform,O);
        Fvector 	xyz;
        On.getXYZ	(xyz);
        it->pObject->NumSetRotation(xyz);
        it->pObject->NumSetPosition(On.c);
    }
}

void CGroupObject::RotateLocal(Fvector& axis, float angle )
{
	inherited::RotateLocal(axis,angle);
    Fmatrix  			Ginv;
    Ginv.set			(FITransformRP);
	UpdateTransform		(true);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	Fmatrix 	O,On;
        O.mul		(Ginv,it->pObject->FTransformRP);
        On.mul		(FTransform,O);
        Fvector 	xyz;
        On.getXYZ	(xyz);
        it->pObject->NumSetRotation(xyz);
        it->pObject->NumSetPosition(On.c);
    }
}

void CGroupObject::Scale(Fvector& amount )
{
	inherited::Scale(amount);
    Fmatrix  m_old;
    m_old.invert(FTransform);
	UpdateTransform(true);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
		it->pObject->ScalePivot(m_old,FTransform,amount);
}

void CGroupObject::Render(int priority, bool strictB2F)
{
	inherited::Render(priority, strictB2F);
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	if (it->pObject->IsRender())
        {
	    	switch (it->pObject->ClassID)
            {
    	    case OBJCLASS_SCENEOBJECT: it->pObject->Render(priority,strictB2F); break;
            default:
                EDevice.SetShader(strictB2F?EDevice.m_SelectionShader:EDevice.m_WireShader);
                RCache.set_xform_world(Fidentity);
                it->pObject->Render(priority,strictB2F);
        	}
    	}
    }
	if ((1==priority) && (false==strictB2F))
    {
    	Fbox bb;
    	if (Selected()&&GetBox(bb))
        {
            EDevice.SetShader(EDevice.m_WireShader);
            RCache.set_xform_world(Fidentity);
            u32 clr = 0xFF7070FF;
            DU_impl.DrawSelectionBoxB(bb,&clr);
        }
    }
}

bool CGroupObject::FrustumPick(const CFrustum& frustum)
{
    if (m_ObjectsInGroup.empty())
    {
        Fbox 		bb;
        GetBox		(bb);
        u32 mask	= u32(-1); 
        return 		(frustum.testAABB(bb.data(),mask));
    }else{
        for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
            if (it->pObject->FrustumPick(frustum)) 
            return true;
    }
    return false;
}

bool CGroupObject::RayPick(float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf)
{
	bool bPick = false;

    for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
        if (it->pObject->RayPick(distance,start,direction,pinf)) 
            bPick=true;

    return bPick;
}

void CGroupObject::OnDeviceCreate()
{
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    	it->pObject->OnDeviceCreate();
}

void CGroupObject::OnDeviceDestroy()
{
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    	it->pObject->OnDeviceDestroy();
}
