////////////////////////////////////////////////////////////////////////////
//	Module 		: damage_manager.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Damage manager
////////////////////////////////////////////////////////////////////////////

#pragma once

class CDamageManager
{
protected:
    float m_default_hit_factor;
    float m_default_wound_factor;
    IGameObject* m_object;

public:
    CDamageManager();
    virtual ~CDamageManager();
    virtual IFactoryObject* _construct();
    virtual void reload(const char* section, CInifile const* ini);
    virtual void reload(const char* section, const char* sub_section, CInifile const* ini);

    virtual void HitScale(const int bone_num, float& hit_scale, float& wound_scale, bool aim_bullet = false);

private:
    void load_section(const char* section, CInifile const* ini);
    // init default params
    void init_bones(const char* section, CInifile const* ini);
};
