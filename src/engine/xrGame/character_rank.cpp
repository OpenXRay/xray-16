//////////////////////////////////////////////////////////////////////////
// character_rank.cpp:	структура представления рангов и отношений между 
//						ними		
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "character_rank.h"


#define GAME_RELATIONS_SECT		"game_relations"
#define RANK_LINE				"rating"
#define RANK_TABLE				"rank_relations"
#define RANK_KILL_TABLE_SECT	"rank_kill_points"

//////////////////////////////////////////////////////////////////////////
RANK_DATA::RANK_DATA (int idx, shared_str idn, LPCSTR threshold_str)
{
	index		= idx;
	id			= idn;
	threshold	= (CHARACTER_RANK_VALUE)atoi(threshold_str);
}
//////////////////////////////////////////////////////////////////////////
CHARACTER_RANK::GOODWILL_TABLE		CHARACTER_RANK::m_relation_table;
CHARACTER_RANK::RANK_KILL_TABLE		CHARACTER_RANK::m_rank_kill_table;


int   CHARACTER_RANK::ValueToIndex    (CHARACTER_RANK_VALUE val)
{
	for(int i=0; i<(int)m_pItemDataVector->size(); i++)
	{
		if(val < (*m_pItemDataVector)[i].threshold)
			return i;
	}
	return GetMaxIndex();
}

void  CHARACTER_RANK::set	(CHARACTER_RANK_VALUE new_val)
{
	m_current_value = new_val;
	m_current_index = ValueToIndex(new_val);
}

shared_str					CHARACTER_RANK::id			() const
{
	return IndexToId(m_current_index);
}



void CHARACTER_RANK::InitIdToIndex	()
{
	section_name	= GAME_RELATIONS_SECT;
	line_name		= RANK_LINE;
	m_relation_table.set_table_params(RANK_TABLE);
	m_rank_kill_table.set_table_params(RANK_KILL_TABLE_SECT, 1);
}


CHARACTER_GOODWILL CHARACTER_RANK::relation		(int to)
{
	return relation(m_current_index, to);
}

CHARACTER_GOODWILL  CHARACTER_RANK::relation		(int from, int to)
{
	VERIFY(from >= 0 && from <(int)m_relation_table.table().size());
	VERIFY(to >= 0 && to <(int)m_relation_table.table().size());

	return m_relation_table.table()[from][to];
}


CHARACTER_RANK_VALUE CHARACTER_RANK::rank_kill_points	(int rank_index)
{
	return m_rank_kill_table.table()[rank_index][0];
}


void CHARACTER_RANK::DeleteIdToIndexData	()
{
	m_relation_table.clear();
	m_rank_kill_table.clear();
	inherited::DeleteIdToIndexData();
}