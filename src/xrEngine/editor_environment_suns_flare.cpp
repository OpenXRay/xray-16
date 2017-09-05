////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_flare.cpp
// Created : 13.12.2007
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns flare class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_suns_flare.hpp"
#include "Include/editor/ide.hpp"
#include "ide.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::suns::flare;

flare::flare() : m_property_holder(0), m_opacity(0.f), m_position(0.f), m_radius(0.f), m_texture("") {}
flare::~flare()
{
    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

XRay::Editor::property_holder_base* flare::object() { return (m_property_holder); }
void flare::fill(XRay::Editor::property_holder_collection* collection)
{
    VERIFY(!m_property_holder);
    m_property_holder = ::ide().create_property_holder("flare", collection, this);
    property_holder* properties = m_property_holder;

    properties->add_property("texture", "flare", "this option is responsible for gradient texture", m_texture.c_str(),
        m_texture, ".dds", "Texture files (*.dds)|*.dds", detail::real_path("$game_textures$", "").c_str(),
        "Select texture...", XRay::Editor::property_holder_base::cannot_enter_text, XRay::Editor::property_holder_base::remove_extension);

    properties->add_property(
        "opacity", "flare", "this option is responsible for gradient opacity", m_opacity, m_opacity);

    properties->add_property(
        "position", "flare", "this option is responsible for gradient position", m_position, m_position);

    properties->add_property("radius", "flare", "this option is responsible for gradient radius", m_radius, m_radius);
}

#endif // #ifdef INGAME_EDITOR
