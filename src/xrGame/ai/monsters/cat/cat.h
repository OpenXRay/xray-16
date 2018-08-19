#pragma once
#include "ai/Monsters/BaseMonster/base_monster.h"

class CCat : public CBaseMonster
{
    typedef CBaseMonster inherited;

public:
    CCat();
    virtual ~CCat();

    virtual void Load(LPCSTR section);
    virtual void reinit();

    virtual void UpdateCL();

    virtual void CheckSpecParams(u32 spec_params);

    void try_to_jump();

    virtual void HitEntityInJump(const CEntity* pEntity);

	//-------------------------------------------------------------------Romann
	virtual	bool	is_relation_enemy	(const CEntityAlive *tpEntityAlive) const;
	xr_vector<shared_str>				m_friend_community_overrides;
	void								load_friend_community_overrides	(LPCSTR section);
	bool								is_community_friend_overrides	(const CEntityAlive *tpEntityAlive) const;
	//-------------------------------------------------------------------Romann

    pcstr get_monster_class_name() override { return "cat"; }
};
