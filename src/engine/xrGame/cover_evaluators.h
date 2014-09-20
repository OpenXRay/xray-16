////////////////////////////////////////////////////////////////////////////
//	Module 		: cover_evaluators.h
//	Created 	: 24.04.2004
//  Modified 	: 24.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Cover evaluators
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "restricted_object.h"
#include "game_graph_space.h"

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorBase
//////////////////////////////////////////////////////////////////////////

class CCoverPoint;
class CAI_Stalker;

namespace smart_cover{
	class cover;
	class loophole;
};

class CCoverEvaluatorBase {

protected:
	CCoverPoint const			*m_selected;
	CCoverPoint const			*m_previous_selected;
	u32							m_last_update;
	u32							m_inertia_time;
	float						m_best_value;
	float						m_best_loophole_value;
	bool						m_initialized;
	Fvector						m_start_position;
	CRestrictedObject			*m_object;
	CAI_Stalker					*m_stalker;
	bool						m_actuality;
	float						m_last_radius;
	smart_cover::loophole		*m_loophole;
	bool						m_use_smart_covers_only;
	bool						m_can_use_smart_covers;

protected:
	virtual	void				evaluate_smart_cover(smart_cover::cover const *smart_cover, float const &weight) = 0;
	virtual	void				evaluate_cover		(const CCoverPoint *cover_point, float weight) = 0;

public:
								CCoverEvaluatorBase	(CRestrictedObject *object);
	IC		const CCoverPoint	*selected			() const;
	IC		smart_cover::loophole const	*loophole	() const;
			bool				inertia				(Fvector const& position, float radius);
	IC		bool				initialized			() const;
	IC		void				setup				();
	IC		void				set_inertia			(u32 inertia_time);
	IC		void				initialize			(const Fvector &start_position, bool fake_call = false);
	virtual void				finalize			();
	IC		bool				accessible			(const Fvector &position);
	IC		bool				actual				() const;
	IC		CRestrictedObject	&object				() const;
	IC		void				invalidate			();
	IC		float				best_value			() const;
	IC		bool				use_smart_covers_only() const;
	IC		void				use_smart_covers_only(bool value);
	IC		bool				can_use_smart_covers() const;
	IC		void				can_use_smart_covers(bool value);
	
			void				evaluate			(const CCoverPoint *cover_point, float weight);
};

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorCloseToEnemy
//////////////////////////////////////////////////////////////////////////

class CCoverEvaluatorCloseToEnemy : public CCoverEvaluatorBase {
protected:
	typedef CCoverEvaluatorBase inherited;

protected:
	Fvector				m_enemy_position;
	float				m_min_distance;
	float				m_max_distance;
	float				m_current_distance;
	float				m_deviation;
	float				m_best_distance;

public:
	IC					CCoverEvaluatorCloseToEnemy	(CRestrictedObject *object);
	IC		void		initialize			(const Fvector &start_position, bool fake_call = false);
	IC		void		setup				(const Fvector &enemy_position, float min_enemy_distance, float	max_enemy_distance, float deviation = 0.f);
	virtual	void		evaluate_cover		(const CCoverPoint *cover_point, float weight);
	virtual	void		evaluate_smart_cover(smart_cover::cover const *smart_cover, float const &weight);
};

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorFarFromEnemy
//////////////////////////////////////////////////////////////////////////

class CCoverEvaluatorFarFromEnemy : public CCoverEvaluatorCloseToEnemy {
protected:
	typedef CCoverEvaluatorCloseToEnemy inherited;

public:
	IC					CCoverEvaluatorFarFromEnemy	(CRestrictedObject *object);
	virtual	void		evaluate_cover		(const CCoverPoint *cover_point, float weight);
	virtual	void		evaluate_smart_cover(smart_cover::cover const *smart_cover, float const &weight);
};

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorBest
//////////////////////////////////////////////////////////////////////////

class CCoverEvaluatorBest : public CCoverEvaluatorCloseToEnemy {
protected:
	typedef CCoverEvaluatorCloseToEnemy inherited;

private:
			bool		threat_on_the_way	(Fvector const& cover_position) const;

public:
	IC					CCoverEvaluatorBest	(CRestrictedObject *object);
	virtual	void		evaluate_cover		(const CCoverPoint *cover_point, float weight);
	virtual	void		evaluate_smart_cover(smart_cover::cover const *smart_cover, float const &weight);
};

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorAngle
//////////////////////////////////////////////////////////////////////////

class CCoverEvaluatorAngle : public CCoverEvaluatorCloseToEnemy {
protected:
	typedef CCoverEvaluatorCloseToEnemy inherited;

protected:
	Fvector				m_direction;
	Fvector				m_best_direction;
	float				m_best_alpha;
	u32					m_level_vertex_id;

public:
	IC					CCoverEvaluatorAngle(CRestrictedObject *object);
	IC		void		setup				(const Fvector &enemy_position, float min_enemy_distance, float	max_enemy_distance, u32 level_vertex_id);
			void		initialize			(const Fvector &start_position, bool fake_call = false);
	virtual	void		evaluate_cover		(const CCoverPoint *cover_point, float weight);
	virtual	void		evaluate_smart_cover(smart_cover::cover const *smart_cover, float const &weight);
};

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorSafe
//////////////////////////////////////////////////////////////////////////

class CCoverEvaluatorSafe : public CCoverEvaluatorBase {
protected:
	typedef CCoverEvaluatorBase inherited;

protected:
	float				m_min_distance;

public:
	IC					CCoverEvaluatorSafe	(CRestrictedObject *object);
	IC		void		setup				(float min_distance);
	virtual	void		evaluate_cover		(const CCoverPoint *cover_point, float weight);
	virtual	void		evaluate_smart_cover(smart_cover::cover const *smart_cover, float const &weight);
};

//////////////////////////////////////////////////////////////////////////
// CCoverEvaluatorAmbush
//////////////////////////////////////////////////////////////////////////

class CCoverEvaluatorAmbush : public CCoverEvaluatorBase {
protected:
	typedef CCoverEvaluatorBase inherited;

private:
	Fvector				m_my_position;
	Fvector				m_enemy_position;
	float				m_min_enemy_distance;

public:
	IC					CCoverEvaluatorAmbush	(CRestrictedObject *object);
			void		setup					(const Fvector &my_position, const Fvector &enemy_position, float min_enemy_distance);
	virtual	void		evaluate_cover			(const CCoverPoint *cover_point, float weight);
	virtual	void		evaluate_smart_cover	(smart_cover::cover const *smart_cover, float const &weight);
};

#include "cover_evaluators_inline.h"