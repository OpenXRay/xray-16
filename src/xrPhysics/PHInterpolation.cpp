#include "StdAfx.h"
#include "PHInterpolation.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "MathUtils.h"

extern CPHWorld* ph_world;
extern float fixed_step;

CPHInterpolation::CPHInterpolation()
{
    m_handle_type = HandleType::None;
    m_handle = 0;
}

void CPHInterpolation::SetBody(BodyHandle body)
{
    if (body == INVALID_BODY_HANDLE)
        return;

    m_handle_type = HandleType::Body;
    m_handle = body;

    Fmatrix tr;
    GetPhysicsCore()->GetBodyTransform(m_handle, tr);

    qPositions.fill_in(tr.c);

    Fquaternion fQ;
    fQ.set(tr);
    qRotations.fill_in(fQ);
}

void CPHInterpolation::SetCharacter(CharacterVirtualHandle character)
{
    if (character == INVALID_CHARACTER_VIRTUAL_HANDLE)
        return;

    m_handle_type = HandleType::Character;
    m_handle = character;

    Fvector pos;
    GetPhysicsCore()->GetCharacterVirtualPosition(m_handle, pos);

    qPositions.fill_in(pos);

    Fquaternion fQ;
    fQ.identity();
    qRotations.fill_in(fQ);
}

void CPHInterpolation::UpdatePositions()
{
    if (m_handle_type == HandleType::None) return;

    Fvector pos;
    if (m_handle_type == HandleType::Body) {
        Fmatrix tr;
        GetPhysicsCore()->GetBodyTransform(m_handle, tr);
        pos = tr.c;
    } else {
        GetPhysicsCore()->GetCharacterVirtualPosition(m_handle, pos);
    }
    qPositions.push_back(pos);
}

void CPHInterpolation::UpdateRotations()
{
    if (m_handle_type == HandleType::None) return;

    Fquaternion fQ;
    if (m_handle_type == HandleType::Body) {
        Fmatrix tr;
        GetPhysicsCore()->GetBodyTransform(m_handle, tr);
        fQ.set(tr);
    } else {
        fQ.identity();
    }
    qRotations.push_back(fQ);
}

void CPHInterpolation::InterpolatePosition(Fvector& pos)
{
    float t = ph_world->m_frame_time / fixed_step;
    clamp(t, 0.f, 1.f); // Защита от выхода за пределы, чтобы не было дерганий
    pos.lerp(qPositions[0], qPositions[1], t);
}

void CPHInterpolation::InterpolateRotation(Fmatrix& rot)
{
    Fquaternion q;
    float t = ph_world->m_frame_time / fixed_step;
    clamp(t, 0.f, 1.f);
    q.slerp(qRotations[0], qRotations[1], t);
    rot.rotation(q);
}

void CPHInterpolation::ResetPositions()
{
    if (m_handle_type == HandleType::None) return;

    Fvector pos;
    if (m_handle_type == HandleType::Body) {
        Fmatrix tr;
        GetPhysicsCore()->GetBodyTransform(m_handle, tr);
        pos = tr.c;
    } else {
        GetPhysicsCore()->GetCharacterVirtualPosition(m_handle, pos);
    }
    qPositions.fill_in(pos);
}

void CPHInterpolation::ResetRotations()
{
    if (m_handle_type == HandleType::None) return;

    Fquaternion fQ;
    if (m_handle_type == HandleType::Body) {
        Fmatrix tr;
        GetPhysicsCore()->GetBodyTransform(m_handle, tr);
        fQ.set(tr);
    } else {
        fQ.identity();
    }
    qRotations.fill_in(fQ);
}

void CPHInterpolation::GetRotation(Fquaternion& q, u16 num)
{
    q = qRotations[num];
}

void CPHInterpolation::GetPosition(Fvector& p, u16 num)
{
    p = qPositions[num];
}

void CPHInterpolation::SetRotation(const Fquaternion& q, u16 num)
{
    qRotations[num] = q;
}

void CPHInterpolation::SetPosition(const Fvector& p, u16 num)
{
    qPositions[num] = p;
}
