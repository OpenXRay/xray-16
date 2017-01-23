#pragma once


enum ETelekineticState {
	TS_None,
	TS_Raise,
	TS_Keep,
	TS_Fire,
};

class CGameObject;
class CPhysicsShellHolder;
class CTelekineticObject;
class CPHUpdateObject;
class CTelekinesis;
class CTelekineticObject {

	ETelekineticState	state;
public:
	CPhysicsShellHolder *object;
	CTelekinesis		*telekinesis;
	float				target_height;

	u32					time_keep_started;
	u32					time_keep_updated;
	u32					time_raise_started;

	u32					time_to_keep;
	
	u32					time_fire_started;

	float				strength;

	bool				m_rotate;
	
	ref_sound			sound_hold;
	ref_sound			sound_throw;

public:
								CTelekineticObject		();
								~CTelekineticObject		();
	
virtual		bool				init					(CTelekinesis* tele,CPhysicsShellHolder *obj, float s, float h, u32 ttk, bool rot = true); 
			void				set_sound				(const ref_sound &snd_hold, const ref_sound &snd_throw);

virtual		void				raise					(float step);
virtual		void				raise_update			();

			void				prepare_keep			();
virtual		void				keep					();
virtual		void				keep_update				();
virtual		void				release					();
virtual		void				fire					(const Fvector &target, float power);
			void				fire_t					(const Fvector &target, float time);
virtual		void				fire_update				();
virtual		void				update_state			();
virtual		bool				can_activate			(CPhysicsShellHolder *obj);
			bool				is_released				(){return state==TS_None;}
			ETelekineticState	get_state				() {return state;}
virtual		void				switch_state			(ETelekineticState new_state);
			CPhysicsShellHolder *get_object				() {return object;}

			bool				check_height			();
			bool				check_raise_time_out	();

			bool				time_keep_elapsed		();
			bool				time_fire_elapsed		();

			

			void				enable					();

			bool				operator==				(const CPhysicsShellHolder *obj) {
				return (object == obj);
			}

			void				rotate					();
private:
			void				update_hold_sound		();

};
