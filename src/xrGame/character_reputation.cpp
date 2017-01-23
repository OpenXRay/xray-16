//////////////////////////////////////////////////////////////////////////
// character_reputation.cpp:	структура представления репутаций и 
//								отношений между ними		
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "character_reputation.h"


#define GAME_RELATIONS_SECT		"game_relations"
#define REPUTATION_LINE			"reputation"
#define REPUTATION_TABLE		"reputation_relations"

//////////////////////////////////////////////////////////////////////////
REPUTATION_DATA::REPUTATION_DATA (int idx, shared_str idn, LPCSTR threshold_str)
{
	index = idx;
	id = idn;
	threshold = (CHARACTER_REPUTATION_VALUE)atoi(threshold_str);
}
//////////////////////////////////////////////////////////////////////////
CHARACTER_REPUTATION::GOODWILL_TABLE CHARACTER_REPUTATION::m_relation_table;

//////////////////////////////////////////////////////////////////////////

int   CHARACTER_REPUTATION::ValueToIndex    (CHARACTER_REPUTATION_VALUE val)
{
	T_VECTOR::iterator it = m_pItemDataVector->begin();
	T_VECTOR::iterator it_e = m_pItemDataVector->end();
	for(;it!=it_e;++it)
	{
		if(val < (*it).threshold)
			return (int)std::distance(m_pItemDataVector->begin(), it);
	}
	return inherited::GetMaxIndex();
}

void  CHARACTER_REPUTATION::set	(CHARACTER_REPUTATION_VALUE new_val)
{
	m_current_value = new_val;
	m_current_index = ValueToIndex(new_val);
}

shared_str					CHARACTER_REPUTATION::id			() const
{
	return IndexToId(m_current_index);
}

void CHARACTER_REPUTATION::InitIdToIndex	()
{
	section_name	= GAME_RELATIONS_SECT;
	line_name		= REPUTATION_LINE;

	m_relation_table.set_table_params(REPUTATION_TABLE);
}


CHARACTER_GOODWILL CHARACTER_REPUTATION::relation		(int to)
{
	return relation(m_current_index, to);
}

CHARACTER_GOODWILL  CHARACTER_REPUTATION::relation		(int from, int to)
{
	VERIFY(from >= 0 && from <(int)m_relation_table.table().size());
	VERIFY(to >= 0 && to <(int)m_relation_table.table().size());
	return m_relation_table.table()[from][to];
}

void CHARACTER_REPUTATION::DeleteIdToIndexData	()
{
	m_relation_table.clear();
	inherited::DeleteIdToIndexData();
}