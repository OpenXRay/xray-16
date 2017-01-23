#pragma once
class CCar;
DEFINE_VECTOR(u16,BIDS,BIDS_I);
struct CCarDamageParticles
{
	BIDS bones1;
	BIDS bones2;
	shared_str							m_wheels_damage_particles1;
	shared_str							m_wheels_damage_particles2;
	shared_str							m_car_damage_particles1;
	shared_str							m_car_damage_particles2;

public:

void			Init			(CCar* car);
void			Clear			();
void			Play1			(CCar* car);
void			Play2			(CCar* car);
void			PlayWheel1		(CCar*car,u16 bone_id);
void			PlayWheel2		(CCar*car,u16 bone_id);
};