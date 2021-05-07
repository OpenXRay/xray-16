#include "stdafx.h"
#pragma hdrstop

#include "xrCore/Animation/Bone.hpp"
#include "xrCore/Animation/Envelope.hpp"

void SJointIKData::clamp_by_limits(Fvector& dest_xyz)
{
    switch (type)
    {
    case jtRigid: dest_xyz.set(0.f, 0.f, 0.f); break;
    case jtJoint:
        clamp(dest_xyz.x, limits[0].limit.x, limits[0].limit.y);
        clamp(dest_xyz.y, limits[1].limit.x, limits[1].limit.y);
        clamp(dest_xyz.z, limits[2].limit.x, limits[2].limit.y);
        break;
    case jtWheel:
        clamp(dest_xyz.x, limits[0].limit.x, limits[0].limit.y);
        dest_xyz.y = 0;
        break;
    case jtSlider:
        dest_xyz.x = 0.f;
        dest_xyz.y = 0.f;
        clamp(dest_xyz.z, limits[1].limit.x, limits[1].limit.y);
        break;
        /*
            case jtWheelXZ:
                clamp(dest_xyz.x,limits[0].limit.x,limits[0].limit.y);		dest_xyz.y=0;
            break;
            case jtWheelXY:
                clamp(dest_xyz.x,limits[0].limit.x,limits[0].limit.y);		dest_xyz.z=0;
            break;
            case jtWheelYX:
                clamp(dest_xyz.y,limits[1].limit.x,limits[1].limit.y);		dest_xyz.z=0;
            break;
            case jtWheelYZ:
                clamp(dest_xyz.y,limits[1].limit.x,limits[1].limit.y);		dest_xyz.x=0;
            break;
            case jtWheelZX:
                clamp(dest_xyz.z,limits[2].limit.x,limits[2].limit.y);		dest_xyz.y=0;
            break;
            case jtWheelZY:
                clamp(dest_xyz.z,limits[2].limit.x,limits[2].limit.y);		dest_xyz.x=0;
            break;
        */
    }
}

void CBone::ShapeScale(const Fvector& _amount, bool /*parentCS = false*/)
{
    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        Fvector amount = _amount;
        //		Fmatrix _IT;_IT.invert(_LTransform());
        //		_IT.transform_dir(amount,_amount);
        //		if (parentCS) _IT.transform_dir(amount);
        shape.box.m_halfsize.add(amount);
        if (shape.box.m_halfsize.x < EPS)
            shape.box.m_halfsize.x = EPS;
        if (shape.box.m_halfsize.y < EPS)
            shape.box.m_halfsize.y = EPS;
        if (shape.box.m_halfsize.z < EPS)
            shape.box.m_halfsize.z = EPS;
    }
    break;
    case SBoneShape::stSphere:
        shape.sphere.R += _amount.x;
        if (shape.sphere.R < EPS)
            shape.sphere.R = EPS;
        break;
    case SBoneShape::stCylinder:
        shape.cylinder.m_height += _amount.z;
        if (shape.cylinder.m_height < EPS)
            shape.cylinder.m_height = EPS;
        shape.cylinder.m_radius += _amount.x;
        if (shape.cylinder.m_radius < EPS)
            shape.cylinder.m_radius = EPS;
        break;
    }
}

void CBone::ShapeRotate(const Fvector& _amount, bool parentCS /*= false*/)
{
    Fvector amount = _amount;
    Fmatrix IT;
    IT.invert(LTransform());
    if (parentCS)
        IT.transform_dir(amount);
    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        Fmatrix R;
        R.setXYZi(amount.x, amount.y, amount.z);
        shape.box.transform(shape.box, R);
    }
    break;
    case SBoneShape::stSphere: break;
    case SBoneShape::stCylinder:
    {
        Fmatrix R;
        R.setXYZi(amount.x, amount.y, amount.z);
        R.transform_dir(shape.cylinder.m_direction);
    }
    break;
    }
}

void CBone::ShapeMove(const Fvector& _amount, bool parentCS /*= false*/)
{
    Fvector amount = _amount;
    Fmatrix IT;
    IT.invert(LTransform());
    if (parentCS)
        IT.transform_dir(amount);
    switch (shape.type)
    {
    case SBoneShape::stBox: shape.box.m_translate.add(amount); break;
    case SBoneShape::stSphere: shape.sphere.P.add(amount); break;
    case SBoneShape::stCylinder: shape.cylinder.m_center.add(amount); break;
    }
}

