#ifndef CONFIGS_DUMPER_INCLUDED
#define CONFIGS_DUMPER_INCLUDED

#include "xr_dsa_signer.h"
#include "mp_config_sections.h"

namespace mp_anticheat
{

extern char const * cd_info_secion;
extern char const * cd_player_name_key;
extern char const * cd_player_digest_key;
extern char const * cd_digital_sign_key;
extern char const * cd_creation_date;

class dump_signer : public xr_dsa_signer
{
public:
			dump_signer				();
			~dump_signer			();
private:
	void	feel_private_dsa_key	();
};

class configs_dumper : public ISheduled
{
public:
	typedef fastdelegate::FastDelegate3<u8 const*, u32, u32, void>	complete_callback_t;
	typedef fastdelegate::FastDelegate1< long >						yield_callback_t;

							configs_dumper		();
	virtual					~configs_dumper		();

	virtual float			shedule_Scale		()			{ return 1.0f; };
	virtual void			shedule_Update		(u32 dt);
	virtual	shared_str		shedule_Name		() const	{ return shared_str("configs_dumper"); };
	virtual bool			shedule_Needed		()			{ return true; };

	void					dump_config			(complete_callback_t complete_cb);
private:
	void					write_configs		();
	void					sign_configs		();
	void					compress_configs	();

	static void				dumper_thread		(void* my_ptr);
	void __stdcall			yield_cb			(long progress);
	void __stdcall			switch_thread		();

	bool const				is_active			() const { return m_state == ds_active; };

	enum enum_dumper_state
	{
		ds_not_active		= 0x00,
		ds_active			= 0x01
	}; //enum_dumper_state

	void					realloc_compress_buffer(u32 need_size);
	u8*						m_buffer_for_compress;
	u32						m_buffer_for_compress_size;
	u32						m_buffer_for_compress_capacity;

	enum_dumper_state		m_state;
	complete_callback_t		m_complete_cb;
	yield_callback_t		m_yield_cb;
	CMemoryWriter			m_dump_result;
	dump_signer				m_dump_signer;
	
	mp_config_sections		m_ltx_configs;
	mp_active_params		m_active_params;

	HANDLE					m_make_start_event;
	HANDLE					m_make_done_event;

#ifdef DEBUG
	CTimer				m_debug_timer;
	u32					m_start_time;
	shared_str			m_timer_comment;
			void		timer_begin		(LPCSTR comment);
			void		timer_end		();
#else
	inline	void		timer_begin		(LPCSTR comment) {}
	inline	void		timer_end		()	{}
#endif
}; //class configs_dumper

} //namespace mp_anticheat

#endif