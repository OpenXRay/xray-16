////////////////////////////////////////////////////////////////////////////
//	Module 		: ide.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : IDE interface class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_IDE_HPP_INCLUDED
#define EDITOR_IDE_HPP_INCLUDED

class CEnvironment;

namespace XRay
{
namespace Editor
{
class property_holder_base;
class property_holder_collection;
class property_holder_holder;

class ide_base
{
public:
    virtual ~ide_base() = default;
    virtual HWND main_handle() = 0;
    virtual HWND view_handle() = 0;
    virtual CEnvironment* environment() = 0;
    virtual void run() = 0;
    virtual void on_load_finished() = 0;
    virtual void pause() = 0;

    virtual property_holder_base* create_property_holder(
        LPCSTR display_name, property_holder_collection* collection = nullptr, property_holder_holder* holder = nullptr) = 0;

    virtual void destroy(property_holder_base*& property_holder) = 0;
    virtual void environment_levels(property_holder_base* property_holder) = 0;
    virtual void environment_weathers(property_holder_base* property_holder) = 0;

    typedef fastdelegate::FastDelegate0<LPCSTR const*> weathers_getter_type;
    typedef fastdelegate::FastDelegate0<u32> weathers_size_getter_type;
    typedef fastdelegate::FastDelegate1<LPCSTR, LPCSTR const*> frames_getter_type;
    typedef fastdelegate::FastDelegate1<LPCSTR, u32> frames_size_getter_type;
    virtual void weather_editor_setup(weathers_getter_type const& weathers_getter,
                                      weathers_size_getter_type const& weathers_size_getter, frames_getter_type const& frames_getter,
                                      frames_size_getter_type const& frames_size_getter) = 0;
};
}
}

#endif // ifndef EDITOR_IDE_HPP_INCLUDED
