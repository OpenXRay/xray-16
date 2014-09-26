////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_ambients_effect_id.cpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment ambients effect identifier class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_ambients_effect_id.hpp"
#include "ide.hpp"
#include "editor_environment_effects_manager.hpp"

using editor::environment::ambients::effect_id;
using editor::environment::effects::manager;

effect_id::effect_id								(
		manager const& manager,
		shared_str const& id
	) :
	m_manager						(manager),
	m_id							(id),
	m_property_holder				(0)
{
}

effect_id::~effect_id								()
{
	if (!Device.editor())
		return;

	::ide().destroy					(m_property_holder);
}

LPCSTR const* effect_id::collection					()
{
	return							(&*m_manager.effects_ids().begin());
}

u32 effect_id::collection_size						()
{
	return							(m_manager.effects_ids().size());
}

void effect_id::fill								(editor::property_holder_collection* collection)
{
	VERIFY							(!m_property_holder);
	m_property_holder				= ::ide().create_property_holder(m_id.c_str(), collection, this);

	typedef editor::property_holder::string_collection_getter_type	collection_getter_type;
	collection_getter_type			collection_getter;
	collection_getter.bind			(this, &effect_id::collection);

	typedef editor::property_holder::string_collection_size_getter_type	collection_size_getter_type;
	collection_size_getter_type		collection_size_getter;
	collection_size_getter.bind		(this, &effect_id::collection_size);

	m_property_holder->add_property	(
		"effect",
		"properties",
		"this option is resposible for effect",
		m_id.c_str(),
		m_id,
		collection_getter,
		collection_size_getter,
		editor::property_holder::value_editor_combo_box,
		editor::property_holder::cannot_enter_text
	);
}

effect_id::property_holder_type* effect_id::object	()
{
	return							(m_property_holder);
}

#endif // #ifdef INGAME_EDITOR