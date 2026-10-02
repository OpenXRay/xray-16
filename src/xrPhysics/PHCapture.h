#pragma once

#include "PHUpdateObject.h"
#include "IPHCapture.h"
#include "xrPhysicsCore/IPhysicsCore.h"
#include "PHJointDestroyInfo.h"

class IPhysicsShellHolder;
class CPHCharacter;
class CPhysicsElement;
class CPhysicsGeom;
struct NearestToPointCallback;
struct SGameMtl;
class CBoneInstance;

class CPHCapture : public CPHUpdateObject, public IPHCapture
{
public:
    CPHCapture(CPHCharacter* a_character, IPhysicsShellHolder* a_taget_object, NearestToPointCallback* cb = nullptr);
    CPHCapture(CPHCharacter* a_character, IPhysicsShellHolder* a_taget_object, u16 a_taget_elemrnt);
    virtual ~CPHCapture();

    bool Failed() { return e_state == cstFree; }
    void Release();
    void RemoveConnection(IPhysicsShellHolder* O);

protected:
    CPHCharacter* m_character;
    CPhysicsElement* m_taget_element;
    IPhysicsShellHolder* m_taget_object;

    JointHandle m_joint;
    JointHandle m_ajoint;
    SPhysicsJointFeedback m_joint_feedback;

    Fvector m_capture_pos;
    float m_back_force;
    float m_pull_force;
    float m_capture_force;
    float m_capture_distance;
    float m_pull_distance;
    u32 m_capture_time;
    u32 m_time_start;
    CBoneInstance* m_capture_bone;

    CharacterVirtualHandle m_char_handle;

    bool b_collide;
    bool b_disabled;
    bool b_character_feedback;

private:
    enum
    {
        cstPulling,
        cstCaptured,
        cstReleased,
        cstFree,
        cstFailed
    } e_state;

    void PullingUpdate();
    void CapturedUpdate();
    void ReleasedUpdate();
    void ReleaseInCallBack();
    void Init();

    void Deactivate();
    void CreateBody();
    bool Invalid();

    static void object_contactCallbackFun(bool& do_colide, bool bo1, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2);
    ///////////CPHObject/////////////////////////////
    virtual void PhDataUpdate(float step);
    virtual void PhTune(float step);
    virtual void NetRelcase(CPhysicsShell* s);
};
