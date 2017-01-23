#include "stdafx.h"
#include "mp_config_sections.h"
#include "weapon.h"

namespace mp_anticheat
{

static char* important_sections[] = 
{
	"mp_actor",
	"mp_actor_damage",
	"mp_actor_immunities",
	"mp_actor_condition",
	"rank_base",
	"rank_0",
	"rank_1",
	"rank_2",
	"rank_3",
	"rank_4",
	"deathmatch_gamedata",
	"deathmatch_team0",
	"teamdeathmatch_team1",
	"teamdeathmatch_team2",
	"artefacthunt_gamedata",
	"artefacthunt_team1",
	"artefacthunt_team2",
	"capturetheartefact_gamedata",
	"capturetheartefact_team1",
	"capturetheartefact_team2",
	"deathmatch_base_cost",
	"mp_bonus_money",
	"mp_bonus_exp"
};

mp_config_sections::mp_config_sections() :
	m_tmp_dumper(NULL, FALSE, FALSE, FALSE)
{
	u32		gcount	=  pSettings->line_count("mp_item_groups");
	LPCSTR	line;
	LPCSTR	name;
	for (u32 i = 0; i < gcount; ++i)
	{
		pSettings->r_line	("mp_item_groups", i, &name, &line);
		u32 itmcount		= _GetItemCount(line);
		string256			tmp_single_item;
		for (u32 j = 0; j < itmcount; ++j)
		{
			m_mp_sections.push_back(_GetItem(line, j, tmp_single_item));
		}
	}
	for (int i = 0; i < (sizeof(important_sections) / sizeof(char*)); ++i)
	{
		m_mp_sections.push_back(shared_str(important_sections[i]));
	}
	m_current_dump_sect = m_mp_sections.end();
}

mp_config_sections::~mp_config_sections()
{
}

void mp_config_sections::start_dump	()
{
	m_current_dump_sect = m_mp_sections.begin();
}

bool mp_config_sections::dump_one	(CMemoryWriter & dest)
{
	if (m_current_dump_sect == m_mp_sections.end())
		return false;

	R_ASSERT(pSettings->section_exist(m_current_dump_sect->c_str()));
	CInifile::Sect & tmp_sect = pSettings->r_section(m_current_dump_sect->c_str());
	
	m_tmp_dumper.sections().push_back	(&tmp_sect);
	m_tmp_dumper.save_as				(dest);
	m_tmp_dumper.sections().pop_back	();
	++m_current_dump_sect;
	return (m_current_dump_sect != m_mp_sections.end());
}

//mp_active_params

mp_active_params::mp_active_params()
{
}

mp_active_params::~mp_active_params()
{
}

char const * active_params_section	= "active_params_section";

void mp_active_params::dump	(IAnticheatDumpable const * dumpable_obj, LPCSTR sect_name_key, CInifile & dest_dumper)
{
	LPCSTR		obj_sect_name = "";
	if (dumpable_obj)
	{
		shared_str const tmp_obj_section = dumpable_obj->GetAnticheatSectionName();
		if (tmp_obj_section.size())
		{
			STRCONCAT(obj_sect_name,
				"ap_",
				tmp_obj_section.c_str());
		}
	}
	dest_dumper.w_string(active_params_section, sect_name_key, obj_sect_name);
	if (dest_dumper.section_exist(obj_sect_name))
		return;

	if (dumpable_obj)
	{
		dumpable_obj->DumpActiveParams(obj_sect_name, dest_dumper);
	}
}

void mp_active_params::load_to	(LPCSTR sect_name, CInifile & dest_dumper)
{
	if (!pSettings->section_exist(sect_name))
		return;

	u32 lines_count = pSettings->line_count(sect_name);
	for (u32 i = 0; i < lines_count; ++i)
	{
		LPCSTR	line_name = NULL;
		LPCSTR	line_value = NULL;
		pSettings->r_line		(sect_name, i, &line_name, &line_value);
		dest_dumper.w_string	(sect_name, line_name, line_value);
	}
}


} //namespace mp_anticheat

