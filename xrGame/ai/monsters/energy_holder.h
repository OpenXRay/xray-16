#pragma once

class CEnergyHolder {

	// энергия
	float		m_value;			// текущее значение энергии
	float		m_restore_vel;		// скорость восстановления (ltx-param)
	float		m_decline_vel;		// скорость уменьшения энергии в активном состоянии (ltx-param)	
	float		m_critical_value;	// критическое значение энергии, меньше которого активность будет отключена (ltx-param)
	float		m_activate_value;	// значение энергии, больше которого активность может быть восстановлена (ltx-param)
	u32			m_time_last_update;	// время последнего обновления энергии
	float		m_aggressive_restore_vel; // скорость агрессивного восстановления (ltx-param)
	
	// активность
	bool		m_active;

	// автоматическая активация/деактивация
	bool		m_auto_activate;
	bool		m_auto_deactivate;

	// обновлене энергии разрешено
	bool		m_enable;

	// состояние агрессивности
	bool		m_aggressive;

public:
					CEnergyHolder			();
	virtual			~CEnergyHolder			();

	virtual	void	reinit					();
	virtual	void	reload					(LPCSTR section, LPCSTR prefix = "", LPCSTR suffix = "");

	virtual	void	schedule_update			();

	virtual	void	on_activate				() {};
	virtual	void	on_deactivate			() {};

			// активность поля
			void	activate				();
			void	deactivate				();
	IC		bool	is_active				(){return m_active;}
	IC		bool	can_activate			(){return (m_value > m_activate_value);}
	IC		bool	should_deactivate		(){return (m_value < m_critical_value);}
	IC		void	set_auto_activate		(bool b_auto = true)  {m_auto_activate = b_auto;}
	IC		void	set_auto_deactivate		(bool b_auto = true)  {m_auto_deactivate = b_auto;}

			void	enable					();
	IC		void	disable					(){m_enable = false;}

	IC		void	set_aggressive			(bool b_val = true) {m_aggressive = b_val;}


	// DEBUG
	IC		float	get_value				() {return m_value;}
};



