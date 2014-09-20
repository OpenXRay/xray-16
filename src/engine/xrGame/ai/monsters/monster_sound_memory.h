#pragma once

#include "ai_monster_defs.h"
#include "../../../xrServerEntities/ai_sounds.h"

class CBaseMonster;

typedef enum {
	WEAPON_SHOOTING = 0,
	MONSTER_ATTACKING,
	WEAPON_BULLET_RICOCHET,
	WEAPON_RECHARGING,

	WEAPON_TAKING,
	WEAPON_HIDING,			
	WEAPON_CHANGING,
	WEAPON_EMPTY_CLICKING,

	MONSTER_DYING,	
	MONSTER_INJURING,
	MONSTER_WALKING,			
	MONSTER_JUMPING,			
	MONSTER_FALLING,			
	MONSTER_TALKING,			

	DOOR_OPENING,		
	DOOR_CLOSING,		
	OBJECT_BREAKING,	
	OBJECT_FALLING,
	NONE_DANGEROUS_SOUND
} TSoundDangerValue;


#define FACTOR_SOUND_TYPE	int(8)
#define FACTOR_DISTANCE		int(1)
#define FACTOR_DELTA_TIME	int(2)
#define FACTOR_SOUND_POWER	int(50)


typedef struct tagSoundElement
{
	const CObject		*who;
	TSoundDangerValue	type;
	Fvector				position;		// позиция звука, не объекта, издавшего звук
	float				power;
	TTime				time;			// время обнаружения звука

	int					value;			// оценочное значение данного звука		

	tagSoundElement() {  
		who = 0; type = NONE_DANGEROUS_SOUND; position.set(0,0,0); power = 0.f; time = 0; value = 0;
	}
	bool operator < (const tagSoundElement &s) const  { 
		return (value < s.value);
	}
	IC void SetConvert(const CObject* who, int eType, const Fvector &position, float power, TTime time) {
		this->who = who; type = ConvertSoundType((ESoundTypes)eType); this->position = position; this->power = power; this->time = time;
	}
	TSoundDangerValue ConvertSoundType(ESoundTypes stype);

	void CalcValue(TTime cur_time, Fvector cur_pos) {
		value = FACTOR_SOUND_TYPE * u32(NONE_DANGEROUS_SOUND - WEAPON_SHOOTING) - iFloor(FACTOR_DISTANCE * cur_pos.distance_to(position)) - FACTOR_DELTA_TIME * iFloor(float((cur_time - time) / 1000)) + FACTOR_SOUND_POWER * iFloor(power);
	}

} SoundElem;

class CMonsterSoundMemory {
	TTime					time_memory;				// время хранения звуков
	xr_vector<SoundElem>	Sounds;

	CBaseMonster			*monster;

	u32						m_time_help_sound;
	u32						m_help_node;

public:
				CMonsterSoundMemory		();
	virtual		~CMonsterSoundMemory	();

	void		init_external			(CBaseMonster *M, TTime mem_time);

	void		HearSound				(const SoundElem &s);
	void		HearSound				(const CObject* who, int eType, const Fvector &Position, float power, TTime time);
	IC	bool	IsRememberSound			() {return (!Sounds.empty());}		
	
	// Lain: added
	int         GetNumSounds            () {return Sounds.size();}
	void        GetFirstSound           (SoundElem& s, bool& bDangerous);

	void		GetSound				(SoundElem& s, bool& bDangerous);	// возвращает самый опасный звук
	SoundElem	&GetSound				();
	bool		get_sound_from_object	(const CObject* who, SoundElem	&value);	

	void		UpdateHearing			();

	bool		is_loud_sound			(float val);
	
	void		clear					() {Sounds.clear();}
	void		remove_links			(CObject *O);

	// help sounds
	bool		hear_help_sound			();
	u32			hear_help_sound_node	(){VERIFY(m_help_node != u32(-1)); return m_help_node;}

	void		check_help_sound		(int eType, u32 node);
};

