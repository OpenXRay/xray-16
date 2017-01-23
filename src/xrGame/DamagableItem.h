class CDamagableItem
{

protected:
	u16								m_levels_num										;
	float							m_max_health										;
	u16								m_level_applied										;
public:
									CDamagableItem		()									;
	virtual		void				Init				(float max_health,u16 level_num)	;
				void				HitEffect			()									;
				void				RestoreEffect		()									;
				float				DamageLevelToHealth	(u16 dl)							;
protected:
				u16 				DamageLevel			()									;
	virtual		float				Health				()									=0;
	virtual		void				ApplyDamage			(u16 level)							;
};

class CDamagableHealthItem : 
	public CDamagableItem
{
	typedef		CDamagableItem		inherited											;
	float							m_health											;
public:
virtual		void					Init			(float max_health,u16 level_num)	;
			void					Hit				(float P)							;
			void					SetHealth		(float health)						{m_health=health;}
protected:
	virtual		float				Health			()									{return m_health;}

};
