#pragma once

class CCoverPoint;
class CCoverEvaluator;
class CBaseMonster;

class CMonsterCoverManager {
	CBaseMonster	*m_object;
	CCoverEvaluator	*m_ce_best;

public:
						CMonsterCoverManager	(CBaseMonster *monster);
						~CMonsterCoverManager	();

	void				load					();
	// найти лучший ковер относительно "position"
	const CCoverPoint	*find_cover				(const Fvector &position, float min_pos_distance, float	max_pos_distance, float deviation = 0.f);

	// найти лучший ковер относительно "src_pos" и "dest_pos"
	const CCoverPoint	*find_cover				(const Fvector &src_pos, const Fvector &dest_pos, float min_pos_distance, float	max_pos_distance, float deviation = 0.f);

	void				less_cover_direction	(Fvector &dir);
};