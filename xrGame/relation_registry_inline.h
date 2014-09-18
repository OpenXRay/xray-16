//////////////////////////////////////////////////////////////////////////
// relation_registry_inline.h:	реестр для хранения данных об отношении 
//								персонажа к другим персонажам
//////////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////////
template<typename T>
ALife::ERelationType RELATION_REGISTRY::GetRelationBetween		(T char1,T char2) const 
{
	ALife::ERelationType rel12 = GetRelationType(char1, char2);
	ALife::ERelationType rel21 = GetRelationType(char2, char1);

	if(ALife::eRelationTypeEnemy == rel12 || ALife::eRelationTypeEnemy == rel21)
		return ALife::eRelationTypeEnemy;
	else if(ALife::eRelationTypeNeutral == rel12 || ALife::eRelationTypeNeutral == rel21)
		return ALife::eRelationTypeNeutral;
	else
		return ALife::eRelationTypeFriend;
}

template<typename T>
void				 RELATION_REGISTRY::SetRelationType		(T from, T to, ALife::ERelationType new_relation)
{
	static int goodwill_enemy	= pSettings->r_s16(GAME_RELATIONS_SECT, "goodwill_enemy");
	static int goodwill_neutral = pSettings->r_s16(GAME_RELATIONS_SECT, "goodwill_neutal");
	static int goodwill_friend	= pSettings->r_s16(GAME_RELATIONS_SECT, "goodwill_friend");

	switch(new_relation)
	{
	case ALife::eRelationTypeEnemy:
		SetGoodwill(from->object_id(), to->object_id(), goodwill_enemy);
		break;
	case ALife::eRelationTypeNeutral:
		SetGoodwill(from->object_id(), to->object_id(), goodwill_neutral);
		break;
	case ALife::eRelationTypeFriend:
		SetGoodwill(from->object_id(), to->object_id(), goodwill_friend);
		break;
	default:
		NODEFAULT;
	}
}

template<typename T>
ALife::ERelationType RELATION_REGISTRY::GetRelationType		(T from, T to) const 
{
	static int attitude_neutral = pSettings->r_s16(GAME_RELATIONS_SECT, "attitude_neutal_threshold");
	static int attitude_friend = pSettings->r_s16(GAME_RELATIONS_SECT, "attitude_friend_threshold");

	CHARACTER_GOODWILL attitude = GetAttitude(from, to);

	if(attitude == NO_GOODWILL)
		return ALife::eRelationTypeNeutral;

	if(attitude<attitude_neutral)
		return ALife::eRelationTypeEnemy;
	else if(attitude<attitude_friend)
		return ALife::eRelationTypeNeutral;
	else
		return ALife::eRelationTypeFriend;
}


//////////////////////////////////////////////////////////////////////////
template<typename T>
CHARACTER_GOODWILL	 RELATION_REGISTRY::GetAttitude	(T from, T to) const 
{
	//личное отношение from к to
	CHARACTER_GOODWILL presonal_goodwill		= GetGoodwill(from->object_id(), to->object_id()); VERIFY(presonal_goodwill != NO_GOODWILL);
	//влияние репутации персонажей
	CHARACTER_GOODWILL reputation_goodwill		= GetReputationRelation(from->Reputation(), to->Reputation());
	//влияние рангов персонажей
	CHARACTER_GOODWILL rank_goodwill			= GetRankRelation(from->Rank(), to->Rank());


	//отношение группировки from персонально к to
	CHARACTER_GOODWILL community_goodwill		= GetCommunityGoodwill(from->Community(), to->object_id()); VERIFY(community_goodwill != NO_GOODWILL);
	//отношение группировки from к группировки to
	CHARACTER_GOODWILL community_to_community	= GetCommunityRelation(from->Community(), to->Community());

	CHARACTER_GOODWILL attitude = presonal_goodwill + 
		reputation_goodwill +
		rank_goodwill +
		community_goodwill + 
		community_to_community;

	return attitude;
}