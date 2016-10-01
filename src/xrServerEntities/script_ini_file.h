////////////////////////////////////////////////////////////////////////////
//	Module 		: script_ini_file.h
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script ini file class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_token_list.h"
#include "script_export_space.h"

class CScriptIniFile : public CInifile {
protected:
	typedef CInifile inherited;

public:
						CScriptIniFile		(IReader *F, LPCSTR path=0);
						CScriptIniFile		(LPCSTR szFileName, BOOL ReadOnly=TRUE, BOOL bLoadAtStart=TRUE, BOOL SaveAtEnd=TRUE, LPCSTR path=NULL);
	virtual 			~CScriptIniFile		();
			bool		line_exist			(LPCSTR S, LPCSTR L);
			bool		section_exist		(LPCSTR S);
			int			r_clsid				(LPCSTR S, LPCSTR L);
			bool		r_bool				(LPCSTR S, LPCSTR L);
			int			r_token				(LPCSTR S, LPCSTR L, const CScriptTokenList &token_list);
			LPCSTR		r_string_wb			(LPCSTR S, LPCSTR L);
			LPCSTR		update				(LPCSTR file_name);
			u32			line_count			(LPCSTR S);
			LPCSTR		r_string			(LPCSTR S, LPCSTR L);
			u32			r_u32				(LPCSTR S, LPCSTR L);
			int			r_s32				(LPCSTR S, LPCSTR L);
			float		r_float				(LPCSTR S, LPCSTR L);
			Fvector		r_fvector3			(LPCSTR S, LPCSTR L);
            //AVO: additional methods to allow writing to ini files
#ifdef INI_FILE_EXTENDED_EXPORTS
            void w_bool(LPCSTR S, LPCSTR L, bool V, LPCSTR comment /* = 0 */);
            void w_color(LPCSTR S, LPCSTR L, u32 V, LPCSTR comment /* = 0 */);
            void w_fcolor(LPCSTR S, LPCSTR L, const Fcolor& V, LPCSTR comment /* = 0 */);
            void w_float(LPCSTR S, LPCSTR L, float V, LPCSTR comment /* = 0 */);
            void w_fvector2(LPCSTR S, LPCSTR L, const Fvector2& V, LPCSTR comment /* = 0 */);
            void w_fvector3(LPCSTR S, LPCSTR L, const Fvector3& V, LPCSTR comment /* = 0 */);
            void w_fvector4(LPCSTR S, LPCSTR L, const Fvector4& V, LPCSTR comment /* = 0 */);
            void w_s16(LPCSTR S, LPCSTR L, s16 V, LPCSTR comment /* = 0 */);
            void w_s32(LPCSTR S, LPCSTR L, s32 V, LPCSTR comment /* = 0 */);
            void w_s64(LPCSTR S, LPCSTR L, s64 V, LPCSTR comment /* = 0 */);
            void w_s8(LPCSTR S, LPCSTR L, s8 V, LPCSTR comment /* = 0 */);
            void w_string(LPCSTR S, LPCSTR L, LPCSTR V, LPCSTR comment /* = 0 */);
            void w_u16(LPCSTR S, LPCSTR L, u16 V, LPCSTR comment /* = 0 */);
            void w_u32(LPCSTR S, LPCSTR L, u32 V, LPCSTR comment /* = 0 */);
            void w_u64(LPCSTR S, LPCSTR L, u64 V, LPCSTR comment /* = 0 */);
            void w_u8(LPCSTR S, LPCSTR L, u8 V, LPCSTR comment /* = 0 */);
            bool save_as(LPCSTR new_fname /* = 0 */);
            void save_at_end(bool b);
            void remove_line(LPCSTR S, LPCSTR L);
            void set_override_names(bool b);
			u32 section_count();
			void set_readonly(bool b);
#endif

			DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptIniFile)
#undef script_type_list
#define script_type_list save_type_list(CScriptIniFile)

#include "script_ini_file_inline.h"