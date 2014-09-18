
#pragma once

#define AXIS_X	(1 << 0)
#define AXIS_Y	(1 << 1)
#define AXIS_Z	(1 << 2)

// параметры движения характерные для конкретной оси в боне
struct bonesAxis {
	float			cur_yaw;
	float			target_yaw;
	float			r_speed;
	float			dist_yaw;		// необходимо лишь для определения текущей скорости по оси
};

// бона с параметрами движения по осям
struct bonesBone {
	CBoneInstance	*bone;
	bonesAxis		params;
	u8				axis;

	bonesBone	() {bone = 0;}
	void	Set			(CBoneInstance *b, u8 a, float ty, float cy, float r_s);
	bool	NeedTurn	();					// необходим поворот по оси p_axis?
	void	Turn		(u32 dt);			// выполнить поворот по оси p_axis
	void	Apply		();								// установить углы у боны

};


// управление движениями костей
class bonesManipulation {
	xr_vector<bonesBone>	m_Bones;
	u32		freeze_time;

	bool	in_return_state;				// если идёт возврат к исходному положению
	u32		time_started;
	u32		time_last_update;
	u32		time_last_delta;

	bool	bActive;
public:
	void 		Reset				();

	void 		AddBone				(CBoneInstance *bone, u8 axis_used);
	void 		SetMotion			(CBoneInstance *bone, u8 axis_used, float target_yaw, float r_speed, u32 t);

	void 		Update				(CBoneInstance *bone, u32 cur_time);
	bool 		IsActive			() {return bActive;}
	bool 		IsReturn			() {return in_return_state;}

	bonesAxis	&GetBoneParams		(CBoneInstance *bone, u8 axis_used);
};


