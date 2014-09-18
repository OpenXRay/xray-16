#pragma once

#define _RANK_COUNT 5

u32 get_rank(const shared_str &section);


typedef struct 
{
	shared_str		name;
	int				n;
} RESTR;

class CRestrictions 
{
public:
							CRestrictions			();
							~CRestrictions			();	

	void 					InitGroups				();
	const u32  				GetRank					() const							{return m_rank;}
	bool 					IsAvailable				(const shared_str& section_name);
    u32  					GetItemCount			(const shared_str& section_name)	const;
	shared_str				GetItemGroup			(const shared_str& section_name)	const;
	u32						GetGroupCount			(const shared_str& group_name)		const;
	void 					SetRank					(u32 rank)							{VERIFY(rank>=0 && rank <5); m_rank = rank;}
	u32						GetRank					()									{return m_rank;};
	const shared_str&		GetRankName				(u32 rank)	const					{return m_names[rank];}
protected:
	void					Dump					() const;

private:	
	void					AddGroup				(LPCSTR group, LPCSTR lst);
	bool					IsGroupExist			(const shared_str& group)							const;
	void					AddRestriction4rank		(u32 rank, const shared_str& lst);
	RESTR					GetRestr				(const shared_str& item);

	u32						m_rank;
	bool					m_bInited;
	
	DEF_VECTOR(group_items, shared_str);
	DEF_MAP(Groups, shared_str, group_items);
	Groups											m_goups;


	typedef		std::pair<shared_str, u32>			restr_item;
	DEF_VECTOR(rank_rest_vec, restr_item);
	rank_rest_vec									m_restrictions[_RANK_COUNT+1];
	shared_str										m_names[_RANK_COUNT];

	const restr_item*		find_restr_item			(const u32& rank, const shared_str& what )const;
	restr_item*				find_restr_item_internal(const u32& rank, const shared_str& what );
};
extern CRestrictions g_mp_restrictions;
