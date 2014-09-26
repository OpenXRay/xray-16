#ifndef FIRST_BULLET_CONTROLLER
#define FIRST_BULLET_CONTROLLER

class first_bullet_controller
{
private:
	u32		m_last_short_time;
	u32		m_shot_timeout;
	float	m_fire_dispertion;
	float	m_actor_velocity_limit;
	bool	m_use_first_bullet;
public:
			first_bullet_controller		();
			void	load				(shared_str const & section);
			bool	is_bullet_first		(float actor_linear_velocity) const;
	inline	float	get_fire_dispertion	() const {return m_fire_dispertion;};
			void	make_shot			();
}; //class first_bullet_controller

#endif //#ifndef FIRST_BULLET_CONTROLLER