////////////////////////////////////////////////////////////////////////////
//	Module 		: script_process.h
//	Created 	: 19.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script process class
////////////////////////////////////////////////////////////////////////////

#pragma once

class CScriptThread;

class CScriptProcess {
public:
	typedef xr_vector<CScriptThread*> SCRIPT_REGISTRY;

private:
	struct CScriptToRun {
		LPSTR						m_script_name;
		bool						m_do_string;
		bool						m_reload;

		IC		CScriptToRun		(LPCSTR script_name, bool do_string, bool reload = false)
		{
			m_script_name			= xr_strdup(script_name);
			m_do_string				= do_string;
			m_reload				= reload;
		}

		IC		CScriptToRun		(const CScriptToRun &script)
		{
			m_script_name			= xr_strdup(script.m_script_name);
			m_do_string				= script.m_do_string;
			m_reload				= script.m_reload;
		}

		virtual ~CScriptToRun		()
		{
			xr_free					(m_script_name);
		}
	};

public:
	typedef xr_vector<CScriptToRun> SCRIPTS_TO_RUN;

protected:
	SCRIPT_REGISTRY					m_scripts;
	SCRIPTS_TO_RUN					m_scripts_to_run;
	shared_str						m_name;

protected:
	u32								m_iterator;				// Oles: iterative update

protected:
			void					run_scripts		();

public:
									CScriptProcess	(shared_str anme, shared_str scripts);
	virtual							~CScriptProcess	();
			void					update			();
			void					add_script		(LPCSTR	script_name, bool string, bool reload);
	IC		const SCRIPT_REGISTRY	&scripts		() const;
	IC		shared_str				name			() const;
};

#include "script_process_inline.h"