#include "StdAfx.h"
#include "CarDamageParticles.h"
#ifdef DEBUG

#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "Include/xrRender/Kinematics.h"
#include "xrPhysics/IPHWorld.h"

// extern CPHWorld*	ph_world;
void read_bones(IKinematics* K, LPCSTR S, xr_vector<u16>& bones)
{
    string64 S1;
    int count = _GetItemCount(S);
    for (int i = 0; i < count; ++i)
    {
        _GetItem(S, i, S1);

        u16 bone_id = K->LL_BoneID(S1);
        R_ASSERT3(bone_id != BI_NONE, "wrong bone", S1);
        xr_vector<u16>::iterator iter = std::find(bones.begin(), bones.end(), bone_id);
        R_ASSERT3(iter == bones.end(), "double bone", S1);
        bones.push_back(bone_id);
    }
}
void CCarDamageParticles::Init(CCar* car)
{
    IKinematics* K = smart_cast<IKinematics*>(car->Visual());
    CInifile* ini = K->LL_UserData();
    if (ini->section_exist("damage_particles"))
    {
        m_car_damage_particles1 = ini->r_string("damage_particles", "car_damage_particles1");
        m_car_damage_particles2 = ini->r_string("damage_particles", "car_damage_particles2");
        m_wheels_damage_particles1 = ini->r_string("damage_particles", "wheels_damage_particles1");
        m_wheels_damage_particles2 = ini->r_string("damage_particles", "wheels_damage_particles2");

        read_bones(K, ini->r_string("damage_particles", "particle_bones1"), bones1);
        read_bones(K, ini->r_string("damage_particles", "particle_bones2"), bones2);
    }
}

void CCarDamageParticles::Play1(CCar* car)
{
    if (*m_car_damage_particles1)
        for (auto& bone : bones1)
            car->StartParticles(m_car_damage_particles1, bone, Fvector().set(0, 1, 0), car->ID());
}

void CCarDamageParticles::Play2(CCar* car)
{
    VERIFY(!physics_world()->Processing());
    if (*m_car_damage_particles2)
        for (auto& bone : bones2)
            car->StartParticles(m_car_damage_particles2, bone, Fvector().set(0, 1, 0), car->ID());
}

/***** added by Ray Twitty (aka Shadows) START *****/
// функции для выключения партиклов дыма
void CCarDamageParticles::Stop1(CCar* car)
{
    if(*m_car_damage_particles1)
        for (auto& bone : bones1)
            car->StopParticles(car->ID(), bone, false);
}

void CCarDamageParticles::Stop2(CCar* car)
{
    VERIFY(!physics_world()->Processing());
    if(*m_car_damage_particles2)
        for (auto& bone : bones2)
            car->StopParticles(car->ID(), bone, false);
}
/***** added by Ray Twitty (aka Shadows) END *****/


void CCarDamageParticles::PlayWheel1(CCar* car, u16 bone_id) const
{
    VERIFY(!physics_world()->Processing());
    if (*m_wheels_damage_particles1)
        car->StartParticles(m_wheels_damage_particles1, bone_id, Fvector().set(0, 1, 0), car->ID());
}

void CCarDamageParticles::PlayWheel2(CCar* car, u16 bone_id) const
{
    VERIFY(!physics_world()->Processing());
    if (*m_wheels_damage_particles2)
        car->StartParticles(m_wheels_damage_particles2, bone_id, Fvector().set(0, 1, 0), car->ID());
}

void CCarDamageParticles::Clear()
{
    bones1.clear();
    bones2.clear();
}
