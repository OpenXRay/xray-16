////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_weathers_weather.hpp
// Created : 11.01.2008
// Modified : 11.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment weathers weather class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "property_collection_forward.hpp"

namespace editor
{
namespace environment
{
class manager;

namespace weathers
{
class manager;
class time;

class weather : public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    using property_holder_type = XRay::Editor::property_holder_base;

    weather(environment::manager* manager, shared_str const& id);
    virtual ~weather();
    void load();
    void save();
    void fill(XRay::Editor::property_holder_collection* holder);
    inline shared_str const& id() const { return m_id; }
    shared_str unique_id(shared_str const& current, shared_str const& id) const;
    shared_str generate_unique_id() const;
    virtual property_holder_type* object() { return m_property_holder; }
    bool save_time_frame(shared_str const& frame_id, char* buffer, u32 const& buffer_size);
    bool paste_time_frame(shared_str const& frame_id, char const* buffer, u32 const& buffer_size);
    void reload_time_frame(shared_str const& frame_id);
    void reload();
    bool add_time_frame(char const* buffer, u32 const& buffer_size);

private:
    bool valid_id(shared_str const& id_) const;
    bool try_hours(u32 hours, u32 minutes, u32 seconds, shared_str& result) const;
    bool try_minutes(u32 hours, u32 minutes, u32 seconds, shared_str& result) const;
    shared_str try_all(u32 hours, u32 minutes, u32 seconds) const;
    shared_str generate_unique_id(shared_str const& start) const;

private:
    pcstr xr_stdcall id_getter() const;
    void xr_stdcall id_setter(pcstr value);

public:
    typedef xr_vector<time*> container_type;

public:
    inline container_type const& times() const { return m_times; }
private:
    typedef property_collection<container_type, weather> collection_type;

private:
    shared_str m_id;
    container_type m_times;
    collection_type* m_collection;
    property_holder_type* m_property_holder;

public:
    environment::manager& m_manager;
}; // class weather
} // namespace weathers
} // namespace environment
} // namespace editor

