#ifndef MP_CONFIG_SECTIONS_INCLUDED
#define MP_CONFIG_SECTIONS_INCLUDED

#include "../../xrCore/xrCore.h"
#include "../../xrCore/LocatorAPI.h"
//#include "anticheat_dumpable_object.h"

class CGameObject;
class IAnticheatDumpable;

namespace mp_anticheat
{

extern char const * active_params_section;

class mp_config_sections
{
public:
	typedef	xr_vector<shared_str>	mp_sections_t;

			mp_config_sections	();
			~mp_config_sections	();

	void	start_dump			();
	bool	dump_one			(CMemoryWriter & dest);
private:
	mp_sections_t					m_mp_sections;
	mp_sections_t::const_iterator	m_current_dump_sect;
	CInifile						m_tmp_dumper;
}; //class mp_config_sections

class mp_active_params
{
public:
			mp_active_params	();
			~mp_active_params	();
	
	void	dump	(IAnticheatDumpable const * dumpable_obj, LPCSTR sect_name_key, CInifile & dest_dumper);	// for cheater
	void	load_to	(LPCSTR sect_name, CInifile & dest_dumper);						// for verifyer
};//class mp_active_params

} //namespace mp_anticheat

#endif //#ifndef MP_CONFIG_SECTIONS_INCLUDED