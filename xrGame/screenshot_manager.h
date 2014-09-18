#ifndef SCREENSHOT_MANAGER_FOR_MP
#define SCREENSHOT_MANAGER_FOR_MP

#define RESULT_WIDTH	640
#define RESULT_HEIGHT	480
class screenshot_manager : public ISheduled
{
public:
	enum ss_manager_state_mask_t
	{
		making_screenshot			=	0x01,
		drawing_download_states		=	0x02
	};
	typedef fastdelegate::FastDelegate3<u8 const*, u32, u32, void> complete_callback_t;

	screenshot_manager();
	virtual	~screenshot_manager();

	virtual float						shedule_Scale		()			{ return 1.0f; };
	virtual void						shedule_Update		(u32 dt);
	virtual	shared_str					shedule_Name		() const	{ return shared_str("screenshot_manager"); };
	virtual bool						shedule_Needed		()			{ return true; };
			void						make_screenshot		(complete_callback_t cb);
			void						set_draw_downloads	(bool draw);
			void	__stdcall			jpeg_compress_cb	(long progress);
private:
	CMemoryWriter						m_result_writer;
			void						make_jpeg_file		();
			void						sign_jpeg_file		();
			void						prepare_image		();
			void						compress_image		();
	u32									m_state;
			
			void						realloc_compress_buffer(u32 need_size);
	u8*									m_buffer_for_compress;
	u32									m_buffer_for_compress_size;
	u32									m_buffer_for_compress_capacity;

			void						realloc_jpeg_buffer	(u32 new_size);
	u8*									m_jpeg_buffer;
	u32									m_jpeg_buffer_size;
	u32									m_jpeg_buffer_capacity;
	
	u32									m_defered_ssframe_counter;
	static	u32	const					defer_framescount				=	30;	//count of frames to defer, must be > 1

	inline	bool						is_making_screenshot()	const	{ return !!(m_state & making_screenshot); };
	inline	bool						is_drawing_downloads()	const	{ return !!(m_state & drawing_download_states); };
	inline	bool						is_active()				const	{ return (is_making_screenshot() || is_drawing_downloads()); };
	complete_callback_t					m_complete_callback;

			void						process_screenshot(bool in_other_thread);	
	HANDLE				m_make_start_event;
	HANDLE				m_make_done_event;
	static	void						screenshot_maker_thread(void* this_ptr);


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
}; //class screenshot_manager

#endif //#ifndef SCREENSHOT_MANAGER_FOR_MP