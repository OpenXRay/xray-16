////////////////////////////////////////////////////////////////////////////
// Module : engine_impl.hpp
// Created : 04.12.2007
// Modified : 04.12.2007
// Author : Dmitriy Iassenev
// Description : engine implementation class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Include/editor/engine.hpp"

class IInputReceiver;

class engine_impl : public XRay::Editor::engine_base
{
public:
    engine_impl();
    virtual ~engine_impl();

private:
    virtual bool on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void on_idle();
    virtual void on_resize();
    virtual void pause(bool const& value);
    virtual void capture_input(bool const& value);
    virtual void disconnect();
    bool quit_requested() const override;

    virtual void value(LPCSTR value, shared_str& result);
    virtual LPCSTR value(shared_str const& value);

    virtual void weather(LPCSTR value);
    virtual LPCSTR weather();

    virtual void current_weather_frame(LPCSTR frame_id);
    virtual LPCSTR current_weather_frame();

    virtual void track_frame(float const& time);
    virtual float track_frame();

    virtual void track_weather(float const& time);
    virtual float track_weather();

    virtual XRay::Editor::property_holder_base* current_frame_property_holder();
    virtual XRay::Editor::property_holder_base* blend_frame_property_holder();
    virtual XRay::Editor::property_holder_base* target_frame_property_holder();

    virtual void save_weathers();

    virtual bool save_time_frame(char* buffer, u32 const& buffer_size);

    virtual bool paste_current_time_frame(char const* buffer, u32 const& buffer_size);
    virtual bool paste_target_time_frame(char const* buffer, u32 const& buffer_size);

    virtual void reload_current_time_frame();
    virtual void reload_target_time_frame();

    virtual void reload_current_weather();
    virtual void reload_weathers();

    virtual bool add_time_frame(char const* buffer, u32 const& buffer_size);

    virtual char const* weather_current_time() const;
    virtual void weather_current_time(char const* time);

private:
    virtual void weather_paused(bool const& value);
    virtual bool weather_paused();

    virtual void weather_time_factor(float const& value);
    virtual float weather_time_factor();

private:
    IInputReceiver* m_input_receiver;
    bool m_input_captured;
}; // class engine_impl
