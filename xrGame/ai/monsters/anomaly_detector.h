#pragma once

class CBaseMonster;

class CAnomalyDetector {
	CBaseMonster			*m_object;

	float					m_radius;
	u32						m_time_to_rememeber;

	bool					m_active;

	struct SAnomalyInfo {
		CObject		*object;
		u32			time_registered;

		bool		operator == (CObject *obj) 	{
			return (object == obj);
		}
	};

	struct remove_predicate {
		u32		time_remember;
		remove_predicate (u32 time) : time_remember(time){}

		IC bool	operator() (const SAnomalyInfo &info) {
			return (info.time_registered + time_remember < Device.dwTimeGlobal);
		}
	};

	DEFINE_VECTOR			(SAnomalyInfo, ANOMALY_INFO_VEC, ANOMALY_INFO_VEC_IT);
	ANOMALY_INFO_VEC		m_storage;

public:
				CAnomalyDetector	(CBaseMonster *monster);
	virtual		~CAnomalyDetector	();

	void		load				(LPCSTR section);
	void		reinit				();

	void		update_schedule		();
	void		on_contact			(CObject *obj);
	
	void		activate			(){m_active = true;}
	void		deactivate			(){m_active = false;}
};