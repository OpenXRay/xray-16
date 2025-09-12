////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_thunderbolt.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts thunderbolt class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "editor_environment_thunderbolts_gradient.hpp"
#include "xrEngine/thunderbolt.h"

namespace editor
{
namespace environment
{
class manager;

namespace thunderbolts
{
class manager;

class thunderbolt : public SThunderboltDesc, public XRay::Editor::property_holder_holder, private Noncopyable
{
private:
    typedef SThunderboltDesc inherited;

public:
    thunderbolt(manager* manager, shared_str const& id);
    virtual ~thunderbolt();
    void load(CInifile& config);
    void save(CInifile& config);
    void fill(::editor::environment::manager& environment, XRay::Editor::property_holder_collection* collection);
    inline pcstr id() const { return m_id.c_str(); }
    virtual void create_top_gradient(const CInifile& pIni, shared_str const& sect);
    virtual void create_center_gradient(const CInifile& pIni, shared_str const& sect);

private:
    pcstr id_getter() const;
    void id_setter(pcstr value);

private:
    typedef XRay::Editor::property_holder_base property_holder_type;

public:
    virtual property_holder_type* object();

private:
    shared_str m_id;
    manager& m_manager;
    property_holder_type* m_property_holder;

private:
    gradient* m_center;
    gradient* m_top;
    shared_str m_color_animator;
    shared_str m_lighting_model;
    shared_str m_sound;
}; // class thunderbolt

} // namespace thunderbolts
} // namespace environment
} // namespace editor

