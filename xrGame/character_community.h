//////////////////////////////////////////////////////////////////////////
// character_community.h:	структура представления группировки
//							
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "ini_id_loader.h"
#include "ini_table_loader.h"

#include "character_info_defs.h"


struct COMMUNITY_DATA
{
	COMMUNITY_DATA (CHARACTER_COMMUNITY_INDEX, CHARACTER_COMMUNITY_ID, LPCSTR);

	CHARACTER_COMMUNITY_ID		id;
	CHARACTER_COMMUNITY_INDEX	index;
	u8 team;
};


class CHARACTER_COMMUNITY;

class CHARACTER_COMMUNITY: 
	public CIni_IdToIndex<1, COMMUNITY_DATA, CHARACTER_COMMUNITY_ID, CHARACTER_COMMUNITY_INDEX, CHARACTER_COMMUNITY>
{
private:
	typedef CIni_IdToIndex<1, COMMUNITY_DATA, CHARACTER_COMMUNITY_ID, CHARACTER_COMMUNITY_INDEX, CHARACTER_COMMUNITY> inherited;
	friend inherited;

public:
	CHARACTER_COMMUNITY			();
	~CHARACTER_COMMUNITY		();

	void						set				(CHARACTER_COMMUNITY_ID);		
	void						set				(CHARACTER_COMMUNITY_INDEX index) {m_current_index = index;};

	CHARACTER_COMMUNITY_ID		id				() const;
	CHARACTER_COMMUNITY_INDEX	index			() const	{return m_current_index;};
	u8							team			() const;

private:
	CHARACTER_COMMUNITY_INDEX	m_current_index;

	static	void				InitIdToIndex	();

public:
	//отношение между группировками
	static CHARACTER_GOODWILL	relation			(CHARACTER_COMMUNITY_INDEX from, CHARACTER_COMMUNITY_INDEX to);
	CHARACTER_GOODWILL			relation			(CHARACTER_COMMUNITY_INDEX to);
	
	static void					set_relation		(CHARACTER_COMMUNITY_INDEX from, CHARACTER_COMMUNITY_INDEX to, CHARACTER_GOODWILL goodwill);

	static float				sympathy			(CHARACTER_COMMUNITY_INDEX);
	
	static void					DeleteIdToIndexData	();
private:
	typedef CIni_Table<CHARACTER_GOODWILL, CHARACTER_COMMUNITY> GOODWILL_TABLE;
	friend GOODWILL_TABLE;
	static GOODWILL_TABLE m_relation_table;

	//таблица коэффициентов "сочуствия" между участниками группировки
	typedef CIni_Table<float, CHARACTER_COMMUNITY> SYMPATHY_TABLE;
	friend SYMPATHY_TABLE;
	static SYMPATHY_TABLE m_sympathy_table;
};