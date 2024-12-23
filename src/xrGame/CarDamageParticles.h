#pragma once

class CCar;
using BIDS = xr_vector<u16>;

struct CCarDamageParticles
{
    BIDS bones1;
    BIDS bones2;
    shared_str m_wheels_damage_particles1;
    shared_str m_wheels_damage_particles2;
    shared_str m_car_damage_particles1;
    shared_str m_car_damage_particles2;

public:
    void Init(CCar* car);
    void Clear();
    void Play1(CCar* car);
    void Play2(CCar* car);
    /***** added by Ray Twitty (aka Shadows) START *****/
    // функции для выключения партиклов дыма
    void Stop1(CCar* car);
    void Stop2(CCar* car);
    /***** added by Ray Twitty (aka Shadows) END *****/

    void PlayWheel1(CCar* car, u16 bone_id) const;
    void PlayWheel2(CCar* car, u16 bone_id) const;
};