void CBone::BindRotate(const Fvector& _amount) { rest_rotate.add(_amount); }
void CBone::BindMove(const Fvector& _amount) { rest_offset.add(_amount); }
bool CBone::Pick(float& dist, const Fvector& S, const Fvector& D, const Fmatrix& parent)
{
    Fvector start, dir;
    Fmatrix M;
    M.mul_43(parent, LTransform());
    M.invert();
    M.transform_tiny(start, S);
    M.transform_dir(dir, D);
    switch (shape.type)
    {
    case SBoneShape::stBox: return shape.box.intersect(start, dir, dist);
    case SBoneShape::stSphere: return shape.sphere.intersect(start, dir, dist);
    case SBoneShape::stCylinder: return shape.cylinder.intersect(start, dir, dist);
    default:
        Fsphere sphere;
        sphere.P.set(0, 0, 0);
        sphere.R = 0.025f;
        return sphere.intersect(start, dir, dist);
    }
}

void CBone::BoneRotate(const Fvector& _axis, float angle, bool parentCS /*= false*/)
{
    if (!fis_zero(angle))
    {
        if (parentCS)
        {
            // bind pose CS
            mot_rotate.x += _axis.x * angle;
            mot_rotate.y += _axis.y * angle;
            mot_rotate.z += _axis.z * angle;

            ClampByLimits();
            /*
                        Fmatrix mBind,mBindI,mLocal,mRotate,mLocalBP;
                        mBind.setXYZi		(rest_rotate);
                        mBindI.invert		(mBind);
                        mLocal.setXYZi		(mot_rotate);
                        Fvector axis;
                        mBind.transform		(axis,_axis);
                        mRotate.rotation	(axis,angle);
                        mLocal.mulA			(mRotate);

                        mLocalBP.mul		(mBindI,mLocal);
                        Fvector mot;
                        mLocalBP.getXYZi	(mot);

                        IK_data.clamp_by_limits(mot);

                        mLocalBP.setXYZi	(mot);
                        mLocal.mul			(mBind,mLocalBP);
                        mLocal.getXYZi		(mot_rotate);
            */
        }
        else
        {
            // local CS
            Fmatrix mBind, mBindI, mRotate, mLocal, mLocalBP;
            mBind.setXYZi(rest_rotate);
            mBindI.invert(mBind);

            Fvector axis;
            MTransform().transform_dir(axis, _axis);

            // rotation
            mRotate.rotation(axis, angle);
            mLocal.mul(mRotate, MTransform());
            mLocal.getXYZi(mot_rotate);

            // local clamp
            Fvector mot;
            mLocalBP.mul(mBindI, mLocal);
            mLocalBP.getXYZi(mot);

            IK_data.clamp_by_limits(mot);

            mLocalBP.setXYZi(mot);
            mLocal.mul(mBind, mLocalBP);
            mLocal.getXYZi(mot_rotate);
        }
    }
}

void CBone::BoneMove(const Fvector& _amount)
{
    Fvector amount = _amount;
    switch (IK_data.type)
    {
    case jtSlider:
        amount.x = 0.f;
        amount.y = 0.f;
        rest_i_transform.transform(mot_offset);
        mot_offset.add(amount);
        clamp(mot_offset.z, rest_offset.z + IK_data.limits[0].limit.x, rest_offset.z + IK_data.limits[0].limit.y);
        rest_transform.transform(mot_offset);
        break;
    } // XXX: other cases are missing
}

void CBone::ClampByLimits()
{
    Fmatrix mBind, mBindI, mLocal, mLocalBP;
    mBind.setXYZi(rest_rotate);
    mBindI.invert(mBind);

    mLocal.setXYZi(mot_rotate);
    mLocalBP.mul(mBindI, mLocal);
    Fvector mot;
    mLocalBP.getXYZi(mot);

    IK_data.clamp_by_limits(mot);

    mLocalBP.setXYZi(mot);
    mLocal.mul(mBind, mLocalBP);
    mLocal.getXYZi(mot_rotate);
}
