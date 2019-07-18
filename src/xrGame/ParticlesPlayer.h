///////////////////////////////////////////////////////////////
// ParticlesPlayer.h
// интерфейс для проигрывания партиклов на объекте
///////////////////////////////////////////////////////////////

#pragma once

#include "ParticlesObject.h"
#include "xrCore/Animation/Bone.hpp"

using PARTICLES_PTR_VECTOR = xr_vector<CParticlesObject*>;

class IGameObject;
class IKinematics;

class CParticlesPlayer
{
public:
    //структура с внутренней информацией о партикле
    struct SParticlesInfo
    {
        CParticlesObject* ps;
        Fvector angles;
        u16 sender_id; // id - объекта, который запустил партиклы
        u32 life_time; //время жизни партикла (-1) - бесконечно
    };
    using ParticlesInfoList = xr_vector<SParticlesInfo>;

    //структура для косточки со списком запущенных партиклов
    struct SBoneInfo
    {
        u16 index;
        Fvector offset;
        ParticlesInfoList particles;
        SParticlesInfo* FindParticles(const shared_str& ps_name);

        SBoneInfo(u16 idx, const Fvector& offs) : index(idx), offset(offs) { ; }
        SParticlesInfo* AppendParticles(IGameObject* object, const shared_str& ps_name);
        void StopParticles(const shared_str& ps_name, bool bDestroy);
        void StopParticles(u16 sender_id, bool bDestroy);
    };
    using BoneInfoVec = xr_vector<SBoneInfo>;

private:
    // список костей
    u64 bone_mask; // используемые кости
    BoneInfoVec m_Bones;
    IGameObject* m_self_object;

protected:
    bool m_bActiveBones; //есть ли косточки на которых играются партиклы

public:
    IC SBoneInfo* get_bone_info(u16 bone_index)
    {
        if (BI_NONE == bone_index)
            return 0;
        for (auto it = m_Bones.begin(); it != m_Bones.end(); ++it)
            if (it->index == bone_index)
                return &(*it);
        return 0;
    }
    SBoneInfo* get_nearest_bone_info(IKinematics* K, u16 bone_index);
    Fvector parent_vel;

    CParticlesPlayer(void);
    virtual ~CParticlesPlayer(void);
    void LoadParticles(IKinematics* K);

    void net_DestroyParticles();
    void net_SpawnParticles();

    void UpdateParticles();

    void StartParticles(const shared_str& ps_name, u16 bone_num, const Fvector& dir, u16 sender_id, int life_time = -1,
        bool auto_stop = true);
    void StartParticles(
        const shared_str& ps_name, const Fvector& dir, u16 sender_id, int life_time = -1, bool auto_stop = true);

    void StartParticles(const shared_str& ps_name, u16 bone_num, const Fmatrix& dir, u16 sender_id, int life_time = -1,
        bool auto_stop = true);
    void StartParticles(
        const shared_str& ps_name, const Fmatrix& dir, u16 sender_id, int life_time = -1, bool auto_stop = true);

    void StopParticles(u16 sender_ID, u16 bone_id, bool bDestroy);
    void StopParticles(const shared_str& particles_name, u16 bone_id, bool bDestroy);
    void AutoStopParticles(const shared_str& ps_name, u16 bone_id, u32 life_time);

    static void MakeXFORM(
        IGameObject* pObject, u16 bone_id, const Fvector& dir, const Fvector& offset, Fmatrix& result);
    static void GetBonePos(IGameObject* pObject, u16 bone_id, const Fvector& offset, Fvector& result);
    u16 GetNearestBone(IKinematics* K, u16 bone_id);
    IC u16 GetRandomBone()
    {
        u16 l_PBCount = u16(m_Bones.size());
        if (l_PBCount)
            return m_Bones[(u16)Random.randI(l_PBCount)].index;
        return BI_NONE;
    }

    void SetParentVel(const Fvector& vel) { parent_vel = vel; }
    bool IsPlaying() { return m_bActiveBones; }
    virtual CParticlesPlayer* cast_particles_player() { return this; }
};
