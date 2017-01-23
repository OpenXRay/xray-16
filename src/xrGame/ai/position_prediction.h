#ifndef POSITION_PREDICTION_H_INCLUDED
#define POSITION_PREDICTION_H_INCLUDED

class position_prediction
{
public:
	position_prediction ()
	{
		reinit										();
	}

	void		reinit	()
	{
		m_predicted_enemy_velocity				=	cr_fvector3(0.f);
		m_last_prediction_time					=	0;
	}

	Fvector		calculate_predicted_enemy_pos 		(float	 const	prediction_factor, 
													 Fvector const	enemy_pos,
													 Fvector const	self_pos,
													 float	 const	self_velocity)
	{

		float const		epsilon					=	0.0001f;
		float const		self2enemy_mag			=	magnitude(enemy_pos - self_pos);
		float const		self2enemy_time			=	self_velocity > epsilon ? 
													self2enemy_mag / self_velocity : 0;

		float const		predictiton_delta_sec	=	(current_time() - m_last_prediction_time) / 1000.f;
		if ( predictiton_delta_sec > 0.4f )
		{
			if ( m_last_prediction_time != 0 )
			{
				if ( predictiton_delta_sec < 2.f )
				{
					Fvector	const move_delta	=	enemy_pos - m_last_update_enemy_pos;
					m_predicted_enemy_velocity	=	move_delta / predictiton_delta_sec;
				}
				else
				{
					m_predicted_enemy_velocity	=	cr_fvector3(0.f);
				}
			}

			m_last_prediction_time				=	current_time();
			m_last_update_enemy_pos				=	enemy_pos;
		}

		return	enemy_pos + (m_predicted_enemy_velocity*self2enemy_time*prediction_factor);
	}

private:
	Fvector											m_predicted_enemy_velocity;
	TTime											m_last_prediction_time;
	Fvector											m_last_update_enemy_pos;

}; // class position_prediction

#endif // #ifndef POSITION_PREDICTION_H_INCLUDED