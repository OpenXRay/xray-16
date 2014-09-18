////////////////////////////////////////////////////////////////////////////
//	Module 		: engine.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : engine interface class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_ENGINE_HPP_INCLUDED
#define EDITOR_ENGINE_HPP_INCLUDED

class shared_str;

namespace editor {

class property_holder;

class engine {
public:
	virtual	bool				on_message						(
									HWND hWnd,
									UINT uMsg,
									WPARAM wParam,
									LPARAM lParam,
									LRESULT &result
								) = 0;
	virtual	void				on_idle							() = 0;
	virtual	void				on_resize						() = 0;
	virtual	void				pause							(bool const &value) = 0;
	virtual	void				capture_input					(bool const &value) = 0;
	virtual	void				disconnect						() = 0;
	// shared_str support
	virtual	void				value							(LPCSTR value, shared_str& result) = 0;
	virtual	LPCSTR				value							(shared_str const& value) = 0;

	// weather editor

public:
	// weather
	virtual	void				weather							(LPCSTR value) = 0;
	virtual	LPCSTR				weather							() = 0;
	// weather frame
	virtual	void				current_weather_frame			(LPCSTR frame_id) = 0;
	virtual	LPCSTR				current_weather_frame			() = 0;

	virtual	void				track_frame						(float const& time) = 0;
	virtual	float				track_frame						() = 0;

	virtual	void				track_weather					(float const& time) = 0;
	virtual	float				track_weather					() = 0;

	virtual	property_holder*	current_frame_property_holder	() = 0;
	virtual	property_holder*	blend_frame_property_holder		() = 0;
	virtual	property_holder*	target_frame_property_holder	() = 0;

	virtual	void				weather_paused					(bool const &value) = 0;
	virtual	bool				weather_paused					() = 0;
	virtual	void				weather_time_factor				(float const &value) = 0;
	virtual	float				weather_time_factor				() = 0;
	virtual	void				save_weathers					() = 0;

	virtual	bool				save_time_frame					(char* buffer, u32 const& buffer_size) = 0;

	virtual	bool				paste_current_time_frame		(char const* buffer, u32 const& buffer_size) = 0;
	virtual	bool				paste_target_time_frame			(char const* buffer, u32 const& buffer_size) = 0;

	virtual	void				reload_current_time_frame		() = 0;
	virtual	void				reload_target_time_frame		() = 0;

	virtual	void				reload_current_weather			() = 0;
	virtual	void				reload_weathers					() = 0;

	virtual	bool				add_time_frame					(char const* buffer, u32 const& buffer_size) = 0;

	virtual	char const*			weather_current_time			() const = 0;
	virtual	void				weather_current_time			(char const*) = 0;
}; // class engine

} // namespace editor

#endif // ifndef EDITOR_ENGINE_HPP_INCLUDED