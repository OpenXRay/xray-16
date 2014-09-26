#ifndef AI_DEBUG_VARIABLES_H_INCLUDED
#define AI_DEBUG_VARIABLES_H_INCLUDED

namespace ai_dbg
{
	void   set_var	(const char* name, float	value);
	void   show_var	(const char* name);
	bool   get_var	(const char* name, float&	value);
	bool   get_var  (const char* name, u32&		value);
	bool   get_var  (const char* name, bool&	value);

} // namespace ai_dbg

#endif // AI_DEBUG_VARIABLES_H_INCLUDED